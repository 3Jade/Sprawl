//#define SPRAWL_NO_VARIADIC_TEMPLATES

#include "../collections/HashMap.hpp"
#include "../string/String.hpp"

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

#include <memory>

bool test_collections()
{
	bool success = true;

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

	map.insert(std::shared_ptr<TestType>(new TestType(1, 5, "str1")), 1);
	map.insert(std::shared_ptr<TestType>(new TestType(2, 6, "str2")), 2);
	map.insert(std::shared_ptr<TestType>(new TestType(3, 7, "str3")), 3);
	map.insert(std::shared_ptr<TestType>(new TestType(3, 8, "str4")), 4);

	map2.insert(std::shared_ptr<TestType>(new TestType(1, 5, "str1")), 1);
	map2.insert(std::shared_ptr<TestType>(new TestType(2, 6, "str2")), 2);
	map2.insert(std::shared_ptr<TestType>(new TestType(3, 7, "str3")), 3);
	map2.insert(std::shared_ptr<TestType>(new TestType(3, 8, "str4")), 4);

	map3.insert(std::shared_ptr<TestType>(new TestType(1, 5, "str1")), 1);
	map3.insert(std::shared_ptr<TestType>(new TestType(2, 6, "str2")), 2);
	map3.insert(std::shared_ptr<TestType>(new TestType(3, 7, "str3")), 3);
	map3.insert(std::shared_ptr<TestType>(new TestType(3, 8, "str4")), 4);

	map4.insert(TestType(1, 5, "str1"), 1);
	map4.insert(TestType(2, 6, "str2"), 2);
	map4.insert(TestType(3, 7, "str3"), 3);
	map4.insert(TestType(3, 8, "str4"), 4);

	map5.insert(TestType(1, 5, "str1"), 1);
	map5.insert(TestType(2, 6, "str2"), 2);
	map5.insert(TestType(3, 7, "str3"), 3);
	map5.insert(TestType(3, 8, "str4"), 4);

	map6.insert(TestType(1, 5, "str1"), 1);
	map6.insert(TestType(2, 6, "str2"), 2);
	map6.insert(TestType(3, 7, "str3"), 3);
	map6.insert(TestType(3, 8, "str4"), 4);

	int testNum = 1;

#define RUN_LOOKUP(map, keyIndex, key) \
	if(!map.has<keyIndex>(key)) \
	{ \
		printf("Failed map lookup %d (" #map ".has<" #keyIndex ">(" #key ")\n... ", testNum); \
		success = false; \
	} \
	++testNum

#define RUN_LOOKUP_AUTO(map, key) \
	if(!map.has(key)) \
	{ \
		printf("Failed map lookup %d (" #map ".has(" #key ")\n... ", testNum); \
		success = false; \
	} \
	++testNum

	RUN_LOOKUP(map, 1, 1);
	RUN_LOOKUP(map, 1, 2);
	RUN_LOOKUP(map, 1, 3);
	RUN_LOOKUP(map, 1, 4);
	RUN_LOOKUP(map, 2, 5);
	RUN_LOOKUP(map, 2, 6);
	RUN_LOOKUP(map, 2, 7);
	RUN_LOOKUP(map, 2, 8);
	RUN_LOOKUP_AUTO(map, "str1");
	RUN_LOOKUP_AUTO(map, "str2");
	RUN_LOOKUP_AUTO(map, "str3");
	RUN_LOOKUP_AUTO(map, "str4");

	RUN_LOOKUP(map2, 1, 1);
	RUN_LOOKUP(map2, 1, 2);
	RUN_LOOKUP(map2, 1, 3);
	RUN_LOOKUP(map2, 1, 4);
	RUN_LOOKUP(map2, 2, 5);
	RUN_LOOKUP(map2, 2, 6);
	RUN_LOOKUP(map2, 2, 7);
	RUN_LOOKUP(map2, 2, 8);

	RUN_LOOKUP_AUTO(map3, 1);
	RUN_LOOKUP_AUTO(map3, 2);
	RUN_LOOKUP_AUTO(map3, 3);
	RUN_LOOKUP_AUTO(map3, 4);

	RUN_LOOKUP(map4, 1, 1);
	RUN_LOOKUP(map4, 1, 2);
	RUN_LOOKUP(map4, 1, 3);
	RUN_LOOKUP(map4, 1, 4);
	RUN_LOOKUP(map4, 2, 5);
	RUN_LOOKUP(map4, 2, 6);
	RUN_LOOKUP(map4, 2, 7);
	RUN_LOOKUP(map4, 2, 8);
	RUN_LOOKUP_AUTO(map4, "str1");
	RUN_LOOKUP_AUTO(map4, "str2");
	RUN_LOOKUP_AUTO(map4, "str3");
	RUN_LOOKUP_AUTO(map4, "str4");

	RUN_LOOKUP(map5, 1, 1);
	RUN_LOOKUP(map5, 1, 2);
	RUN_LOOKUP(map5, 1, 3);
	RUN_LOOKUP(map5, 1, 4);
	RUN_LOOKUP(map5, 2, 5);
	RUN_LOOKUP(map5, 2, 6);
	RUN_LOOKUP(map5, 2, 7);
	RUN_LOOKUP(map5, 2, 8);

	RUN_LOOKUP_AUTO(map6, 1);
	RUN_LOOKUP_AUTO(map6, 2);
	RUN_LOOKUP_AUTO(map6, 3);
	RUN_LOOKUP_AUTO(map6, 4);

	map.erase<1>(4);
	map2.erase<1>(4);
	map3.erase(4);
	map4.erase<1>(4);
	map5.erase<1>(4);
	map6.erase(4);


#define RUN_FIND(map, keyIndex, key) \
	if(!map.find<keyIndex>(key) != map.end()) \
	{ \
		printf("Failed map erase %d (" #map ".find<" #keyIndex ">(" #key ")\n... ", testNum); \
		success = false; \
	} \
	++testNum

	RUN_FIND(map, 1, 4);
	RUN_FIND(map, 2, 8);
	RUN_FIND(map, 3, "str4");

	RUN_FIND(map2, 1, 4);
	RUN_FIND(map2, 2, 8);

	RUN_FIND(map3, 1, 4);

	RUN_FIND(map4, 1, 4);
	RUN_FIND(map4, 2, 8);
	RUN_FIND(map4, 3, "str4");

	RUN_FIND(map5, 1, 4);
	RUN_FIND(map5, 2, 8);

	RUN_FIND(map6, 1, 4);

#define RUN_GET(map, keyIndex, key, expectedResult) \
	if(map.get<keyIndex>(key)->key1 != expectedResult ) \
	{ \
		printf("Failed map get %d (" #map ".get<" #keyIndex ">(" #key ")\n... ", testNum); \
		success = false; \
	} \
	++testNum

#define RUN_GET_AUTO(map, key, expectedResult) \
	if(map.get(key)->key1 != expectedResult ) \
	{ \
		printf("Failed map get %d (" #map ".get(" #key ")\n... ", testNum); \
		success = false; \
	} \
	++testNum

	RUN_GET(map, 1, 3, 3);
	RUN_GET(map, 2, 5, 1);
	RUN_GET_AUTO(map, "str1", 1);

	RUN_GET(map2, 1, 3, 3);
	RUN_GET(map2, 2, 5, 1);

	RUN_GET(map3, 1, 3, 3);

	RUN_GET(map4, 1, 3, 3);
	RUN_GET(map4, 2, 5, 1);
	RUN_GET_AUTO(map4, "str1", 1);

	RUN_GET(map5, 1, 3, 3);
	RUN_GET(map5, 2, 5, 1);

	RUN_GET(map6, 1, 3, 3);

	return success;
}
