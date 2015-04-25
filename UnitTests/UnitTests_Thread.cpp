#include "../threading/thread.hpp"
#include "../threading/mutex.hpp"
#include "../threading/condition_variable.hpp"
#include "../time/time.hpp"
#include "../threading/threadmanager.hpp"

#include <unordered_map>
#include <unordered_set>

sprawl::threading::Mutex mtx;
sprawl::threading::Mutex mtx2;

sprawl::threading::ConditionVariable cond;

bool thread_success = true;
sprawl::threading::Handle mainThread = sprawl::this_thread::GetHandle();

sprawl::threading::ThreadManager manager;

std::unordered_map<int64_t, std::unordered_set<std::string>> threadCheck;

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

bool shouldStop = false;
void DoSomethingInAThread2(int& i, char const* const type)
{
	sprawl::threading::ScopedLock lock(mtx);

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
	for(int count = 0; count < 100; ++count)
	{
		manager.AddTask(std::bind(DoSomethingInAThread2, std::ref(j), "1"), 1);
	}
	for(int count = 0; count < 100; ++count)
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

	return thread_success;
}