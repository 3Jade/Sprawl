#include "../time/time.hpp"
#include <chrono>
#include <iostream>
#include "../threading/thread.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

TEST(TimeTest, SystemClockWorks)
{
	typedef std::chrono::system_clock clock;
	clock::time_point t = clock::now();
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t startTime = nanoseconds.count();

	sprawl::this_thread::Sleep(100 * sprawl::time::Resolution::Milliseconds);

	int64_t now = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

	t = clock::now();
	nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t endTime = nanoseconds.count();

	EXPECT_LE(startTime, now);
	EXPECT_GE(endTime, now);
}

TEST(TimeTest, SteadyClockWorks)
{
#ifdef _WIN32
	typedef std::chrono::steady_clock clock;
	clock::time_point t = clock::now();
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t startTime = nanoseconds.count();
	int64_t sprawlStartTime = sprawl::time::SteadyNow(sprawl::time::Resolution::Nanoseconds);

	sprawl::this_thread::Sleep(100 * sprawl::time::Resolution::Milliseconds);

	t = clock::now();
	nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t endTime = nanoseconds.count();
	int64_t sprawlEndTime = sprawl::time::SteadyNow(sprawl::time::Resolution::Nanoseconds);

	int64_t delta = endTime - startTime;
	int64_t sprawlDelta = sprawlEndTime - sprawlStartTime;
	EXPECT_LT(abs(sprawlDelta - delta), 1000000);
#else
	typedef std::chrono::steady_clock clock;
	clock::time_point t = clock::now();
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t startTime = nanoseconds.count();

	sprawl::this_thread::Sleep(100 * sprawl::time::Resolution::Milliseconds);

	int64_t now = sprawl::time::SteadyNow(sprawl::time::Resolution::Nanoseconds);

	t = clock::now();
	nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch());
	int64_t endTime = nanoseconds.count();

	EXPECT_LT(startTime, now);
	EXPECT_GT(endTime, now);
#endif
}

TEST(TimeTest, ConversionWorks)
{
	int64_t timeInMilliseconds = 10000;
	int64_t timeInSeconds = sprawl::time::Convert(timeInMilliseconds, sprawl::time::Resolution::Milliseconds, sprawl::time::Resolution::Seconds);
	int64_t timeInNanoseconds = sprawl::time::Convert(timeInMilliseconds, sprawl::time::Resolution::Milliseconds, sprawl::time::Resolution::Nanoseconds);
	ASSERT_EQ(int64_t(10), timeInSeconds);
	ASSERT_EQ(int64_t(10LL * 1000LL * 1000LL * 1000LL), timeInNanoseconds);
}
