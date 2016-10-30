#include "../../collections/ConcurrentQueue.hpp"
#include "../../collections/List.hpp"
#include <gtest/gtest.h>
#include "../../collections/HashMap.hpp"
#include "../../collections/Deque.hpp"
#include "../../threading/thread.hpp"
#include "../../threading/mutex.hpp"
#include "../../time/time.hpp"
#include <memory>
#include <math.h>
#ifndef _WIN32
#include <malloc.h>
#endif

#if 1
namespace ConcurrentQueue
{
	constexpr int numInsertsPerThread = 100000;
	constexpr int nThreads = 10;
	sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>* queue;

	class ConcurrentQueue : public testing::Test
	{
		virtual void SetUp() override
		{
			queue = new sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>();
		}

		virtual void TearDown() override
		{
			delete queue;
		}
	};

	void Enqueue(int startingPoint)
	{
		for(int i = startingPoint; i < startingPoint + numInsertsPerThread; ++i)
		{
			queue->Enqueue(i);
		}
	}

	sprawl::collections::BasicHashMap<int, int> results;
	sprawl::threading::Mutex mtx;

	void Dequeue()
	{
		sprawl::collections::BasicHashMap<int, int> localResults;
		for(;;)
		{
			int i;
			if(!queue->Dequeue(i))
			{
				break;
			}
			++localResults[i];
		}

		sprawl::threading::ScopedLock lock(mtx);
		for(auto& kvp : localResults)
		{
			results[kvp.Key()] += kvp.Value();
		}
	}

	std::atomic<int> count;

	void DequeueSimple()
	{
		while(count < numInsertsPerThread * nThreads)
		{
			int i;
			if(queue->Dequeue(i))
			{
				++count;
			}
		}
	}

#if 1
	TEST_F(ConcurrentQueue, WorksOnManyThreads)
	{
		int test;
		queue = new sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>();

		sprawl::collections::List<std::unique_ptr<sprawl::threading::Thread>> threads;
		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Enqueue, numInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Dequeue)));
		}

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		for(int i = 0; i < numInsertsPerThread * nThreads; ++i)
		{
			ASSERT_TRUE(results.Has(i));
			ASSERT_EQ(1, results.Get(i));
		}
		int i;
		ASSERT_FALSE(queue->Dequeue(i));
	}
#endif

	sprawl::collections::Deque<int> deque(sprawl::collections::Capacity(numInsertsPerThread * nThreads));

	void EnqueueDeque(int startingPoint)
	{
		for(int i = startingPoint; i < startingPoint + numInsertsPerThread; ++i)
		{
			sprawl::threading::ScopedLock lock(mtx);
			deque.PushBack(i);
		}
	}

	int countDeque = 0;

	void DequeueDeque()
	{
		while(countDeque < numInsertsPerThread * nThreads)
		{
			sprawl::threading::ScopedLock lock(mtx);
			if(!deque.Empty())
			{
				deque.PopFront();
				++countDeque;
			}
		}
	}

#if 0
	TEST_F(ConcurrentQueue, FasterThanQueueWithLocks)
	{
		count = 0;
		countDeque = 0;
		int test;
		ASSERT_FALSE(queue->Dequeue(test)) << "Queue did not start clean.";

		sprawl::collections::List<std::unique_ptr<sprawl::threading::Thread>> threads;
		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Enqueue, numInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(DequeueSimple)));
		}

		int64_t lockFreeStart = sprawl::time::Now(sprawl::time::Resolution::Milliseconds);

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		int64_t lockFreeTime = sprawl::time::Now(sprawl::time::Resolution::Milliseconds) - lockFreeStart;

		threads.Clear();

		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(EnqueueDeque, numInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(DequeueDeque)));
		}

		int64_t dequeStart = sprawl::time::Now(sprawl::time::Resolution::Milliseconds);

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		int64_t dequeTime = sprawl::time::Now(sprawl::time::Resolution::Milliseconds) - dequeStart;

#if defined(_WIN32)
	#define I64FMT "ll"
#elif defined(__APPLE__)
	#define I64FMT "ll"
#else
	#define I64FMT "l"
#endif
		printf("Lock free: %" I64FMT "d ms, deque: %" I64FMT "d ms\n", lockFreeTime, dequeTime );
		EXPECT_LT(lockFreeTime, dequeTime);
		int i;
		ASSERT_FALSE(queue->Dequeue(i));
	}
