#include "../memory/PoolAllocator.hpp"
#include "../memory/StlWrapper.hpp"
#include "../memory/opaque_type.hpp"
#include <algorithm>
#include "../threading/thread.hpp"
#include "../string/String.hpp"
#include "../time/time.hpp"

#include "gtest_helpers.hpp"
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

namespace OpaquePointersCallCorrectConstructorsAndDestructors
{
	bool constructed = false;
	bool destructed = false;
}

TEST(OpaqueTypeTest, OpaquePointersCallCorrectConstructorsAndDestructors)
{

	struct ConstructDestruct
	{
		ConstructDestruct() { OpaquePointersCallCorrectConstructorsAndDestructors::constructed = true; }
		~ConstructDestruct() { OpaquePointersCallCorrectConstructorsAndDestructors::destructed = true; }
	};

	{
		sprawl::memory::OpaqueType<sizeof(ConstructDestruct)> value = sprawl::memory::OpaqueType<sizeof(ConstructDestruct)>(sprawl::memory::CreateAs<ConstructDestruct>());
		ASSERT_TRUE(OpaquePointersCallCorrectConstructorsAndDestructors::constructed);
		ASSERT_FALSE(OpaquePointersCallCorrectConstructorsAndDestructors::destructed);
	}
	ASSERT_TRUE(OpaquePointersCallCorrectConstructorsAndDestructors::destructed);
}

TEST(OpaqueTypeTest, AccessingElementsWorksCorrectly)
{
	struct AccessTest
	{
		AccessTest(int i, sprawl::String const& s, bool b, sprawl::String const& s2, double d)
			: i(i)
			, s(s)
			, b(b)
			, s2(s2)
			, d(d)
		{

		}

		int i;
		sprawl::String s;
		bool b;
		sprawl::String s2;
		double d;
	};

	double d = 123485.1235199283758919293875; // Random.
	sprawl::memory::OpaqueType<sizeof(AccessTest)> value(sprawl::memory::CreateAs<AccessTest>(), 5, "Hello", false, "World", d);
	ASSERT_EQ(5, value.As<AccessTest>().i);
	ASSERT_FALSE(value.As<AccessTest>().b);
	ASSERT_EQ(sprawl::String("Hello"), value.As<AccessTest>().s);
	ASSERT_EQ(sprawl::String("World"), value.As<AccessTest>().s2);
	ASSERT_EQ(d, value.As<AccessTest>().d);
}

TEST(OpaqueTypeTest, ConversionOperatorWorks)
{
	struct AccessTest
	{
		AccessTest(int i, sprawl::String const& s, bool b, sprawl::String const& s2, double d)
			: i(i)
			, s(s)
			, b(b)
			, s2(s2)
			, d(d)
		{

		}

		int i;
		sprawl::String s;
		bool b;
		sprawl::String s2;
		double d;
	};

	double d = 123485.1235199283758919293875; // Random.
	sprawl::memory::OpaqueType<sizeof(AccessTest)> value(sprawl::memory::CreateAs<AccessTest>(), 5, "Hello", false, "World", d);
	AccessTest& test = value;
	ASSERT_EQ(5, test.i);
	ASSERT_FALSE(test.b);
	ASSERT_EQ(sprawl::String("Hello"), test.s);
	ASSERT_EQ(sprawl::String("World"), test.s2);
	ASSERT_EQ(d, test.d);
}

TEST(OpaqueTypeTest, PolymorphismWorks)
{
	struct AccessTest
	{
		AccessTest(int i, sprawl::String const& s, bool b)
			: i(i)
			, s(s)
			, b(b)
		{

		}

		int i;
		sprawl::String s;
		bool b;
	};

	struct AddExtraData : public AccessTest
	{

		AddExtraData(int i, sprawl::String const& s, bool b, sprawl::String const& s2, double d)
			: AccessTest(i, s, b)
			, s2(s2)
			, d(d)
		{

		}
		sprawl::String s2;
		double d;
	};

	double d = 123485.1235199283758919293875; // Random.
	sprawl::memory::OpaqueType<sizeof(AddExtraData)> value(sprawl::memory::CreateAs<AddExtraData>(), 5, "Hello", false, "World", d);
	AccessTest& test = value.As<AddExtraData>();
	ASSERT_EQ(5, test.i);
	ASSERT_FALSE(test.b);
	ASSERT_EQ(sprawl::String("Hello"), test.s);
}

template<typename T, typename U>
struct is_same_with_compile_error_if_not
{
	static bool const value = false;

	typename T::FirstType_IntentionalCompileTimeError t;
	typename U::SecondType_IntentionalCompileTimeError u;
};

template<typename T>
struct is_same_with_compile_error_if_not<T, T>
{
	static bool const value = true;
};

TEST(OpaqueTypeTest, OpaqueTypeListWorks)
{

	struct ListTest
	{
		char c;
		int i;
		sprawl::String s;
		bool b;
		sprawl::String s2;
		double d;
	};

	struct ListTest2
	{
		char c;
		int i;
		sprawl::String s;
		bool b;
	};

	struct ListTest3
	{
		char c;
		int i;
		sprawl::String s;
		bool b;
		sprawl::String s2;
		double d;
		char c2;
	};

	struct ListTest4
	{
		char c;
		int i;
		sprawl::String s;
		bool b;
		sprawl::String s2;
	};

	struct ListTest5
	{
		int i;
		double d;
		int i2;
		double d2;
	};

	struct ListTest6
	{
		double d;
		int i;
		double d2;
		int i2;
	};

	struct JustAChar
	{
		char c;
	};

	struct SeveralChars
	{
		char c;
		char c2;
		char c3;
		char c4;
		char c5;
		char c6;
		char c7;
		char c8;
		char c9;
		char c10;
	};

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest), alignof(ListTest)>,
			sprawl::memory::OpaqueTypeList<char, int, sprawl::String, bool, sprawl::String, double>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest2), alignof(ListTest2)>,
			sprawl::memory::OpaqueTypeList<char, int, sprawl::String, bool>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest3), alignof(ListTest3)>,
			sprawl::memory::OpaqueTypeList<char, int, sprawl::String, bool, sprawl::String, double, char>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest4), alignof(ListTest4)>,
			sprawl::memory::OpaqueTypeList<char, int, sprawl::String, bool, sprawl::String>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest5), alignof(ListTest5)>,
			sprawl::memory::OpaqueTypeList<int, double, int, double>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(ListTest6), alignof(ListTest6)>,
			sprawl::memory::OpaqueTypeList<double, int, double, int>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(JustAChar), alignof(JustAChar)>,
			sprawl::memory::OpaqueTypeList<char>
		>::value,
		"OpaqueTypeList broke."
	);

	static_assert(
		is_same_with_compile_error_if_not<
			sprawl::memory::OpaqueType<sizeof(SeveralChars), alignof(SeveralChars)>,
			sprawl::memory::OpaqueTypeList<char, char, char, char, char, char, char, char, char, char>
		>::value,
		"OpaqueTypeList broke."
	);
}

TEST(OpaqueTypeTest, DefaultAlignSafeForSmallerTypes)
{
	struct JustAChar
	{
		JustAChar(char c_) : c(c_) {}
		char c;
	};

	sprawl::memory::OpaqueType<sizeof(char)> t(sprawl::memory::CreateAs<JustAChar>(), 'c');

	ASSERT_EQ('c', t.As<JustAChar>().c);
}