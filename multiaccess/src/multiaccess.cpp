/*************************
 *         JHash         *
 *************************
 *      Main Module      *
 *      Source File      *
 *************************/

#include "multiaccess.hpp"
#include <iostream>
#include <boost/shared_ptr.hpp>

namespace sprawl
{
	namespace multiaccess
	{
		std::string strToLower(std::string s, bool ignore_case)
		{
			if(!ignore_case)
				return s;
			int len = s.length();
			for(int i=0;i<len;i++)
				s[i]=tolower(s[i]);
			return s;
		}
	}
}

class TestClass
{
public:
	TestClass(std::string str_, int i_) : str(str_), i(i_) {}
	std::string GetString() { return str; }
	int GetInt() { return i; }
	void SetString(std::string s){ str = s; }
private:
	std::string str;
	int i;
};

boost::shared_ptr<TestClass> tsh;
boost::shared_ptr<TestClass> tsh2;
boost::shared_ptr<TestClass> tsh3;
boost::shared_ptr<TestClass> tsh4;
boost::shared_ptr<TestClass> tsh5;
boost::shared_ptr<TestClass> tsh6;

sprawl::multiaccess::multiaccess_map<TestClass> valmap(&TestClass::GetInt, &TestClass::GetString);
sprawl::multiaccess::multiaccess_map<TestClass*> ptrmap(&TestClass::GetInt, &TestClass::GetString);
sprawl::multiaccess::multiaccess_map<boost::shared_ptr<TestClass>, int(TestClass::*)(), std::string(TestClass::*)()> sharedmap(&TestClass::GetInt, &TestClass::GetString);
sprawl::multiaccess::multiaccess_map<boost::shared_ptr<TestClass>, int(TestClass::*)(), std::string(TestClass::*)()> sharedmap2(&TestClass::GetInt);

int main()
{
	TestClass *tp = new TestClass("Hi", 33);
	TestClass *tp2 = new TestClass("Yo", 1);
	TestClass *tp3 = new TestClass("Zing", 81);
	TestClass *tp4 = new TestClass("fourfourfour", 444);
	TestClass *tp5 = new TestClass("eighttwelve", 812);
	TestClass *tp6 = new TestClass("million", 1000000);
	
	valmap.push(*tp);
	valmap.push(*tp2);
	valmap.push(*tp3);
	valmap.push(*tp4);
	valmap.push(*tp5);
	valmap.push(*tp6);
	valmap.push(TestClass("Something new", 10005512));
	std::cout << "Valmap:" << valmap[33].GetString() << " " << valmap["Hi"].GetInt() << std::endl;
	std::cout << "Valmap:" << valmap[1].GetString() << " " << valmap["Yo"].GetInt() << std::endl;
	std::cout << "Valmap:" << valmap[81].GetString() << " " << valmap["Zing"].GetInt() << std::endl;
	std::cout << "Valmap:" << valmap[444].GetString() << " " << valmap["fourfourfour"].GetInt() << std::endl;
	std::cout << "Valmap:" << valmap[812].GetString() << " " << valmap["eighttwelve"].GetInt() << std::endl;
	std::cout << "Valmap:" << valmap[1000000].GetString() << " " << valmap["million"].GetInt() << std::endl;
	std::cout << std::endl;
	for(auto &test : valmap)
	{
		std::cout << test.GetString() << " " << test.GetInt() << std::endl;
	}
	for(sprawl::multiaccess::multiaccess_map<TestClass>::iterator it = valmap.begin(); it != valmap.end(); it++)
	{
		std::cout << it->GetString() << " " << it->GetInt() << std::endl;
	}
	std::cout << std::endl;

	ptrmap.push(tp);
	ptrmap.push(tp2);
	ptrmap.push(tp3);
	ptrmap.push(tp4);
	ptrmap.push(tp5);
	ptrmap.push(tp6);
	std::cout << "ptrmap:" << ptrmap[33]->GetString() << " " << ptrmap["Hi"]->GetInt() << std::endl;
	std::cout << "ptrmap:" << ptrmap[1]->GetString() << " " << ptrmap["Yo"]->GetInt() << std::endl;
	std::cout << "ptrmap:" << ptrmap[81]->GetString() << " " << ptrmap["Zing"]->GetInt() << std::endl;
	std::cout << "ptrmap:" << ptrmap[444]->GetString() << " " << ptrmap["fourfourfour"]->GetInt() << std::endl;
	std::cout << "ptrmap:" << ptrmap[812]->GetString() << " " << ptrmap["eighttwelve"]->GetInt() << std::endl;
	std::cout << "ptrmap:" << ptrmap[1000000]->GetString() << " " << ptrmap["million"]->GetInt() << std::endl;
	std::cout << std::endl;
	for(auto &test : ptrmap)
	{
		std::cout << test->GetString() << " " << test->GetInt() << std::endl;
	}
	for(sprawl::multiaccess::multiaccess_map<TestClass*>::iterator it = ptrmap.begin(); it != ptrmap.end(); it++)
	{
		std::cout << it->GetString() << " " << it->GetInt() << std::endl;
	}
	std::cout << std::endl;

	tsh.reset(tp);
	tsh2.reset(tp2);
	tsh3.reset(tp3);
	tsh4.reset(tp4);
	tsh5.reset(tp5);
	tsh6.reset(tp6);
	sharedmap.push(tsh);
	sharedmap.push(tsh2);
	sharedmap.push(tsh3);
	sharedmap.push(tsh4);
	sharedmap.push(tsh5);
	sharedmap.push(tsh6);
	std::cout << "sharedmap:" << sharedmap[33]->GetString() << " " << sharedmap["Hi"]->GetInt() << std::endl;
	std::cout << "sharedmap:" << sharedmap[1]->GetString() << " " << sharedmap["Yo"]->GetInt() << std::endl;
	std::cout << "sharedmap:" << sharedmap[81]->GetString() << " " << sharedmap["Zing"]->GetInt() << std::endl;
	std::cout << "sharedmap:" << sharedmap[444]->GetString() << " " << sharedmap["fourfourfour"]->GetInt() << std::endl;
	std::cout << "sharedmap:" << sharedmap[812]->GetString() << " " << sharedmap["eighttwelve"]->GetInt() << std::endl;
	std::cout << "sharedmap:" << sharedmap[1000000]->GetString() << " " << sharedmap["million"]->GetInt() << std::endl;
	std::cout << std::endl;
	for(auto &test : sharedmap)
	{
		std::cout << test->GetString() << " " << test->GetInt() << std::endl;
	}
	for(sprawl::multiaccess::multiaccess_map<boost::shared_ptr<TestClass>, int(TestClass::*)(), std::string(TestClass::*)()>::iterator it = sharedmap.begin(); it != sharedmap.end(); it++)
	{
		std::cout << it->GetString() << " " << it->GetInt() << std::endl;
	}

	for(auto &test : valmap)
	{
		std::cout << "Popping " << test.GetInt() << std::endl;
		valmap.pop(test.GetInt());
	}
	std::cout << std::endl;
	for(auto &test : ptrmap)
	{
		std::cout << "Popping " << test->GetInt() << std::endl;
		ptrmap.pop(test->GetInt());
	}
	std::cout << std::endl;
	for(auto &test : sharedmap)
	{
		std::cout << "Popping " << test->GetInt() << std::endl;
		sharedmap.pop(test->GetInt());
	}
	std::cout << std::endl;
}
