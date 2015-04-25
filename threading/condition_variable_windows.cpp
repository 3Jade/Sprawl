#include "condition_variable.hpp"
#include "../time/time.hpp"

sprawl::threading::ConditionVariable::ConditionVariable()
	: m_conditionVariable()
{
	InitializeConditionVariable(&m_conditionVariable);
}

sprawl::threading::ConditionVariable::~ConditionVariable()
{
	//
}

void sprawl::threading::ConditionVariable::Wait(SharedLock& lock)
{
	SleepConditionVariableCS(&m_conditionVariable, &lock.GetMutex()->GetNativeMutex(), INFINITE);
}

void sprawl::threading::ConditionVariable::WaitFor(SharedLock& lock, int64_t nanoseconds)
{
	SleepConditionVariableCS(&m_conditionVariable, &lock.GetMutex()->GetNativeMutex(), time::Convert(nanoseconds, time::Resolution::Nanoseconds, time::Resolution::Milliseconds));
}

void sprawl::threading::ConditionVariable::WaitUntil(SharedLock& lock, int64_t nanosecondTimestamp)
{
	WaitFor(lock, nanosecondTimestamp - time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ConditionVariable::Notify()
{
	WakeConditionVariable(&m_conditionVariable);
}

void sprawl::threading::ConditionVariable::NotifyAll()
{
	WakeAllConditionVariable(&m_conditionVariable);
}
