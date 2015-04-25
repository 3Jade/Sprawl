#include "../time/time.hpp"
#include <chrono>
#include <iostream>

bool test_time()
{
	bool success = true;

	{
		typedef std::chrono::system_clock clock;
		clock::time_point t = clock::now();
		clock::duration nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
		int64_t startTime = nanoseconds.count();

		int64_t now = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

		t = clock::now();
		nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
		int64_t endTime = nanoseconds.count();

		if(startTime > now || endTime < now)
		{
			success = false;
			printf("System time is not in sync with std::chrono\n...");
		}
	}

	{
		typedef std::chrono::steady_clock clock;
		clock::time_point t = clock::now();
		clock::duration nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
		int64_t startTime = nanoseconds.count();

		int64_t now = sprawl::time::SteadyNow(sprawl::time::Resolution::Nanoseconds);

		t = clock::now();
		nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
		int64_t endTime = nanoseconds.count();

		if(startTime > now || endTime < now)
		{
			success = false;
			printf("Steady time is not in sync with std::chrono\n...");
		}
	}

	int64_t timeInMilliseconds = 10000;
	int64_t timeInSeconds = sprawl::time::Convert(timeInMilliseconds, sprawl::time::Resolution::Milliseconds, sprawl::time::Resolution::Seconds);
	int64_t timeInNanoseconds = sprawl::time::Convert(timeInMilliseconds, sprawl::time::Resolution::Milliseconds, sprawl::time::Resolution::Nanoseconds);

	if(timeInSeconds != 10)
	{
		success = false;
		printf("Convert up failed\n...");
	}

	if(timeInNanoseconds != 10LL * 1000LL * 1000LL * 1000LL)
	{
		success = false;
		printf("Convert down failed\n...");
	}

	return success;
}
