//#define SPRAWL_NO_VARIADIC_TEMPLATES

#include "../collections/BinaryTree.hpp"
#include "../string/String.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

namespace BinaryTreeTestInternal
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

using BinaryTreeTestInternal::TestType;
using BinaryTreeTestInternal::TestTypeAccessor;

class BinaryTreeTest : public testing::Test
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

	sprawl::collections::BinaryTree<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
		, sprawl::PtrFunctionAccessor<TestType, int, &TestTypeAccessor, std::shared_ptr<TestType>>
		, sprawl::PtrMemberAccessor<TestType, sprawl::String, &TestType::GetKey3, std::shared_ptr<TestType>>
	> map;

	sprawl::collections::BinaryTree<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
		, sprawl::PtrFunctionAccessor<TestType, int, &TestTypeAccessor, std::shared_ptr<TestType>>
	> map2;

	sprawl::collections::BinaryTree<
		std::shared_ptr<TestType>
		, sprawl::KeyAccessor<std::shared_ptr<TestType>, int>
	> map3;

	sprawl::collections::BinaryTree<
		TestType
		, sprawl::KeyAccessor<TestType, int>
		, sprawl::FunctionAccessor<TestType, int, &TestTypeAccessor>
		, sprawl::MemberAccessor<TestType, sprawl::String, &TestType::GetKey3>
	> map4;

	sprawl::collections::BinaryTree<
		TestType
		, sprawl::KeyAccessor<TestType, int>
		, sprawl::FunctionAccessor<TestType, int, &TestTypeAccessor>
	> map5;

	sprawl::collections::BinaryTree<
		TestType
		, sprawl::KeyAccessor<TestType, int>
	> map6;
};

TEST_F(BinaryTreeTest, LookupWorks)
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

TEST_F(BinaryTreeTest, EraseWorks)
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

TEST_F(BinaryTreeTest, GetWorks)
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

sprawl::String RandomString()
{
	sprawl::StringBuilder builder;
	for(int i = 3; i < 8; ++i)
	{
		builder << char((rand() % (int('z') - int('a'))) + int('a'));
	}
	return builder.Str();
}

int RandomNumber()
{
	return (rand() % 50000) - 25000;
}

TEST_F(BinaryTreeTest, IterationIsOrdered)
{
	for(int i = 0; i < 300; ++i)
	{
		map4.Insert(RandomNumber(), TestType(RandomNumber(), RandomNumber(), RandomString()));
	}

	int key0 = -50000;
	for(auto& it : map4)
	{
		EXPECT_LT(key0, it.Key<0>());
		key0 = it.Key<0>();
	}

	key0 = -50000;
	for(auto it = map4.begin<0>(); it != map4.end<0>(); ++it)
	{
		EXPECT_LT(key0, it.Key<0>());
		key0 = it.Key<0>();
	}

	int key1 = -50000;
	for(auto it = map4.begin<1>(); it != map4.end<1>(); ++it)
	{
		EXPECT_LT(key1, it.Key<1>());
		key1 = it.Key<1>();
	}

	sprawl::String key2("");
	for(auto it = map4.begin<2>(); it != map4.end<2>(); ++it)
	{
		EXPECT_LT(key2, it.Key<2>());
		key2 = it.Key<2>();
	}
}

TEST_F(BinaryTreeTest, UpperBoundWorks)
{
	sprawl::collections::BinarySet<int> set;
	set.Insert(5);
	set.Insert(3);
	set.Insert(7);
	set.Insert(2);
	set.Insert(4);
	set.Insert(6);
	set.Insert(8);

	EXPECT_EQ(2, set.UpperBound(1).Value());
	EXPECT_EQ(3, set.UpperBound(2).Value());
	EXPECT_EQ(4, set.UpperBound(3).Value());
	EXPECT_EQ(5, set.UpperBound(4).Value());
	EXPECT_EQ(6, set.UpperBound(5).Value());
	EXPECT_EQ(7, set.UpperBound(6).Value());
	EXPECT_EQ(8, set.UpperBound(7).Value());
	EXPECT_EQ(set.end(), set.UpperBound(8));
}

TEST_F(BinaryTreeTest, LowerBoundWorks)
{
	sprawl::collections::BinarySet<int> set;
	set.Insert(5);
	set.Insert(3);
	set.Insert(7);
	set.Insert(2);
	set.Insert(4);
	set.Insert(6);
	set.Insert(8);

	EXPECT_EQ(set.end(), set.LowerBound(1));
	EXPECT_EQ(2, set.LowerBound(2).Value());
	EXPECT_EQ(3, set.LowerBound(3).Value());
	EXPECT_EQ(4, set.LowerBound(4).Value());
	EXPECT_EQ(5, set.LowerBound(5).Value());
	EXPECT_EQ(6, set.LowerBound(6).Value());
	EXPECT_EQ(7, set.LowerBound(7).Value());
	EXPECT_EQ(8, set.LowerBound(8).Value());
	EXPECT_EQ(8, set.LowerBound(9).Value());
}

TEST(MultiKeyTreeTest, MultipleKeyAccessorsWork)
{
	sprawl::collections::BinaryTree<
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
void BinaryTreeCompileTest()
{
	sprawl::collections::BinaryTree<TestType*, sprawl::KeyAccessor<TestType*, sprawl::String>> compileFailureWithImplicitConstruction;
	compileFailureWithImplicitConstruction.Insert("blah", nullptr);

	sprawl::collections::BinarySet<sprawl::String> setTest;
	setTest.Insert("blah");

	sprawl::collections::BasicBinaryTree<sprawl::String, TestType*> basicBinaryTreeCompileTest;
	basicBinaryTreeCompileTest.Insert("blah", nullptr);

	sprawl::collections::BasicBinaryTree<long, long> copyConstructorTest;
	sprawl::collections::BasicBinaryTree<long, long> copyConstructorTest2(copyConstructorTest);
	copyConstructorTest2.Insert(0, 0);
	copyConstructorTest2.Get(0);

	sprawl::collections::FunctionBinaryTree<int, TestType, &TestTypeAccessor> functionBinaryTreeCompileTest;
	functionBinaryTreeCompileTest.Insert(TestType(1,2,"hi"));

	sprawl::collections::MemberBinaryTree<sprawl::String, TestType, &TestType::GetKey3> memberBinaryTreeCompileTest;
	memberBinaryTreeCompileTest.Insert(TestType(1,2,"hi"));

	sprawl::collections::PtrMemberBinaryTree<sprawl::String, TestType*, &TestType::GetKey3> ptrMemberBinaryTreeCompileTest;
	ptrMemberBinaryTreeCompileTest.Insert(new TestType(1,2,"hi"));

	sprawl::collections::PtrFunctionBinaryTree<int, TestType*, &TestTypeAccessor> ptrFunctionBinaryTreeCompileTest;
	ptrFunctionBinaryTreeCompileTest.Insert(new TestType(1,2,"hi"));

	sprawl::collections::PtrFunctionBinaryTree<int, std::shared_ptr<TestType>, &TestTypeAccessor> sharedPtrFunctionBinaryTreeCompileTest;
	sharedPtrFunctionBinaryTreeCompileTest.Insert(std::shared_ptr<TestType>(new TestType(1,2,"hi")));

	sprawl::collections::PtrMemberBinaryTree<sprawl::String, std::shared_ptr<TestType>, &TestType::GetKey3> sharedPtrMemberBinaryTreeCompileTest;
	sharedPtrMemberBinaryTreeCompileTest.Insert(std::shared_ptr<TestType>(new TestType(1,2,"hi")));
}
