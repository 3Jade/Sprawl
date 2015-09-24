//#define SPRAWL_NO_VARIADIC_TEMPLATES

#include "../collections/HashMap.hpp"
#include "../string/String.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

namespace CollectionTest
{
	class TestType
	{
	public:
		int key1;
		int key2;
		sprawl::String key3;

		int GetKey1() { return key1; }
		int GetKey2() { return key2; }
		sprawl::String GetKey3() { return key3; }

		TestType* operator->(){ return this; }

		TestType(int i, int j, sprawl::String const& k)
			: key1(i)
			, key2(j)
			, key3(k)
		{
			//
		}
	};

	int TestTypeAccessor(TestType* obj)
	{
		return obj->key2;
	}

	int TestTypeAccessor2(TestType const& obj)
	{
		return obj.key2;
	}
}

#include <memory>

using CollectionTest::TestType;
using CollectionTest::TestTypeAccessor;

class HashMapTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		map.Insert(1, std::shared_ptr<TestType>(new TestType(1, 5, "str1")));
		map.Insert(2, std::shared_ptr<TestType>(new TestType(2, 6, "str2")));
		map.Insert(3, std::shared_ptr<TestType>(new TestType(3, 7, "str3")));
		map.Insert(4, std::shared_ptr<TestType>(new TestType(3, 8, "str4")));

		map2.Insert(1, std::shared_ptr<TestType>(new TestType(1, 5, "str1")));
		map2.Insert(2, std::shared_ptr<TestType>(new TestType(2, 6, "str2")));
		map2.Insert(3, std::shared_ptr<TestType>(new TestType(3, 7, "str3")));
		map2.Insert(4, std::shared_ptr<TestType>(new TestType(3, 8, "str4")));

		map3.Insert(1, std::shared_ptr<TestType>(new TestType(1, 5, "str1")));
		map3.Insert(2, std::shared_ptr<TestType>(new TestType(2, 6, "str2")));
		map3.Insert(3, std::shared_ptr<TestType>(new TestType(3, 7, "str3")));
		map3.Insert(4, std::shared_ptr<TestType>(new TestType(3, 8, "str4")));

		map4.Insert(1, TestType(1, 5, "str1"));
		map4.Insert(2, TestType(2, 6, "str2"));
		map4.Insert(3, TestType(3, 7, "str3"));
		map4.Insert(4, TestType(3, 8, "str4"));

		map5.Insert(1, TestType(1, 5, "str1"));
		map5.Insert(2, TestType(2, 6, "str2"));
		map5.Insert(3, TestType(3, 7, "str3"));
		map5.Insert(4, TestType(3, 8, "str4"));

		map6.Insert(1, TestType(1, 5, "str1"));
		map6.Insert(2, TestType(2, 6, "str2"));
		map6.Insert(3, TestType(3, 7, "str3"));
		map6.Insert(4, TestType(3, 8, "str4"));
	}

	sprawl::collections::HashMap<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
		, sprawl::PtrFunctionAccessor<TestType, int, &TestTypeAccessor, std::shared_ptr<TestType>>
		, sprawl::PtrMemberAccessor<TestType, sprawl::String, &TestType::GetKey3, std::shared_ptr<TestType>>
	> map;

	sprawl::collections::HashMap<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
		, sprawl::PtrFunctionAccessor<TestType, int, &TestTypeAccessor, std::shared_ptr<TestType>>
	> map2;

	sprawl::collections::HashMap<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
	> map3;

	sprawl::collections::HashMap<
		TestType
		, sprawl::KeyAccessor<TestType, int>
		, sprawl::FunctionAccessor<TestType, int, &TestTypeAccessor>
		, sprawl::MemberAccessor<TestType, sprawl::String, &TestType::GetKey3>
	> map4;

	sprawl::collections::HashMap<
		TestType
		, sprawl::KeyAccessor<TestType, int>
		, sprawl::FunctionAccessor<TestType, int, &TestTypeAccessor>
	> map5;

	sprawl::collections::HashMap<
		TestType
		, sprawl::KeyAccessor<TestType, int>
	> map6;
};

