#include "event.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <sys/select.h>

sprawl::threading::Event::Event()
	: m_event(eventfd(0, 0))
{

}

sprawl::threading::Event::~Event()
{
	close(m_event);
}

void sprawl::threading::Event::Notify() const
{
	uint64_t const i = 1;
	write(m_event, &i, sizeof(uint64_t));
}

void sprawl::threading::Event::Wait() const
{
	uint64_t i;
	read(m_event, &i, sizeof(uint64_t));
}

bool sprawl::threading::Event::WaitFor(int64_t nanoseconds) const
{
	if(nanoseconds < 0)
	{
		return false;
	}

	fd_set inSet;

	FD_ZERO(&inSet);
	FD_SET(m_event, &inSet);
	int max = m_event;

	struct timeval tv;
	int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
	int64_t usec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Microseconds) - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Microseconds);

	tv.tv_sec = time_t(sec);
	tv.tv_usec = suseconds_t(usec);

	select(max + 1, &inSet, nullptr, nullptr, &tv);
	if(FD_ISSET(m_event, &inSet))
	{
		uint64_t i;
		read(m_event, &i, sizeof(uint64_t));
		return true;
	}
	return false;
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAny(EventGroup const& values)
{
	fd_set inSet;

	FD_ZERO(&inSet);
	int max = 0;
	for(auto& event : values)
	{
		FD_SET(event->m_event, &inSet);
		max = event->m_event > max ? event->m_event : max;
	}

	select(max + 1, &inSet, nullptr, nullptr, nullptr);

	Event const* ret = nullptr;

	for(auto& event : values)
	{
		if(FD_ISSET(event->m_event, &inSet))
		{
			if(ret == nullptr)
			{
				ret = event;
			}
			//Clear the signaled state.
			event->Wait();
		}
	}

	return ret;
}

/*static*/ sprawl::threading::Event const* sprawl::threading::Event::WaitAnyFor(EventGroup const& values, int64_t nanoseconds)
{
	fd_set inSet;

	FD_ZERO(&inSet);
	int max = 0;
	for(auto& event : values)
	{
		FD_SET(event->m_event, &inSet);
		max = event->m_event > max ? event->m_event : max;
	}

	struct timeval tv;
	int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
	int64_t usec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Microseconds) - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Microseconds);

	tv.tv_sec = time_t(sec);
	tv.tv_usec = suseconds_t(usec);

	select(max + 1, &inSet, nullptr, nullptr, &tv);

	Event const* ret = nullptr;

	for(auto& event : values)
	{
		if(FD_ISSET(event->m_event, &inSet))
		{
			if(ret == nullptr)
			{
				ret = event;
			}
			//Clear the signaled state.
			event->Wait();
		}
	}

	return ret;
}


/*static*/ void sprawl::threading::Event::WaitAll(EventGroup const& values)
{
	fd_set inSet;

	FD_ZERO(&inSet);
	int max = 0;
	for(auto& event : values)
	{
		FD_SET(event->m_event, &inSet);
		max = event->m_event > max ? event->m_event : max;
	}

	ssize_t total = 0;

	while(total < values.Size())
	{
		total += select(max + 1, &inSet, nullptr, nullptr, nullptr);

		fd_set set = inSet;
		FD_ZERO(&inSet);

		for(auto& event : values)
		{
			if(FD_ISSET(event->m_event, &set))
			{
				//Clear the signaled state.
				event->Wait();
			}
			else
			{
				FD_SET(event->m_event, &inSet);
			}
		}
	}
}

/*static*/ bool sprawl::threading::Event::WaitAllFor(EventGroup const& values, int64_t nanoseconds)
{
	int64_t timeout_time = sprawl::time::Now() + nanoseconds;

	fd_set inSet;

	FD_ZERO(&inSet);
	int max = 0;
	for(auto& event : values)
	{
		FD_SET(event->m_event, &inSet);
		max = event->m_event > max ? event->m_event : max;
	}

	ssize_t total = 0;

	while(total < values.Size())
	{
		// On Linux, tv is updated after select and we don't have to do this
		// But if this code is used on any other platform with select(), that behavior will probably break
		struct timeval tv;
		int64_t sec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);
		int64_t usec = sprawl::time::Convert(nanoseconds, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Microseconds) - sprawl::time::Convert(sec, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Microseconds);

		tv.tv_sec = time_t(sec);
		tv.tv_usec = suseconds_t(usec);

		int ret = select(max + 1, &inSet, nullptr, nullptr, &tv);

		if(ret == 0)
		{
			return false;
		}

		total += ret;

		fd_set set = inSet;
		FD_ZERO(&inSet);

		for(auto& event : values)
		{
			if(FD_ISSET(event->m_event, &set))
			{
				//Clear the signaled state.
				event->Wait();
			}
			else
			{
				FD_SET(event->m_event, &inSet);
			}
		}
		nanoseconds = timeout_time - sprawl::time::Now();
	}

	return true;
}
