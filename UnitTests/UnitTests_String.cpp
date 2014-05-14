
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

#ifndef _WIN32
	TestStruct t;
	sprawl::String str = sprawl::String(sprawl::StringLiteral("{0:03}, {1}, {2}, {3}, {4}")).format(30LL, true, "foo", t, &t);

	if(str != "030, True, foo, TestStruct, TestStruct")
	{
		printf("Failed format test ('%s' != '003, True, foo, TestStruct, TestStruct'')... ", str.c_str());
		success = false;
	}
#endif
	return success;
}
