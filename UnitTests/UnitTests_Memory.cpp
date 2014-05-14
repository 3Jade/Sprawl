#include "../memory/PoolAllocator.hpp"
#include "../memory/StlWrapper.hpp"
#include <algorithm>

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

#define RUN_TESTS(type) \
	RUN_TEST(type, 8, 32); 

ADD_TESTS(MyBigStruct)
ADD_TESTS(MyMediumStruct)
ADD_TESTS(MySmallStruct)
ADD_TESTS(MyTinyStruct)
ADD_TESTS(int64_t)

bool test_memory()
{
	RUN_TESTS(MyBigStruct);
	RUN_TESTS(MyMediumStruct);
	RUN_TESTS(MySmallStruct);
	RUN_TESTS(MyTinyStruct);
	RUN_TESTS(int64_t);

	return true;
}
