#pragma once

#include "../common/noncopyable.hpp"
#include "../common/errors.hpp"
#include <stdint.h>
#include <functional>
#include <stdlib.h>

namespace sprawl
{
	namespace threading
	{
		class Thread;
		class Handle;
		void RunThread(Thread* thread);

		enum class ThreadDestructionBehavior
		{
			Default,
			Join,
			Detach,
			Abort,
		};
	}

	namespace this_thread
	{
		threading::Handle GetHandle();
		void Sleep(uint64_t nanoseconds);
		void SleepUntil(uint64_t nanosecondTimestamp);
		void Yield();
	}
}

#ifdef _WIN32
#	include "thread_windows.hpp"
#	undef Yield
#else
#	include "thread_linux.hpp"
#endif

class sprawl::threading::Thread : public sprawl::noncopyable
{
public:
	static void SetDefaultDestructionBehavior(ThreadDestructionBehavior behavior);
	void SetDestructionBehavior(ThreadDestructionBehavior behavior);

	template<typename Function, typename... Args>
	Thread(Function&& f, Args&&... args);

	template<typename Function, typename... Args>
	Thread(char const* const threadName, Function&& f, Args&&... args);

	template<typename Function>
	Thread(Function&& f);

	template<typename Function>
	Thread(char const* const threadName, Function&& f);

	~Thread();

	bool Joinable() { return m_handle != Handle(); }
	Handle const& GetHandle() { return m_handle; }

	void Join();

	void Detach();

	void Start();

	void SetExceptionHandler();

#if SPRAWL_EXCEPTIONS_ENABLED
	bool HasException() { return bool(m_exception); }
#else
	bool HasException() { return false; }
#endif

protected:
	void PlatformJoin();
	void PlatformDetach();

	friend void RunThread(Thread* thread);

	char const* const m_threadName;
	std::function<void()> m_function;
	Handle m_handle;

	ThreadDestructionBehavior m_destructionBehavior;
#if SPRAWL_EXCEPTIONS_ENABLED
	bool m_handleExceptions;
	std::exception_ptr m_exception;
#endif

	static ThreadDestructionBehavior ms_defaultDestructionBehavior;
private:
	Thread(Thread&& other);
	Thread(Thread const& other);
	Thread& operator=(Thread&& other);
	Thread& operator=(Thread const& other);
};

template<typename Function, typename... Args>
sprawl::threading::Thread::Thread(Function&& f, Args&&... args)
	: m_threadName(nullptr)
	, m_function(std::bind(f, args...))
	, m_handle()
	, m_destructionBehavior(ThreadDestructionBehavior::Default)
#if SPRAWL_EXCEPTIONS_ENABLED
	, m_handleExceptions(false)
	, m_exception()
#endif
{
	//
}

template<typename Function, typename... Args>
sprawl::threading::Thread::Thread(char const* const threadName, Function&& f, Args&&... args)
	: m_threadName(threadName)
	, m_function(std::bind(f, args...))
	, m_handle()
	, m_destructionBehavior(ThreadDestructionBehavior::Default)
#if SPRAWL_EXCEPTIONS_ENABLED
	, m_handleExceptions(false)
	, m_exception()
#endif
{
	//
}

template<typename Function>
sprawl::threading::Thread::Thread(Function&& f)
	: m_threadName(nullptr)
	, m_function(f)
	, m_handle()
	, m_destructionBehavior(ThreadDestructionBehavior::Default)
#if SPRAWL_EXCEPTIONS_ENABLED
	, m_handleExceptions(false)
	, m_exception()
#endif
{

}

template<typename Function>
sprawl::threading::Thread::Thread(char const* const threadName, Function&& f)
	: m_threadName(threadName)
	, m_function(f)
	, m_handle()
	, m_destructionBehavior(ThreadDestructionBehavior::Default)
#if SPRAWL_EXCEPTIONS_ENABLED
	, m_handleExceptions(false)
	, m_exception()
#endif
{

}

inline void sprawl::threading::Thread::Join()
{
	if(!Joinable())
	{
		std::terminate();
	}
	PlatformJoin();
	m_handle = Handle();
#if SPRAWL_EXCEPTIONS_ENABLED
	if(m_exception)
	{
		std::rethrow_exception(m_exception);
	}
#endif
}

inline void sprawl::threading::Thread::Detach()
{
	if(!Joinable())
	{
		std::terminate();
	}
#if SPRAWL_EXCEPTIONS_ENABLED
	m_handleExceptions = false;
#endif
	PlatformDetach();
	m_handle = Handle();
#if SPRAWL_EXCEPTIONS_ENABLED
	if(m_exception)
	{
		std::rethrow_exception(m_exception);
	}
#endif
}

inline void sprawl::threading::RunThread(Thread* thread)
{
#if SPRAWL_EXCEPTIONS_ENABLED
	try
	{
		thread->m_function();
	}
	catch(...)
	{
		if(thread->m_handleExceptions)
		{
			thread->m_exception = std::current_exception();
		}
		else
		{
			throw;
		}
	}
#else
	thread->m_function();
#endif
}

inline /*static*/ void sprawl::threading::Thread::SetDefaultDestructionBehavior(ThreadDestructionBehavior behavior)
{
	if(behavior == ThreadDestructionBehavior::Default)
	{
		behavior = ThreadDestructionBehavior::Abort;
	}
	ms_defaultDestructionBehavior = behavior;
}

inline void sprawl::threading::Thread::SetDestructionBehavior(ThreadDestructionBehavior behavior)
{
	m_destructionBehavior = behavior;
}

inline void sprawl::threading::Thread::SetExceptionHandler()
{
#if SPRAWL_EXCEPTIONS_ENABLED
	m_handleExceptions = true;
#endif
}
