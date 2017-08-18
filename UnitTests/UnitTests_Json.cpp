#include <unordered_map>
#include <vector>
#include "../string/String.hpp"
#include "../serialization/JSONSerializer.hpp"

#include "gtest_helpers.hpp"
#include <gtest/gtest.h>

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
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(map));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(intArray));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(strArray));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(nestedArray));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(str));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(i));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(d));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(b));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(c));
			ASSERT_NO_SPRAWL_EXCEPT(ser % NAME_PROPERTY(u));
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
		ASSERT_NO_SPRAWL_EXCEPT(tok.Insert("test1", 1));
		ASSERT_NO_SPRAWL_EXCEPT(tok.Insert("test2", 2));
		tok["object"] = sprawl::serialization::JSONToken::object();

#if !SPRAWL_EXCEPTIONS_ENABLED
		ASSERT_FALSE(tok["object"].Error());
#endif
		sprawl::serialization::JSONToken& obj = tok["object"];

#if !SPRAWL_EXCEPTIONS_ENABLED
		ASSERT_FALSE(obj.Insert("test1", 1).Error());
		ASSERT_FALSE(obj.Insert("test2", 2).Error());
#else
		obj.Insert("test1", 1);
		obj.Insert("test2", 2);
#endif

		tok["array"] = sprawl::serialization::JSONToken::array();

#if !SPRAWL_EXCEPTIONS_ENABLED
		ASSERT_FALSE(tok["object"].Error());
#endif

		sprawl::serialization::JSONToken& arr = tok["array"];

#if !SPRAWL_EXCEPTIONS_ENABLED
		ASSERT_FALSE(arr.PushBack(1).Error());
		ASSERT_FALSE(arr.PushBack(2).Error());
#else
		arr.PushBack(1);
		arr.PushBack(2);
#endif
	}

	sprawl::serialization::JSONToken tok;
};

TEST_F(JsonTokenTest, JsonTokensWork)
{
	sprawl::String expectedTokValue = "{ \"test1\" : 1, \"test2\" : 2, \"object\" : { \"test1\" : 1, \"test2\" : 2 }, \"array\" : [ 1, 2 ] }";
	ASSERT_EQ(expectedTokValue, tok.ToJSONString());
}

