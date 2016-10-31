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
				EMPTY, // No value written yet
				READY, // A value has been written
				RETRY, // Rather than a value, this element contains a redirect to another element that wasn't successfully read before.
				DEAD, // This element would have been a retry to an element after it, which would have made both unreachable, 
				      // so this one was just killed and another retry was added.
			};

			template<typename T, size_t blockSize>
			class Buffer;
		}

		template<typename T, size_t blockSize = 8192>
		class ConcurrentQueue;
	}
}

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
		, refCount(blockSize + 1)
		, capacity(blockSize)
		, amalgam(false)
		, parent(nullptr)
	{
		memset(buffer, 0, sizeof(BufferElement) * capacity);
	}

	// Amalgam constructor, this creates an array that only owns the last `blockSize` portion of the array.
	Buffer(Buffer* growFrom)
		: firstPart(growFrom)
		, refCount(blockSize + 2)
		, capacity(growFrom->capacity + blockSize)
		, amalgam(true)
		, parent(nullptr)
	{
		growFrom->parent.store(this, std::memory_order_release);
		memset(buffer, 0, sizeof(BufferElement) * blockSize);
	}

	// Get for add, simple case: this will never ever hit a buffer marked for delete because
	// the fact that an index is being used for add indicates the buffer has free space and thus
	// cannot possibly have been fully acquired
	inline BufferElement& GetForAdd(size_t index, Buffer*& bufObtainedFrom)
	{
		if(SPRAWL_LIKELY(amalgam))
		{
			// Amalgam case: if the index is in the section of the buffer we don't own, recurse.
			size_t unownedSection = capacity - blockSize;
			if(SPRAWL_UNLIKELY(index < unownedSection))
			{
				Buffer* first = firstPart.load(std::memory_order_acquire);
				return first->GetForAdd(index, bufObtainedFrom);
			}
			bufObtainedFrom = this;
			return buffer[index - unownedSection];
		}
		bufObtainedFrom = this;
		// Non-amalgam case, index directly into the buffer.
		return buffer[index];
	}

	// Getting a spot for removal is more complicated than getting it for add
	// The main source of the complication is that the buffer we're trying to read from may
	// already have been fully acquired and marked for deletion. If that happens
	// we can't allow the reader to reference it at all or we have a dangling pointer.

	// Technically due to changes in Dequeue(), the same comment from GetForAdd applies and
	// there's no longer actually any reason to check existence and return false. But,
	// it's a nice extra assertion to have. The above comment is slightly inaccurate now,
	// but maintained for historacle reference on why this returns a bool.

	// Eventually, once the assumption that this can never return false is verified,
	// GetForAdd and GetForRemove will merge into just Get
	inline bool GetForRemove(size_t index, Buffer*& bufObtainedFrom, BufferElement*& element)
	{
		if(SPRAWL_LIKELY(amalgam))
		{
			// Amalgam case: if the index is in the section of the buffer we don't own, recurse.
			size_t unownedSection = capacity - blockSize;
			if(SPRAWL_UNLIKELY(index < unownedSection))
			{
				// HOWEVER, if the buffer is null, return false!
				// The buffer can be null only if it's been deleted.
				// If this is ever actually null, there's a critical logic error.
				Buffer* first = firstPart.load(std::memory_order_acquire);
				if(SPRAWL_UNLIKELY(first == nullptr))
				{
					return false;
				}
				// We do have a buffer to recurse into here, so recurse
				return first->GetForRemove(index, bufObtainedFrom, element);
			}
			bufObtainedFrom = this;
			element = &buffer[index - unownedSection];
			return true;
		}
		bufObtainedFrom = this;
		element = &buffer[index];
		return true;
	}

	// This isn't reference counted in terms of how many paths have access to it.
	// The reference count here is:
	// <blockSize> references to count how many blocks have not yet been read (cannot delete this until they are)
	// 1 reference for the ConcurrentQueue's m_buffer
	// 1 reference for amalgam buffers for the parent/firstPart recursive reference
	// Once all blocks are read and m_buffer points at something else, and this buffer's firstPart has been deleted,
	// this buffer will also be deleted.
	SPRAWL_FORCEINLINE int DecRef()
	{
		return refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
	}

	void Cleanup();

	~Buffer();

	SPRAWL_FORCEINLINE size_t Capacity() const { return capacity; }

	SPRAWL_FORCEINLINE Buffer* GetParent() { return parent.load(std::memory_order_acquire); }

	SPRAWL_FORCEINLINE void ReleaseFirst() { firstPart.store(nullptr, std::memory_order_release); }

private:
	SPRAWL_PAD_CACHELINE;
	std::atomic<Buffer*> firstPart;
	SPRAWL_PAD_CACHELINE;
	std::atomic<int> refCount;
	SPRAWL_PAD_CACHELINE;
	std::atomic<Buffer*> parent;
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
	Buffer* first = firstPart.load(std::memory_order_acquire);
	if(first)
	{
		first->Cleanup();
		first->~Buffer();
		bufferAlloc::free(first);
	}
}

