#include "../threading/thread.hpp"
#include "../threading/mutex.hpp"
#include "../threading/condition_variable.hpp"
#include "../time/time.hpp"
#include "../threading/threadmanager.hpp"
#include "../threading/threadlocal.hpp"
#include "../threading/coroutine.hpp"

#include <unordered_map>
#include <unordered_set>

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

sprawl::threading::Mutex mtx;
sprawl::threading::Mutex mtx2;

sprawl::threading::ConditionVariable cond;

sprawl::threading::Handle mainThread = sprawl::this_thread::GetHandle();

sprawl::threading::ThreadManager manager;

std::unordered_map<int64_t, std::unordered_set<std::string>> threadCheck;
std::unordered_map<int64_t, int> threadLocalCheck;
sprawl::threading::ThreadLocal<int> threadLocalValue;
std::unordered_map<int64_t, int*> threadLocalPtrCheck;
sprawl::threading::ThreadLocal<int*> threadLocalPtr;

void DoSomethingInAThread(int& i)
{
	EXPECT_NE(mainThread, sprawl::this_thread::GetHandle()) << "Thread is still main thread.\n... ";

	sprawl::threading::ScopedLock lock(mtx);
	++i;
	if(i >= 5)
	{
		sprawl::this_thread::Sleep(1 * sprawl::time::Resolution::Seconds);
		cond.Notify();
	}
}

int curr = 0;

bool shouldStop = false;
void DoSomethingInAThread2(int& i, char const* const type)
{
	sprawl::threading::ScopedLock lock(mtx);

	if(!threadLocalCheck.count(sprawl::this_thread::GetHandle().GetUniqueId()))
	{
		threadLocalValue = curr;
		threadLocalCheck[sprawl::this_thread::GetHandle().GetUniqueId()] = curr++;
		threadLocalPtrCheck[sprawl::this_thread::GetHandle().GetUniqueId()] = new int(0);
		threadLocalPtr = threadLocalPtrCheck[sprawl::this_thread::GetHandle().GetUniqueId()];
	}

	EXPECT_EQ(threadLocalCheck[sprawl::this_thread::GetHandle().GetUniqueId()], *threadLocalValue) << "Thread local storage failed";
	EXPECT_EQ(threadLocalPtrCheck[sprawl::this_thread::GetHandle().GetUniqueId()], *threadLocalPtr) << "Thread local storage failed";

	threadCheck[sprawl::this_thread::GetHandle().GetUniqueId()].insert(type);
	EXPECT_EQ(size_t(1), threadCheck[sprawl::this_thread::GetHandle().GetUniqueId()].size()) << "Thread manager running a task on the wrong thread";

	++i;
	if(i >= 100)
	{
		if(shouldStop)
		{
			manager.Stop();
		}
		else
		{
			shouldStop = true;
		}
	}
}

int count = 1;
int mod = 2;

void add()
{
	for(int idx = 0; idx < 10; ++idx)
	{
		count += mod;
		sprawl::threading::Coroutine::Yield();
	}
}

void mult()
{
	for(int idx = 0; idx < 10; ++idx)
	{
		count *= mod;
		sprawl::threading::Coroutine::Yield();
	}
}

void addOverTime(int start)
{
	for(;;)
	{
		start += sprawl::threading::Coroutine::Receive<int, int>(start);
	}
}

int addOverTime2Value;

void addOverTime2(int start)
{
	addOverTime2Value = start;
	for(;;)
	{
		addOverTime2Value += sprawl::threading::Coroutine::Receive<int>();
	}
}

void addOverTime3(int start)
{
	for(;;)
	{
		sprawl::threading::Coroutine::Yield(++start);
	}
}

sprawl::threading::Generator<int> numberGenerator(int start)
{
	auto generator = [](int start)
	{
		for(;;)
		{
			sprawl::threading::Coroutine::Yield(start++);
		}
	};
	return sprawl::threading::Generator<int>(generator, start);
}

