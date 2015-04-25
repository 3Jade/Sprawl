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

#include "thread.hpp"
#include "mutex.hpp"
#include "condition_variable.hpp"

class sprawl::threading::ThreadManager
{
public:
	typedef std::function<void()> Task;

	void AddThread(uint64_t threadFlags, char const* const threadName, uint64_t secondaryFlags = 0);
	void AddThread(uint64_t threadFlags, uint64_t secondaryFlags = 0);

	void AddThreads(uint64_t threadFlags, int count, char const* const threadName, uint64_t secondaryFlags = 0);
	void AddThreads(uint64_t threadFlags, int count, uint64_t secondaryFlags = 0);

	void AddTask(Task&& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));
	void AddTask(Task const& task, uint64_t threadFlags, int64_t whenNanosecs = time::Now(time::Resolution::Nanoseconds));

	void AddFutureTask(Task&& task, uint64_t threadFlags, int64_t nanosecondsFromNow);
	void AddFutureTask(Task const& task, uint64_t threadFlags, int64_t nanosecondsFromNow);

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

	/**
	 * @brief	Start all threads but do not block on the calling thread.
	 * @details	If thisThreadFlags is not 0, the calling thread will be added to the thread pool.
	 *			It will then be up to the calling thread to call Pump() to execute the tasks
	 *			that get queued up for it.
	 * @param	thisThreadFlags	The flags that apply to the calling thread
	 */
	void Start(uint64_t thisThreadFlags, uint64_t secondaryFlags = 0);
	void Pump();
	void Wait();
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

	void pushTask_(TaskInfo&& info);
	void eventLoop_(uint64_t flags, uint64_t secondaryFlags);

	collections::List<TaskInfo> m_taskQueue;
	collections::ForwardList<Thread*> m_threads;
	Mutex m_mutex;
	ConditionVariable m_conditionVariable;
	uint64_t m_callingThreadFlags;
	uint64_t m_callingThreadSecondaryFlags;
	int64_t m_secondaryTaskWindow;
	bool m_running;
};

/*class sprawl::threading::StagedThreadManager
{
public:
	void AddThread(uint64_t threadFlags, char const* const threadName);
	void AddThread(uint64_t threadFlags, bool staged = false);

	void AddThreads(uint64_t threadFlags, int count, char const* const threadName);
	void AddThreads(uint64_t threadFlags, int count);

	void AddTask(Task&& task, uint64_t threadFlags, uint64_t stageFlags);
	void AddTask(Task const& task, uint64_t threadFlags, uint64_t stageFlags);

	void AddStage(uint64_t stageId, uint64_t stageFlags);
	void InsertStage(uint64_t afterStageId, uint64_t stageId, uint64_t stageFlags);

	void Run();

	void Start(uint64_t thisThreadFlags);
	void Pump();
	void Wait();
	void FinishStage();
	void AdvanceStage();
};*/