template<typename T, size_t blockSize>
inline sprawl::collections::detail::Buffer<T, blockSize>::~Buffer()
{
	// Set the parent's firstPart to null before this memory's cleaned up.
}

template<typename T, size_t blockSize>
class sprawl::collections::ConcurrentQueue
{
	typedef detail::Buffer<T, blockSize> Buffer;
	typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;

	inline static void consume_(Buffer* buffer)
	{
		// Decrement a buffer's reference count, and if it's 0, delete it.
		// If it has a parent, release that parent's first part.
		// Then repeat on the parent, decrementing reference and checking if ITS parent
		// exists. This is done in a loop rather than having the destructor do it
		// to absolutely ensure that even if a user makes a queue with a tiny
		// block size and lots and lots of blocks, there will never be
		// a stack overflow from destructors recursively deleting other buffers.
		while(buffer && buffer->DecRef() == 0)
		{
			typedef memory::PoolAllocator<sizeof(Buffer)> bufferAlloc;
			Buffer* parent = buffer->GetParent();
			if(SPRAWL_LIKELY(parent != nullptr))
			{
				parent->ReleaseFirst();
			}
			buffer->~Buffer();
			bufferAlloc::free(buffer);
			buffer = parent;
		}
	}

	inline typename Buffer::BufferElement& getNextElement_(size_t& pos, Buffer*& bufObtainedFrom)
	{
		// Reserve a position before doing anything.
		// Incrementing before storing is safe because of the "state" value in each item that gets set later
		// and doing it with a fetch_add here avoids CAS
		pos = m_writePos.fetch_add(1, std::memory_order_seq_cst);

		// If the position is greater than the capacity, we need to grow the buffer.
		while(SPRAWL_UNLIKELY(pos >= m_capacity.load(std::memory_order_acquire)))
		{
			// Only one thread should perform the reallocation, the others should wait. exchange on this bool is used to pick the thread.
			// If it's already captured by another thread, it'll return its previous value of true, triggering a reallocation
			// This is technically a spin-lock and perhaps breaks strict lock-free, but happens rarely enough to not matter
			if(SPRAWL_UNLIKELY(!m_reallocatingBuffer.exchange(true, std::memory_order_seq_cst)))
			{
				// Won the reallocation lottery!
				// If we got in here, one of two things has happened:
				// 1) This thread picked to reallocate the buffer
				// 2) Another thread was picked AND FINISHED between the check in the while loop and the exchange()
				// To distinguish the two, we need to check capacity again (double-check lock pattern)

				size_t capacity = m_capacity.load(std::memory_order_acquire);
				if(pos >= capacity)
				{
					//Reallocate the buffer (see the Buffer implementation for details on why the old one is passed to it.
					Buffer* newBuffer = static_cast<Buffer*>(bufferAlloc::alloc());
					Buffer* buffer = m_buffer.load(std::memory_order_acquire);
					new(newBuffer) Buffer(buffer);
					consume_(buffer);
					m_buffer.store(newBuffer, std::memory_order_release);
					m_capacity.store(newBuffer->Capacity(), std::memory_order_release);
				}
				//We're done reallocating, clear the lottery flag.
				m_reallocatingBuffer.store(false, std::memory_order_release);
			}
			else
			{
				// Let another thread move forward so this one's not just looping endlessly.
				sprawl::this_thread::Yield();
			}
		}
		// Fetch the buffer and get the position from it.
		Buffer* buffer = m_buffer.load(std::memory_order_acquire);
		return buffer->GetForAdd(pos, bufObtainedFrom);
	}
public:

	ConcurrentQueue()
		: m_reallocatingBuffer(false)
		, m_buffer(nullptr)
		, m_capacity(blockSize)
		, m_readPos(0)
		, m_writePos(0)
		, m_readGuard(false)
#ifdef SPRAWL_CONCURRENTQUEUE_COUNT_READ_MISSES
		, m_readMisses(0)
#endif
	{
		Buffer* buffer = static_cast<Buffer*>(bufferAlloc::alloc());
		new(buffer) Buffer();
		m_buffer.store(buffer, std::memory_order_release);
	}

	~ConcurrentQueue()
	{
		Buffer* buf = m_buffer.load(std::memory_order_acquire);
		buf->Cleanup();
		buf->~Buffer();
		bufferAlloc::free(buf);
	}

