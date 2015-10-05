#include "../memory/PoolAllocator.hpp"
#include "../memory/StlWrapper.hpp"
#include <algorithm>
#include "../threading/thread.hpp"
#include "../string/String.hpp"
#include "../time/time.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

const int iterations_alloc = 10000;
const int iterations_threaded = 1000;

struct MyBigStruct
{
	int64_t data[512];
};

struct MyMediumStruct
{
	int64_t data[256];
};

struct MySmallStruct
{
	int64_t data[128];
};
struct MyTinyStruct
{
	int64_t data[64];
};

#define ADD_TEST_DYNAMIC_2(testType, SIZE, counter) \
	static void testType##_test_dynamic_##SIZE() \
	{ \
		typedef sprawl::memory::PoolAllocator<sizeof(testType)> allocator##counter; \
		 \
		testType* arr##counter[SIZE]; \
		 \
		size_t nums##counter[SIZE]; \
		 \
		for(int i = 0; i < SIZE; ++i) \
		{ \
			nums##counter[i] = i; \
		} \
		 \
		for(int j = 0; j < 1; ++j) \
		{ \
			std::random_shuffle(&nums##counter[0], &nums##counter[SIZE]); \
			{ \
				for(int i = 0; i < SIZE; ++i) \
				{ \
					arr##counter[nums##counter[i]] = (testType*)allocator##counter::alloc(); \
				} \
			} \
			std::random_shuffle(&nums##counter[0], &nums##counter[SIZE]); \
			{ \
				for(int i = SIZE; i > 0; --i) \
				{ \
					allocator##counter::free(arr##counter[nums##counter[i-1]]); \
				} \
			} \
		} \
	} \
	static void PoolAllocator_##testType##_##SIZE() \
	{ \
		int64_t total = 0; \
		int64_t high = 0; \
		int64_t low = 0; \
		for(int i = 0; i < iterations_alloc; ++i) \
		{ \
			int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds); \
			testType##_test_dynamic_##SIZE(); \
			int64_t elapsed = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds) - start; \
			total += elapsed; \
			if(high == 0 || elapsed > high) high = elapsed; \
			if(low == 0 || elapsed < low) low = elapsed; \
		} \
		printf("\t" #testType " (" #SIZE ") - %d runs. Best: %" SPRAWL_I64FMT "d ns, Worst: %" SPRAWL_I64FMT "d ns, Average: %" SPRAWL_I64FMT "d ns\n", iterations_alloc, low, high, total / iterations_alloc); \
		fflush(stdout); \
		fprintf(stderr, "Success"); \
		exit(0); \
	} \
	TEST(PoolAllocatorDeathTest, PoolAllocator_##testType##_##SIZE) \
	{ \
		ASSERT_EXIT(PoolAllocator_##testType##_##SIZE(), testing::ExitedWithCode(0), "Success"); \
	}

#define ADD_TEST_DYNAMIC(testType, SIZE, counter) ADD_TEST_DYNAMIC_2(testType, SIZE, counter)

#define ADD_TESTS(type) \
	ADD_TEST_DYNAMIC(type, 8, __COUNTER__) \
	ADD_TEST_DYNAMIC(type, 32, __COUNTER__) \
	ADD_TEST_DYNAMIC(type, 128, __COUNTER__)

ADD_TESTS(MyBigStruct)
ADD_TESTS(MyMediumStruct)
ADD_TESTS(MySmallStruct)
ADD_TESTS(MyTinyStruct)
ADD_TESTS(int64_t)

#ifdef SPRAWL_MULTITHREADED
void alloc_dealloc_sprawl_strings()
{
	for(int i = 0; i < 1000; ++i)
	{
		sprawl::String outerStr("outer");

		{
			sprawl::String str("blah blah blah blah");
			EXPECT_FALSE(str.empty());
			EXPECT_EQ(sprawl::String("blah blah blah blah"), str);
		}

		{
			sprawl::String str("bleh bleh bleh bleh");
			EXPECT_FALSE(str.empty());
			EXPECT_EQ(sprawl::String("bleh bleh bleh bleh"), str);
		}

		{
			sprawl::String str("");
			EXPECT_TRUE(str.empty());
			EXPECT_EQ(sprawl::String(""), str);
		}

		EXPECT_FALSE(outerStr.empty());
		EXPECT_EQ(sprawl::String("outer"), outerStr);
	}
}

TEST(PoolAllocatorTest, TestAllocatorThreadSafe)
{
	printf("(This should take a second)...\n");
	fflush(stdout);
	sprawl::threading::Thread thread1(alloc_dealloc_sprawl_strings);
	sprawl::threading::Thread thread2(alloc_dealloc_sprawl_strings);
	sprawl::threading::Thread thread3(alloc_dealloc_sprawl_strings);
	sprawl::threading::Thread thread4(alloc_dealloc_sprawl_strings);
	sprawl::threading::Thread thread5(alloc_dealloc_sprawl_strings);

	int64_t total = 0;
	int64_t high = 0;
	int64_t low = 0;
	for(int i = 0; i < iterations_threaded; ++i)
	{
		int64_t start = sprawl::time::Now(sprawl::time::Resolution::Microseconds);
		thread1.Start();
		thread2.Start();
		thread3.Start();
		thread4.Start();
		thread5.Start();

		thread1.Join();
		thread2.Join();
		thread3.Join();
		thread4.Join();
		thread5.Join();
		int64_t elapsed = sprawl::time::Now(sprawl::time::Resolution::Microseconds) - start;
		total += elapsed; \
		if(high == 0 || elapsed > high) high = elapsed;
		if(low == 0 || elapsed < low) low = elapsed;
	}
	printf("\tThreaded string allod/dealloc - %d runs. Best: %" SPRAWL_I64FMT "d us, Worst: %" SPRAWL_I64FMT "d us, Average: %" SPRAWL_I64FMT "d us\n", iterations_threaded, low, high, total / iterations_threaded);
}
#endif
