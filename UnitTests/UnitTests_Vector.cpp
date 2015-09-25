#include "../collections/Vector.hpp"
#include <stdio.h>

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

class VectorTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		testVector.PushBack(1);
		testVector.PushBack(2);
		testVector.PushBack(3);
		testVector.PushBack(5);
	}

	sprawl::collections::Vector<int> testVector;
};

TEST_F(VectorTest, BasicSetupWorks)
{
	EXPECT_EQ(ssize_t(4), testVector.Size());
	EXPECT_EQ(1, testVector[0]);
	EXPECT_EQ(2, testVector[1]);
	EXPECT_EQ(3, testVector[2]);
	EXPECT_EQ(5, testVector[3]);
}

TEST_F(VectorTest, FillConstructorsWork)
{
	sprawl::collections::Vector<int> testVector2(sprawl::collections::Fill(5));

	EXPECT_EQ(ssize_t(5), testVector2.Size());

	for(auto& value : testVector2)
	{
		EXPECT_EQ(0, value);
	}

	sprawl::collections::Vector<int> testVector3(sprawl::collections::Fill(5), 3);

	EXPECT_EQ(ssize_t(5), testVector3.Size());

	for(auto& value : testVector3)
	{
		EXPECT_EQ(3, value);
	}
}

TEST_F(VectorTest, CopyConstructorsWork)
{
	sprawl::collections::Vector<int> testVector2(testVector);
	EXPECT_EQ(ssize_t(4), testVector2.Size());
	EXPECT_EQ(1, testVector2[0]);
	EXPECT_EQ(2, testVector2[1]);
	EXPECT_EQ(3, testVector2[2]);
	EXPECT_EQ(5, testVector2[3]);
}

TEST_F(VectorTest, MoveConstructorsWork)
{
	sprawl::collections::Vector<int> testVector2(std::move(testVector));
	EXPECT_EQ(ssize_t(4), testVector2.Size());
	EXPECT_EQ(1, testVector2[0]);
	EXPECT_EQ(2, testVector2[1]);
	EXPECT_EQ(3, testVector2[2]);
	EXPECT_EQ(5, testVector2[3]);

	EXPECT_EQ(ssize_t(0), testVector.Size());
	testVector.PushBack(10);
	testVector.PushBack(20);
	testVector.PushBack(30);
	testVector.PushBack(50);

	//Make sure modifying one doesn't impact the other
	EXPECT_EQ(ssize_t(4), testVector2.Size());
	EXPECT_EQ(1, testVector2[0]);
	EXPECT_EQ(2, testVector2[1]);
	EXPECT_EQ(3, testVector2[2]);
	EXPECT_EQ(5, testVector2[3]);

	testVector2.PushBack(40);
	testVector2.PushBack(90);
	testVector2.PushBack(20);
	testVector2.PushBack(220);

	EXPECT_EQ(ssize_t(4), testVector.Size());
	EXPECT_EQ(10, testVector[0]);
	EXPECT_EQ(20, testVector[1]);
	EXPECT_EQ(30, testVector[2]);
	EXPECT_EQ(50, testVector[3]);
}

TEST_F(VectorTest, SwapWorks)
{
	sprawl::collections::Vector<int> testVector2;

	testVector2.PushBack(40);
	testVector2.PushBack(90);
	testVector2.PushBack(20);
	testVector2.PushBack(220);
	testVector2.PushBack(400);

	testVector2.Swap(testVector);

	EXPECT_EQ(ssize_t(5), testVector.Size());
	EXPECT_EQ(40, testVector[0]);
	EXPECT_EQ(90, testVector[1]);
	EXPECT_EQ(20, testVector[2]);
	EXPECT_EQ(220, testVector[3]);
	EXPECT_EQ(400, testVector[4]);

	EXPECT_EQ(ssize_t(4), testVector2.Size());
	EXPECT_EQ(1, testVector2[0]);
	EXPECT_EQ(2, testVector2[1]);
	EXPECT_EQ(3, testVector2[2]);
	EXPECT_EQ(5, testVector2[3]);
}

TEST_F(VectorTest, FrontWorks)
{
	ASSERT_EQ(1, testVector.Front());
}

TEST_F(VectorTest, BackWorks)
{
	ASSERT_EQ(5, testVector.Back());
}

TEST_F(VectorTest, IterationAndInsertWorks)
{
	for(auto it = testVector.begin(); it != testVector.end(); ++it)
	{
		if(it.Value() == 5)
		{
			testVector.Insert(it, 4);
			break;
		}
	}

	int value = 0;

	for(auto it = testVector.begin(); it != testVector.end(); ++it)
	{
		EXPECT_EQ(value + 1, it.Value());
		value = it.Value();
	}
}

TEST_F(VectorTest, PopBackWorks)
{
	testVector.PopBack();
	ASSERT_EQ(3, testVector.Back());
}

TEST_F(VectorTest, EraseWorks)
{
	for(auto it = testVector.begin(); it != testVector.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testVector.Erase(it);
			break;
		}
	}

	for(auto it = testVector.begin(); it != testVector.end(); ++it)
	{
		ASSERT_NE(3, it.Value());
	}
}

TEST_F(VectorTest, GrowWorks)
{
	sprawl::collections::Vector<int> vector2(sprawl::collections::Capacity(5));

	for(int i = 1; i <= 100; ++i)
	{
		vector2.PushBack(i);
	}

	EXPECT_EQ(1, vector2.Front());

	int value = 0;
	for(auto& num : vector2)
	{
		EXPECT_EQ(value + 1, num);
		value = num;
	}

	ASSERT_EQ(100, vector2.Back());
}

TEST_F(VectorTest, NegativeIndexingWorks)
{
	ASSERT_EQ(testVector[-1], testVector.Back());
}
