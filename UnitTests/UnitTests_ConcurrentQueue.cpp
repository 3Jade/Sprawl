#define SPRAWL_CONCURRENT_QUEUE_DEBUG_ASSERTS 1

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
	constexpr int numInsertsPerThread = 1000000;
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
		sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>::ReadReservationTicket ticket;
		queue->InitializeReservationTicket(ticket);
		for (int i = 0; i < numInsertsPerThread; ++i)
		{
			int j;
			while(!queue->Dequeue(j, ticket))
			{
			}
			++localResults[j];
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
		sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>::ReadReservationTicket ticket;
		queue->InitializeReservationTicket(ticket);
		while(count < numInsertsPerThread * nThreads)
		{
			int i;
			if(queue->Dequeue(i, ticket))
			{
				++count;
			}
		}
	}

#if 1
	TEST_F(ConcurrentQueue, WorksOnManyThreads)
	{
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
			EXPECT_TRUE(results.Has(i)) << "Item " << i << " is missing.";
			if (results.Has(i))
				ASSERT_EQ(1, results.Get(i)) << "Item " << i << " dequeued more than once.";
		}
		int i;
		sprawl::collections::ConcurrentQueue<int, numInsertsPerThread * nThreads>::ReadReservationTicket ticket;
		queue->InitializeReservationTicket(ticket);
		ASSERT_FALSE(queue->Dequeue(i, ticket)) << "Item " << i << " was still in the queue?";
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

	const int numSlowInsertsPerThread = 1000;
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
		sprawl::collections::ConcurrentQueue<int, 10>::ReadReservationTicket ticket;
		queueSlow->InitializeReservationTicket(ticket);
		while(totalResults < numSlowInsertsPerThread * nThreads)
		{
			int i;
			if(!queueSlow->Dequeue(i, ticket))
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
		sprawl::collections::ConcurrentQueue<int, 10>::ReadReservationTicket ticket;
		queueSlow->InitializeReservationTicket(ticket);
		ASSERT_FALSE(queueSlow->Dequeue(i, ticket)) << "Item " << i << " was still in the queue?";

		results.Clear();

		delete queueSlow;
	}
#endif
}
#endif
