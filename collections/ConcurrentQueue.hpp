#pragma once

#include <atomic>
#include <memory>
#include <string.h>
#include <assert.h>
#include "../common/compat.hpp"
#include "../common/CachePad.hpp"
#include "../threading/thread.hpp"
#include "../memory/PoolAllocator.hpp"

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			enum class ElementState : int16_t
			{
				EMPTY,
				READY,
				RETRY,
				DEAD,
			};

			template<typename T, size_t blockSize>
			class Buffer;

			template<typename T, size_t blockSize>
			struct SharedBuffer;

			template<typename T, size_t blockSize>
			class LocalBuffer;
		}

		template<typename T, size_t blockSize = 8192>
		class ConcurrentQueue;
	}
}

template<typename T, size_t blockSize>
struct sprawl::collections::detail::SharedBuffer
{
	typedef LocalBuffer<T, blockSize> LocalBuffer;
	typedef Buffer<T, blockSize> Buffer;

	inline SharedBuffer(Buffer* buf)
		: m_buf(buf)
	{
		if(SPRAWL_LIKELY(buf))
		{
			buf->IncRef();
		}
	}

	inline SharedBuffer(LocalBuffer& other)
		: m_buf(*other)
	{
		Buffer* buf = m_buf.load(std::memory_order_acquire);
		if(SPRAWL_LIKELY(buf))
		{
			buf->IncRef();
		}
	}

	inline SharedBuffer(nullptr_t buf)
		: m_buf(buf)
	{
	}

	inline SharedBuffer()
		: m_buf(nullptr)
	{
	}

	inline ~SharedBuffer()
	{
		Buffer* buf = m_buf.load(std::memory_order_acquire);;
		if(SPRAWL_LIKELY(buf))
		{
			typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
			if(SPRAWL_UNLIKELY(buf->DecRef() == 0))
			{
				buf->~Buffer();
				bufferAlloc::free(buf);
			}
		}
	}

	Buffer* operator->() { return m_buf.load(std::memory_order_acquire); }
	Buffer* operator*() { return m_buf.load(std::memory_order_acquire); }

	SharedBuffer(SharedBuffer const& other) = delete;
	SharedBuffer(SharedBuffer&& other) = delete;

	inline SharedBuffer& operator=(Buffer* other)
	{
		Reset(other);
		return *this;
	}
	inline SharedBuffer& operator=(SharedBuffer& other)
	{
		Reset(other.m_buf.load(std::memory_order_acquire));
		return *this;
	}
	inline SharedBuffer& operator=(LocalBuffer& other)
	{
		Reset(other.m_buf);
		return *this;
	}

	inline void Reset(Buffer* other)
	{
		if(SPRAWL_LIKELY(other))
		{
			other->IncRef();
		}
		Buffer* buf = m_buf.exchange(other, std::memory_order_acq_rel);
		if(buf)
		{
			typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
			if(SPRAWL_UNLIKELY(buf->DecRef() == 0))
			{
				buf->~Buffer();
				bufferAlloc::free(buf);
			}
		}
	}

	std::atomic<Buffer*> m_buf;
};

template<typename T, size_t blockSize>
struct sprawl::collections::detail::LocalBuffer
{
	typedef SharedBuffer<T, blockSize> SharedBuffer;
	typedef Buffer<T, blockSize> Buffer;

	inline LocalBuffer(Buffer* buf)
		: m_buf(buf)
	{
		if(SPRAWL_LIKELY(buf))
		{
			buf->IncRef();
		}
	}

	inline LocalBuffer(SharedBuffer& other)
		: m_buf(*other)
	{
		if(SPRAWL_LIKELY(m_buf))
		{
			m_buf->IncRef();
		}
	}

	inline LocalBuffer(nullptr_t buf)
		: m_buf(buf)
	{
	}

	inline LocalBuffer()
		: m_buf(nullptr)
	{
	}

