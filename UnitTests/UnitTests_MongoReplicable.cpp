#include "../serialization/mongo/MongoReplicable.hpp"

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

bool test_mongo_replicable()
{
	bool success = true;
	TestStruct2 t( { TestStruct1({1, 2, 3}), TestStruct1({4, 5, 6}) } );

	sprawl::serialization::MongoReplicableSerializer m;

	m % NAME_PROPERTY(t);
	m.Mark();

	t.vect.pop_back();
	t.vect[0].vect.pop_back();

	m % NAME_PROPERTY(t);

	std::vector<mongo::BSONObj> deltas = m.generateUpdateQuery();

#define ASSERT(condition, message) if(!(!(condition))==false){ success = false; printf("%s\n... ", message); }

	ASSERT(deltas.size() == 3, "Test case should have returned three queries.");
	ASSERT(
		deltas[0].jsonString(mongo::TenGen) ==
			R"raw({ "$set" : { "t" : { "vect" : [ { "vect" : [] } ] } }, "$unset" : { "t" : "", "t.vect" : "", "t.vect.0" : "", "t.vect.0.vect" : "", "t.vect.0.vect.2" : "", "t.vect.1" : "" } })raw",
			"Test case returned invalid first query");
	ASSERT(
		deltas[1].jsonString(mongo::TenGen) ==
			R"raw({ "$pull" : { "t.vect.0.vect" : null } })raw",
			"Test case returned invalid second query");
	ASSERT(
		deltas[2].jsonString(mongo::TenGen) ==
			R"raw({ "$pull" : { "t.vect" : null } })raw",
			"Test case returned invalid third query");
	return success;
}
