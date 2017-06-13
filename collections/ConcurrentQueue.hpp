#pragma once

#include <atomic>
#include <memory>
#include <string.h>
#include "../common/compat.hpp"
#include "../common/errors.hpp"
#include "../common/CachePad.hpp"

#ifndef SPRAWL_CONCURRENT_QUEUE_DEBUG_ASSERTS
#	define SPRAWL_CONCURRENT_QUEUE_DEBUG_ASSERTS 0
#endif

#if SPRAWL_CONCURRENT_QUEUE_DEBUG_ASSERTS
#define SPRAWL_CONCURRENT_QUEUE_ASSERT(val) if(!(val)) { SPRAWL_ABORT_MSG_F("Assertion failed: %s", #val); }
#else
#define SPRAWL_CONCURRENT_QUEUE_ASSERT(val)
#endif

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename t_ElementType, size_t t_BlockSize>
			class Buffer;

			// These functions collectively find the next power of 2 of a number
			// Which allows modulus using the faster & rather than %.
			constexpr uint32_t pow2_16(uint32_t x) { return (x | x >> 16) + 1; }
			constexpr uint32_t pow2_8(uint32_t x) { return pow2_16(x | x >> 8); }
			constexpr uint32_t pow2_4(uint32_t x) { return pow2_8(x | x >> 4); }
			constexpr uint32_t pow2_2(uint32_t x) { return pow2_4(x | x >> 2); }
			constexpr uint32_t pow2_1(uint32_t x) { return pow2_2(x | x >> 1); }

			constexpr uint64_t pow2_32(uint64_t x) { return (x | x >> 32) + 1; }
			constexpr uint64_t pow2_16(uint64_t x) { return pow2_32(x | x >> 16); }
			constexpr uint64_t pow2_8(uint64_t x) { return pow2_16(x | x >> 8); }
			constexpr uint64_t pow2_4(uint64_t x) { return pow2_8(x | x >> 4); }
			constexpr uint64_t pow2_2(uint64_t x) { return pow2_4(x | x >> 2); }
			constexpr uint64_t pow2_1(uint64_t x) { return pow2_2(x | x >> 1); }

			constexpr uint64_t nextPowerOf2(uint64_t x)
			{
				return pow2_1(x - 1);
			}

			constexpr uint32_t nextPowerOf2(uint32_t x)
			{
				return pow2_1(x - 1);
			}

			static_assert(nextPowerOf2(uint64_t(1)) == 1, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(2)) == 2, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(3)) == 4, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(4)) == 4, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(5)) == 8, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(9)) == 16, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(17)) == 32, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(1025)) == 2048, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint64_t(32000)) == 32768, "nextPowerOf2 failed");

			static_assert(nextPowerOf2(uint32_t(1)) == 1, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(2)) == 2, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(3)) == 4, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(4)) == 4, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(5)) == 8, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(9)) == 16, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(17)) == 32, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(1025)) == 2048, "nextPowerOf2 failed");
			static_assert(nextPowerOf2(uint32_t(32000)) == 32768, "nextPowerOf2 failed");
		}

		template<typename t_ElementType, size_t t_BlockSize = 8192, typename t_AllocatorType = std::allocator<t_ElementType>>
		struct ReadReservationTicket;

		template<typename t_ElementType>
		struct BoundedReadReservationTicket;

		template<typename t_ElementType>
		struct BoundedWriteReservationTicket;

		template<typename t_ElementType, size_t t_BlockSize = 8192, typename t_AllocatorType = std::allocator<t_ElementType>>
		class ConcurrentQueue;

		template<typename t_ElementType, size_t t_QueueSize, typename t_AllocatorType = std::allocator<t_ElementType>>
		class ConcurrentBoundedQueue;
	}
}

/**
 * @class   sprawl::collections::detail::Buffer
 * 
 * @brief   Simple buffer class representing a single allocated block within an unbounded concurrent queue.
 * 
 * @details Provides concurrent read and write support. This class is the one that actually handles
 *          the majority of the atomic operations, as the read and write position are both
 *          contained within this class.
 */
template<typename t_ElementType, size_t t_BlockSize>
class sprawl::collections::detail::Buffer
{
public:
	struct BufferElement
	{
		std::atomic<bool> ready;
		t_ElementType item;
	};

	Buffer()
		: m_next(nullptr)
		, m_refCount(t_BlockSize + 2) // One for each element, one for the writeBuffer pointer, and one for the readBuffer pointer
		, m_readPos(reinterpret_cast<BufferElement*>(m_buffer))
		, m_writePos(reinterpret_cast<BufferElement*>(m_buffer))
		, m_end(reinterpret_cast<BufferElement*>(m_buffer) + t_BlockSize)
	{
		// Memset the buffer block to 0 so all 'ready' flags read as 'false'
		memset(m_buffer, 0, sizeof(BufferElement) * t_BlockSize);
	}

	/**
	 * @brief   Reset a buffer back to its original state.
	 *          Does NOT reset the read and write positions, those are done by the functions below.
	 * 
	 * @details The way this works is like this:
	 *          When a buffer has been completely used up, rather than freeing it, it's marked for reuse later.
	 *          The reason for this is that freeing it isn't safe - it may still be accessed after or while it's
	 *          being freed. Instead, this algorithm takes used blocks and moves them to the end of the block list
	 *          to be used again later. When we start to write it again, we set the write position back to the start,
	 *          and likewise, when we start to read it again, we set the read position back to the start.
	 *          
	 *          This is safe even though it may be read again because the read and write positions aren't set.
	 *          When the read and write positions are obtained, one of two results can occur:
	 *          1) It will get a value that's past the end of the buffer and go to retrieve (or allocate) the next buffer
	 *          2) It will read it while or after the write/read position is reset, in which case this buffer is already ready
	 *          for use again and it gets a valid item that it's absolutely permitted to continue operating on.
	 *          
	 *          Either of these situations is fine, meaning that using this queue after it's been put on the back is never
	 *          a problem.
	 */
	inline void Clear()
	{
		m_refCount.store(t_BlockSize + 2);
		memset(m_buffer, 0, sizeof(BufferElement) * t_BlockSize);
	}

	/**
	 * @brief   Resets the write position
	 */
	inline void SetWritePosition()
	{
		m_writePos.store(reinterpret_cast<BufferElement*>(m_buffer));
	}

	/**
	* @brief   Resets the read position
	*/
	inline void SetReadPosition()
	{
		m_readPos.store(reinterpret_cast<BufferElement*>(m_buffer));
	}
	