TEST_F(JsonTokenTest, CopyingWorks)
{
	sprawl::String expectedTokValue = "{ \"test1\" : 1, \"test2\" : -2, \"object\" : { \"test1\" : 1, \"test2\" : -2, \"Tok\" : null }, \"array\" : [ -1, 2 ] }";
	sprawl::String expectedTok2Value = "{ \"test1\" : -1, \"test2\" : 2, \"object\" : { \"test1\" : -1, \"test2\" : 2, \"Tok2\" : null }, \"array\" : [ 1, -2 ] }";
	//With shallow copy, test1 and test2 will be unchanged, nested objects will mirror tok
	sprawl::String expectedTok3Value = "{ \"test1\" : 1, \"test2\" : 2, \"object\" : { \"test1\" : 1, \"test2\" : -2, \"Tok\" : null }, \"array\" : [ -1, 2 ] }";

	sprawl::serialization::JSONToken tok2 = tok.DeepCopy();
	sprawl::serialization::JSONToken tok3 = tok.ShallowCopy();

	tok2["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tok["test2"] = sprawl::serialization::JSONToken::number((long long)-2);

#if !SPRAWL_EXCEPTIONS_ENABLED
	ASSERT_FALSE(tok2["object"].Error());
	ASSERT_FALSE(tok["object"].Error());
#endif

	sprawl::serialization::JSONToken& tok2obj = tok2["object"];
	sprawl::serialization::JSONToken& tokobj = tok["object"];

#if !SPRAWL_EXCEPTIONS_ENABLED
	ASSERT_FALSE(tok2obj["test1"].Error());
	ASSERT_FALSE(tokobj["test2"].Error());
#endif

	tok2obj["test1"] = sprawl::serialization::JSONToken::number((long long)-1);
	tokobj["test2"] = sprawl::serialization::JSONToken::number((long long)-2);

#if !SPRAWL_EXCEPTIONS_ENABLED
	ASSERT_FALSE(tokobj.Insert("Tok", sprawl::serialization::JSONToken::null()).Error());
	ASSERT_FALSE(tok2obj.Insert("Tok2", sprawl::serialization::JSONToken::null()).Error());
#else
	tokobj.Insert("Tok", sprawl::serialization::JSONToken::null());
	tok2obj.Insert("Tok2", sprawl::serialization::JSONToken::null());
#endif


#if !SPRAWL_EXCEPTIONS_ENABLED
	ASSERT_FALSE(tok2["array"].Error());
	ASSERT_FALSE(tok["array"].Error());
#endif

	sprawl::serialization::JSONToken& tok2arr = tok2["array"];
	sprawl::serialization::JSONToken& tokarr = tok["array"];

#if !SPRAWL_EXCEPTIONS_ENABLED
	ASSERT_FALSE(tok2arr[1].Error());
	ASSERT_FALSE(tokarr[0].Error());
#endif

	tokarr[0] = sprawl::serialization::JSONToken::number((long long)-1);
	tok2arr[1] = sprawl::serialization::JSONToken::number((long long)-2);

	ASSERT_EQ(expectedTokValue, tok.ToJSONString());
	ASSERT_EQ(expectedTok2Value, tok2.ToJSONString());
	ASSERT_EQ(expectedTok3Value, tok3.ToJSONString());
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
	sprawl::String expectedValue1 = "{ \"__version__\" : 0, \"t\" : { \"map\" : { \"test\" : 1.0000000000000000715e-18, \"test2\" : 1e+22, \"test3\" : 1000000000 }, \"intArray\" : [ 1, -2, 3, -4, 5 ], \"strArray\" : [ \"hi\", \"there\", \"y'all\" ], \"nestedArray\" : [ [ 6, 7, 8, 9, 10 ] ], \"str\" : \"\\\\escapes\\\\ \\\" \\n\", \"i\" : 3, \"d\" : 2.5, \"b\" : true, \"c\" : \"f\", \"u\" : 32 } }";

	sprawl::serialization::JSONSerializer s;

	ASSERT_NO_SPRAWL_EXCEPT(s % NAME_PROPERTY(t));

	sprawl::String str1 = s.Str();

	ASSERT_EQ(expectedValue1, str1);
}

TEST_F(JsonSerializerTest, PrettySerializationWorks)
{
	sprawl::String expectedValue2 = "{\n\t\"__version__\" : 0,\n\t\"map\" : {\n\t\t\"test\" : 1.0000000000000000715e-18,\n\t\t\"test2\" : 1e+22,\n\t\t\"test3\" : 1000000000\n\t},\n\t\"intArray\" : [\n\t\t1,\n\t\t-2,\n\t\t3,\n\t\t-4,\n\t\t5\n\t],\n\t\"strArray\" : [\n\t\t\"hi\",\n\t\t\"there\",\n\t\t\"y'all\"\n\t],\n\t\"nestedArray\" : [\n\t\t[\n\t\t\t6,\n\t\t\t7,\n\t\t\t8,\n\t\t\t9,\n\t\t\t10\n\t\t]\n\t],\n\t\"str\" : \"\\\\escapes\\\\ \\\" \\n\",\n\t\"i\" : 3,\n\t\"d\" : 2.5,\n\t\"b\" : true,\n\t\"c\" : \"f\",\n\t\"u\" : 32\n}";

	sprawl::serialization::JSONSerializer s2;
	s2.SetPrettyPrint(true);

	t.Serialize(s2);

	sprawl::String str2 = s2.Str();

	ASSERT_EQ(expectedValue2, str2);
}

TEST_F(JsonSerializerTest, DeserializationWorks)
{
	sprawl::serialization::JSONSerializer s;

	ASSERT_NO_SPRAWL_EXCEPT(s % NAME_PROPERTY(t));

	sprawl::String str1 = s.Str();

	sprawl::serialization::JSONDeserializer d(str1);

	TestType t1;

	ASSERT_NO_SPRAWL_EXCEPT(d % sprawl::serialization::prepare_data(t1, "t"));

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