	inline ~LocalBuffer()
	{
		if(SPRAWL_LIKELY(m_buf))
		{
			typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
			if(SPRAWL_UNLIKELY(m_buf->DecRef() == 0))
			{
				m_buf->~Buffer();
				bufferAlloc::free(m_buf);
			}
		}
	}

	Buffer* operator->() { return m_buf; }
	Buffer* operator*() { return m_buf; }

	LocalBuffer(LocalBuffer const& other) = delete;
	LocalBuffer(LocalBuffer&& other) = delete;

	inline LocalBuffer& operator=(Buffer* other)
	{
		Reset(other);
		return *this;
	}
	inline LocalBuffer& operator=(SharedBuffer& other)
	{
		Reset(other.m_buf.load(std::memory_order_acquire));
		return *this;
	}
	inline LocalBuffer& operator=(LocalBuffer& other)
	{
		Reset(other.m_buf);
		return *this;
	}

	inline void Reset(Buffer* other)
	{
		if(SPRAWL_LIKELY(other))
		{
			other->IncRef();
		}
		Buffer* buf = m_buf;
		m_buf = other;
		if(buf)
		{
			typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
			if(SPRAWL_UNLIKELY(buf->DecRef() == 0))
			{
				buf->~Buffer();
				bufferAlloc::free(buf);
			}
		}
	}

	Buffer* m_buf;
};

// The Buffer class is a little unique - it's basically a black hole buffer with a constantly moving window of available data.
// Data that's been acquired eventually disappears and becomes inaccessible; the fact that you can access index 400 doesn't imply there are 400 elements allocated.

// Each buffer holds a block of data and a pointer to the buffer that came before it.
// When indexing into the buffer, it checks if that index falls within the section of the buffer this instance is holding.
// If it is, it directly indexes that buffer. If it isn't, it recurses into the previous buffer and accesses that one.
// When all items in a given buffer have been acquired, it's marked for deletion and then deleted by GC when nothing is referencing it.

// Theoretically this could get expensive if accessing early members of the array, but, so long as the acquirers are not thousands of times slower than the producers,
// and so long as the block size is reasonably large (i.e., not like 1 or 10), almost all reads will come from the directly owned part of the array and not recurse.
// So this should be very efficient 99.99% of the time.
template<typename T, size_t blockSize>
class sprawl::collections::detail::Buffer
{
public:
	struct BufferElement
	{
		std::atomic<ElementState> state;
		union
		{
			size_t retry;
			T item;
		} data;
	};

	// Initial constructor, this creates a buffer that owns its entire range.
	Buffer()
		: firstPart(nullptr)
		, refCount(blockSize)
		, capacity(blockSize)
		, amalgam(false)
		, parent(nullptr)
	{
		memset(buffer, 0, sizeof(BufferElement) * capacity);
	}

	// Amalgam constructor, this creates an array that only owns the last `blockSize` portion of the array.
	Buffer(Buffer* growFrom)
		: firstPart(growFrom)
		, refCount(blockSize)
		, capacity(growFrom->capacity + blockSize)
		, amalgam(true)
		, parent(nullptr)
	{
		growFrom->parent = this;
		memset(buffer, 0, sizeof(BufferElement) * blockSize);
	}

	// Get for add, simple case: this will never ever hit a buffer marked for delete because
	// the fact that an index is being used for add indicates the buffer has free space and thus
	// cannot possibly have been fully acquired
	inline BufferElement& GetForAdd(size_t index, LocalBuffer<T, blockSize>& bufObtainedFrom)
	{
		if(SPRAWL_LIKELY(amalgam))
		{
			// Amalgam case: if the index is in the section of the buffer we don't own, recurse.
			size_t unownedSection = capacity - blockSize;
			if(SPRAWL_UNLIKELY(index < unownedSection))
			{
				bufObtainedFrom = firstPart;
				return firstPart->GetForAdd(index, bufObtainedFrom);
			}
			return buffer[index - unownedSection];
		}
		// Non-amalgam case, index directly into the buffer.
		return buffer[index];
	}