	/**
	 * @brief   Set the next pointer for this buffer.
	 * 
	 * @details The caller (the enclosing queue) is responsible for detecting when
	 *          the buffer element it's received is outside the bounds of the buffer,
	 *          allocating a new buffer in a synchronized way, and then setting the next
	 *          pointer on the current buffer. This burden is placed on the caller for
	 *          performance reasons.
	 *          
	 * @param   next   The pointer to a newly allocated buffer
	 */
	inline void SetNext(Buffer* next)
	{
		m_next.store(next, std::memory_order_release);
	}
	
	/**
	 * @brief   Get the next pointer for this buffer.
	 * 
	 * @details Like with SetNext, the caller is responsible for detecting when the element
	 *          they received is outside the boundaries of the buffer and retrieving the next buffer.
	 *          
	 * @return  Pointer to the next buffer. If there is no next, returns nullptr.
	 */
	inline Buffer* GetNext()
	{
		return m_next.load(std::memory_order_acquire);
	}
	
	/**
	 * @brief   Retrieve a pointer to an element for dequeue.
	 * 
	 * @details This function is thread-safe and is guaranteed to return an element reserved
	 *          for only the caller. There's no need to synchronize access to this element.
	 *          However, this element is NOT guaranteed to actually have valid data stored in it yet.
	 *          It's the responsibility of the caller to check the 'ready' flag on the element and
	 *          to handle the case where it's not yet ready. Also, the buffer does not support
	 *          putting an element back in the read queue - once an element is retrieved for read,
	 *          it must be read as retrieved, as that element will never be returned from this
	 *          method again.
	 *          
	 *          Additionally, note that the returned pointer may be beyond the end of the buffer,
	 *          and it is the responsibility of the caller to handle that case by retrieving the next
	 *          buffer.
	 * 
	 * @return  A pointer to an element. If the pointer is < this->GetEnd(), it is valid to read from.
	 */
	inline BufferElement* GetForRead()
	{
		return m_readPos.fetch_add(1, std::memory_order_acq_rel);
	}
	
	/**
	 * @brief   Retrieve a pointer to an element for enqueue.
	 * 
	 * @details This function is thread-safe and is guaranteed to return an element reserved
	 *          for only the caller. There's no need to synchronize access to this element.
	 *          Note, however, that the returned pointer may be beyond the end of the buffer,
	 *          and it is the responsibility of the caller to handle that case by allocating a new
	 *          buffer.
	 *          
	 * @return  A pointer to an element. If the pointer is < this->GetEnd(), it is valid to write to.
	 */
	inline BufferElement* GetForWrite()
	{
		return m_writePos.fetch_add(1, std::memory_order_acq_rel);
	}
	
	/**
	 * @brief   Get a pointer to the end of the queue. If a returned pointer is >= this value, it's not valid,
	 *          and a reallocation or call to GetNext() is required.
	 *          
	 * @return  A pointer to the end of the buffer.
	 */
	inline BufferElement const* GetEnd() const
	{
		return m_end;
	}

	/**
	 * @brief   Decrement the ref count.
	 * 
	 * @details This isn't a traditional reference count. Rather than dealing in terms of the number of current references,
	 *          this actually indicates the number of unread elements, plus 2 additional references for the writeBuffer and
	 *          readBuffer elements of the queue. Once all elements have been read and those two pointers are pointing at
	 *          something else, we know nothing else is pointing at this and it's safe to move it to the end of the buffer list - 
	 *          therefore we don't have to worry about incrementing the reference count ever.
	 * 
	 * @return  The new reference count after this operation has completed. If the result is 0, the buffer should be moved to the end of the buffer list.
	 */
	inline int DecRef()
	{
		return m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
	}

	/**
	 * @brief   Special version of DecRef that will decrease the reference count multiple times with a single atomic operation
	 * 
	 * @param   amount   the amount by which to decrement the count
	 * @return  The new reference count after this operation has completed. If the result is 0, the buffer should be moved to the end of the buffer list.
	 */
	inline int DecRef(int amount)
	{
		return m_refCount.fetch_sub(amount, std::memory_order_acq_rel) - amount;
	}

	/**
	 * @brief   Clean up the buffer. This is NOT thread-safe.
	 */
	void Cleanup()
	{
		BufferElement* element = this->GetForRead();
		while (element < m_end && element->ready)
		{
			element->item.~t_ElementType();
			element = this->GetForRead();
		}
	}

private:
	SPRAWL_PAD_CACHELINE;
	std::atomic<Buffer*> m_next;
	SPRAWL_PAD_CACHELINE;
	std::atomic<int> m_refCount;
	SPRAWL_PAD_CACHELINE;
	std::atomic<BufferElement*> m_readPos;
	SPRAWL_PAD_CACHELINE;
	std::atomic<BufferElement*> m_writePos;
	SPRAWL_PAD_CACHELINE;

	char m_buffer[t_BlockSize * sizeof(BufferElement)];
	BufferElement const* const m_end;
};

/**
 * @class   sprawl::collections::ReadReservationTicket
 * 
 * @brief   Represents a reservation to read an element that hasn't been written to yet.
 * 
 * @warning You must call queue.InitializeReservationTicket() on this before using it!
 */
template<typename t_ElementType, size_t t_BlockSize, typename t_AllocatorType>
struct sprawl::collections::ReadReservationTicket
{
	detail::Buffer<t_ElementType, t_BlockSize>* buffer{nullptr};
	typename detail::Buffer<t_ElementType, t_BlockSize>::BufferElement* ptr{ nullptr };
	sprawl::collections::ConcurrentQueue<t_ElementType, t_BlockSize, t_AllocatorType>* queue{ nullptr };
	size_t count{ 0 };

	ReadReservationTicket(){}

	~ReadReservationTicket();

	ReadReservationTicket(ReadReservationTicket const& other) = delete;
	ReadReservationTicket& operator=(ReadReservationTicket const& other) = delete;

	ReadReservationTicket(ReadReservationTicket&& other) noexcept
		: buffer(other.buffer)
		, ptr(other.ptr)
	{
		other.buffer = nullptr;
		other.ptr = nullptr;
	}
	ReadReservationTicket& operator=(ReadReservationTicket&& other) noexcept
	{
		buffer = other.buffer;
		ptr = other.ptr;
		other.buffer = nullptr;
		other.ptr = nullptr;
		return *this;
	}
};

