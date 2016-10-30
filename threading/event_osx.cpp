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
	: m_event({EventStatic::highId_++, kqueue()})
{
	struct kevent event;
	EV_SET(&event, m_event.ident, EVFILT_USER, EV_ADD|EV_CLEAR, NOTE_FFNOP, 0, nullptr);
	kevent(m_event.queue, &event, 1, nullptr, 0, nullptr);
}

sprawl::threading::Event::~Event()
{
	close(m_event.queue);
}

void sprawl::threading::Event::Notify() const
{
	fflush(stdout);
	struct kevent event;
	EV_SET(&event, m_event.ident, EVFILT_USER, EV_ADD|EV_CLEAR, NOTE_TRIGGER, 0, nullptr);
	kevent(m_event.queue, &event, 1, nullptr, 0, nullptr);
}

void sprawl::threading::Event::Wait() const
{
	fflush(stdout);
	struct kevent event;
	kevent(m_event.queue, nullptr, 0, &event, 1,  nullptr);
}

bool sprawl::threading::Event::WaitFor(int64_t nanoseconds) const
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
	return (kevent(m_event.queue, nullptr, 0, &event, 1,  &ts) != 0);
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAny(EventGroup const& values)
{
	int multiKqueue = kqueue();

	struct kevent events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event const* event = values[i];
		EV_SET(&events[i], event->m_event.queue, EVFILT_READ, EV_ADD, 0, 0, (void*)event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);
	int nret = kevent(multiKqueue, nullptr, 0, events, SPRAWL_EVENT_MAX_MULTIPLE_WAIT, nullptr);

	for(int i = 0; i < nret; ++i)
	{
		//Clear the signaled state.
		((Event*)(events[i].udata))->Wait();
	}

	Event* ret = (Event*)(events[0].udata);

	close(multiKqueue);

	return ret;
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAnyFor(EventGroup const& values, int64_t nanoseconds)
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

	struct kevent events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event const* event = values[i];
		EV_SET(&events[i], event->m_event.queue, EVFILT_READ, EV_ADD, 0, 0, (void*)event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);
	int nret = kevent(multiKqueue, nullptr, 0, events, SPRAWL_EVENT_MAX_MULTIPLE_WAIT, &ts);

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

/*static*/ void sprawl::threading::Event::WaitAll(EventGroup const& values)
{
	int multiKqueue = kqueue();

	struct kevent events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event const* event = values[i];
		EV_SET(&events[i], event->m_event.queue, EVFILT_READ, EV_ADD|EV_ONESHOT, 0, 0, (void*)event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);

	ssize_t total = 0;
	while(total < values.Size())
	{
		int nret = kevent(multiKqueue, nullptr, 0, events, SPRAWL_EVENT_MAX_MULTIPLE_WAIT, nullptr);

		for(int i = 0; i < nret; ++i)
		{
			//Clear the signaled state.
			((Event*)(events[i].udata))->Wait();
		}

		total += nret;
	}

	close(multiKqueue);
}

/*static*/ bool sprawl::threading::Event::WaitAllFor(EventGroup const& values, int64_t nanoseconds)
{
	if(nanoseconds < 0)
	{
		return nullptr;
	}

	int64_t timeout_time = sprawl::time::Now() + nanoseconds;

	int multiKqueue = kqueue();

	struct kevent events[SPRAWL_EVENT_MAX_MULTIPLE_WAIT];

	for(ssize_t i = 0; i < values.Size(); ++i)
	{
		Event const* event = values[i];
		EV_SET(&events[i], event->m_event.queue, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, (void*)event);
	}
	kevent(multiKqueue, events, values.Size(), nullptr, 0, nullptr);

	ssize_t total = 0;
	while(total < values.Size())
	{
		struct timespec ts;
		int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
		int64_t nsec = nanoseconds - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);

		ts.tv_sec = time_t(sec);
		ts.tv_nsec = long(nsec);

		int nret = kevent(multiKqueue, nullptr, 0, events, SPRAWL_EVENT_MAX_MULTIPLE_WAIT, &ts);

		for(int i = 0; i < nret; ++i)
		{
			//Clear the signaled state.
			((Event*)(events[i].udata))->Wait();
		}

		if(nret == 0)
		{
			break;
		}
		total += nret;
		nanoseconds = timeout_time - sprawl::time::Now();
	}

	close(multiKqueue);

	return total == values.Size();
}