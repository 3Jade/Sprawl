#include "threadmanager.hpp"


inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo()
	: what(nullptr)
	, taken(false)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task&& what_, uint64_t where_, int64_t when_)
	: what(std::move(what_))
	, where(where_)
	, when(when_)
	, taken(false)
	, stage(0)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task const& what_, uint64_t where_, int64_t when_)
	: what(what_)
	, where(where_)
	, when(when_)
	, taken(false)
	, stage(0)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task&& what_, uint64_t where_, int64_t when_, int64_t stage_)
	: what(std::move(what_))
	, where(where_)
	, when(when_)
	, taken(false)
	, stage(stage_)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(sprawl::threading::ThreadManager::Task const& what_, uint64_t where_, int64_t when_, int64_t stage_)
	: what(what_)
	, where(where_)
	, when(when_)
	, taken(false)
	, stage(stage_)
{
	//
}

inline sprawl::threading::ThreadManager::TaskInfo::TaskInfo(TaskInfo&& other)
	: what(std::move(other.what))
	, where(other.where)
	, when(other.when)
	, taken(false)
	, stage(0)
{

}

sprawl::threading::ThreadManager::TaskInfo& sprawl::threading::ThreadManager::TaskInfo::operator=(TaskInfo&& other)
{
	what = std::move(other.what);
	where = other.where;
	when = other.when;
	stage = other.stage;
	return *this;
}


sprawl::threading::ThreadManager::ThreadManager()
	: m_taskQueue()
	, m_flagGroups()
	, m_mainThreadMailbox(nullptr)
	, m_mainThreadQueue(nullptr)
	, m_threads()
	, m_mailmanThread(std::bind(&ThreadManager::mailMan_, this))
	, m_running(false)
	, m_currentStage(0)
	, m_maxStage(0)
	, m_syncState(SyncState::None)
	, m_workerSyncEvent()
	, m_mailmanSyncEvent()
	, m_syncCount(0)
	, m_mailReady()
{
	//
}

sprawl::threading::ThreadManager::~ThreadManager()
{
	TaskInfo* task;
	while(m_taskQueue.Dequeue(task))
	{
		delete task;
	}
	for(auto& group : m_flagGroups)
	{
		delete group.Value();
	}
	for(auto& threadInfo : m_threads)
	{
		delete threadInfo.data;
		delete threadInfo.thread;
	}
}

void sprawl::threading::ThreadManager::AddThread(uint64_t threadFlags, const char* const threadName)
{
	ThreadData* data = new ThreadData(threadFlags);
	m_threads.PushBack(ThreadInfo(data, threadName, std::bind(&ThreadManager::eventLoop_, this, data)));
	FlagGroup*& group = m_flagGroups[threadFlags];
	if(group == nullptr)
	{
		group = new FlagGroup();
	}
	group->events.PushBack(&data->mailbox);
}


void sprawl::threading::ThreadManager::AddThread(uint64_t threadFlags)
{
	ThreadData* data = new ThreadData(threadFlags);
	m_threads.PushBack(ThreadInfo(data, std::bind(&ThreadManager::eventLoop_, this, data)));
	FlagGroup*& group = m_flagGroups[threadFlags];
	if(group == nullptr)
	{
		group = new FlagGroup();
	}
	group->events.PushBack(&data->mailbox);
}


void sprawl::threading::ThreadManager::AddThreads(uint64_t threadFlags, int count, const char* const threadName)
{
	for(int i = 0; i < count; ++i)
	{
		AddThread(threadFlags, threadName);
	}
}


void sprawl::threading::ThreadManager::AddThreads(uint64_t threadFlags, int count)
{
	for(int i = 0; i < count; ++i)
	{
		AddThread(threadFlags);
	}
}

void sprawl::threading::ThreadManager::AddTask(sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(new TaskInfo(std::move(task), threadFlags, whenNanosecs));
}

void sprawl::threading::ThreadManager::AddTask(sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(new TaskInfo(task, threadFlags, whenNanosecs));
}

void sprawl::threading::ThreadManager::AddTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(new TaskInfo(std::move(task), threadFlags, whenNanosecs, stage));
}

void sprawl::threading::ThreadManager::AddTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t whenNanosecs)
{
	pushTask_(new TaskInfo(task, threadFlags, whenNanosecs, stage));
}

void sprawl::threading::ThreadManager::SetMaxStage(uint64_t maxStage)
{
	m_maxStage = maxStage;
}

void sprawl::threading::ThreadManager::AddFutureTask(sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(std::move(task), threadFlags, nanosecondsFromNow + time::Now());
}

void sprawl::threading::ThreadManager::AddFutureTask(sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTask(task, threadFlags, nanosecondsFromNow + time::Now());
}