/**
 * class    sprawl::collections::ConcurrentQueue
 * 
 * @brief   Concurrent queue, supporting multi-consumer, multi-producer access 
 *          from multiple threads with no synchronization required. Unbounded, capable of
 *          resizing itself when it's out of space.
 * 
 * @details Strictly speaking, this is not a lock-free queue. When it needs to allocate,
 *          it does acquire a spin lock. It also does this when it needs to fetch a new
 *          read queue because the current one is exhausted.
 *          
 *          However, while it's not STRICTLY speaking lock-free, PRACTICALLY speaking,
 *          it's wait-free population agnostic for the vast majority of enqueues and dequeues.
 *          So long as the reservation ticket used for dequeues is kept alive, this queue is extremely fast.
 *          Be warned, however, that if you don't keep the reservation ticket alive, the queue will still work,
 *          but dequeues will be somewhat slower - while the ticket stays alive, it batches reference counting
 *          operations, but each time the reservation ticket destructs it has to apply those reference count changes.
 *          Which means if it destructs after every dequeue, there's an atomic fetch_sub that will happen after each
 *          queue. It sounds like it's not a big deal, but removing one fetch_sub operation from each dequeue
 *          can have a surprisingly large performance impact.
 *          
 *          Note, though, that the reservation tickets MUST BE KEPT ALIVE if dequeue returns false, or an element
 *          in the queue will become unreachable and will never be read, and memory for the buffer containing it
 *          will not be able to be reused and will cause a memory leak.
 *          
 * @tparam  t_ElementType       the type of element to store in the queue
 * 						      
 * @tparam  t_BlockSize         the number of elements to allocate at a time. 
 *                              Generally speaking, most queues will end up seeing double this number in use,
 *                              assuming it's reasonably large and enqueue operations don't outpace dequeue operations.
 *                              Once the first block is used up a new one will be allocated and the first will be reused
 *                              if it's empty, rather than being freed, hence seeing double this number in memory usage after
 *                              the initial t_BlockSize reads have been completed.
 *                          
 * @tparam   t_AllocatorType    An allocator class compatible with std::allocator. Does not actually allocate individual elements;
 *                              rather, allocates blocks of type detail::Buffer<t_Element, t_BlockSize>, hence this class
 *                              must support `rebind`. For ticket-free dequeue operations, ReadReservationTickets will also
 *                              be allocated after failed reads, and deallocated on subsequent successful reads.
 */
template<typename t_ElementType, size_t t_BlockSize, typename t_AllocatorType>
class sprawl::collections::ConcurrentQueue
{
public:
	typedef ReadReservationTicket<t_ElementType, t_BlockSize, t_AllocatorType> ReadReservationTicket;
	typedef detail::Buffer<t_ElementType, t_BlockSize> Buffer;

	friend struct sprawl::collections::ReadReservationTicket<t_ElementType, t_BlockSize, t_AllocatorType >>;
protected:

