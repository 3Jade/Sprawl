#include "time.hpp"
#include <time.h>
#include <type_traits>
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/clock.h>

namespace sprawl
{
	namespace time
	{
		static double GetTimeBase()
		{
			mach_timebase_info_data_t info;
			mach_timebase_info(&info);
			return double(info.numer) / double(info.denom);
		}
		static double s_timeBase = GetTimeBase();

		static struct ClockPort
		{
			ClockPort()
			{
				mach_port_t hostPort = mach_host_self();
				host_get_clock_service(hostPort, CALENDAR_CLOCK, &port);
				mach_port_deallocate(mach_task_self(), hostPort);
			}

			~ClockPort()
			{
				mach_port_deallocate(mach_task_self(), port);
			}

			mach_port_t port;
		} s_clockPort;

		int64_t Now(Resolution resolution)
		{
			mach_timespec_t ts;
			clock_get_time(s_clockPort.port, &ts);
			uint64_t nanosecondResult = ts.tv_sec;
			nanosecondResult *= 1000000000;
			nanosecondResult += ts.tv_nsec;
			return nanosecondResult / std::underlying_type<Resolution>::type(resolution);
		}

		int64_t SteadyNow(Resolution resolution)
		{
			uint64_t absTime = mach_absolute_time();
			uint64_t nanosecondResult = absTime * s_timeBase;
			return nanosecondResult / std::underlying_type<Resolution>::type(resolution);
		}
	}
}
