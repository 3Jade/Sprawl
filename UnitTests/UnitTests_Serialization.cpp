#include <unordered_map>
#include <vector>
#include "../string/String.hpp"
#include "../serialization/JSONSerializer.hpp"

namespace sprawl
{
	void throw_exception(std::exception const& e)
	{
		//
	}
}

namespace JsonTests
{
	struct TestType
	{
		std::unordered_map<sprawl::String, double> map;
		std::vector<int> intArray;
		std::vector<sprawl::String> strArray;
		std::vector<std::vector<int>> nestedArray;
		sprawl::String str;
		int i;
		double d;
		bool b;
		char c;
		unsigned char u;

		void Serialize(sprawl::serialization::SerializerBase& ser)
		{
			ser
				% NAME_PROPERTY(map)
				% NAME_PROPERTY(intArray)
				% NAME_PROPERTY(strArray)
				% NAME_PROPERTY(nestedArray)
				% NAME_PROPERTY(str)
				% NAME_PROPERTY(i)
				% NAME_PROPERTY(d)
				% NAME_PROPERTY(b)
				% NAME_PROPERTY(c)
				% NAME_PROPERTY(u)
			;
		}

		bool operator==(TestType const& other)
		{
			return map == other.map
					&& intArray == other.intArray
					&& strArray == other.strArray
					&& nestedArray == other.nestedArray
					&& str == other.str
					&& i == other.i
					&& d == other.d
					&& b == other.b
					&& c == other.c
					&& u == other.u;
		}

		bool operator!=(TestType const& other)
		{
			return !operator==(other);
		}
	};
}

using namespace JsonTests;

sprawl::String expectedValue1 = "{ \"__version__\" : 0, \"t\" : { \"map\" : { \"test3\" : 1000000000, \"test2\" : 1e+22, \"test\" : 1.0000000000000000715e-18 }, \"intArray\" : [ 1, -2, 3, -4, 5 ], \"strArray\" : [ \"hi\", \"there\", \"y'all\" ], \"nestedArray\" : [ [ 6, 7, 8, 9, 10 ] ], \"str\" : \"\\\\escapes\\\\ \\\" \\n\", \"i\" : 3, \"d\" : 2.5, \"b\" : true, \"c\" : \"f\", \"u\" : 32 } }";

sprawl::String expectedValue2 = "{\n\t\"__version__\" : 0,\n\t\"map\" : {\n\t\t\"test3\" : 1000000000,\n\t\t\"test2\" : 1e+22,\n\t\t\"test\" : 1.0000000000000000715e-18\n\t},\n\t\"intArray\" : [\n\t\t1,\n\t\t-2,\n\t\t3,\n\t\t-4,\n\t\t5\n\t],\n\t\"strArray\" : [\n\t\t\"hi\",\n\t\t\"there\",\n\t\t\"y'all\"\n\t],\n\t\"nestedArray\" : [\n\t\t[\n\t\t\t6,\n\t\t\t7,\n\t\t\t8,\n\t\t\t9,\n\t\t\t10\n\t\t]\n\t],\n\t\"str\" : \"\\\\escapes\\\\ \\\" \\n\",\n\t\"i\" : 3,\n\t\"d\" : 2.5,\n\t\"b\" : true,\n\t\"c\" : \"f\",\n\t\"u\" : 32\n}";

sprawl::String expectedTokValue = "{ \"test1\" : 1, \"test2\" : -2, \"object\" : { \"test1\" : 1, \"test2\" : -2, \"Tok\" : null }, \"array\" : [ -1, 2 ] }";
sprawl::String expectedTok2Value = "{ \"test1\" : -1, \"test2\" : 2, \"object\" : { \"test1\" : -1, \"test2\" : 2, \"Tok2\" : null }, \"array\" : [ 1, -2 ] }";

bool test_json()
{
	bool success = true;

	//Test copy-on-write for json tokens

	sprawl::serialization::JSONToken tok = sprawl::serialization::JSONToken::object();
	tok.Insert("test1", 1);
	tok.Insert("test2", 2);
	tok["object"] = sprawl::serialization::JSONToken::object();
	tok["object"].Insert("test1", 1);
	tok["object"].Insert("test2", 2);
	tok["array"] = sprawl::serialization::JSONToken::array();
	tok["array"].PushBack(1);
	tok["array"].PushBack(2);

	sprawl::serialization::JSONToken tok2 = tok;
	tok2["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tok["test2"] = sprawl::serialization::JSONToken::number((long long)-2);
	tok2["object"]["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tok["object"]["test2"] = sprawl::serialization::JSONToken::number((long long)-2);
	tok["object"].Insert("Tok", sprawl::serialization::JSONToken::null());
	tok2["object"].Insert("Tok2", sprawl::serialization::JSONToken::null());
	tok["array"][0] = sprawl::serialization::JSONToken::number((long long)-1);
	tok2["array"][1] = sprawl::serialization::JSONToken::number((long long)-2);


	if(tok.ToJSONString() != expectedTokValue || tok2.ToJSONString() != expectedTok2Value)
	{
		success = false;
		printf("\n\nFailed copy-on-write json token building...\n\n");
	}

	// Test serialization...

	TestType t;
	t.str = "\\escapes\\ \" \n";
	t.i = 3;
	t.d = 2.5;
	t.b = true;
	t.c = 'f';
	t.u = 32;

	t.map["test"] = 0.000000000000000001;
	t.map["test2"] = 10000000000000000000000.1;
	t.map["test3"] = 1000000000.00000001;

	t.intArray.push_back(1);
	t.intArray.push_back(-2);
	t.intArray.push_back(3);
	t.intArray.push_back(-4);
	t.intArray.push_back(5);

	t.strArray.push_back("hi");
	t.strArray.push_back("there");
	t.strArray.push_back("y'all");

	std::vector<int> nested;
	nested.push_back(6);
	nested.push_back(7);
	nested.push_back(8);
	nested.push_back(9);
	nested.push_back(10);
	t.nestedArray.push_back(nested);

	sprawl::serialization::JSONSerializer s;

	s % NAME_PROPERTY(t);

	sprawl::String str1 = s.Str();

	if(str1 != expectedValue1)
	{
		success = false;
		printf("\n\nFailed plain print...\n\n");
		puts(str1.c_str());
		puts(expectedValue1.c_str());
	}

	sprawl::serialization::JSONSerializer s2;
	s2.SetPrettyPrint(true);

	t.Serialize(s2);

	sprawl::String str2 = s2.Str();

	if(str2 != expectedValue2)
	{
		success = false;
		printf("\n\nFailed pretty print...\n\n");
		puts(str2.c_str());
		puts(expectedValue2.c_str());
	}

	// Test deserialization...

	sprawl::serialization::JSONDeserializer d(str1);
	sprawl::serialization::JSONDeserializer d2(str2);

	TestType t1;
	TestType t2;

	d % sprawl::serialization::prepare_data(t1, "t");
	t2.Serialize(d2);

	if(t1 != t)
	{
		success = false;
		printf("\n\nFailed deserialization...\n\n");
	}

	if(t2 != t)
	{
		success = false;
		printf("\n\nFailed pretty deserialization...\n\n");
	}

	return success;
}