TEST(ThreadingTest, ThreadsAndMutexesWork)
{
	printf("(This should take a second)...\n");
	fflush(stdout);
	int i = 0;
	sprawl::threading::Thread thread1(DoSomethingInAThread, std::ref(i));
	sprawl::threading::Thread thread2(DoSomethingInAThread, std::ref(i));
	sprawl::threading::Thread thread3(DoSomethingInAThread, std::ref(i));
	sprawl::threading::Thread thread4(DoSomethingInAThread, std::ref(i));
	sprawl::threading::Thread thread5(DoSomethingInAThread, std::ref(i));

	thread1.Start();
	thread2.Start();
	thread3.Start();
	thread4.Start();
	thread5.Start();

	sprawl::threading::SharedLock lock(mtx2);
	cond.Wait(lock);

	thread1.Join();
	thread2.Join();
	thread3.Join();
	thread4.Join();
	thread5.Join();

	ASSERT_EQ(5, i) << "Thread increment failed.";
}

TEST(ThreadingTest, ThreadManagerWorks)
{
	printf("(This should take a second)...\n");
	fflush(stdout);
	int j = 0;
	int k = 0;
	manager.AddThreads(1, 5);
	manager.AddThreads(2, 5);
	for(int idx = 0; idx < 100; ++idx)
	{
		manager.AddTask(std::bind(DoSomethingInAThread2, std::ref(j), "1"), 1);
	}
	for(int idx = 0; idx < 100; ++idx)
	{
		manager.AddFutureTask(std::bind(DoSomethingInAThread2, std::ref(k), "2"), 2, sprawl::time::Resolution::Seconds * 2);
	}
	manager.Run(0);
	manager.ShutDown();

	EXPECT_EQ(100, j) << "ThreadManager failed to properly increment j";
	EXPECT_EQ(100, k) << "ThreadManager failed to properly increment k";
}

int stage = 0;
int stageValue = 0;

void SetupStageOne()
{
	stage = 1;
}

void RunStageOne()
{
	{
		sprawl::threading::ScopedLock lock(mtx);
		EXPECT_EQ(1, stage);
		stageValue += 1;
	}
	sprawl::this_thread::Sleep((rand() % 10) * sprawl::time::Resolution::Milliseconds);
}

void SetupStageTwo()
{
	stage = 2;
}

void RunStageTwo()
{
	{
		sprawl::threading::ScopedLock lock(mtx);
		EXPECT_EQ(2, stage);
		stageValue *= 2;
	}
	sprawl::this_thread::Sleep((rand() % 10) * sprawl::time::Resolution::Milliseconds);
}

TEST(ThreadingTest, StagedThreadManagerWorks)
{
	enum Stages : uint64_t
	{
		AnyStage = 0,
		StageOne_Setup = 1 << 0,
		StageOne_Run = 1 << 1,
		StageTwo_Setup = 1 << 2,
		StageTwo_Run = 1 << 3,
	};

	enum ThreadFlags : uint64_t
	{
		Any = 1
	};

	sprawl::threading::ThreadManager stagedManager;
	stagedManager.SetMaxStage(Stages::StageTwo_Run);
	stagedManager.AddTaskStaged(StageOne_Setup, SetupStageOne, Any);
	for(int i = 0; i < 100; ++i)
	{
		stagedManager.AddTaskStaged(StageOne_Run, RunStageOne, Any);
	}
	stagedManager.AddTaskStaged(StageTwo_Setup, SetupStageTwo, Any);
	for(int i = 0; i < 20; ++i)
	{
		stagedManager.AddTaskStaged(StageTwo_Run, RunStageTwo, Any);
	}

	stagedManager.AddThreads(Any, 5);

	stagedManager.Start(Any);

	EXPECT_EQ(StageOne_Setup, stagedManager.RunNextStage());
	EXPECT_EQ(1, stage);
	EXPECT_EQ(0, stageValue);
	EXPECT_EQ(StageOne_Run, stagedManager.RunNextStage());
	EXPECT_EQ(1, stage);
	EXPECT_EQ(100, stageValue);
	EXPECT_EQ(StageTwo_Setup, stagedManager.RunNextStage());
	EXPECT_EQ(2, stage);
	EXPECT_EQ(100, stageValue);
	EXPECT_EQ(StageTwo_Run, stagedManager.RunNextStage());
	EXPECT_EQ(2, stage);
	EXPECT_EQ(100 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2, stageValue);

	//Should start over now, but NOT rerun the old task...
	EXPECT_EQ(StageOne_Setup, stagedManager.RunNextStage());
	EXPECT_EQ(2, stage);
	EXPECT_EQ(100 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2, stageValue);

	stagedManager.ShutDown();
}

