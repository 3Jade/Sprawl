#include "event.hpp"
#include <atomic>
#include <sys/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../time/time.hpp"

namespace EventStatic
{
	static std::atomic<intptr_t> highId_(0);
}

sprawl::threading::Event::Event()
	: m_event(EventStatic::highId_++)
	, m_kqueue(kqueue())
{
	struct kevent event;
	EV_SET(&event, m_event, EVFILT_USER, EV_ADD|EV_CLEAR, NOTE_FFNOP, 0, nullptr);
	kevent(m_kqueue, &event, 1, nullptr, 0, nullptr);
}

sprawl::threading::Event::~Event()
{
	close(m_kqueue);
}

void sprawl::threading::Event::Notify()
{
	fflush(stdout);
	struct kevent event;
	EV_SET(&event, m_event, EVFILT_USER, EV_ADD|EV_CLEAR, NOTE_TRIGGER, 0, nullptr);
	kevent(m_kqueue, &event, 1, nullptr, 0, nullptr);
}

void sprawl::threading::Event::Wait()
{
	fflush(stdout);
	struct kevent event;
	kevent(m_kqueue, nullptr, 0, &event, 1,  nullptr);
}

bool sprawl::threading::Event::WaitFor(int64_t nanoseconds)
{
	if(nanoseconds < 0)
	{
		return false;
	}

	struct timespec ts;
	int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
	int64_t nsec = nanoseconds - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);

	ts.tv_sec = time_t(sec);
	ts.tv_nsec = long(nsec);

	struct kevent event;
	return (kevent(m_kqueue, nullptr, 0, &event, 1,  &ts) != 0);
}

/*static*/ sprawl::threading::Event* sprawl::threading::Event::WaitMultiple(EventGroup& values)
{
	int multiKqueue = kqueue();

	struct kevent events[256];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event* event = values[i];
		EV_SET(&events[i], event->m_kqueue, EVFILT_READ, EV_ADD, 0, 0, event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);
	int nret = kevent(multiKqueue, nullptr, 0, events, 256, nullptr);

	for(int i = 0; i < nret; ++i)
	{
		//Clear the signaled state.
		((Event*)(events[i].udata))->Wait();
	}

	Event* ret = (Event*)(events[0].udata);

	close(multiKqueue);

	return ret;
}

/*static*/ sprawl::threading::Event* sprawl::threading::Event::WaitMultipleFor(EventGroup& values, int64_t nanoseconds)
{
	if(nanoseconds < 0)
	{
		return nullptr;
	}

	struct timespec ts;
	int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
	int64_t nsec = nanoseconds - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);

	ts.tv_sec = time_t(sec);
	ts.tv_nsec = long(nsec);

	int multiKqueue = kqueue();

	struct kevent events[256];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event* event = values[i];
		EV_SET(&events[i], event->m_kqueue, EVFILT_READ, EV_ADD, 0, 0, event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);
	int nret = kevent(multiKqueue, nullptr, 0, events, 256, &ts);

	for(int i = 0; i < nret; ++i)
	{
		//Clear the signaled state.
		((Event*)(events[i].udata))->Wait();
	}

	Event* ret = nullptr;
	if(nret != 0)
	{
		ret = (Event*)(events[0].udata);
	}

	close(multiKqueue);

	return ret;
}