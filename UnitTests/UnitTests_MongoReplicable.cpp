#include "../serialization/mongo/MongoReplicable.hpp"

#include "gtest_printers.hpp"
#include <gtest/gtest.h>

struct TestStruct1
{
	TestStruct1()
		: vect()
	{
		//
	}

	TestStruct1(std::vector<int>&& vect_)
		: vect(vect_)
	{
		//
	}

	void Serialize(sprawl::serialization::SerializerBase& s)
	{
		s % NAME_PROPERTY(vect);
	}

	std::vector<int> vect;
};

struct TestStruct2
{
	TestStruct2(std::vector<TestStruct1>&& vect_)
		: vect(vect_)
	{
		//
	}

	void Serialize(sprawl::serialization::SerializerBase& s)
	{
		s % NAME_PROPERTY(vect);
	}

	std::vector<TestStruct1> vect;
};

TEST(MongoReplicable, RemovingArrayWithNestedArrayInsideNestedObjectWorks)
{
	TestStruct2 t( { TestStruct1({1, 2, 3}), TestStruct1({4, 5, 6}) } );

	sprawl::serialization::MongoReplicableSerializer m;

	m % NAME_PROPERTY(t);
	m.Mark();

	t.vect.pop_back();
	t.vect[0].vect.pop_back();

	m % NAME_PROPERTY(t);

	std::vector<mongo::BSONObj> deltas = m.generateUpdateQuery();

	EXPECT_EQ(size_t(3), deltas.size()) << "Test case should have returned three queries.";

	if(deltas.size() >= 1)
	{
		EXPECT_EQ(
			R"raw({ "$unset" : { "t.vect.0.vect.2" : "", "t.vect.1" : "" } })raw",
			deltas[0].jsonString(mongo::TenGen)
		) << "Invalid first query";
	}

	if(deltas.size() >= 2)
	{
		EXPECT_EQ(
			R"raw({ "$pull" : { "t.vect.0.vect" : null } })raw",
			deltas[1].jsonString(mongo::TenGen)
		) << "Invalid second query";
	}

	if(deltas.size() >= 3)
	{
		EXPECT_EQ(
			R"raw({ "$pull" : { "t.vect" : null } })raw",
			deltas[2].jsonString(mongo::TenGen)
		) << "Invalid third query";
	}
}
