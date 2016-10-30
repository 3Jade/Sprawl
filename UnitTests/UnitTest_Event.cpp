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

TEST_F(EventTest, EventDoesntReturnUntilNotified)
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
		ASSERT_TRUE(event.WaitFor(1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	event.Notify();
	t.Join();
}

TEST_F(EventTest, EventDoesntReturnUntilNotifiedWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_TRUE(event.WaitFor(1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
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
		ASSERT_FALSE(event.WaitFor(500 * sprawl::time::Resolution::Milliseconds));
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

TEST_F(EventTest, WaitingForAnyEventWorks)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));
	});
	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, WaitingForAnyEventWorks_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(event, event2, event3, event4, event5));
	});
	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventDoesntReturnUntilNotified)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventDoesntReturnUntilNotified_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(event, event2, event3, event4, event5));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventTriggersWhenNotifyBeforeWait)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));
	});

	t.Start();

	event4.Notify();
	event5.Notify();
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventTriggersWhenNotifyBeforeWait_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(event, event2, event3, event4, event5));
	});

	t.Start();

	event4.Notify();
	event5.Notify();
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventTimesOutWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitAnyFor(group, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, AnyEventTimesOutWithWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, AnyEventWokenUpEarlyWhenSignaledDuringWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAnyFor(group, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	event3.Notify();
	t.Join();
}

TEST_F(EventTest, AnyEventWokenUpEarlyWhenSignaledDuringWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	event3.Notify();
	t.Join();
}

TEST_F(EventTest, AnyEventDoesntReturnUntilNotifiedWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAnyFor(group, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventDoesntReturnUntilNotifiedWithWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventMultipleNotifiesWhileNotWaitingOnlyWakeOnce)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitAnyFor(group, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();

		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, AnyEventMultipleNotifiesWhileNotWaitingOnlyWakeOnce_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		ASSERT_EQ(nullptr, sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();

		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();
	event3.Notify();

	t.Join();
}

TEST_F(EventTest, NotifyDuringAnyEventProperlyClearsStateForSingleEvent)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(group));

		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(event3.WaitFor(500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();

	t.Join();
}

TEST_F(EventTest, NotifyDuringAnyEventProperlyClearsStateForSingleEvent_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_EQ(&event3, sprawl::threading::Event::WaitAny(event, event2, event3, event4, event5));

		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(event3.WaitFor(500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	event3.Notify();

	t.Join();
}

TEST_F(EventTest, WaitingForAllEventWorks)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::threading::Event::WaitAll(group);
	});
	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);

	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, WaitingForAllEventWorks_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::threading::Event::WaitAll(event, event2, event3, event4, event5);
	});
	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}

TEST_F(EventTest, AllEventDoesntReturnUntilNotified)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		sprawl::threading::Event::WaitAll(group);
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, AllEventDoesntReturnUntilNotified_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		sprawl::threading::Event::WaitAll(event, event2, event3, event4, event5);
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}

TEST_F(EventTest, AllEventTriggersWhenNotifyBeforeWait)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		sprawl::threading::Event::WaitAll(group);
	});

	t.Start();
	
	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, AllEventTriggersWhenNotifyBeforeWait_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		sprawl::threading::Event::WaitAll(event, event2, event3, event4, event5);
	});

	t.Start();

	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}

TEST_F(EventTest, AllEventTimesOutWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_FALSE(sprawl::threading::Event::WaitAllFor(group, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, AllEventTimesOutWithWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_FALSE(sprawl::threading::Event::WaitAllFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	t.Join();
}

TEST_F(EventTest, AllEventTimesOutWithWaitForWhenAllButOneEventNotified)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_FALSE(sprawl::threading::Event::WaitAllFor(group, 500 * sprawl::time::Resolution::Milliseconds));
	});

	sprawl::threading::Event::EventGroup subgroup;
	subgroup.PushBack(&event);
	subgroup.PushBack(&event2);
	subgroup.PushBack(&event3);
	subgroup.PushBack(&event4);
	t.Start();
	sprawl::threading::Event::NotifyAll(subgroup);
	t.Join();
}

TEST_F(EventTest, AllEventTimesOutWithWaitForWhenAllButOneEventNotified_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		ASSERT_FALSE(sprawl::threading::Event::WaitAllFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
	});

	t.Start();
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4);
	t.Join();
}

TEST_F(EventTest, AllEventWokenUpEarlyWhenSignaledDuringWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_TRUE(sprawl::threading::Event::WaitAllFor(group, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	sprawl::threading::Event::NotifyAll(group);
	t.Join();
}

TEST_F(EventTest, AllEventWokenUpEarlyWhenSignaledDuringWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_TRUE(sprawl::threading::Event::WaitAllFor(event, event2, event3, event4, event5, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);
	t.Join();
}

TEST_F(EventTest, AllEventDoesntReturnUntilNotifiedWithWaitFor)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_TRUE(sprawl::threading::Event::WaitAllFor(group, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, AllEventDoesntReturnUntilNotifiedWithWaitFor_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		int64_t start = sprawl::time::Now();
		ASSERT_TRUE(sprawl::threading::Event::WaitAllFor(event, event2, event3, event4, event5, 1000 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
		ASSERT_LT(end - start, 1000 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}

TEST_F(EventTest, AllEventMultipleNotifiesWhileNotWaitingOnlyWakeOnce)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		sprawl::threading::Event::WaitAll(group);

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(sprawl::threading::Event::WaitAnyFor(group, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();

		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::threading::Event::NotifyAll(group);
	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, AllEventMultipleNotifiesWhileNotWaitingOnlyWakeOnce_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::this_thread::Sleep(500 * sprawl::time::Resolution::Milliseconds);
		sprawl::threading::Event::WaitAll(event, event2, event3, event4, event5);

		//Second wait should take the full duration.
		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();

		ASSERT_GT(end - start, 450 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);
	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}

TEST_F(EventTest, NotifyDuringAllEventProperlyClearsStateForSingleEvent)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::threading::Event::WaitAll(group);

		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(sprawl::threading::Event::WaitAnyFor(group, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::threading::Event::NotifyAll(group);

	t.Join();
}

TEST_F(EventTest, NotifyDuringAllEventProperlyClearsStateForSingleEvent_Templated)
{
	sprawl::threading::Thread t([&]()
	{
		sprawl::threading::Event::WaitAll(event, event2, event3, event4, event5);

		int64_t start = sprawl::time::Now();
		ASSERT_FALSE(sprawl::threading::Event::WaitAnyFor(event, event2, event3, event4, event5, 500 * sprawl::time::Resolution::Milliseconds));
		int64_t end = sprawl::time::Now();
		ASSERT_GE(end - start, 500 * sprawl::time::Resolution::Milliseconds);
	});

	t.Start();

	sprawl::threading::Event::NotifyAll(event, event2, event3, event4, event5);

	t.Join();
}