void sprawl::threading::ThreadManager::AddFutureTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTaskStaged(stage, std::move(task), threadFlags, nanosecondsFromNow + time::Now());
}

void sprawl::threading::ThreadManager::AddFutureTaskStaged(uint64_t stage, sprawl::threading::ThreadManager::Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow)
{
	AddTaskStaged(stage, task, threadFlags, nanosecondsFromNow + time::Now());
}

void sprawl::threading::ThreadManager::RunStaged(uint64_t thisThreadFlags)
{
	m_running = true;
	m_currentStage = 1;
	m_syncState = SyncState::Threads;
	for(auto& threadData : m_threads)
	{
		threadData.thread->Start();
	}
	AddThread(thisThreadFlags, "Main Thread");
	m_mainThreadMailbox = &m_threads.Back().data->mailbox;
	m_mainThreadQueue = &m_flagGroups.Get(m_threads.Back().data->flags)->taskQueue;

	size_t threadCount = m_threads.Size() - 1;

	while(m_syncCount != threadCount)
	{
		m_workerSyncEvent.Wait();
	}

	m_mailmanThread.Start();
	while(m_running)
	{
		RunNextStage();
	}
}

void sprawl::threading::ThreadManager::Run(uint64_t thisThreadFlags)
{
	m_running = true;
	for(auto& threadData : m_threads)
	{
		threadData.thread->Start();
	}
	AddThread(thisThreadFlags, "Main Thread");
	m_mainThreadMailbox = &m_threads.Back().data->mailbox;
	m_mainThreadQueue = &m_flagGroups.Get(m_threads.Back().data->flags)->taskQueue;

	m_mailmanThread.Start();
	eventLoop_(m_threads.Back().data);
}

void sprawl::threading::ThreadManager::Start(uint64_t thisThreadFlags)
{
	m_running = true;

	if(m_maxStage != 0)
	{
		m_syncState = SyncState::Threads;
		m_currentStage = 1;
	}
	for(auto& threadInfo : m_threads)
	{
		threadInfo.thread->Start();
	}
	AddThread(thisThreadFlags, "Main Thread");
	m_mainThreadMailbox = &m_threads.Back().data->mailbox;
	m_mainThreadQueue = &m_flagGroups.Get(m_threads.Back().data->flags)->taskQueue;

	if(m_maxStage != 0)
	{
		size_t threadCount = m_threads.Size() - 1;

		while(m_syncCount != threadCount)
		{
			m_workerSyncEvent.Wait();
		}
	}

	m_mailmanThread.Start();
}

void sprawl::threading::ThreadManager::Pump()
{
	TaskInfo* task;
	while(m_mainThreadQueue->Dequeue(task))
	{
		bool expected = false;
		if(task->taken.compare_exchange_strong(expected, true))
		{
			task->what();
			delete task;
		}
	}
}

void sprawl::threading::ThreadManager::pump_()
{
	TaskInfo* task;
	while(m_mainThreadQueue->Dequeue(task))
	{
		bool expected = false;
		if(task->taken.compare_exchange_strong(expected, true))
		{
			task->what();
			delete task;
		}
	}
}

uint64_t sprawl::threading::ThreadManager::RunNextStage()
{
	//Don't wait on the main thread...
	size_t threadCount = m_threads.Size() - 1;
	m_syncCount = 0;
	m_syncState = SyncState::None;

	for(auto& threadInfo : m_threads)
	{
		threadInfo.data->mailbox.Notify();
	}

	while(m_syncCount != threadCount)
	{
		m_workerSyncEvent.Wait();
	}

	//Ensure any existing events on the mailmanSyncEvent are consumed
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	//Let the mailman deliver mail... new mail for this stage will come in the next frame.
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	//Stop mail delivery and run threads with only the mail that's already been delivered.
	m_syncState = SyncState::Mailman;
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	pump_();

	m_syncCount = 0;
	m_syncState = SyncState::Threads;

	for(auto& threadInfo : m_threads)
	{
		threadInfo.data->mailbox.Notify();
	}

	while(m_syncCount != threadCount)
	{
		m_workerSyncEvent.Wait();
	}

	uint64_t stageJustRun = m_currentStage;
	m_currentStage *= 2;
	if(m_currentStage > m_maxStage)
	{
		m_currentStage = 1;
	}
	return stageJustRun;
}

