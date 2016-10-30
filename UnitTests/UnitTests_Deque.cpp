#include "../collections/Deque.hpp"

#include "gtest_helpers.hpp"
#include <gtest/gtest.h>

class DequeTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		testDeque.PushBack(3);
		testDeque.PushBack(5);
		testDeque.PushFront(2);
		testDeque.PushFront(1);
	}

	sprawl::collections::Deque<int> testDeque;
};

TEST_F(DequeTest, BasicSetupWorks)
{
	EXPECT_EQ(1, testDeque[0]);
	EXPECT_EQ(2, testDeque[1]);
	EXPECT_EQ(3, testDeque[2]);
	EXPECT_EQ(5, testDeque[3]);
}


TEST_F(DequeTest, FillConstructorsWork)
{
	sprawl::collections::Deque<int> testDeque2(sprawl::collections::Fill(5));

	EXPECT_EQ(ssize_t(5), testDeque2.Size());

	for(auto& value : testDeque2)
	{
		EXPECT_EQ(0, value);
	}

	sprawl::collections::Deque<int> testDeque3(sprawl::collections::Fill(5), 3);

	EXPECT_EQ(ssize_t(5), testDeque3.Size());

	for(auto& value : testDeque3)
	{
		EXPECT_EQ(3, value);
	}
}

TEST_F(DequeTest, CopyConstructorsWork)
{
	sprawl::collections::Deque<int> testDeque2(testDeque);
	EXPECT_EQ(ssize_t(4), testDeque2.Size());
	EXPECT_EQ(1, testDeque2[0]);
	EXPECT_EQ(2, testDeque2[1]);
	EXPECT_EQ(3, testDeque2[2]);
	EXPECT_EQ(5, testDeque2[3]);
}

TEST_F(DequeTest, MoveConstructorsWork)
{
	sprawl::collections::Deque<int> testDeque2(std::move(testDeque));
	EXPECT_EQ(ssize_t(4), testDeque2.Size());
	EXPECT_EQ(1, testDeque2[0]);
	EXPECT_EQ(2, testDeque2[1]);
	EXPECT_EQ(3, testDeque2[2]);
	EXPECT_EQ(5, testDeque2[3]);

	EXPECT_EQ(ssize_t(0), testDeque.Size());
	testDeque.PushFront(20);
	testDeque.PushFront(10);
	testDeque.PushBack(30);
	testDeque.PushBack(50);

	//Make sure modifying one doesn't impact the other
	EXPECT_EQ(ssize_t(4), testDeque2.Size());
	EXPECT_EQ(1, testDeque2[0]);
	EXPECT_EQ(2, testDeque2[1]);
	EXPECT_EQ(3, testDeque2[2]);
	EXPECT_EQ(5, testDeque2[3]);

	testDeque2.PushFront(40);
	testDeque2.PushFront(90);
	testDeque2.PushBack(20);
	testDeque2.PushBack(220);

	EXPECT_EQ(ssize_t(4), testDeque.Size());
	EXPECT_EQ(10, testDeque[0]);
	EXPECT_EQ(20, testDeque[1]);
	EXPECT_EQ(30, testDeque[2]);
	EXPECT_EQ(50, testDeque[3]);
}

TEST_F(DequeTest, FrontWorks)
{
	ASSERT_EQ(1, testDeque.Front());
}

TEST_F(DequeTest, BackWorks)
{
	ASSERT_EQ(5, testDeque.Back());
}

TEST_F(DequeTest, IterationAndInsertWorks)
{
	for(auto it = testDeque.begin(); it != testDeque.end(); ++it)
	{
		if(it.Value() == 5)
		{
			testDeque.Insert(it, 4);
			break;
		}
	}

	int value = 0;

	for(auto it = testDeque.begin(); it != testDeque.end(); ++it)
	{
		EXPECT_EQ(value + 1, it.Value());
		value = it.Value();
	}
}

TEST_F(DequeTest, PopFrontWorks)
{
	testDeque.PopFront();
	ASSERT_EQ(2, testDeque.Front());
}

TEST_F(DequeTest, PopBackWorks)
{
	testDeque.PopBack();
	ASSERT_EQ(3, testDeque.Back());
}

TEST_F(DequeTest, EraseWorks)
{

	for(auto it = testDeque.begin(); it != testDeque.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testDeque.Erase(it);
			break;
		}
	}

	for(auto it = testDeque.begin(); it != testDeque.end(); ++it)
	{
		ASSERT_NE(3, it.Value());
	}
}

TEST_F(DequeTest, GrowWorks)
{
	sprawl::collections::Deque<int> deque2(sprawl::collections::Capacity(5));

	for(int i = 50; i < 75; ++i)
	{
		deque2.PushBack(i);
	}
	for(int i = 49; i > 0; --i)
	{
		deque2.PushFront(i);
	}
	for(int i = 75; i <= 100; ++i)
	{
		deque2.PushBack(i);
	}

	int value = 0;

	EXPECT_EQ(1, deque2.Front());

	for(auto& num : deque2)
	{
		EXPECT_EQ(value + 1, num);
		value = num;
	}

	ASSERT_EQ(100, deque2.Back());
}

TEST_F(DequeTest, NegativeIndexingWorks)
{
	ASSERT_EQ(testDeque[-1], testDeque.Back());
}