	// Getting a spot for removal is more complicated than getting it for add
	// The main source of the complication is that the buffer we're trying to read from may
	// already have been fully acquired and marked for deletion. If that happens
	// we can't allow the reader to reference it at all or we have a dangling pointer.
	inline bool GetForRemove(size_t index, LocalBuffer<T, blockSize>& bufObtainedFrom, BufferElement*& element)
	{
		if(SPRAWL_LIKELY(amalgam))
		{
			// Amalgam case: if the index is in the section of the buffer we don't own, recurse.
			size_t unownedSection = capacity - blockSize;
			if(SPRAWL_UNLIKELY(index < unownedSection))
			{
				// HOWEVER, if the buffer is null, return false!
				// The buffer can be null for two reasons:
				// 1) Ref count reached 0 in another thread while we were taking the reference, which prevents us from taking it
				// 2) It has been fully deleted already and our firstPart variable was actually set to null
				// In either of this cases, the caller just has to re-acquire its read index and try again.
				if(SPRAWL_UNLIKELY(*firstPart == nullptr))
				{
					return false;
				}
				// We do have a buffer to recurse into here, so recurse
				bufObtainedFrom = firstPart;
				return firstPart->GetForRemove(index, bufObtainedFrom, element);
			}
			element = &buffer[index - unownedSection];
			return true;
		}
		element = &buffer[index];
		return true;
	}
	
	SPRAWL_FORCEINLINE void IncRef()
	{
		refCount.fetch_add(1, std::memory_order_relaxed);
	}

	SPRAWL_FORCEINLINE int DecRef()
	{
		auto ret = refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
		if(ret == 1)
		{
			LocalBuffer<T, blockSize> parentGuard = parent;
			Buffer* parentBuffer = *parentGuard;
			if(parentBuffer)
			{
				parentBuffer->firstPart = nullptr;
			}
		}
		return ret;
	}

	void Cleanup();

	~Buffer();

	SPRAWL_FORCEINLINE size_t Capacity() const { return capacity; }

private:
	SPRAWL_PAD_CACHELINE;
	SharedBuffer<T, blockSize> firstPart;
	SPRAWL_PAD_CACHELINE;
	std::atomic<int> refCount;
	SPRAWL_PAD_CACHELINE;
	SharedBuffer<T, blockSize> parent;
	SPRAWL_PAD_CACHELINE;

	BufferElement buffer[blockSize];
	size_t const capacity;
	bool const amalgam;
};

template<typename T, size_t blockSize>
inline void sprawl::collections::detail::Buffer<T, blockSize>::Cleanup()
{
	typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
	// Delete firstPart if we have it. This is not in the destructor because it only gets called when cleaning up the whole queue.
	// Otherwise all firstParts should just be cleared by GC
	Buffer* first = *firstPart;
	if(first)
	{
		first->Cleanup();
	}
}

template<typename T, size_t blockSize>
inline sprawl::collections::detail::Buffer<T, blockSize>::~Buffer()
{
	// Set the parent's firstPart to null before this memory's cleaned up.
	if(SPRAWL_LIKELY(*parent != nullptr))
	{
		parent->firstPart = nullptr;
	}
}

template<typename T, size_t blockSize>
class sprawl::collections::ConcurrentQueue
{
	typedef detail::Buffer<T, blockSize> Buffer;
	typedef detail::LocalBuffer<T, blockSize> LocalBuffer;
	typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;

