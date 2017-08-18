
#include <map>
#include <unordered_map>
#define SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY 1
#include "../string/String.hpp"
#include "../collections/HashMap.hpp"

#include "gtest_helpers.hpp"
#include <gtest/gtest.h>

struct TestStruct{};

sprawl::StringBuilder& operator<<(sprawl::StringBuilder& builder, TestStruct const&)
{
	builder << "TestStruct";
	return builder;
}

TestStruct t;

TEST(StringTest, FormatWorks_AllNumbers)
{
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{0:03}, {1}, {2}, {3}, {4}")).format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("030, True, foo, TestStruct, TestStruct"), str);
}

#ifndef _WIN32
TEST(StringTest, FormatLiteralWorks)
{
	sprawl::String str = "{0:03}, {1}, {2}, {3}, {4}"_format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("030, True, foo, TestStruct, TestStruct"), str);
}
#endif

TEST(StringTest, FormatWorks_AllEmpty)
{
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{:03}, {}, {}, {}, {}")).format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("030, True, foo, TestStruct, TestStruct"), str);
}

TEST(StringTest, FormatWorks_Mixed)
{
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{0:03}, {1}, {2}, {}, {}")).format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("030, True, foo, TestStruct, TestStruct"), str);
}

TEST(StringTest, FormatWorks_BackwardNumbers)
{
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{4}, {3}, {2}, {1}, {0:03}")).format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("TestStruct, TestStruct, foo, True, 030"), str);
}

TEST(StringTest, FormatWorks_RepeatNumbers)
{
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{4}, {1}, {3}, {2}, {1}, {4}, {0:03}")).format(30LL, true, "foo", t, &t);

	EXPECT_EQ(sprawl::String("TestStruct, True, TestStruct, foo, True, TestStruct, 030"), str);
}
