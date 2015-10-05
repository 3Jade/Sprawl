#include "../../threading/event.hpp"
#include "../../threading/thread.hpp"
#include "../../time/time.hpp"
#include <gtest/gtest.h>
#include "../../threading/condition_variable.hpp"
#include "../../collections/ConcurrentQueue.hpp"

class EventTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		group.PushBack(&event);
		group.PushBack(&event2);
		group.PushBack(&event3);
		group.PushBack(&event4);
		group.PushBack(&event5);
	}

	void WaitForStartup()
	{
		do
		{
			startupEvent.Wait();
		} while(threadCount != 0);
	}

	std::atomic<int> threadCount;
	sprawl::threading::Event startupEvent;
	sprawl::threading::Event event;
	sprawl::threading::Event event2;
	sprawl::threading::Event event3;
	sprawl::threading::Event event4;
	sprawl::threading::Event event5;
	sprawl::threading::Event::EventGroup group;
};

TEST_F(EventTest, SimpleEventWorks)
{
	sprawl::threading::Thread t([&]()
	{
		event.Wait();
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event.Notify();

	t.Join();
}

TEST_F(EventTest, EventDoesntReturnEarly)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		event.Wait();
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event.Notify();

	t.Join();
}

TEST_F(EventTest, EventTriggersWhenNotifyBeforeWait)
{
	sprawl::threading::Thread t([&](){
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		event.Wait();
	});

	t.Start();

	event.Notify();

	t.Join();
}

TEST_F(EventTest, EventTimesOutWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_FALSE(event.WaitFor(500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, EventWokenUpEarlyWhenSignaledDuringWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		event.WaitFor(1000 * sprawl::time::Resolution::Milliseconds);
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	event.Notify();
	t.Join();
}

TEST_F(EventTest, EventDoesntReturnEarlyWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		event.WaitFor(1000 * sprawl::time::Resolution::Milliseconds);
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event.Notify();

	t.Join();
}

TEST_F(EventTest, MultipleNotifiesWhileNotWaitingOnlyWakeOnce)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		event.Wait();

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		event.WaitFor(500 * sprawl::time::Resolution::Milliseconds);
		int64_t end = sprawl::time::Now();

		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event.Notify();
	event.Notify();

	t.Join();
}

TEST_F(EventTest, NotifyOnlyWakesOneThread)
{
	std::atomic<int> nWoken(0);
	sprawl::threading::Thread t([&]()
	{
		if(event.WaitFor(500 * sprawl::time::Resolution::Milliseconds))
		{
			++nWoken;
		}
	});
	sprawl::threading::Thread t2([&]()
	{
		if(event.WaitFor(500 * sprawl::time::Resolution::Milliseconds))
		{
			++nWoken;
		}
	});

	t.Start();
	t2.Start();

	event.Notify();

	t.Join();
	t2.Join();

	ASSERT_EQ(1, nWoken.load());
}

TEST_F(EventTest, WaitingForMultipleEventsWorks)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitMultiple(group));
	});
	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, MultiEventTriggersWhenNotifyBeforeWait)
{
	sprawl::threading::Thread t([&](){
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitMultiple(group));
	});

	t.Start();

	event4.Notify();
	event5.Notify();
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, MultiEventTimesOutWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitMultipleFor(group, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, MultiEventWokenUpEarlyWhenSignaledDuringWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitMultipleFor(group, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	event3.Notify();
	t.Join();
}

TEST_F(EventTest, MultiEventMultipleNotifiesWhileNotWaitingOnlyWakeOnce)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitMultiple(group));

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitMultipleFor(group, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();

		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();
	event3.Notify();

	t.Join();
}
// Invalid test. Multi-event + multi-thread is verbotin. Not reliable and not supported.
//
//TEST_F(EventTest, MultiEventNotifyOnlyWakesOneThread)
//{
//	std::atomic<int> nWoken(0);
//	sprawl::threading::Thread t([&]()
//	{
//		if(sprawl::threading::Event::WaitMultipleFor(group, 500 * sprawl::time::Resolution::Milliseconds) != nullptr)
//		{
//			++nWoken;
//		}
//	});
//	sprawl::threading::Thread t2([&]()
//	{
//		if(sprawl::threading::Event::WaitMultipleFor(group, 500 * sprawl::time::Resolution::Milliseconds) != nullptr)
//		{
//			++nWoken;
//		}
//	});
//
//	t.Start();
//	t2.Start();
//
//	event3.Notify();
//
//	t.Join();
//	t2.Join();
//
//	ASSERT_EQ(1, nWoken.load());
//}

TEST_F(EventTest, NotifyDuringMultiEventProperlyClearsStateForSingleEvent)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitMultiple(group));

		int64_t start = sprawl::time::Now();
		event3.WaitFor(500 * sprawl::time::Resolution::Milliseconds);
		int64_t end = sprawl::time::Now();
		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();

	t.Join();
}