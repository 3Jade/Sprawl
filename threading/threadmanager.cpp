#include "threadmanager.hpp"


inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task&& what_, uint64_t where_, int64_t when_)
	: what(std::move(what_))
	, where(where_)
	, when(when_)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(const sprawl::threading::ThreadManager::Task& what_, uint64_t where_, int64_t when_)
	: what(what_)
	, where(where_)
	, when(when_)
{
	//
}


void sprawl::threading::ThreadManager::AddThread(uint64_t threadFlags, const char* const threadName, uint64_t secondaryFlags)
{
	m_threads.PushFront(new Thread(threadName, std::bind(&ThreadManager::eventLoop_, this, threadFlags, secondaryFlags)));
}


void sprawl::threading::ThreadManager::AddThread(uint64_t threadFlags, uint64_t secondaryFlags)
{
	m_threads.PushFront(new Thread(std::bind(&ThreadManager::eventLoop_, this, threadFlags, secondaryFlags)));
}


void sprawl::threading::ThreadManager::AddThreads(uint64_t threadFlags, int count, const char* const threadName, uint64_t secondaryFlags)
{
	for(int i = 0; i < count; ++i)
	{
		AddThread(threadFlags, threadName, secondaryFlags);
	}
}


void sprawl::threading::ThreadManager::AddThreads(uint64_t threadFlags, int count, uint64_t secondaryFlags)
{
	for(int i = 0; i < count; ++i)
	{
		AddThread(threadFlags, secondaryFlags);
	}
}

void sprawl::threading::ThreadManager::AddTask(sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(TaskInfo(std::move(task), threadFlags, whenNanosecs));
}

void sprawl::threading::ThreadManager::AddTask(const sprawl::threading::ThreadManager::Task& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(TaskInfo(task, threadFlags, whenNanosecs));
}

void sprawl::threading::ThreadManager::AddFutureTask(sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(std::move(task), threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ThreadManager::AddFutureTask(const sprawl::threading::ThreadManager::Task& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(task, threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ThreadManager::Run(uint64_t thisThreadFlags, uint64_t secondaryFlags)
{
	m_running = true;
	for(auto& thread : m_threads)
	{
		thread->Start();
	}
	eventLoop_(thisThreadFlags, secondaryFlags);
}

void sprawl::threading::ThreadManager::Start(uint64_t thisThreadFlags, uint64_t secondaryFlags)
{
	m_running = true;
	m_callingThreadFlags = thisThreadFlags;
	m_callingThreadSecondaryFlags = secondaryFlags;
	for(auto& thread : m_threads)
	{
		thread->Start();
	}
}

void sprawl::threading::ThreadManager::Pump()
{

	Task taskToExecute;
	collections::List<TaskInfo>::iterator secondaryTask = m_taskQueue.end();
	{
		ScopedLock lock(m_mutex);
		int64_t now = time::Now(time::Resolution::Nanoseconds);
		for(auto it = m_taskQueue.begin(); it != m_taskQueue.end(); )
		{
			auto delete_it = it++;
			if(delete_it->when > now)
			{
				break;
			}

			if((delete_it->where & m_callingThreadFlags) == 0)
			{
				if((delete_it->where & m_callingThreadSecondaryFlags) == 0 && secondaryTask == m_taskQueue.end())
				{
					secondaryTask = delete_it;
				}
				continue;
			}

			taskToExecute = delete_it->what;
			m_taskQueue.Erase(delete_it);
			break;
		}
		if(!taskToExecute && secondaryTask != m_taskQueue.end() && (secondaryTask->when - now) > m_secondaryTaskWindow)
		{
			taskToExecute = secondaryTask->what;
			m_taskQueue.Erase(secondaryTask);
		}
	}
	if(taskToExecute)
	{
		taskToExecute();
	}
}

void sprawl::threading::ThreadManager::Wait()
{

	SharedLock lock(m_mutex);
	while(m_running)
	{
		int64_t now = time::Now(time::Resolution::Nanoseconds);
		int64_t nextTaskTime = -1;
		for(auto it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it)
		{
			if((it->where & m_callingThreadFlags) == 0 && (it->where & m_callingThreadSecondaryFlags) == 0)
			{
				continue;
			}

			if(it->when <= now)
			{
				return;
			}

			nextTaskTime = it->when;
			break;
		}

		if(nextTaskTime >= 0)
		{
			m_conditionVariable.WaitUntil(lock, nextTaskTime);
		}
		else
		{
			m_conditionVariable.Wait(lock);
		}
	}
}

void sprawl::threading::ThreadManager::Stop()
{
	{
		ScopedLock lock(m_mutex);
		m_running = false;
	}
	m_conditionVariable.NotifyAll();
}

void sprawl::threading::ThreadManager::ShutDown()
{
	for(auto& thread : m_threads)
	{
		thread->Join();
		delete thread;
	}
	m_threads.Clear();
}

void sprawl::threading::ThreadManager::pushTask_(TaskInfo&& task)
{
	{
		ScopedLock lock(m_mutex);
		bool inserted = false;

		for(auto it = m_taskQueue.begin(); it != m_taskQueue.end(); ++it)
		{
			if(task.when < it->when)
			{
				m_taskQueue.Insert(it, std::move(task));
				inserted = true;
				break;
			}
		}

		if(!inserted)
		{
			m_taskQueue.PushBack(std::move(task));
		}
	}
	m_conditionVariable.NotifyAll();
}

void sprawl::threading::ThreadManager::eventLoop_(uint64_t flags, uint64_t secondaryFlags)
{
	while(m_running)
	{
		Task taskToExecute;
		{
			SharedLock lock(m_mutex);
			collections::List<TaskInfo>::iterator secondaryTask = m_taskQueue.end();
			while(m_running && !taskToExecute)
			{
				int64_t now = time::Now(time::Resolution::Nanoseconds);
				int64_t nextTaskTime = -1;
				for(auto it = m_taskQueue.begin(); it != m_taskQueue.end(); )
				{
					auto delete_it = it++;
					if((delete_it->where & flags) == 0)
					{
						if((delete_it->where & secondaryFlags) != 0 && delete_it->when <= now && secondaryTask == m_taskQueue.end())
						{
							secondaryTask = delete_it;
						}
						continue;
					}

					if(delete_it->when <= now && !taskToExecute)
					{
						taskToExecute = delete_it->what;
						m_taskQueue.Erase(delete_it);
						continue;
					}

					nextTaskTime = delete_it->when;
					break;
				}
				if(!taskToExecute)
				{
					if(secondaryTask != m_taskQueue.end() && (secondaryTask->when - now) > m_secondaryTaskWindow)
					{
						taskToExecute = secondaryTask->what;
						m_taskQueue.Erase(secondaryTask);
					}
					else
					{
						if(nextTaskTime >= 0)
						{
							m_conditionVariable.WaitUntil(lock, nextTaskTime);
						}
						else
						{
							m_conditionVariable.Wait(lock);
						}
					}
				}
			}
			if(!m_running)
			{
				return;
			}
		}
		taskToExecute();
		taskToExecute = nullptr;
	}
}