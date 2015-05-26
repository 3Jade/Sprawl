#include "../threading/thread.hpp"
#include "../threading/mutex.hpp"
#include "../threading/condition_variable.hpp"
#include "../time/time.hpp"
#include "../threading/threadmanager.hpp"
#include "../threading/threadlocal.hpp"
#include "../threading/coroutine.hpp"

#include <unordered_map>
#include <unordered_set>

sprawl::threading::Mutex mtx;
sprawl::threading::Mutex mtx2;

sprawl::threading::ConditionVariable cond;

bool thread_success = true;
sprawl::threading::Handle mainThread = sprawl::this_thread::GetHandle();

sprawl::threading::ThreadManager manager;

std::unordered_map<int64_t, std::unordered_set<std::string>> threadCheck;
std::unordered_map<int64_t, int> threadLocalCheck;
sprawl::threading::ThreadLocal<int> threadLocalValue;
std::unordered_map<int64_t, int*> threadLocalPtrCheck;
sprawl::threading::ThreadLocal<int*> threadLocalPtr;

void DoSomethingInAThread(int& i)
{
	if(sprawl::this_thread::GetHandle() == mainThread)
	{
		printf("Thread is still main thread.\n... ");
		thread_success = false;
	}

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

	if(*threadLocalValue != threadLocalCheck[sprawl::this_thread::GetHandle().GetUniqueId()] || *threadLocalPtr != threadLocalPtrCheck[sprawl::this_thread::GetHandle().GetUniqueId()])
	{
		printf("Thread local storage fail\n... ");
		thread_success = false;
	}

	threadCheck[sprawl::this_thread::GetHandle().GetUniqueId()].insert(type);
	if(threadCheck[sprawl::this_thread::GetHandle().GetUniqueId()].size() != 1)
	{
		printf("Thread manager running a task on the wrong thread\n... ");
		thread_success = false;
	}
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
	return sprawl::threading::Generator<int>(std::bind(generator, start));
}

bool test_thread()
{
	printf("(This should take 3-4 seconds)... ");
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

	if(i != 5)
	{
		printf("Thread increment failed.\n... ");
		thread_success = false;
	}

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

	if(j != 100 || k != 100)
	{
		printf("ThreadManager increment failed. %d %d\n... ", j, k);
		thread_success = false;
	}

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
	if(count != ((((((((((((((((((((1+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2)+2)*2))
	{
		printf("Coroutines failed. %d\n... ", count);
		thread_success = false;
	}

	sprawl::threading::CoroutineWithChannel<int,int> addOverTimeCrt(std::bind(addOverTime, 1));
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
	if(num != 1 + 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512)
	{
		printf("Coroutine with channel failed. %d\n... ", num);
		thread_success = false;
	}

	sprawl::threading::CoroutineWithChannel<int, void> addOverTime2Crt(std::bind(addOverTime2, 1));
	num = 1;
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
	if(addOverTime2Value != 11)
	{
		printf("Coroutine with channel failed. %d\n... ", addOverTime2Value);
		thread_success = false;
	}

	sprawl::threading::CoroutineWithChannel<void, int> addOverTime3Crt(std::bind(addOverTime3, 1));
	num = 1;
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
	if(num != 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11)
	{
		printf("Coroutine with channel failed. %d\n... ", num);
		thread_success = false;
	}

	sprawl::threading::Generator<int> generator = numberGenerator(1);

	for(int i = 1; i < 10; ++i)
	{
		if(i % 2 != 0)
		{
			if(generator.Resume() != i)
			{
				printf("Generator failed. %d\n... ", num);
				thread_success = false;
			}
		}
		else
		{
			if(generator() != i)
			{
				printf("Generator failed. %d\n... ", num);
				thread_success = false;
			}
		}
	}

	return thread_success;
}