	inline void Enqueue(T const& val)
	{
		// pos and buffer are dummy elements because only Dequeue needs that information.
		size_t pos;
		Buffer* buffer;
		Buffer::BufferElement& element = getNextElement_(pos, buffer);
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.data.item) T(val);
		element.state.store(detail::ElementState::READY, std::memory_order_release);
	}

	inline void Enqueue(T&& val)
	{
		// pos and buffer are dummy elements because only Dequeue needs that information.
		size_t pos;
		Buffer* buffer;
		Buffer::BufferElement& element = getNextElement_(pos, buffer);
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.data.item) T(std::move(val));
		element.state.store(detail::ElementState::READY, std::memory_order_release);
	}

	inline bool Dequeue(T& val)
	{
		Buffer::BufferElement* element;
		Buffer* buffer;
		// Do a fetch-add to reserve a position to read from. We'll be the only thread reading.
		// This is a little less safe than the write case because we could get a slot that hasn't been written yet.
		// In that case we can't read it, but we also can't safely decrement the readPos back for a variety of reasons!
		// We'll handle that below.
		size_t pos = m_readPos.fetch_add(1, std::memory_order_acq_rel);
		for(;;)
		{
			size_t writePos = m_writePos.load(std::memory_order_acquire);
			if(pos >= writePos)
			{
#ifdef SPRAWL_CONCURRENTQUEUE_COUNT_READ_MISSES
				m_readMisses.fetch_add(1, std::memory_order_relaxed);
#endif
				// In this case, we have in fact gotten an element that's outside the readable range.
				// Rather than attempting to revert m_readPos, we're going to write a new value reminding the next reader thread to please
				// re-check this slot that we couldn't get.
				size_t insertPos;
				for(;;)
				{
					// Get an element as if we were writing... because we technically are!
					Buffer::BufferElement& element = getNextElement_(insertPos, buffer);
					if(insertPos < pos)
					{
						// Not going to tell slot 1 to redirect to slot 2, but we do need to still redirect to 2.
						// Try again until we get a slot that's capable of being read - i.e., after our read position.
						// This handles a case of two read threads getting slots 1 and 2 for read, then writing redirects
						// to slots 2 and 1, respectively. In this case, they're both dead slots but they don't get consumed.
						// By detecting this, instead of 1 <-> 2, we get 3 -> 2 -> 1.
						// We'll mark this slot dead and then go back to the start of the loop to write a new slot for this redirect.
						element.state.store(detail::ElementState::DEAD, std::memory_order_release);
						continue;
					}

					if(insertPos == pos)
					{
						// Pointless to redirect an element to itself. If we wanted to read 3 and we get told to write 3, well good,
						// we successfully read 3. We can just skip writing any actual data to it and mark this slot as having been consumed.
						consume_(buffer);
					}
					else
					{
						// Set the retry position so that whatever thread ends up reading this element will go back and look at the position
						// we were originally assigned to read here.
						element.data.retry = pos;
						element.state.store(detail::ElementState::RETRY, std::memory_order_release);
					}
					return false;
				}
			}

			while(SPRAWL_UNLIKELY(pos >= m_capacity.load(std::memory_order_acquire)))
			{
				//In the middle of an exchange, wait for it to complete.
			}

			// We now have an assigned position to read from.
			// We know it has been written to already.
			// We are the only one with this element and no one else has ever read it.
			// And the value in m_buffer contains enough data to contain at least the index we got.
			// Time to actually read it!
			buffer = m_buffer.load(std::memory_order_acquire);
			bool ret = buffer->GetForRemove(pos, buffer, element);
			//We're the only one with this element and we can guarantee that it's not been read before.
			//It had BETTER still be available!
			assert(ret);

			detail::ElementState state;
			while(SPRAWL_UNLIKELY((state = element->state.load(std::memory_order_acquire)) == detail::ElementState::EMPTY))
			{
				//Wait for write to finish
			}

			if(state == detail::ElementState::RETRY)
			{
				// We're being redirected to another element that another thread failed to read.
				// Go try to read it now.
				// Set our read position to the retry value and go back to the start to do another read at that index.
				pos = element->data.retry;
				consume_(buffer);
				continue;
			}
			else if(state == detail::ElementState::DEAD)
			{
				// This element is just flat-out dead. There's nothing to do with it.
				// Get a new index to read from and mark this one as having been finally consumed.
				// Then go back to the start to do another read at the new index.
				pos = m_readPos.fetch_add(1, std::memory_order_acq_rel);
				consume_(buffer);
				continue;
			}

			// Finally we've gotten an actual element with an actual value.
			// Return it back to the client.
			val = std::move(element->data.item);
			element->data.item.~T();

			consume_(buffer);
			return true;
		}
	}

#ifdef SPRAWL_CONCURRENTQUEUE_COUNT_READ_MISSES
	size_t NumReadMisses() { return m_readMisses.load(std::memory_order_acquire); }
#endif

private:

	SPRAWL_PAD_CACHELINE;
	std::atomic<bool> m_reallocatingBuffer;
	SPRAWL_PAD_CACHELINE;
	std::atomic<Buffer*> m_buffer;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_capacity;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_readPos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_writePos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<bool> m_readGuard;
	SPRAWL_PAD_CACHELINE;
#ifdef SPRAWL_CONCURRENTQUEUE_COUNT_READ_MISSES
	std::atomic<size_t> m_readMisses;
	SPRAWL_PAD_CACHELINE;
#endif
};
