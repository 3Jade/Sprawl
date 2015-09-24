#include "threadmanager.hpp"


inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task&& what_, uint64_t where_, int64_t when_)
	: what(std::move(what_))
	, where(where_)
	, when(when_)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task const& what_, uint64_t where_, int64_t when_)
	: what(what_)
	, where(where_)
	, when(when_)
{
	//
}


sprawl::threading::ThreadManager::ThreadManager()
	: m_taskQueue()
	, m_currentStage(0)
	, m_maxStage(0)
	, m_threads()
	, m_mutex()
	, m_conditionVariable()
	, m_callingThreadFlags(0)
	, m_callingThreadSecondaryFlags(0)
	, m_secondaryTaskWindow(0)
	, m_running(false)
	, m_syncing(false)
	, m_syncWait()
	, m_threadSyncWait()
	, m_numThreadsSynced(0)
{

}

sprawl::threading::ThreadManager::~ThreadManager()
{
	for(auto& thread : m_threads)
	{
		delete thread;
	}
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
	pushTask_(TaskInfo(std::move(task), threadFlags, whenNanosecs), m_currentStage);
}

void sprawl::threading::ThreadManager::AddTask(sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(TaskInfo(task, threadFlags, whenNanosecs), m_currentStage);
}

void sprawl::threading::ThreadManager::AddTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(TaskInfo(std::move(task), threadFlags, whenNanosecs), stage);
}

void sprawl::threading::ThreadManager::AddTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(TaskInfo(task, threadFlags, whenNanosecs), stage);
}

void sprawl::threading::ThreadManager::SetNumStages(uint64_t stageCount)
{
	m_maxStage = stageCount - 1;
}