	/**
	 * @brief   Decrement the ref count on the buffer and move it to the end of the queue if necessary
	 *
	 * @param   buffer   The buffer to decref and free
	 */
	inline void consume_(Buffer* buffer, size_t amount)
	{
		size_t ret = buffer->DecRef(amount);
		if (SPRAWL_UNLIKELY(ret == 0))
		{
			while (m_reallocatingBuffer.exchange(true, std::memory_order_seq_cst)) {}
			swapToEnd_(buffer);
			m_reallocatingBuffer.store(false);
		}
	}
private:
	ConcurrentQueue(ConcurrentQueue const& other) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue const& other) = delete;
	ConcurrentQueue(ConcurrentQueue&& other) = delete;
	ConcurrentQueue& operator=(ConcurrentQueue&& other) = delete;

	/**
	 * @brief   Move a buffer to the end of the buffer list
	 * 
	 * @details This needs to be called within the m_reallocatingBuffer guard.
	 */
	inline void swapToEnd_(Buffer* buffer)
	{
		Buffer* tail = m_tail.load(std::memory_order_acquire);
		buffer->Clear();
		SPRAWL_CONCURRENT_QUEUE_ASSERT(tail->GetNext() == nullptr);
		SPRAWL_CONCURRENT_QUEUE_ASSERT(buffer != m_writeBuffer.load());
		SPRAWL_CONCURRENT_QUEUE_ASSERT(buffer != m_readBuffer.load());
		SPRAWL_CONCURRENT_QUEUE_ASSERT(buffer != tail);
		tail->SetNext(buffer);
		buffer->SetNext(nullptr);
		m_tail.store(buffer, std::memory_order_release);
	}

	/**
	 * @brief   Decrement the ref count on the buffer and move it to the end of the queue if necessary
	 *
	 * @details This needs to be called within the m_reallocatingBuffer guard.
	 * 
	 * @param   buffer   The buffer to decref and free
	 */
	inline void consumeUnlocked_(Buffer* buffer)
	{
		size_t ret = buffer->DecRef();
		if (SPRAWL_UNLIKELY(ret == 0))
		{
			swapToEnd_(buffer);
		}
	}

	/**
	 * @brief   Decrement the ref count on the buffer and move it to the end of the queue if necessary
	 *
	 * @details This needs to be called within the m_reallocatingBuffer guard.
	 *
	 * @param   buffer   The buffer to decref and free
	 * @param   amount   the amount by which to decrement the count
	 */
	inline void consumeUnlocked_(Buffer* buffer, size_t amount)
	{
		size_t ret = buffer->DecRef(amount);
		if (SPRAWL_UNLIKELY(ret == 0))
		{
			swapToEnd_(buffer);
		}
	}

	/**
	 * @brief   Fetch the next write buffer.
	 * 
	 * @details This function is forced not inlined because it's called very rarely, and when it gets inlined,
	 *          it ends up driving the calling function's assembly size high enough to fall outside cache lines,
	 *          which results in slower performance for the common case. Forcing this to be a non-inlined function
	 *          keeps the code for the COMMON case small, and the cost of a function call for the uncommon case
	 *          is largely irrelevant.
	 */
	SPRAWL_FORCE_NO_INLINE void fetchNextWriteBuffer_(typename Buffer::BufferElement*& element, Buffer*& buffer)
	{
		// Just because we won the lottery, though, doesn't mean we're the only ones who won.
		// Someone else may have already claimed the prize. We need to make sure we still
		// need to do this before we actually do it.
		// We do that by re-fetching the buffer and element and re-doing the above check.
		buffer = m_writeBuffer.load(std::memory_order_acquire);
		element = buffer->GetForWrite();

		if (element >= buffer->GetEnd())
		{
			// If we're still past the end, time to replace the write buffer.
			// First we get the next buffer in the list. If one exists, it's one
			// we've previously used up and are now taking for reuse.
			Buffer* newBuffer = buffer->GetNext();
			if (!newBuffer)
			{
				// If we get nullptr back from this, then we actually need to allocate.
				newBuffer = m_allocator.allocate(1);
				new(newBuffer) Buffer();
				buffer->SetNext(newBuffer);
				if (buffer == m_tail.load(std::memory_order_acquire))
				{
					m_tail.store(newBuffer, std::memory_order_release);
				}
			}
			SPRAWL_CONCURRENT_QUEUE_ASSERT(newBuffer != buffer);
			// Once we've either obtained or allocated the new buffer, we need to make sure
			// the write position's set to the start of the queue, otherwise we'll just
			// end up throwing it away again.
			newBuffer->SetWritePosition();

			// Now that it's ready for writing, we can store m_writeBuffer and let other threads
			// start using it.
			m_writeBuffer.store(newBuffer, std::memory_order_release);

			// Then we consume the old buffer to tell it that we're no longer pointing m_writeBuffer at it,
			// then we get a new element from it.
			// This shouldn't be past the end, but it's theoretically possible it could be, so we reassign buffer
			// as well so we can do this in a while loop
			consumeUnlocked_(buffer);
			buffer = newBuffer;
			element = buffer->GetForWrite();
		}
	}

	/**
	* @brief   Fetch the next read buffer.
	*
	* @details This function is forced not inlined because it's called very rarely, and when it gets inlined,
	*          it ends up driving the calling function's assembly size high enough to fall outside cache lines,
	*          which results in slower performance for the common case. Forcing this to be a non-inlined function
	*          keeps the code for the COMMON case small, and the cost of a function call for the uncommon case
	*          is largely irrelevant.
	*/
	SPRAWL_FORCE_NO_INLINE bool fetchNextReadBuffer_(typename Buffer::BufferElement*& element, Buffer*& buffer, ReadReservationTicket& ticket)
	{
		buffer = m_readBuffer.load(std::memory_order_acquire);
		element = buffer->GetForRead();

		if (element >= buffer->GetEnd())
		{
			Buffer* nextBuffer = buffer->GetNext();
			if (nextBuffer == nullptr)
			{
				// If there isn't a new buffer to read from, we're just going to return false.
				// In this case, we're not updating any information in the ticket.
				// By virtue of the fact that we're here, the ticket's ptr is already null
				// And since there's no next buffer to read from, and we don't want to allocate one when we're just reading,
				// we're just going to keep it null and redo this work next time.
				return false;
			}
			nextBuffer->SetReadPosition();
			SPRAWL_CONCURRENT_QUEUE_ASSERT(nextBuffer != buffer);

			m_readBuffer.store(nextBuffer, std::memory_order_release);
			consumeUnlocked_(buffer);
			buffer = nextBuffer;
			element = buffer->GetForRead();
		}
		// This bit allows us to be very efficient about reference counting
		// by keeping the count on a non-shared variable and only performing operations on
		// the shared variable in batches. Only when a consumer thread stops reading from a buffer
		// do we adjust that buffer's reference count, meaning each buffer only has to have a DecRef()
		// performed once per consumer thread, plus 2 additional times for m_readBuffer and m_writeBuffer changing.
		// This is a fairly significant performance boost.
		if (ticket.count != 0)
		{
			consumeUnlocked_(ticket.buffer, ticket.count);
		}
		ticket.count = 0;
		// We then set the ticket's buffer to the new buffer we've obtained.
		ticket.buffer = buffer;
		return true;
	}

	/**
	 * @brief   Retrieve the next element to write to.
	 * 
	 * @details This method does all the work of both incrementing the write pointer
	 *          and detecting when it's past the end of the write buffer. If it is,
	 *          this function will move on to the next buffer, or allocate a new one if needed,
	 *          and then return an element guaranteed to be valid to write to.
	 *          
	 * @return  The next viable write element for the queue
	 */
	inline typename Buffer::BufferElement& getNextElement_()
	{
		// First we try to retrieve an element for write from our write buffer.
		Buffer* buffer = m_writeBuffer.load(std::memory_order_acquire);
		typename Buffer::BufferElement* element = buffer->GetForWrite();

		// The write buffer may be full. If it is, it'll return a pointer past the end of the buffer.
		// If that happens we have to retrieve or allocate a new buffer.
		// Strictly speaking, this section violates lock-free because the allocation happens within a spin-lock.
		// Practically speaking, this spin-lock happens so infrequently in a queue with a proper block size that
		// it may as well never happen at all.
		while (SPRAWL_UNLIKELY(element >= buffer->GetEnd()))
		{
			// When we get here, we use a simple atomic boolean as a spin lock.
			// We perform an exchange() on it - if it returns false, that means we won the lottery
			// because we were the first to set it true.
			if (!m_reallocatingBuffer.exchange(true, std::memory_order_seq_cst))
			{
				fetchNextWriteBuffer_(element, buffer);
				m_reallocatingBuffer.store(false, std::memory_order_release);
			}
		}

		// Now we've gotten an element! We can return it back to the caller!
		return *element;
	}