	inline typename Buffer::BufferElement& getNextElement_(size_t& pos, LocalBuffer& bufObtainedFrom)
	{
		bufObtainedFrom = m_buffer;
		Buffer* buffer = *bufObtainedFrom;
		// Reserve a position before doing anything.
		// Incrementing before storing is safe because of the "state" value in each item that gets set later
		// and doing it with a fetch_add here avoids CAS
		pos = m_writePos.fetch_add(1, std::memory_order_seq_cst);

		// The buffer will return null if we happened to grab a buffer that's been filled, completely acquired, and is set for deallocation.
		// That's a very unlikely case.
		// Alternatively, if the position is greater than the capacity, we need to grow the buffer.
		while(SPRAWL_UNLIKELY(buffer == nullptr || pos >= buffer->Capacity()))
		{
			// Only one thread should perform the reallocation, the others should wait. CAS on this bool is used to pick the thread.
			// If it's already captured by another thread, it'll return false because m_reallocatingBuffer won't match the expected 'false' value
			if(SPRAWL_UNLIKELY(!m_reallocatingBuffer.exchange(true, std::memory_order_seq_cst)))
			{
				// Won the reallocation lottery!
				// If we got in here, one of two things has happened:
				// 1) This thread picked to reallocate the buffer
				// 2) Another thread was picked AND FINISHED between the check in the while loop and the CAS
				// To distinguish the two, load the buffer again and double-check that pos is >= capacity.
				bufObtainedFrom = m_buffer;
				buffer = *bufObtainedFrom;
				size_t capacity = buffer->Capacity();
				if(pos >= capacity)
				{
					//Reallocate the buffer (see the Buffer implementation for details on why the old one is passed to it.
					Buffer* newBuffer = static_cast<Buffer*>(bufferAlloc::alloc());
					new(newBuffer) Buffer(buffer);
					m_buffer = newBuffer;
				}
				//We're done reallocating, clear the lottery flag.
				m_reallocatingBuffer.store(false, std::memory_order_release);
			}
			else
			{
				// Let another thread move forward so this one's not just looping endlessly.
				sprawl::this_thread::Yield();
			}
			// Whether we reallocated or not, we need to reacquire the buffer now.
			bufObtainedFrom = m_buffer;
			buffer = *bufObtainedFrom;
		}
		auto& ret = buffer->GetForAdd(pos, bufObtainedFrom);
		return ret;
	}
public:

	ConcurrentQueue()
		: m_reallocatingBuffer(false)
		, m_buffer(nullptr)
		, m_readPos(0)
		, m_writePos(0)
		, m_readGuard(false)
	{
		Buffer* buffer = static_cast<Buffer*>(bufferAlloc::alloc());
		new(buffer) Buffer();
		m_buffer = buffer;
	}

	~ConcurrentQueue()
	{
		Buffer* buf = *m_buffer;
		buf->Cleanup();
	}

