
#include <map>
#include <unordered_map>
#define SPRAWL_STRINGBUILDER_FAVOR_SPEED_OVER_MEMORY 1
#include "../string/String.hpp"
#include "../collections/HashMap.hpp"

struct TestStruct{};

sprawl::StringBuilder& operator<<(sprawl::StringBuilder& builder, TestStruct const&)
{
	builder << "TestStruct";
	return builder;
}

bool test_string()
{
	bool success = true;

	TestStruct t;
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{0:03}, {1}, {2}, {3}, {4}")).format(30LL, true, "foo", t, &t);

	if(str != "030, True, foo, TestStruct, TestStruct")
	{
		printf("Failed format test ('%s' != '030, True, foo, TestStruct, TestStruct'')\n... ", str.c_str());
		success = false;
	}

	str = sprawl::String(sprawl::StringLiteral("{:03}, {}, {}, {}, {}")).format(30LL, true, "foo", t, &t);

	if (str != "030, True, foo, TestStruct, TestStruct")
	{
		printf("Failed format test ('%s' != '030, True, foo, TestStruct, TestStruct'')\n... ", str.c_str());
		success = false;
	}

	str = sprawl::String(sprawl::StringLiteral("{0:03}, {1}, {2}, {}, {}")).format(30LL, true, "foo", t, &t);

	if (str != "030, True, foo, TestStruct, TestStruct")
	{
		printf("Failed format test ('%s' != '030, True, foo, TestStruct, TestStruct'')\n... ", str.c_str());
		success = false;
	}

	str = sprawl::String(sprawl::StringLiteral("{4}, {3}, {2}, {1}, {0:03}")).format(30LL, true, "foo", t, &t);

	if (str != "TestStruct, TestStruct, foo, True, 030")
	{
		printf("Failed format test ('%s' != 'TestStruct, TestStruct, foo, True, 030'')\n... ", str.c_str());
		success = false;
	}

	return success;
}