public:

	ConcurrentQueue()
		: m_readBuffer(nullptr)
		, m_reallocatingBuffer(false)
		, m_writeBuffer(nullptr)
		, m_tail(nullptr)
		, m_ticketList(nullptr)
	{
		Buffer* buffer = m_allocator.allocate(1);
		new(buffer) Buffer();
		m_readBuffer = buffer;
		m_writeBuffer = buffer;
		m_tail = buffer;
	}

	~ConcurrentQueue()
	{
		for(TicketListNode* node = m_ticketList.load(std::memory_order_acquire), *node_next = node ? node->next : nullptr; node != nullptr; node = node_next)
		{
			node->~TicketListNode();
			m_ticketListAllocator.deallocate(node, 1);
		}

		Buffer* buffer = m_readBuffer.load(std::memory_order_acquire);
		while(buffer)
		{
			Buffer* nextBuffer = buffer->GetNext();
			buffer->Cleanup();
			buffer->~Buffer();
			m_allocator.deallocate(buffer, 1);
			buffer = nextBuffer;
		}
	}

	/**
	 * @brief   Initialize a reservation ticket. Must be called on a ticket before it can be used.
	 * 
	 * @details This isn't a particularly expensive operation, but needs to be called on a buffer
	 *          when it's constructed. The main purpose of this is to save Dequeue() from having to
	 *          add an if-check to detect an uninitialized buffer. Branching is expensive.
	 *          
	 * @param   ticket   the ticket to initialize
	 */
	void InitializeReservationTicket(ReadReservationTicket& ticket)
	{
		ticket.buffer = m_readBuffer.load(std::memory_order_acquire);
		ticket.queue = this;
	}

	/**
	 * @brief   Enqueue an item by reference, calling the copy constructor. Will not fail (unless OOM).
	 * 
	 * @param   val   The value to equeue
	 */
	inline void Enqueue(t_ElementType const& val)
	{
		typename Buffer::BufferElement& element = getNextElement_();
		new(&element.item) t_ElementType(val);
		SPRAWL_CONCURRENT_QUEUE_ASSERT(element.ready.load() == false);
		element.ready.store(true, std::memory_order_release);
	}

	/**
	* @brief   Enqueue an item by rvalue, calling the move constructor. Will not fail (unless OOM).
	 * 
	 * @param   val   The value to equeue
	*/
	inline void Enqueue(t_ElementType&& val)
	{
		typename Buffer::BufferElement& element = getNextElement_();
		new(&element.item) t_ElementType(std::move(val));
		SPRAWL_CONCURRENT_QUEUE_ASSERT(element.ready.load() == false);
		element.ready.store(true, std::memory_order_release);
	}

	/**
	 * @brief   Attempt to dequeue an item. Not guaranteed to succeed, as the queue may be empty.
	 * 
	 * @details To improve performance, all dequeue operations will cache data in the ReadReservationTicket parameter.
	 *          
	 *          If the dequeue operation returns false, this parameter MUST be held onto and passed back into Dequeue() 
	 *          or an element in the queue will become permanently inaccessible. 
	 *          
	 *          It doesn't matter what thread passes the ticket back in, but it cannot be disposed of so long as
	 *          Dequeue() has returned false.
	 *          
	 *          If Dequeue() returns true, it is still highly recommended to keep the ticket alive and pass it back in.
	 *          The only reason for this is performance - the performance drop from having to adjust reference counts
	 *          on each dequeue operation isn't crippling, but it is noticeable.
	 *          
	 *          
	 * @param   val      A reference to a value, which will be filled with the contents of the dequeued element, if any.
	 *                   The move assignment operator will be called on the value, if one exists.
	 * @param   ticket   A reservation ticket which will hold cached data to improve performance.
	 * 
	 * @return  true if the dequeue succeeded and tha value holds a valid item, false if the dequeue failed.
	 */
	inline bool Dequeue(t_ElementType& val, ReadReservationTicket& ticket)
	{
		// For reads, we'll start out by checking our reservation ticket. If it's got cached data, we can skip a lot of work we already did.
		typename Buffer::BufferElement* element = ticket.ptr;
		Buffer* buffer = ticket.buffer;

		// There are a few cases we can run into in the dequeue operation.
		// The easiest case is after a failed dequeue, in which case we already have our element and can check it again.
		if (SPRAWL_LIKELY(!element))
		{
			// The second case is when the ticket passed in has been held over from a previous successful dequeue.
			// In this case we don't have to worry about acquiring the read buffer, because it's cached. We only have
			// to do that if the current one is exhausted.

			// Step one, get the next element and determine if the current buffer is exhausted!
			element = buffer->GetForRead();
			while (SPRAWL_UNLIKELY(element >= buffer->GetEnd()))
			{
				// If the buffer is exhausted, we have to acquire the next one.
				// This is done under the same spin-lock as allocating a new buffer for writes, and the logic is almost identical.
				// The only difference is that, if buffer->GetNext() returns nullptr, instead of allocating a new one,
				// we just return false; for more details on this logic, see the comments in getNextElement_()
				if (!m_reallocatingBuffer.exchange(true, std::memory_order_seq_cst))
				{
					if(!fetchNextReadBuffer_(element, buffer, ticket))
					{
						m_reallocatingBuffer.store(false, std::memory_order_release);
						return false;
					}
					m_reallocatingBuffer.store(false, std::memory_order_release);
				}
			}
		}
		
		// Now that we have an element to read, we have to check if there's any actual data in it.
		// If not, we're going to remember this element in the reservation ticket and come back to it later.
		// This definitively prevents any race conditions involved in attempting to correct for overcommit.
		bool ready = element->ready.load(std::memory_order_acquire);
		if(SPRAWL_LIKELY(ready == true))
		{
			// If the element did have valid data, we need to make sure our ticket's not holding any cache information.
			// Otherwise we'd just keep ending up reading the same cached element over and over.
			ticket.ptr = nullptr;

			// Increase the ticket's read count, which is used in the block above to perform bulk DecRefs
			++ticket.count;

			// Finally, we'll go ahead and pull the data from the element, destroy it, and decrement and possibly free the buffer.
			// Then we can return true - success!
			val = std::move(element->item);
			element->item.~t_ElementType();
			SPRAWL_CONCURRENT_QUEUE_ASSERT(element->ready.exchange(false) == true);

			return true;
		}
		ticket.ptr = element;
		return false;
	}

	/**
	 * @brief   Attempt to dequeue an item without passing in any tickets.
	 * 
	 * @details This version of Dequeue() does not require persistent tickets even on a return value of false
	 *          (or any tickets, for that matter). The performance of the common case will be similar to the other
	 *          version of Dequeue() with non-persistent tickets. In the case of failed reads, performance will be
	 *          somewhat hampered, but still superior to the performance of a successful read.
	 *          
	 *          
	 * @param   val      A reference to a value, which will be filled with the contents of the dequeued element, if any.
	 *                   The move assignment operator will be called on the value, if one exists.
	 * 
	 * @return  true if the dequeue succeeded and tha value holds a valid item, false if the dequeue failed.
	 */
	inline bool Dequeue(t_ElementType& val)
	{
		// This version of Dequeue() still needs tickets and needs them to persist after a failed read,
		// but since the user isn't providing us with any storage space for it, we have to store it on the heap.
		// We'll store it in this m_ticketList linked list, which means when we come to dequeue, the first
		// thing we have to do is check the list to see if we need to retry any failed reads.
		TicketListNode* node = m_ticketList.load(std::memory_order_acquire);
		if(node)
		{
			// Unfortunately this can't be done with a simple fetch_add like the normal Dequeue()
			// which means this version is not wait-free, but only lock-free, and there will be no
			// vertical scaling of this section of code. Fortunately this is the uncommon case.

			// First thing we have to do is obtain exclusive ownership of the first node in
			// the list. That means trying to remove it and set it to a new value, but another
			// thread may also be doing that, so we have to try until we win.
			while(m_ticketList.compare_exchange_weak(node, node->next) == false)
			{
				// If another thread won, we might have ended up with no node, in which case we should just move on
				// to the normal logic flow with a local ticket.
				if(!node)
				{
					// Goto here avoids an ifcheck. To maximize performance we need to minimize branching.
					goto no_node;
				}
			}

			// Once we got a node, we try to read using its ticket.
			if (!Dequeue(val, node->ticket))
			{
				// If we fail, put it back on the front of the ticket list. Again, this is a compare/swap loop.
				node->next = m_ticketList.load(std::memory_order_acquire);
				while (m_ticketList.compare_exchange_weak(node->next, node) == false) {}
				return false;
			}
			// If we succeed, deallocate that ticket and return true.
			node->~TicketListNode();
			m_ticketListAllocator.deallocate(node, 1);
			return true;
		}

	no_node:
		// The common successful read case is the same as the non-persistent tickets case, create a local
		// ticket on the stack and do a read with it.
		ReadReservationTicket ticket;
		InitializeReservationTicket(ticket);
		if (!Dequeue(val, ticket))
		{
			// If that failed, we transfer the ticket to the heap and put it on the failed read list.
			node = m_ticketListAllocator.allocate(1);
			new(node) TicketListNode(std::move(ticket), m_ticketList.load(std::memory_order_acquire));
			while (m_ticketList.compare_exchange_weak(node->next, node) == false) {}
			return false;
		}

		return true;
	}