void sprawl::threading::ThreadManager::AddFutureTask(sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(std::move(task), threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ThreadManager::AddFutureTask(sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(task, threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ThreadManager::AddFutureTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTaskStaged(stage, std::move(task), threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ThreadManager::AddFutureTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTaskStaged(stage, task, threadFlags, nanosecondsFromNow + time::Now(time::Resolution::Nanoseconds));
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

void sprawl::threading::ThreadManager::RunStaged(uint64_t thisThreadFlags, uint64_t secondaryFlags)
{
	Start(thisThreadFlags, secondaryFlags);

	m_running = true;
	for(auto& thread : m_threads)
	{
		thread->Start();
	}

	while(m_running)
	{
		RunNextStage();
	}
}

void sprawl::threading::ThreadManager::Start(uint64_t thisThreadFlags, uint64_t secondaryFlags)
{
	m_running = true;
	m_callingThreadFlags = thisThreadFlags;
	m_callingThreadSecondaryFlags = secondaryFlags;
	if(m_maxStage != 0)
	{
		m_syncing = true;
	}
	for(auto& thread : m_threads)
	{
		thread->Start();
	}
	if(m_maxStage != 0)
	{
		SharedLock lock(m_mutex);
		while(m_numThreadsSynced < m_threads.Size())
		{
			m_syncWait.Wait(lock);
		}
	}
}

void sprawl::threading::ThreadManager::Pump()
{
	while(true)
	{
		Task taskToExecute;
		{
			ScopedLock lock(m_mutex);
			collections::List<TaskInfo>& queue = m_taskQueue[m_currentStage];
			collections::List<TaskInfo>::iterator secondaryTask = queue.end();
			{
				int64_t now = time::Now(time::Resolution::Nanoseconds);
				for(auto it = queue.begin(); it != queue.end(); )
				{
					auto delete_it = it++;
					if(delete_it->when > now)
					{
						break;
					}

					if((delete_it->where & m_callingThreadFlags) == 0)
					{
						if((delete_it->where & m_callingThreadSecondaryFlags) == 0 && secondaryTask == queue.end())
						{
							secondaryTask = delete_it;
						}
						continue;
					}

					taskToExecute = delete_it->what;
					queue.Erase(delete_it);
					break;
				}
				if(!taskToExecute && secondaryTask != queue.end() && (secondaryTask->when - now) > m_secondaryTaskWindow)
				{
					taskToExecute = secondaryTask->what;
					queue.Erase(secondaryTask);
				}
			}
		}
		if(taskToExecute)
		{
			taskToExecute();
		}
		else
		{
			return;
		}
	}
}

uint64_t sprawl::threading::ThreadManager::RunNextStage()
{
	{
		SharedLock lock(m_mutex);

		m_numThreadsSynced = 0;
		m_syncing = false;
		m_threadSyncWait.NotifyAll();

		while(m_numThreadsSynced < m_threads.Size())
		{
			m_conditionVariable.NotifyAll();
			m_syncWait.Wait(lock);
		}
		m_numThreadsSynced = 0;
	}

	Pump();

	SharedLock lock(m_mutex);

	m_syncing = true;

	while(m_numThreadsSynced < m_threads.Size())
	{
		m_conditionVariable.NotifyAll();
		m_syncWait.Wait(lock);
	}

	++m_currentStage;
	if(m_currentStage > m_maxStage)
	{
		m_currentStage = 0;
	}

	return m_currentStage == 0 ? m_maxStage : m_currentStage - 1;
}

void sprawl::threading::ThreadManager::Wait()
{
	SharedLock lock(m_mutex);
	collections::List<TaskInfo>& queue = m_taskQueue[m_currentStage];
	while(m_running)
	{
		int64_t now = time::Now(time::Resolution::Nanoseconds);
		int64_t nextTaskTime = -1;
		for(auto it = queue.begin(); it != queue.end(); ++it)
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

void sprawl::threading::ThreadManager::Sync()
{
	SharedLock lock(m_mutex);
	while(!m_taskQueue[m_currentStage].Empty())
	{
		m_conditionVariable.NotifyAll();
		m_syncWait.Wait(lock);
	}
	m_threadSyncWait.NotifyAll();
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
	if(m_running)
	{
		Stop();
	}
	m_syncing = false;
	m_threadSyncWait.NotifyAll();
	for(auto& thread : m_threads)
	{
		thread->Join();
		delete thread;
	}
	m_threads.Clear();
}

void sprawl::threading::ThreadManager::pushTask_(TaskInfo&& task, uint64_t stage)
{
	{
		ScopedLock lock(m_mutex);
		bool inserted = false;

		collections::List<TaskInfo>& queue = m_taskQueue[stage];
		for(auto it = queue.begin(); it != queue.end(); ++it)
		{
			if(task.when < it->when)
			{
				queue.Insert(it, std::move(task));
				inserted = true;
				break;
			}
		}

		if(!inserted)
		{
			queue.PushBack(std::move(task));
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
			collections::List<TaskInfo>& queue = m_taskQueue[m_currentStage];
			collections::List<TaskInfo>::iterator secondaryTask = queue.end();
			while(m_running && !taskToExecute)
			{
				int64_t now = time::Now(time::Resolution::Nanoseconds);
				int64_t nextTaskTime = -1;
				for(auto it = queue.begin(); it != queue.end(); )
				{
					auto delete_it = it++;
					if((delete_it->where & flags) == 0)
					{
						if((delete_it->where & secondaryFlags) != 0 && delete_it->when <= now && secondaryTask == queue.end())
						{
							secondaryTask = delete_it;
						}
						continue;
					}

					if(delete_it->when <= now && !taskToExecute)
					{
						taskToExecute = delete_it->what;
						queue.Erase(delete_it);
						continue;
					}

					nextTaskTime = delete_it->when;
					break;
				}
				if(!taskToExecute)
				{
					if(secondaryTask != queue.end() && (secondaryTask->when - now) > m_secondaryTaskWindow)
					{
						taskToExecute = secondaryTask->what;
						queue.Erase(secondaryTask);
					}
					else
					{
						if(!m_syncing)
						{
							//This notify is for the Sync() function.
							//The other m_syncing case is for RunNextStage()
							//Easiest to reuse the same variable for both cases.
							m_syncWait.Notify();
							if(nextTaskTime >= 0)
							{
								m_conditionVariable.WaitUntil(lock, nextTaskTime);
							}
							else
							{
								m_conditionVariable.Wait(lock);
							}
						}

						if(m_syncing)
						{
							++m_numThreadsSynced;
							while(m_syncing)
							{
								m_syncWait.Notify();
								m_threadSyncWait.Wait(lock);
							}
							++m_numThreadsSynced;
							m_syncWait.Notify();
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
