#ifndef _WIN32
#include "gtest_printers.hpp"
#include "third-party-comparisons/json/jsoncpp/json/json.h"
#include "third-party-comparisons/json/rapidjson/document.h"
#include "../../serialization/JSONSerializer.hpp"
#include "gtest/gtest.h"
#include "../../filesystem/filesystem.hpp"
#include "../../filesystem/path.hpp"
#include "../../time/time.hpp"

class JSONPerformance : public testing::Test
{
public:
	virtual void SetUp() override
	{
		ASSERT_TRUE(sprawl::path::Exists("../../../../../UnitTests/testfiles/sample.json"));
		sprawl::filesystem::File f = sprawl::filesystem::Open("../../../../../UnitTests/testfiles/sample.json", "r");
		data = f.Read();
	}
protected:
	sprawl::String data;
};

TEST_F(JSONPerformance, SprawlVsJsonCpp_Parse)
{
	int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	for(int i = 0; i < 100; ++i)
	{
		Json::Reader r;
		Json::Value root;
		r.parse(data.c_str(), data.c_str() + data.length(), root);
	}
	int64_t swap = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	for(int i = 0; i < 100; ++i)
	{
		sprawl::serialization::JSONToken tok = sprawl::serialization::JSONToken::fromString(data);
	}
	int64_t end = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	int64_t jsoncppDuration = swap - start;
	int64_t sprawlDuration = end - swap;
	puts(sprawl::Format("Sprawl: {} seconds, JsonCpp: {} seconds", double(sprawlDuration) / double(sprawl::time::Resolution::Seconds), double(jsoncppDuration) / double(sprawl::time::Resolution::Seconds)).c_str());
	ASSERT_LT(sprawlDuration, jsoncppDuration);
}

struct StringOrInt
{
	StringOrInt(size_t i)
		: isString(false)
		, str()
		, i(i)
	{

	}

	StringOrInt(sprawl::String const& str)
		: isString(true)
		, str(str.GetOwned())
		, i(0)
	{

	}

	bool isString;
	sprawl::String str;
	size_t i;
};

void CheckNest(sprawl::serialization::JSONToken& tok, sprawl::collections::Vector<StringOrInt>& path, sprawl::collections::Vector<StringOrInt>& longestPath, StringOrInt const& key)
{
	if(key.isString)
	{
		sprawl::String checkStr = sprawl::serialization::JSONToken::UnescapeString(sprawl::serialization::StringData(key.str.c_str(), key.str.length()));
		if(strlen(checkStr.c_str()) < checkStr.length())
		{
			//Rapidjson has no way of querying keys with null terminators in the name... even though it claims to support them
			return;
		}
	}
	path.PushBack(key);
	if(path.Size() > longestPath.Size())
	{
		longestPath = path;
	}
	if(tok.Type() == sprawl::serialization::JSONToken::JSONType::Object || tok.Type() == sprawl::serialization::JSONToken::JSONType::Array)
	{
		for(auto it = tok.begin(); it != tok.end(); ++it)
		{
			if(it.Type() == sprawl::serialization::JSONToken::JSONType::Array)
			{
				CheckNest(it.Value(), path, longestPath, it.Index());
			}
			else
			{
				CheckNest(it.Value(), path, longestPath, it.Key());
			}
		}
	}
	path.PopBack();
}

#ifndef _WIN32
TEST_F(JSONPerformance, SprawlVsRapidJson)
{
	int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	for(int i = 0; i < 100; ++i)
	{
		rapidjson::Document d;
		d.Parse(data.c_str());
	}
	int64_t swap = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	for(int i = 0; i < 100; ++i)
	{
		sprawl::serialization::JSONToken tok = sprawl::serialization::JSONToken::fromString(data);
	}
	int64_t end = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	int64_t rapidjsonDuration = swap - start;
	int64_t sprawlDuration = end - swap;
	puts(sprawl::Format("Sprawl: {} seconds, rapidjson: {} seconds", double(sprawlDuration) / double(sprawl::time::Resolution::Seconds), double(rapidjsonDuration) / double(sprawl::time::Resolution::Seconds)).c_str());
	int64_t delta = std::abs(sprawlDuration - rapidjsonDuration);
	ASSERT_TRUE(sprawlDuration < rapidjsonDuration || delta < 150 * sprawl::time::Resolution::Milliseconds);
}
#endif

