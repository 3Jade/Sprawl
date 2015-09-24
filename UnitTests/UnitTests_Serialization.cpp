#include <unordered_map>
#include <vector>
#include "../string/String.hpp"
#include "../serialization/JSONSerializer.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

namespace sprawl
{
	void throw_exception(std::exception const&)
	{
		//
	}
}

namespace JsonTests
{
	struct TestType
	{
		std::map<sprawl::String, double> map;
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

		bool operator==(TestType const& other) const
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

class JsonTokenTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
		tok = sprawl::serialization::JSONToken::object();
		tok.Insert("test1", 1);
		tok.Insert("test2", 2);
		tok["object"] = sprawl::serialization::JSONToken::object();
		tok["object"].Insert("test1", 1);
		tok["object"].Insert("test2", 2);
		tok["array"] = sprawl::serialization::JSONToken::array();
		tok["array"].PushBack(1);
		tok["array"].PushBack(2);
	}

	sprawl::serialization::JSONToken tok;
};

TEST_F(JsonTokenTest, JsonTokensWork)
{
	sprawl::String expectedTokValue = "{ \"test1\" : 1, \"test2\" : 2, \"object\" : { \"test1\" : 1, \"test2\" : 2 }, \"array\" : [ 1, 2 ] }";
	ASSERT_EQ(expectedTokValue, tok.ToJSONString());
}

TEST_F(JsonTokenTest, CopyOnWriteWorks)
{
	sprawl::String expectedTokValue = "{ \"test1\" : 1, \"test2\" : -2, \"object\" : { \"test1\" : 1, \"test2\" : -2, \"Tok\" : null }, \"array\" : [ -1, 2 ] }";
	sprawl::String expectedTok2Value = "{ \"test1\" : -1, \"test2\" : 2, \"object\" : { \"test1\" : -1, \"test2\" : 2, \"Tok2\" : null }, \"array\" : [ 1, -2 ] }";

	sprawl::serialization::JSONToken tok2 = tok;
	tok2["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tok["test2"] = sprawl::serialization::JSONToken::number((long long)-2);
	tok2["object"]["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tok["object"]["test2"] = sprawl::serialization::JSONToken::number((long long)-2);
	tok["object"].Insert("Tok", sprawl::serialization::JSONToken::null());
	tok2["object"].Insert("Tok2", sprawl::serialization::JSONToken::null());
	tok["array"][0] = sprawl::serialization::JSONToken::number((long long)-1);
	tok2["array"][1] = sprawl::serialization::JSONToken::number((long long)-2);

	ASSERT_EQ(expectedTokValue, tok.ToJSONString());
	ASSERT_EQ(expectedTok2Value, tok2.ToJSONString());
}

class JsonSerializerTest : public testing::Test
{
protected:
	virtual void SetUp() override
	{
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
	}

	TestType t;
};


TEST_F(JsonSerializerTest, SerializationWorks)
{
#ifdef _WIN32
	sprawl::String expectedValue1 = "{ \"__version__\" : 0, \"t\" : { \"map\" : { \"test\" : 1.0000000000000001e-018, \"test2\" : 1e+022, \"test3\" : 1000000000 }, \"intArray\" : [ 1, -2, 3, -4, 5 ], \"strArray\" : [ \"hi\", \"there\", \"y'all\" ], \"nestedArray\" : [ [ 6, 7, 8, 9, 10 ] ], \"str\" : \"\\\\escapes\\\\ \\\" \\n\", \"i\" : 3, \"d\" : 2.5, \"b\" : true, \"c\" : \"f\", \"u\" : 32 } }";
#else
	sprawl::String expectedValue1 = "{ \"__version__\" : 0, \"t\" : { \"map\" : { \"test\" : 1.0000000000000000715e-18, \"test2\" : 1e+22, \"test3\" : 1000000000 }, \"intArray\" : [ 1, -2, 3, -4, 5 ], \"strArray\" : [ \"hi\", \"there\", \"y'all\" ], \"nestedArray\" : [ [ 6, 7, 8, 9, 10 ] ], \"str\" : \"\\\\escapes\\\\ \\\" \\n\", \"i\" : 3, \"d\" : 2.5, \"b\" : true, \"c\" : \"f\", \"u\" : 32 } }";
#endif

	sprawl::serialization::JSONSerializer s;

	s % NAME_PROPERTY(t);

	sprawl::String str1 = s.Str();

	ASSERT_EQ(expectedValue1, str1);
}

TEST_F(JsonSerializerTest, PrettySerializationWorks)
{
#ifdef _WIN32
	sprawl::String expectedValue2 = "{\n\t\"__version__\" : 0,\n\t\"map\" : {\n\t\t\"test\" : 1.0000000000000001e-018,\n\t\t\"test2\" : 1e+022,\n\t\t\"test3\" : 1000000000\n\t},\n\t\"intArray\" : [\n\t\t1,\n\t\t-2,\n\t\t3,\n\t\t-4,\n\t\t5\n\t],\n\t\"strArray\" : [\n\t\t\"hi\",\n\t\t\"there\",\n\t\t\"y'all\"\n\t],\n\t\"nestedArray\" : [\n\t\t[\n\t\t\t6,\n\t\t\t7,\n\t\t\t8,\n\t\t\t9,\n\t\t\t10\n\t\t]\n\t],\n\t\"str\" : \"\\\\escapes\\\\ \\\" \\n\",\n\t\"i\" : 3,\n\t\"d\" : 2.5,\n\t\"b\" : true,\n\t\"c\" : \"f\",\n\t\"u\" : 32\n}";
#else
	sprawl::String expectedValue2 = "{\n\t\"__version__\" : 0,\n\t\"map\" : {\n\t\t\"test\" : 1.0000000000000000715e-18,\n\t\t\"test2\" : 1e+22,\n\t\t\"test3\" : 1000000000\n\t},\n\t\"intArray\" : [\n\t\t1,\n\t\t-2,\n\t\t3,\n\t\t-4,\n\t\t5\n\t],\n\t\"strArray\" : [\n\t\t\"hi\",\n\t\t\"there\",\n\t\t\"y'all\"\n\t],\n\t\"nestedArray\" : [\n\t\t[\n\t\t\t6,\n\t\t\t7,\n\t\t\t8,\n\t\t\t9,\n\t\t\t10\n\t\t]\n\t],\n\t\"str\" : \"\\\\escapes\\\\ \\\" \\n\",\n\t\"i\" : 3,\n\t\"d\" : 2.5,\n\t\"b\" : true,\n\t\"c\" : \"f\",\n\t\"u\" : 32\n}";
#endif

	sprawl::serialization::JSONSerializer s2;
	s2.SetPrettyPrint(true);

	t.Serialize(s2);

	sprawl::String str2 = s2.Str();

	ASSERT_EQ(expectedValue2, str2);
}

TEST_F(JsonSerializerTest, DeserializationWorks)
{
	sprawl::serialization::JSONSerializer s;

	s % NAME_PROPERTY(t);

	sprawl::String str1 = s.Str();

	sprawl::serialization::JSONDeserializer d(str1);

	TestType t1;

	d % sprawl::serialization::prepare_data(t1, "t");

	ASSERT_EQ(t, t1);
}

TEST_F(JsonSerializerTest, PrettyDeserializationWorks)
{
	sprawl::serialization::JSONSerializer s2;
	s2.SetPrettyPrint(true);

	t.Serialize(s2);

	sprawl::String str2 = s2.Str();

	sprawl::serialization::JSONDeserializer d2(str2);

	TestType t2;

	t2.Serialize(d2);

	ASSERT_EQ(t, t2);
}
