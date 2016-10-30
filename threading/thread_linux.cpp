#include "thread.hpp"
#include "../time/time.hpp"
#include <time.h>

#ifdef __APPLE__
#include <sched.h>
#	define pthread_yield sched_yield
#endif

#include "../common/compat.hpp"

namespace ThreadStatic
{
	static void* EntryPoint(void *data)
	{
		sprawl::threading::Thread* thread = reinterpret_cast<sprawl::threading::Thread*>(data);
		sprawl::threading::RunThread(thread);
		return nullptr;
	}
}

/*static*/ sprawl::threading::ThreadDestructionBehavior sprawl::threading::Thread::ms_defaultDestructionBehavior = sprawl::threading::ThreadDestructionBehavior::Abort;

int64_t sprawl::threading::Handle::GetUniqueId() const
{
	return int64_t(m_thread);
}

void sprawl::threading::Thread::Start()
{
	int result = pthread_create(&m_handle.GetNativeHandle(), nullptr, &ThreadStatic::EntryPoint, this);
#ifndef __APPLE__
	if(result == 0)
	{
		if(m_threadName != nullptr)
		{
			pthread_setname_np(m_handle.GetNativeHandle(), m_threadName);
		}
	}
#endif
}

void sprawl::threading::Thread::PlatformJoin()
{
	void* value;
	pthread_join(m_handle.GetNativeHandle(), &value);
}

void sprawl::threading::Thread::PlatformDetach()
{
	pthread_detach(m_handle.GetNativeHandle());
}

sprawl::threading::Handle sprawl::this_thread::GetHandle()
{
	return sprawl::threading::Handle(pthread_self());
}

sprawl::threading::Thread::~Thread()
{
	if(m_destructionBehavior == ThreadDestructionBehavior::Default)
	{
		m_destructionBehavior = ms_defaultDestructionBehavior;
	}

	if(Joinable())
	{
#if SPRAWL_EXCEPTIONS_ENABLED
		if(m_exception)
		{
			try
			{
				std::rethrow_exception(m_exception);
			}
			catch(std::exception& e)
			{
				fprintf(stderr, "Thread %" SPRAWL_I64FMT "d destroyed without being joined after thread was terminated with a std::exception. e.what(): %s\n", GetHandle().GetUniqueId(), e.what());
			}
			catch(...)
			{
				fprintf(stderr, "Thread %" SPRAWL_I64FMT "d destroyed without being joined after thread was terminated with an exception of unknown type.\n", GetHandle().GetUniqueId());
			}
			fflush(stderr);
			std::terminate();
		}
#endif
		switch(m_destructionBehavior)
		{
		case ThreadDestructionBehavior::Abort:
		case ThreadDestructionBehavior::Default:
		default:
			std::terminate();
			break;
		case ThreadDestructionBehavior::Detach:
			PlatformDetach();
			break;
		case ThreadDestructionBehavior::Join:
			PlatformJoin();
			break;
		}
	}
}

void sprawl::this_thread::Sleep(uint64_t nanoseconds)
{
	struct timespec ts;

	ts.tv_sec = time_t(sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds));
	int64_t secsAsNanosecs = sprawl::time::Convert(ts.tv_sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);
	ts.tv_nsec = long(nanoseconds - secsAsNanosecs);

	nanosleep(&ts, nullptr);
}

void sprawl::this_thread::SleepUntil(uint64_t nanosecondTimestamp)
{
	Sleep(nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
}

void sprawl::this_thread::Yield()
{
	pthread_yield();
}
