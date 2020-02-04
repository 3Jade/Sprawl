#pragma once

#include <type_traits>
#include <stdint.h>

namespace sprawl
{
	namespace time
	{
		enum Resolution : int64_t
		{
			Nanoseconds = 1,
			Microseconds = 1000 * Nanoseconds,
			Milliseconds = 1000 * Microseconds,
			Seconds = 1000 * Milliseconds,
			Minutes = 60 * Seconds,
			Hours = 60 * Minutes,
			Days = 24 * Hours,
			Weeks = 7 * Days,
			Years = 365 * Days
		};

		int64_t Now(Resolution resolution = Resolution::Nanoseconds);
		int64_t SteadyNow(Resolution resolution = Resolution::Nanoseconds);

		inline int64_t Convert(int64_t time, Resolution fromResolution, Resolution toResolution)
		{
			return (time * std::underlying_type<Resolution>::type(fromResolution)) / std::underlying_type<Resolution>::type(toResolution);
		}

		template<Resolution fromResolution, Resolution toResolution>
		int64_t Convert(int64_t time)
		{
			return (time * std::underlying_type<Resolution>::type(fromResolution)) / std::underlying_type<Resolution>::type(toResolution);
		}
	}
}
