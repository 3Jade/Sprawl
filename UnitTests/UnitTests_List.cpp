#include "../collections/List.hpp"
#include "../collections/ForwardList.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

class ListTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		testList.PushBack(3);
		testList.PushBack(5);
		testList.PushFront(2);
		testList.PushFront(1);
	}
	sprawl::collections::List<int> testList;
};

TEST_F(ListTest, BasicSetupWorks)
{
	EXPECT_EQ(size_t(4), testList.Size());

	auto it = testList.begin();
	EXPECT_EQ(1, it.Value());
	++it;
	EXPECT_EQ(2, it.Value());
	++it;
	EXPECT_EQ(3, it.Value());
	++it;
	EXPECT_EQ(5, it.Value());
}

TEST_F(ListTest, FrontWorks)
{
	ASSERT_EQ(1, testList.Front());
}

TEST_F(ListTest, BackWorks)
{
	ASSERT_EQ(5, testList.Back());
}

TEST_F(ListTest, IterationAndInsertWorks)
{
	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() == 5)
		{
			testList.Insert(it, 4);
			break;
		}
	}

	int value = 0;

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		EXPECT_EQ(value + 1, it.Value());
		value = it.Value();
	}
}

TEST_F(ListTest, PopFrontWorks)
{
	testList.PopFront();
	ASSERT_EQ(2, testList.Front());
}

TEST_F(ListTest, PopBackWorks)
{
	testList.PopBack();
	ASSERT_EQ(3, testList.Back());
}

TEST_F(ListTest, EraseWorks)
{

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testList.Erase(it);
			break;
		}
	}

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		ASSERT_NE(3, it.Value());
	}
}

class ForwardListTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		testList.PushFront(5);
		testList.PushFront(3);
		testList.PushFront(2);
		testList.PushFront(1);
	}
	sprawl::collections::ForwardList<int> testList;
};

TEST_F(ForwardListTest, BasicSetupWorks)
{
	EXPECT_EQ(size_t(4), testList.Size());

	auto it = testList.begin();
	EXPECT_EQ(1, it.Value());
	++it;
	EXPECT_EQ(2, it.Value());
	++it;
	EXPECT_EQ(3, it.Value());
	++it;
	EXPECT_EQ(5, it.Value());
}

TEST_F(ForwardListTest, FrontWorks)
{
	ASSERT_EQ(1, testList.Front());
}

TEST_F(ForwardListTest, IterationAndInsertWorks)
{
	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if((it+1).Value() == 5)
		{
			testList.InsertAfter(it, 4);
			break;
		}
	}

	int value = 0;

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		EXPECT_EQ(value + 1, it.Value());
		value = it.Value();
	}
}

TEST_F(ForwardListTest, PopFrontWorks)
{
	testList.PopFront();
	ASSERT_EQ(2, testList.Front());
}

TEST_F(ForwardListTest, EraseWorks)
{

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if((it+1).Value() == 3)
		{
			testList.EraseAfter(it);
			break;
		}
	}

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		ASSERT_NE(3, it.Value());
	}
}

TEST_F(ForwardListTest, CopyWorks)
{

	sprawl::collections::ForwardList<int> forwardListCopyTest;
	forwardListCopyTest.PushFront(1);
	forwardListCopyTest.PushFront(2);
	forwardListCopyTest.PushFront(3);
	forwardListCopyTest.PushFront(4);
	forwardListCopyTest.PushFront(5);

	int lastItem = 6;
	for(auto& item : forwardListCopyTest)
	{
		EXPECT_EQ(lastItem - 1, item);
		lastItem = item;
	}

	lastItem = 6;
	sprawl::collections::ForwardList<int> listCopy(forwardListCopyTest);
	for(auto& item : listCopy)
	{
		EXPECT_EQ(lastItem - 1, item);
		lastItem = item;
	}

	EXPECT_EQ(1, lastItem);
}
