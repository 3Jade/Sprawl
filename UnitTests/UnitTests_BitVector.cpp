#include "../collections/BitVector.hpp"
#include <stdio.h>

#include "gtest_helpers.hpp"
#include <gtest/gtest.h>

class BitSetTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		bs.Set(0);
		bs.Set(2);
		bs.Set(4);
		bs.Set(6);
		bs.Set(8);
		bs.Set(10);
		bs.Set(12);
		bs.Set(14);
	}
	sprawl::collections::BitSet<16> bs;
};

TEST_F(BitSetTest, SetWorks)
{
	EXPECT_TRUE(bs.HasBit(0));
	EXPECT_TRUE(bs.HasBit(2));
	EXPECT_TRUE(bs.HasBit(4));
	EXPECT_TRUE(bs.HasBit(6));
	EXPECT_TRUE(bs.HasBit(8));
	EXPECT_TRUE(bs.HasBit(10));
	EXPECT_TRUE(bs.HasBit(12));
	EXPECT_TRUE(bs.HasBit(14));
	EXPECT_EQ(size_t(16), bs.Size());
}

TEST_F(BitSetTest, CountWorks)
{
	ASSERT_EQ(size_t(8), bs.Count());
}

TEST_F(BitSetTest, FlipWorks)
{
	bs.Flip(5);

	ASSERT_TRUE(bs.HasBit(5));

	bs.Flip(5);

	ASSERT_FALSE(bs.HasBit(5));
}

TEST_F(BitSetTest, UnsetWorks)
{
	bs.Unset(2);

	ASSERT_FALSE(bs.HasBit(2));
}

TEST_F(BitSetTest, ToStringWorks)
{

	sprawl::String str = bs.ToString();
	ASSERT_EQ(sprawl::String("0101010101010101"), str);
}

TEST_F(BitSetTest, NoneAllAnyWork)
{
	EXPECT_FALSE(bs.None());
	EXPECT_FALSE(bs.All());
	EXPECT_TRUE(bs.Any());

	bs.Reset();

	EXPECT_TRUE(bs.None());
	EXPECT_FALSE(bs.All());
	EXPECT_FALSE(bs.Any());

	for(int i = 0; i < 16; ++i)
	{
		bs.Set(i);
	}

	EXPECT_FALSE(bs.None());
	EXPECT_TRUE(bs.All());
	EXPECT_TRUE(bs.Any());
}

class BitVectorTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		bv.Set(0);
		bv.Set(2);
		bv.Set(4);
		bv.Set(6);
		bv.Set(8);
		bv.Set(10);
		bv.Set(12);
		bv.Set(14);
	}
	sprawl::collections::BitVector bv;
};

TEST_F(BitVectorTest, SetWorks)
{
	EXPECT_TRUE(bv.HasBit(0));
	EXPECT_TRUE(bv.HasBit(2));
	EXPECT_TRUE(bv.HasBit(4));
	EXPECT_TRUE(bv.HasBit(6));
	EXPECT_TRUE(bv.HasBit(8));
	EXPECT_TRUE(bv.HasBit(10));
	EXPECT_TRUE(bv.HasBit(12));
	EXPECT_TRUE(bv.HasBit(14));
	EXPECT_EQ(size_t(15), bv.Size());
}

TEST_F(BitVectorTest, CountWorks)
{
	ASSERT_EQ(size_t(8), bv.Count());
}

TEST_F(BitVectorTest, FlipWorks)
{
	bv.Flip(5);

	ASSERT_TRUE(bv.HasBit(5));

	bv.Flip(5);

	ASSERT_FALSE(bv.HasBit(5));
}

TEST_F(BitVectorTest, UnsetWorks)
{
	bv.Unset(2);

	ASSERT_FALSE(bv.HasBit(2));
}

TEST_F(BitVectorTest, ToStringWorks)
{

	sprawl::String str = bv.ToString();
	ASSERT_EQ(sprawl::String("101010101010101"), str);
}

TEST_F(BitVectorTest, NoneAllAnyWork)
{
	EXPECT_FALSE(bv.None());
	EXPECT_FALSE(bv.All());
	EXPECT_TRUE(bv.Any());

	bv.Reset();

	EXPECT_TRUE(bv.None());
	EXPECT_FALSE(bv.All());
	EXPECT_FALSE(bv.Any());

	for(int i = 0; i < 16; ++i)
	{
		bv.Set(i);
	}

	EXPECT_FALSE(bv.None());
	EXPECT_TRUE(bv.All());
	EXPECT_TRUE(bv.Any());
}

TEST_F(BitVectorTest, GrowWorks)
{
	bv.Set(12768);

	for(int i = 0; i < 16; ++i)
	{
		if(i % 2 == 0)
		{
			EXPECT_TRUE(bv.HasBit(i));
		}
		else
		{
			EXPECT_FALSE(bv.HasBit(i));
		}
	}
	for(int i = 16; i < 12768; ++i)
	{
		EXPECT_FALSE(bv.HasBit(i));
	}

	EXPECT_TRUE(bv.HasBit(12768));
}