TEST_F(HashMapTest, LookupWorks)
{
	EXPECT_TRUE(map.Has<0>(1));
	EXPECT_TRUE(map.Has<0>(2));
	EXPECT_TRUE(map.Has<0>(3));
	EXPECT_TRUE(map.Has<0>(4));
	EXPECT_TRUE(map.Has<1>(5));
	EXPECT_TRUE(map.Has<1>(6));
	EXPECT_TRUE(map.Has<1>(7));
	EXPECT_TRUE(map.Has<1>(8));
	EXPECT_TRUE(map.Has("str1"));
	EXPECT_TRUE(map.Has("str2"));
	EXPECT_TRUE(map.Has("str3"));
	EXPECT_TRUE(map.Has("str4"));

	EXPECT_TRUE(map2.Has<0>(1));
	EXPECT_TRUE(map2.Has<0>(2));
	EXPECT_TRUE(map2.Has<0>(3));
	EXPECT_TRUE(map2.Has<0>(4));
	EXPECT_TRUE(map2.Has<1>(5));
	EXPECT_TRUE(map2.Has<1>(6));
	EXPECT_TRUE(map2.Has<1>(7));
	EXPECT_TRUE(map2.Has<1>(8));

	EXPECT_TRUE(map3.Has(1));
	EXPECT_TRUE(map3.Has(2));
	EXPECT_TRUE(map3.Has(3));
	EXPECT_TRUE(map3.Has(4));

	EXPECT_TRUE(map4.Has<0>(1));
	EXPECT_TRUE(map4.Has<0>(2));
	EXPECT_TRUE(map4.Has<0>(3));
	EXPECT_TRUE(map4.Has<0>(4));
	EXPECT_TRUE(map4.Has<1>(5));
	EXPECT_TRUE(map4.Has<1>(6));
	EXPECT_TRUE(map4.Has<1>(7));
	EXPECT_TRUE(map4.Has<1>(8));
	EXPECT_TRUE(map4.Has("str1"));
	EXPECT_TRUE(map4.Has("str2"));
	EXPECT_TRUE(map4.Has("str3"));
	EXPECT_TRUE(map4.Has("str4"));

	EXPECT_TRUE(map5.Has<0>(1));
	EXPECT_TRUE(map5.Has<0>(2));
	EXPECT_TRUE(map5.Has<0>(3));
	EXPECT_TRUE(map5.Has<0>(4));
	EXPECT_TRUE(map5.Has<1>(5));
	EXPECT_TRUE(map5.Has<1>(6));
	EXPECT_TRUE(map5.Has<1>(7));
	EXPECT_TRUE(map5.Has<1>(8));

	EXPECT_TRUE(map6.Has(1));
	EXPECT_TRUE(map6.Has(2));
	EXPECT_TRUE(map6.Has(3));
	EXPECT_TRUE(map6.Has(4));
}

TEST_F(HashMapTest, EraseWorks)
{
	map.Erase<0>(4);
	map2.Erase<0>(4);
	map3.Erase(4);
	map4.Erase<0>(4);
	map5.Erase<0>(4);
	map6.Erase(4);

	EXPECT_EQ(map.end(), map.find<0>(4));
	EXPECT_EQ(map.end(), map.find<1>(8));
	EXPECT_EQ(map.end(), map.find<2>("str4"));

	EXPECT_EQ(map2.end(), map2.find<0>(4));
	EXPECT_EQ(map2.end(), map2.find<1>(8));

	EXPECT_EQ(map3.end(), map3.find<0>(4));

	EXPECT_EQ(map4.end(), map4.find<0>(4));
	EXPECT_EQ(map4.end(), map4.find<1>(8));
	EXPECT_EQ(map4.end(), map4.find<2>("str4"));

	EXPECT_EQ(map5.end(), map5.find<0>(4));
	EXPECT_EQ(map5.end(), map5.find<1>(8));

	EXPECT_EQ(map6.end(), map6.find<0>(4));
}

TEST_F(HashMapTest, GetWorks)
{
	EXPECT_EQ(3, map.Get<0>(3)->key1);
	EXPECT_EQ(1, map.Get<1>(5)->key1);
	EXPECT_EQ(1, map.Get("str1")->key1);

	EXPECT_EQ(3, map2.Get<0>(3)->key1);
	EXPECT_EQ(1, map2.Get<1>(5)->key1);

	EXPECT_EQ(3, map3.Get<0>(3)->key1);

	EXPECT_EQ(3, map4.Get<0>(3)->key1);
	EXPECT_EQ(1, map4.Get<1>(5)->key1);
	EXPECT_EQ(1, map4.Get("str1")->key1);

	EXPECT_EQ(3, map5.Get<0>(3)->key1);
	EXPECT_EQ(1, map5.Get<1>(5)->key1);

	EXPECT_EQ(3, map6.Get<0>(3)->key1);
}

