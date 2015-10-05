#include "event.hpp"

sprawl::threading::Event::Event()
	: m_event(CreateEvent(nullptr, false, false, nullptr))
{

}

sprawl::threading::Event::~Event()
{
	CloseHandle(m_event);
}

void sprawl::threading::Event::Notify()
{
	SetEvent(m_event);
}

void sprawl::threading::Event::Wait()
{
	WaitForSingleObject(m_event, INFINITE);
}

bool sprawl::threading::Event::WaitFor(int64_t nanoseconds)
{
	if(nanoseconds < 0)
	{
		return false;
	}
	DWORD ret = WaitForSingleObject(m_event, time::Convert(nanoseconds, time::Resolution::Nanoseconds, time::Resolution::Milliseconds));
	return ret == WAIT_OBJECT_0;
}

/*static*/ sprawl::threading::Event* sprawl::threading::Event::WaitMultiple(EventGroup& values)
{
	HANDLE events[256];
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

/*static*/ sprawl::threading::Event* sprawl::threading::Event::WaitMultipleFor(EventGroup& values, int64_t nanoseconds)
{
	HANDLE events[256];
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
