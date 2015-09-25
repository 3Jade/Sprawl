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
#include "../collections/ForwardList.hpp"
#include "../collections/List.hpp"
#include "../collections/HashMap.hpp"

#include "thread.hpp"
#include "mutex.hpp"
#include "condition_variable.hpp"

class sprawl::threading::ThreadManager
{
public:
	typedef std::function<void()> Task;

	ThreadManager();
	~ThreadManager();

	void AddThread(uint64_t threadFlags, char const* const threadName, uint64_t secondaryFlags = 0);
	void AddThread(uint64_t threadFlags, uint64_t secondaryFlags = 0);

	void AddThreads(uint64_t threadFlags, int count, char const* const threadName, uint64_t secondaryFlags = 0);
	void AddThreads(uint64_t threadFlags, int count, uint64_t secondaryFlags = 0);

	void AddTask(Task&& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));
	void AddTask(Task const& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));

	void AddTaskStaged(uint64_t stage, Task&& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));
	void AddTaskStaged(uint64_t stage, Task const& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));

	void SetNumStages(uint64_t stageCount);

	void AddFutureTask(Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow);
	void AddFutureTask(Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow);

	void AddFutureTaskStaged(uint64_t stage, Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow);
	void AddFutureTaskStaged(uint64_t stage, Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow);

	/**
	 * @brief	Prevents the thread manager from executing a secondary task if there is a primary task queued within this many nanoseconds
	 * @param	windowInNanoseconds	Number of free nanoseconds before the next primary task required before a thread can execute a secondary task
	 */
	void SetSecondaryTaskWindow(int64_t windowInNanoseconds);

	/**
	 * @brief	Start all threads and include the calling thread in a loop controlled by the thread manager
	 * @param	thisThreadFlags	The flags that apply to the calling thread
	 */
	void Run(uint64_t thisThreadFlags, uint64_t secondaryFlags = 0);
	void RunStaged(uint64_t thisThreadFlags, uint64_t secondaryFlags = 0);

	/**
	 * @brief	Start all threads but do not block on the calling thread.
	 * @details	If thisThreadFlags is not 0, the calling thread will be added to the thread pool.
	 *			It will then be up to the calling thread to call Pump() to execute the tasks
	 *			that get queued up for it.
	 * @param	thisThreadFlags	The flags that apply to the calling thread
	 */
	void Start(uint64_t thisThreadFlags, uint64_t secondaryFlags = 0);

	uint64_t RunNextStage();
	void Pump();
	void Wait();
	void Sync();
	void Stop();
	void ShutDown();
private:
	struct TaskInfo
	{
		TaskInfo(Task&& what_, uint64_t where_, int64_t when_);
		TaskInfo(Task const& what_, uint64_t where_, int64_t when_);
		Task what;
		uint64_t where;
		int64_t when;
	};

	void pushTask_(TaskInfo&& info, uint64_t stage);
	void eventLoop_(uint64_t flags, uint64_t secondaryFlags);

	collections::BasicHashMap<uint64_t, collections::List<TaskInfo>> m_taskQueue;
	uint64_t m_currentStage;
	uint64_t m_maxStage;
	collections::ForwardList<Thread*> m_threads;
	Mutex m_mutex;
	ConditionVariable m_conditionVariable;
	uint64_t m_callingThreadFlags;
	uint64_t m_callingThreadSecondaryFlags;
	int64_t m_secondaryTaskWindow;
	bool m_running;

	bool m_syncing;
	ConditionVariable m_syncWait;
	ConditionVariable m_threadSyncWait;
	size_t m_numThreadsSynced;
};
