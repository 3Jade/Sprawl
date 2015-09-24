#pragma once

#include <atomic>
#include <memory>
#include <string.h>
#include <assert.h>
#include "../common/compat.hpp"
#include "../common/CachePad.hpp"
#include "../threading/thread.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename T, size_t blockSize = 32>
		class ConcurrentQueue;
	}
}

template<typename T, size_t blockSize /*= 32*/>
class sprawl::collections::ConcurrentQueue
{
public:

	ConcurrentQueue()
		: m_reallocatingBuffer(false)
		, m_buffer(new Buffer(blockSize))
		, m_readPos(0)
		, m_writePos(0)
	{
		//
	}

	~ConcurrentQueue()
	{
		Buffer* buf = m_buffer.load(std::memory_order_relaxed);
		buf->Cleanup();
		delete buf;
	}

	void Enqueue(T const& val)
	{
		BufferElement& element = getNextElement_();
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.item) T(val);
		element.state.store(ElementState::READY, std::memory_order_release);
	}

	void Enqueue(T&& val)
	{
		BufferElement& element = getNextElement_();
		//Construct the item and then set "state" to "READY" - order matters.
		new(&element.item) T(std::move(val));
		element.state.store(ElementState::READY, std::memory_order_release);
	}

	bool Dequeue(T& val)
	{
		//Loop because we may have to try a few times to get a value.
		//Using a label instead of a for loop to avoid needing an extra boolean to get out of nested loop
		startLoop:
		RefCounter buffer(m_buffer.load(std::memory_order_acquire));

		while(SPRAWL_UNLIKELY(*buffer == nullptr))
		{
			//In the middle of an exchange, try again RIGHT NOW, don't return.
			buffer = m_buffer.load(std::memory_order_acquire);
		}

		// Get the current position to read from. We're not incrementing it yet because it may be empty
		// If that's the case, nothing has been added to the queue, so we return false.
		size_t pos = m_readPos.load(std::memory_order_acquire);
		size_t writePos = m_writePos.load(std::memory_order_acquire);
		if(SPRAWL_UNLIKELY(pos >= writePos || pos >= buffer->Capacity()))
		{
			return false;
		}

		size_t readPos = pos;

		Buffer* bufObtainedFrom;
		BufferElement* element;

		for(;pos < writePos; ++pos)
		{
			//Try to get an item for removal. It's possible this will fail due to the buffer we have being released.
			//If this fails we need to go back and reobtain the buffer and try again.
			if(SPRAWL_UNLIKELY(!buffer->GetForRemove(pos, bufObtainedFrom, element)))
			{
				//Break out of the inner loop and jump to the top of the outer loop - goto is more efficient than using a bool
				goto startLoop;
			}

			//If this fails, another thread beat us to the value at this position.
			//At this point we try again with the next value.
			ElementState expected = ElementState::READY;
			if(element->state.compare_exchange_strong(expected, ElementState::CONSUMED, std::memory_order_acq_rel))
			{
				break;
			}

			// If we find that the element in readPos has been consumed, we can increment m_readPos past it if another thread hasn't already done so
			// Then we can increment our own readPos value regardless - this is so that on the next pass through the loop, we can detect this case again.
			if(pos == readPos && expected == ElementState::CONSUMED)
			{
				size_t attemptIncrement = readPos;
				m_readPos.compare_exchange_strong(attemptIncrement, attemptIncrement + 1, std::memory_order_acq_rel);
				++readPos;
			}
		}

		if(pos == writePos)
		{
			//Ran out of positions to look at without managing to take ownership of any of them. Nothing we can return.
			return false;
		}

		//Now we've gotten our item. We should be the ONLY ones to have this item.

		//Hold onto the buffer, but tell it to be destroyed later.
		RefCounter counter(bufObtainedFrom);
		bufObtainedFrom->ConsumeBlock();

		val = std::move(element->item);
		element->item.~T();
		return true;
	}