TEST_F(JSONPerformance, SprawlVsJsonCpp_Access)
{
	sprawl::serialization::JSONToken tok = sprawl::serialization::JSONToken::fromString(data);
	sprawl::collections::Vector<StringOrInt> path;
	sprawl::collections::Vector<StringOrInt> longestPath;
	sprawl::collections::Vector<StringOrInt> longestPathUnescaped;
	CheckNest(tok, path, longestPath, StringOrInt(""));

	Json::Reader r;
	Json::Value root;
	r.parse(data.c_str(), data.c_str() + data.length(), root);

	int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

	for(int i = 0; i < longestPath.Size(); ++i)
	{
		if(longestPath[i].isString)
		{
			longestPathUnescaped.PushBack(sprawl::serialization::JSONToken::UnescapeString(sprawl::serialization::StringData(longestPath[i].str.c_str(), longestPath[i].str.length())));
		}
		else
		{
			longestPathUnescaped.PushBack(longestPath[i].i);
		}
	}

	for(int i = 0; i < 10; ++i)
	{
		Json::Value cur = root;
		for(ssize_t j = 1; j < longestPathUnescaped.Size(); ++j)
		{
			EXPECT_FALSE(cur.isNull()) << " " << j;
			if(cur.isNull())
			{
				break;
			}
			if(longestPathUnescaped[j].isString)
			{
				cur = cur[longestPathUnescaped[j].str.c_str()];
			}
			else
			{
				cur = cur[int(longestPathUnescaped[j].i)];
			}
		}
	}

	int64_t swap = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

	for(int i = 0; i < 10; ++i)
	{
		sprawl::serialization::JSONToken cur2 = tok;
		for(ssize_t j = 1; j < longestPath.Size(); ++j)
		{
			if(longestPath[j].isString)
			{
				cur2 = cur2[longestPath[j].str];
			}
			else
			{
				cur2 = cur2[longestPath[j].i];
			}
			ASSERT_FALSE(cur2.IsEmpty()) << " " << j;
		}
	}

	int64_t end = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	int64_t jsoncppDuration = swap - start;
	int64_t sprawlDuration = end - swap;
	puts(sprawl::Format("Sprawl: {} seconds, JsonCpp: {} seconds", double(sprawlDuration) / double(sprawl::time::Resolution::Seconds), double(jsoncppDuration) / double(sprawl::time::Resolution::Seconds)).c_str());
	ASSERT_LT(sprawlDuration, jsoncppDuration);
}


#ifndef _WIN32
TEST_F(JSONPerformance, SprawlVsRapidJson_Access)
{
	sprawl::serialization::JSONToken tok = sprawl::serialization::JSONToken::fromString(data);
	sprawl::collections::Vector<StringOrInt> path;
	sprawl::collections::Vector<StringOrInt> longestPath;
	sprawl::collections::Vector<StringOrInt> longestPathUnescaped;
	CheckNest(tok, path, longestPath, StringOrInt(""));

	int64_t rapidjson_spentParsing = 0;

	int64_t start = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

	for(int i = 0; i < longestPath.Size(); ++i)
	{
		if(longestPath[i].isString)
		{
			longestPathUnescaped.PushBack(sprawl::serialization::JSONToken::UnescapeString(sprawl::serialization::StringData(longestPath[i].str.c_str(), longestPath[i].str.length())));
		}
		else
		{
			longestPathUnescaped.PushBack(longestPath[i].i);
		}
	}

	for(int i = 0; i < 100; ++i)
	{
		int64_t parseStart = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
		rapidjson::Document d;
		d.Parse(data.c_str());
		rapidjson_spentParsing += (sprawl::time::Now(sprawl::time::Resolution::Nanoseconds) - parseStart);

		rapidjson::Value cur;
		if(longestPath[1].isString)
		{
			cur = d[longestPathUnescaped[1].str.c_str()];
		}
		else
		{
			cur = d[longestPathUnescaped[1].i];
		}
		for(ssize_t j = 2; j < longestPathUnescaped.Size(); ++j)
		{
			EXPECT_FALSE(cur.IsNull()) << " " << j;
			if(cur.IsNull())
			{
				break;
			}
			if(longestPathUnescaped[j].isString)
			{
				cur = cur[longestPathUnescaped[j].str.c_str()];
			}
			else
			{
				cur = cur[longestPathUnescaped[j].i];
			}
		}
	}

	int64_t swap = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);

	for(int i = 0; i < 100; ++i)
	{
		sprawl::serialization::JSONToken cur2 = tok;
		for(ssize_t j = 1; j < longestPath.Size(); ++j)
		{
			if(longestPath[j].isString)
			{
				cur2 = cur2[longestPath[j].str];
			}
			else
			{
				cur2 = cur2[longestPath[j].i];
			}
			ASSERT_FALSE(cur2.IsEmpty()) << " " << j;
		}
	}

	int64_t end = sprawl::time::Now(sprawl::time::Resolution::Nanoseconds);
	int64_t rapidjsonDuration = swap - start - rapidjson_spentParsing;
	int64_t sprawlDuration = end - swap;
	puts(sprawl::Format("Sprawl: {} seconds, rapidjson: {} seconds", double(sprawlDuration) / double(sprawl::time::Resolution::Seconds), double(rapidjsonDuration) / double(sprawl::time::Resolution::Seconds)).c_str());
	int64_t delta = std::abs(sprawlDuration - rapidjsonDuration);
	ASSERT_TRUE(sprawlDuration < rapidjsonDuration || delta < 150 * sprawl::time::Resolution::Microseconds);
}
#endif
#endif