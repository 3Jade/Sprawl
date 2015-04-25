//Use STL here because unit tests by default should not trust our own code.
#include <vector>
#include <string>
#include <tuple>
#include <string.h>

int main(int argc, char* argv[])
{
	std::vector<std::tuple<std::string, bool, bool(*)(void)>> tests;

#define ADD_TEST(name) extern bool test_##name(); tests.push_back(std::make_tuple(#name, false, &test_##name))

	ADD_TEST(memory);
	ADD_TEST(hashmap);
	ADD_TEST(list);
	ADD_TEST(string);
	ADD_TEST(json);
	ADD_TEST(time);
	ADD_TEST(thread);
#ifdef WITH_MONGO
	ADD_TEST(mongo_replicable);
#endif

	char validArguments[512];
	validArguments[0] = '\0';

	bool all = false;

	for(auto& test : tests)
	{
		sprintf(validArguments, "%s --%s", validArguments, std::get<0>(test).c_str());
	}

	if(argc <= 1)
	{
		all = true;
	}

	for(int i = 1; i < argc; ++i)
	{
		if(!strcmp(argv[i], "--all"))
		{
			all = true;
			continue;
		}

		bool found = false;

		for(auto& test : tests)
		{
			char arg[64];
			sprintf(arg, "--%s", std::get<0>(test).c_str());
			if(!strcmp(argv[i], arg))
			{
				std::get<1>(test) = true;
				found = true;
				break;
			}
		}

		if(!found)
		{
			printf("Invalid argument ignored: %s\n", argv[i]);
			printf("Valid arguments: %s --all\n", validArguments);
		}
	}

	int exitcode = 0;

	for(auto& test : tests)
	{
		if(all || std::get<1>(test))
		{
			printf("Running test '%s'... ", std::get<0>(test).c_str());
			if(std::get<2>(test)())
			{
				puts("Success!");
			}
			else
			{
				++exitcode;
				puts("FAILED!");
			}
		}
	}
	fflush(stdout);
	return exitcode;
}
