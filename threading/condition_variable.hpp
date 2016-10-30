#pragma once

#include "mutex.hpp"
#include <stdint.h>

#ifdef _WIN32
typedef	CONDITION_VARIABLE ConditionVariableType;
#else
typedef pthread_cond_t ConditionVariableType;
#endif

namespace sprawl
{
	namespace threading
	{
		class ConditionVariable;
	}
}

class sprawl::threading::ConditionVariable
{
public:
	ConditionVariable();
	~ConditionVariable();

	void Wait(SharedLock& lock);
	void WaitFor(SharedLock& lock, int64_t nanoseconds);
	void WaitUntil(SharedLock& lock, int64_t nanosecondTimestamp);

	void Notify();
	void NotifyAll();
private:
	ConditionVariableType m_conditionVariable;
};