private:
	struct TicketListNode
	{
		TicketListNode(ReadReservationTicket&& ticket_, TicketListNode* next_)
			: ticket(std::move(ticket_))
			, next(next_)
		{}

		ReadReservationTicket ticket;
		TicketListNode* next;
	};

	SPRAWL_PAD_CACHELINE;
	// Read head, not necessarily the same as the write head
	std::atomic<Buffer*> m_readBuffer;
	SPRAWL_PAD_CACHELINE;
	// Spin lock used when swapping buffers - not technically lock free, but lock free isn't always faster.
	// And this is used rarely enough that the simplicity of the code around it is far more valuable.
	// The performance improvement of making this lock free would be imperceptible, and the increased amount
	// of code to get it to work right would likely bloat code size and cause more cache misses in execution.
	std::atomic<bool> m_reallocatingBuffer;
	SPRAWL_PAD_CACHELINE;
	// Write head, not necessarily the same as the read head
	std::atomic<Buffer*> m_writeBuffer;
	SPRAWL_PAD_CACHELINE;
	// Tail. Obviously.
	std::atomic<Buffer*> m_tail;
	SPRAWL_PAD_CACHELINE;
	// List of failed read tickets for the ticket-free Dequeue
	std::atomic<TicketListNode*> m_ticketList;
	SPRAWL_PAD_CACHELINE;

	typename t_AllocatorType::template rebind<Buffer>::other m_allocator;
	typename t_AllocatorType::template rebind<TicketListNode>::other m_ticketListAllocator;
};

template<typename t_ElementType, size_t t_BlockSize, typename t_AllocatorType>
sprawl::collections::ReadReservationTicket<t_ElementType, t_BlockSize, t_AllocatorType>::~ReadReservationTicket()
{
	if (buffer && count != 0)
	{
		queue->consume_(buffer, count);
	}
}


/**
* @class sprawl::collections::BoundedReadReservationTicket
*
* @brief Represents a reservation to read an element that hasn't been written to yet.
*/
template<typename t_ElementType>
struct sprawl::collections::BoundedReadReservationTicket
{
	void* ptr{ nullptr };

	BoundedReadReservationTicket() {}

	BoundedReadReservationTicket(BoundedReadReservationTicket const& other) = delete;
	BoundedReadReservationTicket& operator=(BoundedReadReservationTicket const& other) = delete;

	BoundedReadReservationTicket(BoundedReadReservationTicket&& other) noexcept
		: ptr(other.ptr)
	{
		other.ptr = nullptr;
	}
	BoundedReadReservationTicket& operator=(BoundedReadReservationTicket&& other) noexcept
	{
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}
};

/**
* @class sprawl::collections::BoundedReadReservationTicket
*
* @brief Represents a reservation to write an element that's already holding unread data
*/
template<typename t_ElementType>
struct sprawl::collections::BoundedWriteReservationTicket
{
	void* ptr{ nullptr };

	BoundedWriteReservationTicket() {}

	BoundedWriteReservationTicket(BoundedWriteReservationTicket const& other) = delete;
	BoundedWriteReservationTicket& operator=(BoundedWriteReservationTicket const& other) = delete;

	BoundedWriteReservationTicket(BoundedWriteReservationTicket&& other) noexcept
		: ptr(other.ptr)
	{
		other.ptr = nullptr;
	}
	BoundedWriteReservationTicket& operator=(BoundedWriteReservationTicket&& other) noexcept
	{
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}
};

/**
 * @class   sprawl::collections::ConcurrentBoundedQueue
 * 
 * @brief   A bounded implementation of ConcurrentQueue.
 * 
 * @details The core algorithm of this queue is essentially the same algorithm as the
 *          unbounded version of this queue - however, there are a few key differences:
 *          
 *          First, and probably most importantly, this queue cannot grow. It works as
 *          a circular buffer, and can only hold the specified number of elements at
 *          one time. Elements that are read by Dequeue() become available to be
 *          written again, but if no consumer threads are running, or producer threads
 *          significantly outpace consumer threads, the queue can become full,
 *          causing Enqueue() to return false.
 *          
 *          Secondly, unlike the unbounded version, this queue is truly lock-free
 *          and wait-free. There are no situations that involve taking a lock.
 *          
 *          Thirdly, reservation tickets are required for both enqueue AND dequeue;
 *          however, they only need to be kept alive after a return of false from either
 *          method. If the return value is true, the ticket can be safely thrown away.
 *          
 *          Note that there is one situation that can cause an enqueue thread to become
 *          blocked: if a dequeue thread gets a return of false and doesn't call Dequeue()
 *          again with that ticket, an enqueue thread will be blocked waiting for that
 *          spot to be read, even after other enqueue threads successfully move on and continue
 *          writing.
 *          
 *          Also note that t_QueueSize will be adjusted up to the nearest power of 2 for performance
 *          reasons.
 *          
 * @tparam  t_ElementType       the type of element to store in the queue
 * 						      
 * @tparam  t_QueueSize         the maximum number of elements that can be in the queue at a time.
 *                              Once this number has been reached, enqueue() operations will fail until
 *                              elements have been dequeued. This is not a maximum number of elements
 *                              ever inserted, only a maximum number that can be held unread at a time -
 *                              representing overhead between enqueue and dequeue operations.
 *                          
 * @tparam  t_AllocatorType     Allocator used to allocate tickets for the ticket-free enqueue
 *                              and dequeue operations. The allocators are NOT used in the operations
 *                              that do accept ticket parameters; those are alloc-free.
 */
