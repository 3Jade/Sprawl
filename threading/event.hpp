#pragma once

#include <stdint.h>
#include "../time/time.hpp"
#include "../collections/Vector.hpp"
#include <atomic>

#ifndef SPRAWL_EVENT_MAX_MULTIPLE_WAIT
#	define SPRAWL_EVENT_MAX_MULTIPLE_WAIT 256
#endif

#if defined(_WIN32)
#	include <Windows.h>
	typedef HANDLE EventType;
#elif defined(__APPLE__)
	struct EventType
	{
		intptr_t ident;
		int queue;
	};
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
	typedef collections::Vector<Event const*> EventGroup;

	Event();
	~Event();

	void Notify() const;
	void Wait() const;
	bool WaitFor(int64_t nanoseconds) const;
	bool WaitUntil(int64_t nanosecondTimestamp) const
	{
		return WaitFor(nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
	}

	static Event const* WaitAny(EventGroup const& values);
	static Event const* WaitAnyFor(EventGroup const& values, int64_t nanoseconds);
	static Event const* WaitAnyUntil(EventGroup const& values, int64_t nanosecondTimestamp)
	{
		return WaitAnyFor(values, nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
	}

	static void WaitAll(EventGroup const& values);
	static bool WaitAllFor(EventGroup const& values, int64_t nanoseconds);
	static bool WaitAllUntil(EventGroup const& values, int64_t nanosecondTimestamp)
	{
		return WaitAllFor(values, nanosecondTimestamp - sprawl::time::Now(sprawl::time::Resolution::Nanoseconds));
	}

	static void NotifyAll(EventGroup const& values)
	{
		for(auto& value : values)
		{
			value->Notify();
		}
	}

	static void NotifyAll(Event& event)
	{
		event.Notify();
	}

	template<typename... Params>
	static void NotifyAll(Event& event, Params&... params)
	{
		event.Notify();
		NotifyAll(params...);
	}

	// Template wrappers

	static Event const* WaitAny(EventGroup& values, Event const& ev)
	{
		values.PushBack(&ev);
		return WaitAny(values);
	}

	template<typename... Params>
	static Event const* WaitAny(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		return WaitAny(values, params...);
	}

	template<typename... Params>
	static Event const* WaitAny(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		return WaitAny(values, params...);
	}

	static Event const* WaitAnyFor(EventGroup& values, Event const& ev, int64_t nanoseconds)
	{
		values.PushBack(&ev);
		return WaitAnyFor(values, nanoseconds);
	}

	template<typename... Params>
	static Event const* WaitAnyFor(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		return WaitAnyFor(values, params...);
	}

	template<typename... Params>
	static Event const* WaitAnyFor(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		return WaitAnyFor(values, params...);
	}

	static Event const* WaitAnyUntil(EventGroup& values, Event const& ev, int64_t nanosecondTimestamp)
	{
		values.PushBack(&ev);
		return WaitAnyUntil(values, nanosecondTimestamp);
	}

	template<typename... Params>
	static Event const* WaitAnyUntil(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		return WaitAnyUntil(values, params...);
	}

	template<typename... Params>
	static Event const* WaitAnyUntil(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		return WaitAnyUntil(values, params...);
	}




	static void WaitAll(EventGroup& values, Event const& ev)
	{
		values.PushBack(&ev);
		WaitAll(values);
	}

	template<typename... Params>
	static void WaitAll(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		WaitAll(values, params...);
	}

	template<typename... Params>
	static void WaitAll(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		WaitAll(values, params...);
	}

	static bool WaitAllFor(EventGroup& values, Event const& ev, int64_t nanoseconds)
	{
		values.PushBack(&ev);
		return WaitAllFor(values, nanoseconds);
	}

	template<typename... Params>
	static bool WaitAllFor(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		return WaitAllFor(values, params...);
	}

	template<typename... Params>
	static bool WaitAllFor(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		return WaitAllFor(values, params...);
	}

	static bool WaitAllUntil(EventGroup& values, Event const& ev, int64_t nanosecondTimestamp)
	{
		values.PushBack(&ev);
		return WaitAllUntil(values, nanosecondTimestamp);
	}

	template<typename... Params>
	static bool WaitAllUntil(EventGroup& values, Event const& ev, Params const&... params)
	{
		values.PushBack(&ev);
		return WaitAllUntil(values, params...);
	}

	template<typename... Params>
	static bool WaitAllUntil(Event const& ev, Params const&... params)
	{
		EventGroup values(sprawl::collections::Capacity(sizeof...(Params)));
		values.PushBack(&ev);
		return WaitAllUntil(values, params...);
	}

	EventType m_event;
};