private:

	enum class ElementState : int16_t
	{
		EMPTY,
		READY,
		CONSUMED
	};

	struct BufferElement
	{
		std::atomic<ElementState> state;
		T item;
	};

	inline BufferElement& getNextElement_()
	{
		RefCounter buffer(m_buffer.load(std::memory_order_acquire));
		// Reserve a position before doing anything.
		// Incrementing before storing is safe because of the "state" value in each item that gets set later
		// and doing it with a fetch_add here avoids CAS
		size_t pos = m_writePos.fetch_add(1, std::memory_order_acq_rel);

		// The buffer will return null if we happened to grab a buffer that's been filled, completely acquired, and is set for deallocation.
		// That's a very unlikely case.
		// Alternatively, if the position is greater than the capacity, we need to grow the buffer.
		while(SPRAWL_UNLIKELY(*buffer == nullptr || pos >= buffer->Capacity()))
		{
			// Only one thread should perform the reallocation, the others should wait. CAS on this bool is used to pick the thread.
			// If it's already captured by another thread, it'll return false because m_reallocatingBuffer won't match the expected 'false' value
			bool notReallocating = false;
			if(SPRAWL_UNLIKELY(m_reallocatingBuffer.compare_exchange_strong(notReallocating, true, std::memory_order_acq_rel)))
			{
				// Won the reallocation lottery!
				// If we got in here, one of two things has happened:
				// 1) This thread picked to reallocate the buffer
				// 2) Another thread was picked AND FINISHED between the check in the while loop and the CAS
				// To distinguish the two, load the buffer again and double-check that pos is >= capacity.
				buffer = m_buffer.load(std::memory_order_acquire);
				size_t capacity = buffer->Capacity();
				if(pos >= capacity)
				{
					//Reallocate the buffer (see the Buffer implementation for details on why the old one is passed to it.
					Buffer* newBuffer = new Buffer(*buffer);
					m_buffer.store(newBuffer, std::memory_order_release);
					buffer->DecRef();
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
			buffer = m_buffer.load(std::memory_order_acquire);
		}
		return buffer->GetForAdd(pos);
	}

	class Buffer;

	// Reference counter class to handle GC so we can safely delete buffers.
	struct RefCounter
	{
		inline RefCounter(Buffer* buf)
			: m_buf(buf)
		{
			if(SPRAWL_LIKELY(m_buf))
			{
				m_buf->IncRef();
			}
		}

		inline ~RefCounter()
		{
			if(SPRAWL_LIKELY(m_buf))
			{
				if(SPRAWL_UNLIKELY(m_buf->DecRef() == 0))
				{
					delete m_buf;
				}
			}
		}

		Buffer* operator->() { return m_buf; }
		Buffer* operator*() { return m_buf; }

		RefCounter(RefCounter const& other) = delete;
		RefCounter(RefCounter&& other) = delete;

		inline RefCounter& operator=(Buffer* other)
		{
			if(SPRAWL_LIKELY(other))
			{
				other->IncRef();
			}
			if(SPRAWL_LIKELY(m_buf))
			{
				if(SPRAWL_UNLIKELY(m_buf->DecRef() == 0))
				{
					delete m_buf;
				}
			}
			m_buf = other;
			return *this;
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
	class Buffer
	{
	public:
		// Initial constructor, this creates a buffer that owns its entire range.
		Buffer(size_t capacity_)
			: firstPart(nullptr)
			, refCount(2)
			, usedBlocks(capacity_)
			, buffer((BufferElement*)malloc(sizeof(BufferElement) * capacity_))
			, capacity(capacity_)
			, amalgam(false)
			, parent(nullptr)
		{
			memset(buffer, 0, sizeof(BufferElement) * capacity);
		}

		// Amalgam constructor, this creates an array that only owns the last `blockSize` portion of the array.
		Buffer(Buffer* growFrom)
			: firstPart(growFrom)
			, refCount(2)
			, usedBlocks(blockSize)
			, buffer((BufferElement*)malloc(sizeof(BufferElement)*growFrom->capacity))
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
		inline BufferElement& GetForAdd(size_t index)
		{
			if(SPRAWL_LIKELY(amalgam))
			{
				// Amalgam case: if the index is in the section of the buffer we don't own, recurse.
				size_t unownedSection = capacity - blockSize;
				if(SPRAWL_UNLIKELY(index < unownedSection))
				{
					// No need to ref count here. Also no need for a strong memory order here because we trust this hasn't been changed since it was initialized for this case.
					return firstPart.load(std::memory_order_relaxed)->GetForAdd(index);
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
		inline bool GetForRemove(size_t index, Buffer*& bufObtainedFrom, BufferElement*& element)
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
					RefCounter counter(firstPart.load(std::memory_order_acquire));
					if(SPRAWL_UNLIKELY(*counter == nullptr))
					{
						return false;
					}
					// We do have a buffer to recurse into here, so recurse
					return counter->GetForRemove(index, bufObtainedFrom, element);
				}
				bufObtainedFrom = this;
				element = &buffer[index - unownedSection];
				return true;
			}
			bufObtainedFrom = this;
			element = &buffer[index];
			return true;
		}

		SPRAWL_FORCEINLINE void ConsumeBlock()
		{
			// Decrement the used block count. If the used block count reahes 0, release the last reference we're holding to mark this for deletion by GC.
			if(SPRAWL_UNLIKELY(usedBlocks.fetch_sub(1, std::memory_order_acq_rel) == 0))
			{
				DecRef();
			}
		}

		void Cleanup()
		{
			// Delete firstPart if we have it. This is not in the destructor because it only gets called when cleaning up the whole queue.
			// Otherwise all firstParts should just be cleared by GC
			Buffer* first = firstPart;
			if(first)
			{
				first->Cleanup();
				delete first;
			}
		}

		~Buffer()
		{
			// Set the parent's firstPart to null before this memory's cleaned up.
			if(SPRAWL_LIKELY(*parent != nullptr))
			{
				parent->firstPart.store(nullptr, std::memory_order_release);
			}

			free(buffer);
		}

		SPRAWL_FORCEINLINE size_t Capacity() { return capacity; }

		SPRAWL_FORCEINLINE void IncRef()
		{
			refCount.fetch_add(1, std::memory_order_acq_rel);
		}

		SPRAWL_FORCEINLINE int DecRef()
		{
			//DecRef is a simple fetch_sub.
			return refCount.fetch_sub(1, std::memory_order_acq_rel);
		}

	private:
		SPRAWL_PAD_CACHELINE;
		std::atomic<Buffer*> firstPart;
		SPRAWL_PAD_CACHELINE;
		std::atomic<int> refCount;
		SPRAWL_PAD_CACHELINE;
		std::atomic<int> usedBlocks;
		SPRAWL_PAD_CACHELINE;

		BufferElement* const buffer;
		size_t const capacity;
		bool const amalgam;
		RefCounter parent;
	};

	SPRAWL_PAD_CACHELINE;
	std::atomic<bool> m_reallocatingBuffer;
	SPRAWL_PAD_CACHELINE;
	std::atomic<Buffer*> m_buffer;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_readPos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_writePos;
	SPRAWL_PAD_CACHELINE;
};