template<typename t_ElementType, size_t t_QueueSize, typename t_AllocatorType>
class sprawl::collections::ConcurrentBoundedQueue
{
public:
	typedef BoundedReadReservationTicket<t_ElementType> ReadReservationTicket;
	typedef BoundedWriteReservationTicket<t_ElementType> WriteReservationTicket;

	struct BufferElement
	{
		std::atomic<bool> ready;
		t_ElementType item;
	};

	ConcurrentBoundedQueue()
		: m_readIdx(0)
		, m_writeIdx(0)
		, m_readTicketList(nullptr)
		, m_writeTicketList(nullptr)
	{
		memset(m_buffer, 0, c_adjustedSize * sizeof(BufferElement));
	}

	~ConcurrentBoundedQueue()
	{
		for (TicketListNode<ReadReservationTicket>* node = m_readTicketList.load(std::memory_order_acquire), *node_next = node ? node->next : nullptr; node != nullptr; node = node_next)
		{
			node->~TicketListNode();
			m_readTicketListAllocator.deallocate(node, 1);
		}
		for (TicketListNode<WriteReservationTicket>* node = m_writeTicketList.load(std::memory_order_acquire), *node_next = node ? node->next : nullptr; node != nullptr; node = node_next)
		{
			node->~TicketListNode();
			m_writeTicketListAllocator.deallocate(node, 1);
		}
		BufferElement* buffer = reinterpret_cast<BufferElement*>(m_buffer);
		for(size_t idx = 0; idx < c_adjustedSize; ++idx)
		{
			BufferElement* element = buffer + (idx & (c_adjustedSize - 1));
			if(element->ready.load())
			{
				element->item.~t_ElementType();
			}
		}
	}

	/**
	 * @brief   Enqueue an item by reference, calling the copy constructor. Will fail if the queue is full.
	 *
	 * @param   val      The value to equeue
	 * @param   ticket   A reservation ticket that will hold cached data in the event of a return of false
	 * 
	 * @return  true if the element was successfully enqueued, false otherwise
	 */
	inline bool Enqueue(t_ElementType const& val, WriteReservationTicket& ticket)
	{
		// This case is much simpler than the unbounded case!
		// First we check to see if the reservation ticket contains an element we're supposed to retry a write to
		BufferElement* element = reinterpret_cast<BufferElement*>(ticket.ptr);

		if (SPRAWL_LIKELY(!element))
		{
			// If not, then we get a new one with a simple fetch_add on the write index, wrapping it appropriately.
			size_t idx = m_writeIdx.fetch_add(1, std::memory_order_acq_rel);
			element = m_buffer + (idx & (c_adjustedSize - 1));
		}

		// Check the ready flag. If it's already set, we can't overwrite it and have to return false,
		// storing this element on the reservation ticket to make sure we try it again later.
		bool ready = element->ready.load(std::memory_order_acquire);
		if (SPRAWL_LIKELY(ready == false))
		{
			// If it's not already ready, we make sure the reservation ticket is clear so we don't write it again...
			ticket.ptr = nullptr;

			// ...then we construct the new element...
			new(&element->item) t_ElementType(val);
			SPRAWL_CONCURRENT_QUEUE_ASSERT(element->ready.load() == false);

			// ...then we signal that the element is ready to read and return true.
			element->ready.store(true, std::memory_order_release);
			return true;
		}
		ticket.ptr = element;
		return false;
	}

	/**
	 * @brief   Enqueue an item by rvalue reference, calling the move constructor. Will fail if the queue is full.
	 *
	 * @param   val      The value to equeue
	 * @param   ticket   A reservation ticket that will hold cached data in the event of a return of false
	 * 
	 * @return  true if the element was successfully enqueued, false otherwise
	 */
	inline bool Enqueue(t_ElementType&& val, WriteReservationTicket& ticket)
	{
		// See above for comments; this algorithm is identical except for construction via move.
		BufferElement* element = reinterpret_cast<BufferElement*>(ticket.ptr);

		if (SPRAWL_LIKELY(!element))
		{
			size_t idx = m_writeIdx.fetch_add(1, std::memory_order_acq_rel);
			element = m_buffer + (idx & (c_adjustedSize - 1));
		}

		bool ready = element->ready.load(std::memory_order_acquire);
		if (SPRAWL_LIKELY(ready == false))
		{
			ticket.ptr = nullptr;

			new(&element->item) t_ElementType(std::move(val));
			SPRAWL_CONCURRENT_QUEUE_ASSERT(element->ready.load() == false);

			element->ready.store(true, std::memory_order_release);
			return true;
		}
		ticket.ptr = element;
		return false;
	}

	/**
	 * @brief   Dequeue an item.  Will fail if the queue is empty.
	 *
	 * @param   val      A reference to a value, which will be filled with the contents of the dequeued element, if any.
	 *                   The move assignment operator will be called on the value, if one exists.
	 * @param   ticket   A reservation ticket which will hold cached data to improve performance.
	 * 
	 * @return  true if the element was successfully enqueued, false otherwise
	 */
	inline bool Dequeue(t_ElementType& val, ReadReservationTicket& ticket)
	{
		// See above for comments; this algorithm is identical except we're operating on m_readIdx
		// instead of m_writeIdx, and destructing the element instead of constructing it.
		BufferElement* element = reinterpret_cast<BufferElement*>(ticket.ptr);

		if (SPRAWL_LIKELY(!element))
		{
			size_t idx = m_readIdx.fetch_add(1, std::memory_order_acq_rel);
			element = m_buffer + (idx & (c_adjustedSize - 1));
		}

		bool ready = element->ready.load(std::memory_order_acquire);
		if (SPRAWL_LIKELY(ready == true))
		{
			ticket.ptr = nullptr;

			val = std::move(element->item);
			element->item.~t_ElementType();
			SPRAWL_CONCURRENT_QUEUE_ASSERT(element->ready.load() == true);
			element->ready.store(false, std::memory_order_release);
			return true;
		}
		ticket.ptr = element;
		return false;
	}