//Sync is basically the opposite of RunNextStage... instead of doing start stage, run, finish stage, it does finish stage, start stage, and expects the user to call Pump()
uint64_t sprawl::threading::ThreadManager::Sync()
{
	//Don't wait on the main thread...
	size_t threadCount = m_threads.Size() - 1;

	m_syncCount = 0;
	m_syncState = SyncState::Threads;

	for(auto& threadInfo : m_threads)
	{
		threadInfo.data->mailbox.Notify();
	}

	while(m_syncCount != threadCount)
	{
		m_workerSyncEvent.Wait();
	}

	uint64_t stageJustRun = m_currentStage;
	m_currentStage *= 2;
	if(m_currentStage > m_maxStage)
	{
		m_currentStage = 1;
	}

	m_syncCount = 0;
	m_syncState = SyncState::None;

	for(auto& threadInfo : m_threads)
	{
		threadInfo.data->mailbox.Notify();
	}

	while(m_syncCount != threadCount)
	{
		m_workerSyncEvent.Wait();
	}

	//Ensure any existing events on the mailmanSyncEvent are consumed
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	//Let the mailman deliver mail... new mail for this stage will come in the next frame.
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	//Stop mail delivery and run threads with only the mail that's already been delivered.
	m_syncState = SyncState::Mailman;
	m_mailReady.Notify();
	m_mailmanSyncEvent.Wait();

	return stageJustRun;
}

void sprawl::threading::ThreadManager::Wait()
{
	m_mainThreadMailbox->Wait();
}

void sprawl::threading::ThreadManager::Stop()
{
	m_syncState = SyncState::None;
	m_running = false;
	m_mailReady.Notify();
	for(auto& threadInfo : m_threads)
	{
		threadInfo.data->mailbox.Notify();
	}
}

void sprawl::threading::ThreadManager::ShutDown()
{
	if(m_running)
	{
		Stop();
	}
	for(auto& threadInfo : m_threads)
	{
		if(threadInfo.thread->Joinable() && threadInfo.thread->GetHandle() != sprawl::this_thread::GetHandle())
		{
			threadInfo.thread->Join();
		}
	}
	m_mailmanThread.Join();
	m_threads.Clear();
}

void sprawl::threading::ThreadManager::pushTask_(TaskInfo* task)
{
	m_taskQueue.Enqueue(task);
	m_mailReady.Notify();
}

void sprawl::threading::ThreadManager::eventLoop_(ThreadData* threadData)
{
	collections::ConcurrentQueue<TaskInfo*>& queue = m_flagGroups.Get(threadData->flags)->taskQueue;
	Event& mailbox = threadData->mailbox;

	while(m_running)
	{
		TaskInfo* task;
		while(queue.Dequeue(task))
		{
			bool expected = false;
			if(task->taken.compare_exchange_strong(expected, true))
			{
				task->what();
				delete task;
			}
		}
		if(m_syncState == SyncState::Threads)
		{
			++m_syncCount;
			m_workerSyncEvent.Notify();
			while(m_syncState == SyncState::Threads)
			{
				mailbox.Wait();
			}
			++m_syncCount;
			m_workerSyncEvent.Notify();
		}
		else
		{
			mailbox.Wait();
		}
	}
}

#include <map>

void sprawl::threading::ThreadManager::mailMan_()
{
	std::map<int64_t, TaskInfo*> prioritizedTasks;
	while(m_running)
	{
		TaskInfo* task;
		while(m_taskQueue.Dequeue(task))
		{
			while(prioritizedTasks.find(task->when) != prioritizedTasks.end())
			{
				++task->when;
			}
			prioritizedTasks.emplace(task->when, task);
		}
		collections::HashSet<int64_t> keysToDelete;
		int64_t lastTime = 0;
		for(auto& flagGroup : m_flagGroups)
		{
			bool addedTask = false;
			auto it = prioritizedTasks.begin();
			for(;it != prioritizedTasks.end() && it->first <= time::Now();++it)
			{
				if(it->second->where != 0 && (it->second->where & flagGroup.Key()) == 0)
				{
					continue;
				}
				if(m_currentStage != 0 && it->second->stage != 0 && (it->second->stage & m_currentStage) == 0)
				{
					continue;
				}
				flagGroup.Value()->taskQueue.Enqueue(it->second);
				addedTask = true;
				keysToDelete.Insert(it->first);
			}
			if(addedTask)
			{
				for(auto& event : flagGroup.Value()->events)
				{
					event->Notify();
				}
			}
			if(it != prioritizedTasks.end())
			{
				lastTime = it->first;
			}
		}

		for(auto& key : keysToDelete)
		{
			prioritizedTasks.erase(key.Key());
		}

		auto syncStatePreNotify = m_syncState.load();
		m_mailmanSyncEvent.Notify();
		if (syncStatePreNotify != SyncState::None)
		{
			while(m_syncState != SyncState::None)
			{
				m_mailReady.Wait();
			}
		}
		else
		{
			if(lastTime != 0)
			{
				m_mailReady.WaitUntil(lastTime);
			}
			else
			{
				m_mailReady.Wait();
			}
		}
	}

	for(auto& task : prioritizedTasks)
	{
		delete task.second;
	}
}
