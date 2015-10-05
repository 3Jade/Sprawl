#pragma once

#include <stdint.h>
#include "../time/time.hpp"
#include "../collections/Vector.hpp"
#include <atomic>

#if defined(_WIN32)
#	include <Windows.h>
	typedef HANDLE EventType;
#elif defined(__APPLE__)
	typedef intptr_t EventType;
#else
	typedef int EventType;
#endif

namespace sprawl
{
	namespace threading
	{
		class Event;
	}
}

class sprawl::threading::Event
{
public:
	typedef collections::Vector<Event*> EventGroup;

	Event();
	~Event();

	void Notify();
	void Wait();
	bool WaitFor(int64_t nanoseconds);
	bool WaitUntil(int64_t nanosecondTimestamp)
	{
		return WaitFor(nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
	}

	static Event* WaitMultiple(EventGroup& values);
	static Event* WaitMultipleFor(EventGroup& values, int64_t nanoseconds);
	static Event* WaitMultipleUntil(EventGroup& values, int64_t nanosecondTimestamp)
	{
		return WaitMultipleFor(values, nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
	}

	//TODO: Wait for multiple events static function

private:
	EventType m_event;
#if defined(__APPLE__)
	int m_kqueue;
#endif
};
