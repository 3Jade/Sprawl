#pragma once

#include "../common/noncopyable.hpp"
#include <stdint.h>
#include <functional>
#include <stdlib.h>

namespace sprawl
{
	namespace threading
	{
		class Thread;
		class Handle;
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
#else
#	include "thread_linux.hpp"
#endif

class sprawl::threading::Thread : public sprawl::noncopyable
{
public:
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

private:
	static void* EntryPoint(void* data);
	void PlatformJoin();
	void PlatformDetach();

	char const* const m_threadName;
	std::function<void()> m_function;
	Handle m_handle;
};

template<typename Function, typename... Args>
sprawl::threading::Thread::Thread(Function&& f, Args&&... args)
	: m_threadName(nullptr)
	, m_function(std::bind(f, args...))
	, m_handle()
{
	//
}

template<typename Function, typename... Args>
sprawl::threading::Thread::Thread(char const* const threadName, Function&& f, Args&&... args)
	: m_threadName(threadName)
	, m_function(std::bind(f, args...))
	, m_handle()
{
	//
}

template<typename Function>
sprawl::threading::Thread::Thread(Function&& f)
	: m_threadName(nullptr)
	, m_function(f)
	, m_handle()
{

}

template<typename Function>
sprawl::threading::Thread::Thread(char const* const threadName, Function&& f)
	: m_threadName(threadName)
	, m_function(f)
	, m_handle()
{

}

inline sprawl::threading::Thread::~Thread()
{
	if(Joinable())
	{
		abort();
	}
}

inline void sprawl::threading::Thread::Join()
{
	if(!Joinable())
	{
		abort();
	}
	PlatformJoin();
	m_handle = Handle();
}

inline void sprawl::threading::Thread::Detach()
{
	if(!Joinable())
	{
		abort();
	}
	PlatformDetach();
	m_handle = Handle();
}

inline /*static*/ void* sprawl::threading::Thread::EntryPoint(void *data)
{
	Thread* thread = reinterpret_cast<Thread*>(data);
	thread->m_function();
	return nullptr;
}
