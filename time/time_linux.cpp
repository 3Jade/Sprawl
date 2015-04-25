#include "time.hpp"
#include <time.h>
#include <type_traits>

namespace sprawl
{
	namespace time
	{
		int64_t Now(Resolution resolution)
		{
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			uint64_t nanosecondResult = ts.tv_sec;
			nanosecondResult *= 1000000000;
			nanosecondResult += ts.tv_nsec;
			return nanosecondResult / std::underlying_type<Resolution>::type(resolution);
		}

		int64_t SteadyNow(Resolution resolution)
		{
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			uint64_t nanosecondResult = ts.tv_sec;
			nanosecondResult *= 1000000000;
			nanosecondResult += ts.tv_nsec;
			return nanosecondResult / std::underlying_type<Resolution>::type(resolution);
		}
	}
}