TEST(MultiKeyMapTest, MultipleKeyAccessorsWork)
{
	sprawl::collections::HashMap<
		int
		, sprawl::KeyAccessor<int, int>
		, sprawl::KeyAccessor<int, sprawl::String>
		, sprawl::KeyAccessor<int, double>
	> multiKeyMapTest;

	multiKeyMapTest.Insert(1, "one", 1.0, 1);
	multiKeyMapTest.Insert(2, "two", 2.0, 2);
	multiKeyMapTest.Insert(3, "three", 3.0, 3);
	multiKeyMapTest.Insert(4, "four", 4.0, 4);

	EXPECT_EQ(1, multiKeyMapTest.Get(1));
	EXPECT_EQ(1, multiKeyMapTest.Get("one"));
	EXPECT_EQ(1, multiKeyMapTest.Get(1.0));

	EXPECT_EQ(2, multiKeyMapTest.Get(2));
	EXPECT_EQ(2, multiKeyMapTest.Get("two"));
	EXPECT_EQ(2, multiKeyMapTest.Get(2.0));

	EXPECT_EQ(3, multiKeyMapTest.Get(3));
	EXPECT_EQ(3, multiKeyMapTest.Get("three"));
	EXPECT_EQ(3, multiKeyMapTest.Get(3.0));

	EXPECT_EQ(4, multiKeyMapTest.Get(4));
	EXPECT_EQ(4, multiKeyMapTest.Get("four"));
	EXPECT_EQ(4, multiKeyMapTest.Get(4.0));
}

//This just tests that these various aliases all compile. It doesn't run since it doesn't actually do anything; functionality's tested above.
void HashMapCompileTest()
{
	sprawl::collections::HashMap<TestType*, sprawl::KeyAccessor<TestType*, sprawl::String>> compileFailureWithImplicitConstruction;
	compileFailureWithImplicitConstruction.Insert("blah", nullptr);

	sprawl::collections::HashSet<sprawl::String> setTest;
	setTest.Insert("blah");

	sprawl::collections::BasicHashMap<sprawl::String, TestType*> basicHashMapCompileTest;
	basicHashMapCompileTest.Insert("blah", nullptr);

	sprawl::collections::BasicHashMap<long, long> copyConstructorTest;
	sprawl::collections::BasicHashMap<long, long> copyConstructorTest2(copyConstructorTest);
	copyConstructorTest2.Insert(0, 0);
	copyConstructorTest2.Get(0);

	sprawl::collections::FunctionHashMap<int, TestType, &TestTypeAccessor> functionHashMapCompileTest;
	functionHashMapCompileTest.Insert(TestType(1,2,"hi"));

	sprawl::collections::MemberHashMap<sprawl::String, TestType, &TestType::GetKey3> memberHashMapCompileTest;
	memberHashMapCompileTest.Insert(TestType(1,2,"hi"));

	sprawl::collections::PtrMemberHashMap<sprawl::String, TestType*, &TestType::GetKey3> ptrMemberHashMapCompileTest;
	ptrMemberHashMapCompileTest.Insert(new TestType(1,2,"hi"));

	sprawl::collections::PtrFunctionHashMap<int, TestType*, &TestTypeAccessor> ptrFunctionHashMapCompileTest;
	ptrFunctionHashMapCompileTest.Insert(new TestType(1,2,"hi"));

	sprawl::collections::PtrFunctionHashMap<int, std::shared_ptr<TestType>, &TestTypeAccessor> sharedPtrFunctionHashMapCompileTest;
	sharedPtrFunctionHashMapCompileTest.Insert(std::shared_ptr<TestType>(new TestType(1,2,"hi")));

	sprawl::collections::PtrMemberHashMap<sprawl::String, std::shared_ptr<TestType>, &TestType::GetKey3> sharedPtrMemberHashMapCompileTest;
	sharedPtrMemberHashMapCompileTest.Insert(std::shared_ptr<TestType>(new TestType(1,2,"hi")));
}
