#include "../memory/PoolAllocator.hpp"
#include "../memory/StlWrapper.hpp"
#include <algorithm>
#include "../threading/thread.hpp"
#include "../string/String.hpp"

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

#define ADD_TEST_DYNAMIC_2(testType, chunkSize, SIZE, counter) \
	static void testType##_test_dynamic_##chunkSize##_##SIZE() \
	{ \
		typedef sprawl::memory::DynamicPoolAllocator<sizeof(testType), chunkSize> allocator##counter; \
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
	}

#define ADD_TEST_STATIC_2(testType, SIZE, counter) \
	static void testType##_test_static_##SIZE() \
	{ \
		typedef sprawl::memory::StaticPoolAllocator<sizeof(testType), sizeof(testType) * SIZE> allocator##counter; \
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
	}

#define ADD_TEST_DYNAMIC(testType, chunkSize, SIZE, counter) ADD_TEST_DYNAMIC_2(testType, chunkSize, SIZE, counter)
#define ADD_TEST_STATIC(testType, SIZE, counter) ADD_TEST_STATIC_2(testType, SIZE, counter)

#define ADD_TEST(type, chunkSize, arraySize) \
	ADD_TEST_DYNAMIC(type, chunkSize, arraySize, __COUNTER__) \
	ADD_TEST_STATIC(type, arraySize, __COUNTER__)

#define RUN_TEST(type, chunkSize, arraySize) \
	type##_test_dynamic_##chunkSize##_##arraySize(); \
	type##_test_static_##arraySize()

#define ADD_TESTS(type) \
	ADD_TEST(type, 8, 32); 

#include "../time/time.hpp"

const int iterations_alloc = 10000;
const int iterations_threaded = 1000;

#define RUN_TESTS(type) \
{ \
	int64_t total = 0; \
	int64_t high = 0; \
	int64_t low = 0; \
	for(int i = 0; i < iterations_alloc; ++i) \
	{ \
		int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds); \
		RUN_TEST(type, 8, 32);  \
		int64_t elapsed = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds) - start; \
		total += elapsed; \
		if(high == 0 || elapsed > high) high = elapsed; \
		if(low == 0 || elapsed < low) low = elapsed; \
	} \
	printf("\t" #type " - %d runs. Best: %ld ns, Worst: %ld ns, Average: %ld ns\n", iterations_alloc, low, high, total / iterations_alloc); \
}


ADD_TESTS(MyBigStruct)
ADD_TESTS(MyMediumStruct)
ADD_TESTS(MySmallStruct)
ADD_TESTS(MyTinyStruct)
ADD_TESTS(int64_t)

bool success = true;

void alloc_dealloc_sprawl_strings()
{
	for(int i = 0; i < 1000; ++i)
	{
		sprawl::String outerStr("outer");

		{
			sprawl::String str("blah blah blah blah");
			if(str != "blah blah blah blah")
			{
				success = false;
			}
		}
		{
			sprawl::String str("bleh bleh bleh bleh");
			if(str != "bleh bleh bleh bleh")
			{
				success = false;
			}
		}
		{
			sprawl::String str("");
			if(str != "")
			{
				success = false;
			}
		}

		if(outerStr != "outer")
		{
			success = false;
		}
	}
}

bool test_memory()
{
	puts("");
	puts("");
	RUN_TESTS(MyBigStruct);
	RUN_TESTS(MyMediumStruct);
	RUN_TESTS(MySmallStruct);
	RUN_TESTS(MyTinyStruct);
	RUN_TESTS(int64_t);

#ifdef SPRAWL_MULTITHREADED
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
	printf("\tThreaded string allod/dealloc - %d runs. Best: %ld us, Worst: %ld us, Average: %ld us\n", iterations_threaded, low, high, total / iterations_threaded);
#endif

	puts("");
	printf(" ... ");
	return success;
}
