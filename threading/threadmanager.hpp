namespace sprawl
{
	namespace threading
	{
		class ThreadManager;
	}
}

#include <stdint.h>
#include <functional>

#include "../time/time.hpp"
#include "../collections/Vector.hpp"
#include "../collections/ConcurrentQueue.hpp"
#include "../collections/BinaryTree.hpp"
#include "../collections/HashMap.hpp"

#include "thread.hpp"
#include "mutex.hpp"
#include "condition_variable.hpp"
#include "event.hpp"

#include <atomic>

class sprawl::threading::ThreadManager
{

public:
	typedef std::function<void()> Task;
private:
	struct TaskInfo
	{
		TaskInfo();
		TaskInfo(Task&& what_, uint64_t where_, int64_t when_);
		TaskInfo(Task const& what_, uint64_t where_, int64_t when_);
		TaskInfo(Task&& what_, uint64_t where_, int64_t when_, int64_t stage);
		TaskInfo(Task const& what_, uint64_t where_, int64_t when_, int64_t stage);
		TaskInfo(TaskInfo&& other);
		TaskInfo& operator=(TaskInfo&& other);

		Task what;
		uint64_t where;
		int64_t when;
		std::atomic<bool> taken;
		uint64_t stage;
		inline int64_t When()
		{
			return when;
		}
	};

	struct ThreadData
	{
		explicit ThreadData(uint64_t flags_)
			: flags(flags_)
			, mailbox()
		{

		}

		uint64_t flags;
		Event mailbox;
	};

	struct ThreadInfo
	{
		ThreadInfo(ThreadData* data_, std::function<void()> fn)
			: data(data_)
			, thread(new Thread(fn))
		{

		}

		ThreadInfo(ThreadData* data_, char const* const threadName, std::function<void()> fn)
			: data(data_)
			, thread(new Thread(threadName, fn))
		{

		}

		ThreadInfo(ThreadData* data_)
			: data(data_)
			, thread(nullptr)
		{

		}

		ThreadInfo(ThreadInfo&& other)
			: data(other.data)
			, thread(other.thread)
		{
			other.data = nullptr;
			other.thread = nullptr;
		}

		~ThreadInfo()
		{
			if (data)
			{
				delete data;
			}
			if (thread)
			{
				delete thread;
			}
		}

		ThreadData* data;
		Thread* thread;
	};

	struct FlagGroup
	{
		collections::Vector<Event*> events;
		collections::ConcurrentQueue<TaskInfo*> taskQueue;
	};
public:
	typedef collections::ConcurrentQueue<TaskInfo*>::ReadReservationTicket ReservationTicket;

	ThreadManager();
	~ThreadManager();

	void AddThread(uint64_t threadFlags, char const* const threadName);
	void AddThread(uint64_t threadFlags);

	void AddThreads(uint64_t threadFlags, int count, char const* const threadName);
	void AddThreads(uint64_t threadFlags, int count);

	void AddTask(Task&& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));
	void AddTask(Task const& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));

	void AddTaskStaged(uint64_t stage, Task&& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));
	void AddTaskStaged(uint64_t stage, Task const& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));

	void SetMaxStage(uint64_t maxStage);

	void AddFutureTask(Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow);
	void AddFutureTask(Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow);

	void AddFutureTaskStaged(uint64_t stage, Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow);
	void AddFutureTaskStaged(uint64_t stage, Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow);

	/**
	 * @brief	Start all threads and include the calling thread in a loop controlled by the thread manager
	 * @param	thisThreadFlags	The flags that apply to the calling thread
	 */
	void Run(uint64_t thisThreadFlags);
	void RunStaged(uint64_t thisThreadFlags);

	/**
	 * @brief	Start all threads but do not block on the calling thread.
	 * @details	If thisThreadFlags is not 0, the calling thread will be added to the thread pool.
	 *			It will then be up to the calling thread to call Pump() to execute the tasks
	 *			that get queued up for it.
	 * @param	thisThreadFlags	The flags that apply to the calling thread
	 */
	void Start(uint64_t thisThreadFlags);

	uint64_t RunNextStage(ReservationTicket& ticket);
	void Pump(ReservationTicket& ticket);
	void Wait();
	uint64_t Sync();
	void Stop();
	void ShutDown();
private:
	void pump_(ReservationTicket& ticket);
	void pushTask_(TaskInfo* info);
	void eventLoop_(ThreadData* threadData);
	void mailMan_();

	collections::ConcurrentQueue<TaskInfo*> m_taskQueue;
	collections::BasicHashMap<int64_t, FlagGroup*> m_flagGroups;
	Event* m_mainThreadMailbox;
	collections::ConcurrentQueue<TaskInfo*>* m_mainThreadQueue;

	collections::Vector<ThreadInfo> m_threads;
	Thread m_mailmanThread;
	std::atomic<bool> m_running;
	uint64_t m_currentStage;
	uint64_t m_maxStage;

	enum class SyncState
	{
		None,
		Mailman,
		Threads,
	};

	std::atomic<SyncState> m_syncState;
	Event m_workerSyncEvent;
	Event m_mailmanSyncEvent;
	std::atomic<size_t> m_syncCount;

	Event m_mailReady;
};
