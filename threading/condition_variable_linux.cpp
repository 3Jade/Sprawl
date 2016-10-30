#include "condition_variable.hpp"
#include "../time/time.hpp"

sprawl::threading::ConditionVariable::ConditionVariable()
	: m_conditionVariable()
{
	pthread_cond_init(&m_conditionVariable, nullptr);
}

sprawl::threading::ConditionVariable::~ConditionVariable()
{
	pthread_cond_destroy(&m_conditionVariable);
}

void sprawl::threading::ConditionVariable::Wait(SharedLock& lock)
{
	pthread_cond_wait(&m_conditionVariable, &lock.GetMutex()->GetNativeMutex());
}

void sprawl::threading::ConditionVariable::WaitFor(SharedLock& lock, int64_t nanoseconds)
{
	struct timespec ts;


	ts.tv_sec = time_t(sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds));
	int64_t secsAsNanosecs = sprawl::time::Convert(ts.tv_sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);
	ts.tv_nsec = long(nanoseconds - secsAsNanosecs);

	pthread_cond_timedwait(&m_conditionVariable, &lock.GetMutex()->GetNativeMutex(), &ts);
}

void sprawl::threading::ConditionVariable::WaitUntil(SharedLock& lock, int64_t nanosecondTimestamp)
{
	WaitFor(lock, nanosecondTimestamp - time::Now(time::Resolution::Nanoseconds));
}

void sprawl::threading::ConditionVariable::Notify()
{
	pthread_cond_signal(&m_conditionVariable);
}

void sprawl::threading::ConditionVariable::NotifyAll()
{
	pthread_cond_broadcast(&m_conditionVariable);
}