	inline void Enqueue(T const& val)
	{
		size_t pos;
		LocalBuffer buffer;
		Buffer::BufferElement& element = getNextElement_(pos, buffer);
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.data.item) T(val);
		element.state.store(detail::ElementState::READY, std::memory_order_release);
	}

	inline void Enqueue(T&& val)
	{
		size_t pos;
		LocalBuffer buffer;
		Buffer::BufferElement& element = getNextElement_(pos, buffer);
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.data.item) T(std::move(val));
		element.state.store(detail::ElementState::READY, std::memory_order_release);
	}

	inline bool Dequeue(T& val)
	{
		Buffer::BufferElement* element;
		LocalBuffer guard = m_buffer;
		Buffer* buffer = *guard;
		size_t pos = m_readPos.fetch_add(1, std::memory_order_acq_rel);
		for(;;)
		{
			size_t writePos = m_writePos.load(std::memory_order_acquire);
			if(pos >= writePos)
			{
				size_t insertPos;
				for(;;)
				{
					Buffer::BufferElement& element = getNextElement_(insertPos, guard);
					buffer = *guard;
					if(insertPos < pos)
					{
						// Not going to tell slot 1 to redirect to slot 2, but we do need to still redirect to 2.
						// Try again until we get a slot that's capable of being read - i.e., after our read position.
						// This handles a case of two read threads getting slots 1 and 2 for read, then writing redirects
						// to slots 2 and 1, respectively. In this case, they're both dead slots but they don't get consumed.
						// By detecting this, instead of 1 <-> 2, we get 3 -> 2 -> 1.
						element.state.store(detail::ElementState::DEAD, std::memory_order_release);
						buffer->DecRef();
						continue;
					}

					if(insertPos == pos)
					{
						buffer->DecRef();
					}
					else
					{
						element.data.retry = pos;
						element.state.store(detail::ElementState::RETRY, std::memory_order_release);
					}
					return false;
				}
			}

			while(SPRAWL_UNLIKELY(buffer == nullptr) || pos >= buffer->Capacity())
			{
				//In the middle of an exchange, try again RIGHT NOW, don't return.
				guard = m_buffer;
				buffer = *guard;
			}

			if(!buffer->GetForRemove(pos, guard, element))
			{
				guard = m_buffer;
				buffer = *guard;
				pos = m_readPos.fetch_add(1, std::memory_order_acq_rel);
				continue;
			}
			buffer = *guard;

			detail::ElementState state;
			while(SPRAWL_UNLIKELY((state = element->state.load(std::memory_order_acquire)) == detail::ElementState::EMPTY))
			{
				//Wait for write to finish
			}

			if(state == detail::ElementState::RETRY)
			{
				pos = element->data.retry;
				buffer->DecRef();
				continue;
			}
			else if(state == detail::ElementState::DEAD)
			{
				pos = m_readPos.fetch_add(1, std::memory_order_acq_rel);
				continue;
			}

			break;
		}

		/*size_t pos = m_readPos.load(std::memory_order_acquire);
		Buffer::BufferElement* element;
		Buffer* buffer;
		for(;;)
		{
			size_t writePos = m_writePos.load(std::memory_order_acquire);
			if(pos >= writePos)
			{
				return false;
			}
			
			buffer = m_buffer.load(std::memory_order_acquire);

			if(SPRAWL_UNLIKELY(buffer == nullptr) || pos >= buffer->Capacity())
			{
				return false;
			}

			if(!buffer->GetForRemove(pos, buffer, element))
			{
				return false;
			}

			if(element->state.load(std::memory_order_acquire) == detail::ElementState::EMPTY)
			{
				return false;
			}

			if(!m_readPos.compare_exchange_weak(pos, pos + 1, std::memory_order_release))
			{
				pos = m_readPos.load(std::memory_order_acquire);
				continue;
			}
			break;
		}*/

		/*{
			TinySpinlock lock(m_readGuard);
			// Obtain our read position.
			// If we advance past the end of what's written we need to revert our acquire atomically with the acquire
			// Otherwise bad things will happen.
			// That's why this is in a tiny spin lock guard.
			pos = m_readPos.fetch_add(1, std::memory_order_seq_cst);
			size_t writePos = m_writePos.load(std::memory_order_acquire);
			if(pos >= writePos) 
			{
				// Whoops, can't use this slot yet! Revert, revert! Criss-cross!
				m_readPos.store(pos, std::memory_order_release);
				return false;
			}
		}

		Buffer::BufferElement* element;

		Buffer* buffer = m_buffer.load(std::memory_order_acquire);

		while(SPRAWL_UNLIKELY(buffer == nullptr) || pos >= buffer->Capacity())
		{
			//In the middle of an exchange, try again RIGHT NOW, don't return.
			buffer = m_buffer.load(std::memory_order_acquire);
		}

		buffer->GetForRemove(pos, buffer, element);

		//Now we've gotten our item. We should be the ONLY ones to have this item.
		//If it hasn't been written to yet, give it a moment...
		while(element->state.load(std::memory_order_acquire) == detail::ElementState::EMPTY)
		{}*/

		val = std::move(element->data.item);
		element->data.item.~T();

		// If we are the last to consume a block, then all blocks have been fully read and
		// it is safe to free this memory.
		buffer->DecRef();
		return true;
	}

private:

	SPRAWL_PAD_CACHELINE;
	std::atomic<bool> m_reallocatingBuffer;
	SPRAWL_PAD_CACHELINE;
	detail::SharedBuffer<T, blockSize> m_buffer;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_readPos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_writePos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<bool> m_readGuard;
	SPRAWL_PAD_CACHELINE;
};