TEST(CoroutineTest, BasicCoroutinesWork)
{
	sprawl::threading::Coroutine addCrt(add);
	sprawl::threading::Coroutine multCrt(mult);

	bool continuing = true;
	while(continuing)
	{
		continuing = false;
		if(addCrt.State() != sprawl::threading::Coroutine::CoroutineState::Completed)
		{
			continuing = true;
			addCrt();
		}
		if(multCrt.State() != sprawl::threading::Coroutine::CoroutineState::Completed)
		{
			continuing = true;
			multCrt.Resume();
		}
	}
	//Order of operations test - if the two functions didn't alternate as intended this will fail.
	ASSERT_EQ( ((((((((((((((((((((1+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2), count ) << "Basic coroutines failed.";
}

TEST(CoroutineTest, BidirectionalChannelsWork)
{
	sprawl::threading::CoroutineWithChannel<int,int> addOverTimeCrt(addOverTime, 1);
	int num = addOverTimeCrt.Start();
	for(int j = 0; j < 10; ++j)
	{
		if(j % 2 == 0)
		{
			num = addOverTimeCrt.Send(num);
		}
		else
		{
			num = addOverTimeCrt(num);
		}
	}
	ASSERT_EQ(1 + 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512, num) << "Bidirectional channel failed.";
}

TEST(CoroutineTest, SendOnlyChannelsWork)
{
	sprawl::threading::CoroutineWithChannel<int, void> addOverTime2Crt(addOverTime2, 1);
	int num = 1;
	addOverTime2Crt.Start();
	for(int j = 0; j < 10; ++j)
	{
		if(j % 2 == 0)
		{
			addOverTime2Crt.Send(num);
		}
		else
		{
			addOverTime2Crt(num);
		}
	}
	ASSERT_EQ(11, addOverTime2Value) << "Send-only channel failed";
}

TEST(CoroutineTest, ReceiveOnlyChannelsWork)
{
	sprawl::threading::CoroutineWithChannel<void, int> addOverTime3Crt(addOverTime3, 1);
	int num = 1;
	for(int j = 0; j < 10; ++j)
	{
		if(j % 2 == 0)
		{
			num += addOverTime3Crt.Resume();
		}
		else
		{
			num += addOverTime3Crt();
		}
	}
	ASSERT_EQ(1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11, num) << "Receive-only channel failed";
}

TEST(CoroutineTest, GeneratorsWork)
{
	sprawl::threading::Generator<int> generator = numberGenerator(1);

	for(int i = 1; i < 10; ++i)
	{
		if(i % 2 != 0)
		{
			EXPECT_EQ(i, generator.Resume()) << "Generator Resume() failed";
		}
		else
		{
			EXPECT_EQ(i, generator()) << "Generator operator() failed";
		}
	}
}

void DoAThing()
{

}

void DoAThing2(int)
{

}

TEST(CoroutineTest, StackSizeInitializedProperly)
{
	sprawl::threading::Coroutine c1(nullptr, 200);
	ASSERT_EQ(200ul, c1.StackSize());

	std::function<void()> fn(nullptr);
	sprawl::threading::Coroutine c2(fn, 200);
	ASSERT_EQ(200ul, c2.StackSize());

	sprawl::threading::Coroutine c3(DoAThing, 200);
	ASSERT_EQ(200ul, c3.StackSize());

	sprawl::threading::Coroutine c4(std::bind(DoAThing2, 3), 200);
	ASSERT_EQ(200ul, c4.StackSize());
}