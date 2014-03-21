#include "PoolAllocator.hpp"
#include <boost/progress.hpp>
#include <algorithm>
#include "StlWrapper.hpp"
#include <map>
#include <unordered_map>

struct MyBigStruct
{
	int64_t data[512];
};

struct MyMediumStruct
{
	int64_t data[256];
};

struct MySmallStruct
{
	int64_t data[128];
};
struct MyTinyStruct
{
	int64_t data[64];
};

int main()
{
	typedef MyBigStruct testType;
	typedef sprawl::memory::StaticPoolAllocator<sizeof(testType), 32, 10241> allocator;
	typedef sprawl::memory::StlWrapper<
				std::pair<const int64_t, int64_t>,
				sprawl::memory::StaticPoolAllocator<sizeof(std::pair<const int64_t, int64_t>), 32, 10241>
			> stlWrapper;

	{
		//std::map<int64_t, int64_t, std::less<int64_t>, stlWrapper> testMap;
		std::unordered_map<int64_t, int64_t, std::hash<int64_t>, std::equal_to<int64_t>, stlWrapper> testMap;

		{
			boost::progress_timer t;

			for(size_t i = 0; i < 327680; ++i)
			{
				testMap[i] = i;
			}
		}

		for(auto& kvp : testMap)
		{
			//printf("%lu %lu\n", kvp.first, kvp.second);
		}
	}

	return 0;

	puts("Starting.");

#define SIZE 327680
#define MULT 1

	testType* datas[SIZE];
	size_t randoms[SIZE];

	for(size_t i = 0; i < SIZE; ++i)
	{
		randoms[i] = i;
	}
	std::random_shuffle(&randoms[0], &randoms[SIZE]);

	{
		boost::progress_timer t;

		for(int i = 0; i < MULT; ++i)
		{
			for(size_t i = 0; i < SIZE; ++i)
			{
				datas[i] = (testType*)allocator::alloc();
			}
		}
	}

//	{
//		boost::progress_timer t;
//		for(size_t i = 0; i < SIZE; ++i)
//		{
//			allocator::free(datas[i]);
//		}
//	}
	{
		boost::progress_timer t;
		for(size_t i = 0; i < SIZE; ++i)
		{
			if( i % 2 == 0 )
			{
				datas[randoms[i]] = (testType*)allocator::alloc();
			}
			else
			{
				allocator::free(datas[randoms[i]]);
			}
		}
	}

	{
		boost::progress_timer t;

		for(size_t i = 0; i < MULT; ++i)
		{
			for(int i = 0; i < SIZE; ++i)
			{
				datas[i] = new testType();
			}
		}
	}
//	{
//		boost::progress_timer t;

//		for(size_t i = 0; i < SIZE; ++i)
//		{
//			delete datas[i];
//		}
//	}
	{
		boost::progress_timer t;

		for(size_t i = 0; i < SIZE; ++i)
		{
			if( i % 2 == 0 )
			{
				datas[randoms[i]] = new testType();
			}
			else
			{
				delete datas[randoms[i]];
			}
		}
	}
}
