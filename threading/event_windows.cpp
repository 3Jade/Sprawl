#include "event.hpp"

sprawl::threading::Event::Event()
	: m_event(CreateEvent(nullptr, false, false, nullptr))
{

}

sprawl::threading::Event::~Event()
{
	CloseHandle(m_event);
}

void sprawl::threading::Event::Notify() const
{
	SetEvent(m_event);
}

void sprawl::threading::Event::Wait() const
{
	WaitForSingleObject(m_event, INFINITE);
}

bool sprawl::threading::Event::WaitFor(int64_t nanoseconds) const
{
	if(nanoseconds < 0)
	{
		return false;
	}
	DWORD ret = WaitForSingleObject(m_event, time::Convert(nanoseconds, time::Resolution::Nanoseconds, time::Resolution::Milliseconds));
	return ret == WAIT_OBJECT_0;
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAny(EventGroup const& values)
{
	HANDLE events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];
	for(int i = 0; i < values.Size(); ++i)
	{
		events[i] = values[i]->m_event;
	}

	DWORD ret = WaitForMultipleObjects(values.Size(), events, false, INFINITE);
	if(ret < WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + values.Size())
	{
		return nullptr;
	}
	return values[ret - WAIT_OBJECT_0];
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAnyFor(EventGroup const& values, int64_t nanoseconds)
{
	HANDLE events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];
	for(int i = 0; i < values.Size(); ++i)
	{
		events[i] = values[i]->m_event;
	}

	DWORD ret = WaitForMultipleObjects(values.Size(), events, false, time::Convert(nanoseconds, time::Resolution::Nanoseconds, time::Resolution::Milliseconds));
	if(ret < WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + values.Size())
	{
		return nullptr;
	}
	return values[ret - WAIT_OBJECT_0];
}

/*static*/ void sprawl::threading::Event::WaitAll(EventGroup const& values)
{
	HANDLE events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];
	for(int i = 0; i < values.Size(); ++i)
	{
		events[i] = values[i]->m_event;
	}

	DWORD ret = WaitForMultipleObjects(values.Size(), events, true, INFINITE);
}

/*static*/ bool sprawl::threading::Event::WaitAllFor(EventGroup const& values, int64_t nanoseconds)
{
	HANDLE events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];
	for(int i = 0; i < values.Size(); ++i)
	{
		events[i] = values[i]->m_event;
	}

	DWORD ret = WaitForMultipleObjects(values.Size(), events, true, time::Convert(nanoseconds, time::Resolution::Nanoseconds, time::Resolution::Milliseconds));
	if(ret < WAIT_OBJECT_0 || ret >= WAIT_OBJECT_0 + values.Size())
	{
		return false;
	}
	return true;
}
