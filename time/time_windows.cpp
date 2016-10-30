#include "time.hpp"
#include <Windows.h>
#include <Winbase.h>
#include <type_traits>

namespace sprawl
{
	namespace time
	{
		static int64_t GetPerformanceFrequency()
		{
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			return frequency.QuadPart;
		}
		static int64_t s_performanceFrequency = GetPerformanceFrequency();

		int64_t Now(Resolution resolution)
		{
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			uint64_t tt = ft.dwHighDateTime;
			tt <<=32;
			tt |= ft.dwLowDateTime;
			tt -= 116444736000000000ULL;
			tt *= 100;
			return int64_t(tt) / std::underlying_type<Resolution>::type(resolution);
		}

		int64_t SteadyNow(Resolution resolution)
		{
			LARGE_INTEGER time;

			QueryPerformanceCounter(&time);

			double nanoseconds = time.QuadPart * double(Resolution::Seconds);
			nanoseconds /= s_performanceFrequency;
			return int64_t(nanoseconds / std::underlying_type<Resolution>::type(resolution));
		}
	}
}