	/**
	 * @brief   Attempt to enqueue an item without passing in any tickets.
	 *
	 * @details This version of Enqueue() does not require persistent tickets even on a return value of false
	 *          (or any tickets, for that matter). The performance of the common case will be similar to the other
	 *          version of Enqueue(). In the case of failed writes, performance will be somewhat hampered,
	 *          but still superior to the performance of a successful write.
	 *
	 *
	 * @param   val      The value to equeue
	 *
	 * @return  true if the enqueue succeeded, false if the enqueue failed.
	 */
	inline bool Enqueue(t_ElementType& val)
	{
		// This version of Enqueue() still needs tickets and needs them to persist after a failed write,
		// but since the user isn't providing us with any storage space for it, we have to store it on the heap.
		// We'll store it in this m_ticketList linked list, which means when we come to enqueue, the first
		// thing we have to do is check the list to see if we need to retry any failed writes.
		TicketListNode<WriteReservationTicket>* node = m_writeTicketList.load(std::memory_order_acquire);
		if (node)
		{
			// Unfortunately this can't be done with a simple fetch_add like the normal Enqueue()
			// which means this version is not wait-free, but only lock-free, and there will be no
			// vertical scaling of this section of code. Fortunately this is the uncommon case.

			// First thing we have to do is obtain exclusive ownership of the first node in
			// the list. That means trying to remove it and set it to a new value, but another
			// thread may also be doing that, so we have to try until we win.
			while (m_writeTicketList.compare_exchange_weak(node, node->next) == false)
			{
				// If another thread won, we might have ended up with no node, in which case we should just move on
				// to the normal logic flow with a local ticket.
				if (!node)
				{
					// Goto here avoids an ifcheck. To maximize performance we need to minimize branching.
					goto no_node;
				}
			}

			// Once we got a node, we try to reawrited using its ticket.
			if (!Enqueue(val, node->ticket))
			{
				// If we fail, put it back on the front of the ticket list. Again, this is a compare/swap loop.
				node->next = m_writeTicketList.load(std::memory_order_acquire);
				while (m_writeTicketList.compare_exchange_weak(node->next, node) == false) {}
				return false;
			}
			// If we succeed, deallocate that ticket and return true.
			node->~TicketListNode();
			m_writeTicketListAllocator.deallocate(node, 1);
			return true;
		}

	no_node:
		// The common successful write case is the same as the non-persistent tickets case, create a local
		// ticket on the stack and do a write with it.
		WriteReservationTicket ticket;
		InitializeReservationTicket(ticket);
		if (!Enqueue(val, ticket))
		{
			// If that failed, we transfer the ticket to the heap and put it on the failed write list.
			node = m_writeTicketListAllocator.allocate(1);
			new(node) TicketListNode<WriteReservationTicket>(std::move(ticket), m_writeTicketList.load(std::memory_order_acquire));
			while (m_writeTicketList.compare_exchange_weak(node->next, node) == false) {}
			return false;
		}

		return true;
	}

	/**
	 * @brief   Attempt to dequeue an item without passing in any tickets.
	 *
	 * @details This version of Dequeue() does not require persistent tickets even on a return value of false
	 *          (or any tickets, for that matter). The performance of the common case will be similar to the other
	 *          version of Dequeue(). In the case of failed reads, performance will be somewhat hampered, 
	 *          but still superior to the performance of a successful read.
	 *
	 *
	 * @param   val      A reference to a value, which will be filled with the contents of the dequeued element, if any.
	 *                   The move assignment operator will be called on the value, if one exists.
	 *
	 * @return  true if the dequeue succeeded and tha value holds a valid item, false if the dequeue failed.
	 */
	inline bool Dequeue(t_ElementType& val)
	{
		// This version of Dequeue() still needs tickets and needs them to persist after a failed read,
		// but since the user isn't providing us with any storage space for it, we have to store it on the heap.
		// We'll store it in this m_ticketList linked list, which means when we come to dequeue, the first
		// thing we have to do is check the list to see if we need to retry any failed reads.
		TicketListNode<ReadReservationTicket>* node = m_readTicketList.load(std::memory_order_acquire);
		if (node)
		{
			// Unfortunately this can't be done with a simple fetch_add like the normal Dequeue()
			// which means this version is not wait-free, but only lock-free, and there will be no
			// vertical scaling of this section of code. Fortunately this is the uncommon case.

			// First thing we have to do is obtain exclusive ownership of the first node in
			// the list. That means trying to remove it and set it to a new value, but another
			// thread may also be doing that, so we have to try until we win.
			while (m_readTicketList.compare_exchange_weak(node, node->next) == false)
			{
				// If another thread won, we might have ended up with no node, in which case we should just move on
				// to the normal logic flow with a local ticket.
				if (!node)
				{
					// Goto here avoids an ifcheck. To maximize performance we need to minimize branching.
					goto no_node;
				}
			}

			// Once we got a node, we try to read using its ticket.
			if (!Dequeue(val, node->ticket))
			{
				// If we fail, put it back on the front of the ticket list. Again, this is a compare/swap loop.
				node->next = m_readTicketList.load(std::memory_order_acquire);
				while (m_readTicketList.compare_exchange_weak(node->next, node) == false) {}
				return false;
			}
			// If we succeed, deallocate that ticket and return true.
			node->~TicketListNode();
			m_readTicketListAllocator.deallocate(node, 1);
			return true;
		}

	no_node:
		// The common successful read case is the same as the non-persistent tickets case, create a local
		// ticket on the stack and do a read with it.
		ReadReservationTicket ticket;
		InitializeReservationTicket(ticket);
		if (!Dequeue(val, ticket))
		{
			// If that failed, we transfer the ticket to the heap and put it on the failed read list.
			node = m_readTicketListAllocator.allocate(1);
			new(node) TicketListNode<ReadReservationTicket>(std::move(ticket), m_readTicketList.load(std::memory_order_acquire));
			while (m_readTicketList.compare_exchange_weak(node->next, node) == false) {}
			return false;
		}

		return true;
	}

private:
	template<typename t_TicketType>
	struct TicketListNode
	{
		TicketListNode(t_TicketType&& ticket_, TicketListNode* next_)
			: ticket(std::move(ticket_))
			, next(next_)
		{}

		t_TicketType ticket;
		TicketListNode* next;
	};

	constexpr static size_t c_adjustedSize = detail::nextPowerOf2(t_QueueSize);

	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_readIdx;
	SPRAWL_PAD_CACHELINE;
	std::atomic<size_t> m_writeIdx;
	SPRAWL_PAD_CACHELINE;
	BufferElement m_buffer[c_adjustedSize];
	SPRAWL_PAD_CACHELINE;
	std::atomic<TicketListNode<ReadReservationTicket>*> m_readTicketList;
	SPRAWL_PAD_CACHELINE;
	std::atomic<TicketListNode<WriteReservationTicket>*> m_writeTicketList;
	SPRAWL_PAD_CACHELINE;

	typename t_AllocatorType::template rebind<TicketListNode<ReadReservationTicket>>::other m_readTicketListAllocator;
	typename t_AllocatorType::template rebind<TicketListNode<WriteReservationTicket>>::other m_writeTicketListAllocator;
};