#endif
	template<typename T>
	class mpmc_bounded_queue
	{
	public:
	  mpmc_bounded_queue(size_t buffer_size)
		: buffer_(new cell_t [buffer_size])
		, buffer_mask_(buffer_size - 1)
	  {
		assert((buffer_size >= 2) &&
		  ((buffer_size & (buffer_size - 1)) == 0));
		for (size_t i = 0; i != buffer_size; i += 1)
		  buffer_[i].sequence_.store(i, std::memory_order_relaxed);
		enqueue_pos_.store(0, std::memory_order_relaxed);
		dequeue_pos_.store(0, std::memory_order_relaxed);
	  }

	  ~mpmc_bounded_queue()
	  {
		delete [] buffer_;
	  }

	  bool enqueue(T const& data)
	  {
		cell_t* cell;
		size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
		  cell = &buffer_[pos & buffer_mask_];
		  size_t seq =
			cell->sequence_.load(std::memory_order_acquire);
		  intptr_t dif = (intptr_t)seq - (intptr_t)pos;
		  if (dif == 0)
		  {
			if (enqueue_pos_.compare_exchange_weak
				(pos, pos + 1, std::memory_order_relaxed))
			  break;
		  }
		  else if (dif < 0)
			return false;
		  else
			pos = enqueue_pos_.load(std::memory_order_relaxed);
		}
		cell->data_ = data;
		cell->sequence_.store(pos + 1, std::memory_order_release);
		return true;
	  }

	  bool dequeue(T& data)
	  {
		cell_t* cell;
		size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
		  cell = &buffer_[pos & buffer_mask_];
		  size_t seq =
			cell->sequence_.load(std::memory_order_acquire);
		  intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
		  if (dif == 0)
		  {
			if (dequeue_pos_.compare_exchange_weak
				(pos, pos + 1, std::memory_order_relaxed))
			  break;
		  }
		  else if (dif < 0)
			return false;
		  else
			pos = dequeue_pos_.load(std::memory_order_relaxed);
		}
		data = cell->data_;
		cell->sequence_.store
		  (pos + buffer_mask_ + 1, std::memory_order_release);
		return true;
	  }

	private:
	  struct cell_t
	  {
		std::atomic<size_t>   sequence_;
		T					 data_;
	  };

	  static size_t const	 cacheline_size = 64;
	  typedef char			cacheline_pad_t [cacheline_size];

	  cacheline_pad_t		 pad0_;
	  cell_t* const		   buffer_;
	  size_t const			buffer_mask_;
	  cacheline_pad_t		 pad1_;
	  std::atomic<size_t>	 enqueue_pos_;
	  cacheline_pad_t		 pad2_;
	  std::atomic<size_t>	 dequeue_pos_;
	  cacheline_pad_t		 pad3_;

	  mpmc_bounded_queue(mpmc_bounded_queue const&);
	  void operator = (mpmc_bounded_queue const&);
	};

	mpmc_bounded_queue<int> queue1024(pow(2, ceil(log(numInsertsPerThread * nThreads)/log(2))));

	void Enqueue1024Cores(int startingPoint)
	{
		for(int i = startingPoint; i < startingPoint + numInsertsPerThread; ++i)
		{
			queue1024.enqueue(i);
		}
	}

	std::atomic<int> count1024(0);

	void Dequeue1024Cores()
	{
		while(count1024 < numInsertsPerThread * nThreads)
		{
			int i;
			if(queue1024.dequeue(i))
			{
				++count1024;
			}
		}
	}
#if 0
	TEST_F(ConcurrentQueue, PerformanceComparesTo1024CoresQueue)
	{
		count = 0;
		count1024 = 0;
		int test;

		sprawl::collections::List<std::unique_ptr<sprawl::threading::Thread>> threads;
		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Enqueue, numInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(DequeueSimple)));
		}

		int64_t lockFreeStart = sprawl::time::Now(sprawl::time::Resolution::Milliseconds);

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		int64_t lockFreeTime = sprawl::time::Now(sprawl::time::Resolution::Milliseconds) - lockFreeStart;

		threads.Clear();

		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Enqueue1024Cores, numInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(Dequeue1024Cores)));
		}

		int64_t start1024 = sprawl::time::Now(sprawl::time::Resolution::Milliseconds);

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		int64_t time1024 = sprawl::time::Now(sprawl::time::Resolution::Milliseconds) - start1024;

#if defined(_WIN32)
	#define I64FMT "ll"
#elif defined(__APPLE__)
	#define I64FMT "ll"
#else
	#define I64FMT "l"
#endif
		printf("Lock free: %" I64FMT "d ms, 1024 cores queue: %" I64FMT "d ms\n", lockFreeTime, time1024 );
		EXPECT_LT(lockFreeTime, time1024);

		int i;
		ASSERT_FALSE(queue->Dequeue(i)) << "Item " << i << " was still in queue?";
	}
#endif

	const int numSlowInsertsPerThread = 10;
	sprawl::collections::ConcurrentQueue<int, 10>* queueSlow;
	std::atomic<int> totalResults(0);

	void EnqueueSlow(int startingPoint)
	{
		for(int i = startingPoint; i < startingPoint + numSlowInsertsPerThread; ++i)
		{
			queueSlow->Enqueue(i);
			sprawl::this_thread::Sleep(10);
		}
	}

	void DequeueSlow()
	{
		sprawl::collections::BasicHashMap<int, int> localResults;
		while(totalResults < numSlowInsertsPerThread * nThreads)
		{
			int i;
			if(!queueSlow->Dequeue(i))
			{
				sprawl::this_thread::Sleep(1);
				continue;
			}
			++localResults[i];
			++totalResults;
		}

		sprawl::threading::ScopedLock lock(mtx);
		for(auto& kvp : localResults)
		{
			results[kvp.Key()] += kvp.Value();
		}
	}
#if 1
	TEST_F(ConcurrentQueue, WorksWhenDequeueFasterThanEnqueue)
	{
		queueSlow = new sprawl::collections::ConcurrentQueue<int, 10>();
		results.Clear();;

		sprawl::collections::List<std::unique_ptr<sprawl::threading::Thread>> threads;
		for(int i = 0; i < nThreads; ++i)
		{
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(EnqueueSlow, numSlowInsertsPerThread * i)));
			threads.PushBack(std::unique_ptr<sprawl::threading::Thread>(new sprawl::threading::Thread(DequeueSlow)));
		}

		for(auto& thread : threads)
		{
			thread->Start();
		}
		for(auto& thread : threads)
		{
			thread->Join();
		}

		for(int i = 0; i < numSlowInsertsPerThread * nThreads; ++i)
		{
			EXPECT_TRUE(results.Has(i)) << "Item " << i << " is missing.";
			if(results.Has(i))
				ASSERT_EQ(1, results.Get(i)) << "Item " << i << " dequeued more than once.";
		}
		int i;
		ASSERT_FALSE(queueSlow->Dequeue(i)) << "Item " << i << " was still in the queue?";

		results.Clear();

		delete queueSlow;
	}
#endif
}
#endif
