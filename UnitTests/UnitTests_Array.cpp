#include "../collections/Array.hpp"
#include <stdio.h>

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

class ArrayTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		testArray[0] = 1;
		testArray[1] = 2;
		testArray[2] = 3;
		testArray[3] = 5;
	}

	sprawl::collections::Array<int, 5> testArray;
};

TEST_F(ArrayTest, BasicSetupWorks)
{
	EXPECT_EQ(ssize_t(5), testArray.Size());
	EXPECT_EQ(1, testArray[0]);
	EXPECT_EQ(2, testArray[1]);
	EXPECT_EQ(3, testArray[2]);
	EXPECT_EQ(5, testArray[3]);
}

TEST_F(ArrayTest, FillWorks)
{
	testArray.Fill(5);

	for(auto& value : testArray)
	{
		EXPECT_EQ(5, value);
	}
}

TEST_F(ArrayTest, CopyConstructorsWork)
{
	sprawl::collections::Array<int, 5> testArray2(testArray);
	EXPECT_EQ(ssize_t(5), testArray2.Size());
	EXPECT_EQ(1, testArray2[0]);
	EXPECT_EQ(2, testArray2[1]);
	EXPECT_EQ(3, testArray2[2]);
	EXPECT_EQ(5, testArray2[3]);

	EXPECT_EQ(1, testArray[0]);
	EXPECT_EQ(2, testArray[1]);
	EXPECT_EQ(3, testArray[2]);
	EXPECT_EQ(5, testArray[3]);
}

TEST_F(ArrayTest, MoveConstructorsWork)
{
	sprawl::collections::Array<int, 5> testArray2(std::move(testArray));
	EXPECT_EQ(ssize_t(5), testArray2.Size());
	EXPECT_EQ(1, testArray2[0]);
	EXPECT_EQ(2, testArray2[1]);
	EXPECT_EQ(3, testArray2[2]);
	EXPECT_EQ(5, testArray2[3]);

	EXPECT_EQ(1, testArray[0]);
	EXPECT_EQ(2, testArray[1]);
	EXPECT_EQ(3, testArray[2]);
	EXPECT_EQ(5, testArray[3]);
}

TEST_F(ArrayTest, FrontWorks)
{
	ASSERT_EQ(1, testArray.Front());
}

TEST_F(ArrayTest, BackWorks)
{
	testArray[4] = 6;
	ASSERT_EQ(6, testArray.Back());
}

TEST_F(ArrayTest, IterationWorks)
{
	testArray[3] = 4;
	testArray[4] = 5;

	int value = 0;

	for(auto it = testArray.begin(); it != testArray.end(); ++it)
	{
		EXPECT_EQ(value + 1, it.Value());
		value = it.Value();
	}
}

TEST_F(ArrayTest, NegativeIndexingWorks)
{
	ASSERT_EQ(testArray[-1], testArray.Back());
}

