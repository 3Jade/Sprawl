#include "../collections/List.hpp"
#include "../collections/ForwardList.hpp"

bool test_list()
{
	bool success = true;

	sprawl::collections::List<int> testList;

	testList.PushBack(3);
	testList.PushBack(5);
	testList.PushFront(2);
	testList.PushFront(1);

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testList.Insert(it, 4);
			break;
		}
	}

	if(testList.front() != 1)
	{
		printf("Failed testList.front()\n... ");
		success = false;
	}

	if(testList.back() != 5)
	{
		printf("Failed testList.front()\n... ");
		success = false;
	}
	int value = 0;

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() <= value)
		{
			printf("Failed iteration (%d >= %d)\n... ", it.Value(), value);
			success = false;
		}
		value = it.Value();
	}

	testList.PopFront();
	testList.PopBack();

	if(testList.front() != 2)
	{
		printf("Failed testList.PopFront()\n... ");
		success = false;
	}

	if(testList.back() != 4)
	{
		printf("Failed testList.PopBack()\n... ");
		success = false;
	}

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testList.Erase(it);
			break;
		}
	}

	for(auto it = testList.begin(); it != testList.end(); ++it)
	{
		if(it.Value() == 3)
		{
			printf("Failed erase\n... ");
			success = false;
			break;
		}
	}

	sprawl::collections::ForwardList<int> testList2;

	testList2.PushFront(5);
	testList2.PushFront(3);
	testList2.PushFront(2);
	testList2.PushFront(1);

	for(auto it = testList2.begin(); it != testList2.end(); ++it)
	{
		if(it.Value() == 3)
		{
			testList2.Insert(it, 4);
			break;
		}
	}

	if(testList2.front() != 1)
	{
		printf("Failed testList2.front()\n... ");
		success = false;
	}

	value = 0;

	for(auto it = testList2.begin(); it != testList2.end(); ++it)
	{
		if(it.Value() <= value)
		{
			printf("Failed iteration (%d >= %d)\n... ", it.Value(), value);
			success = false;
		}
		value = it.Value();
	}

	testList2.PopFront();
	if(testList2.front() != 2)
	{
		printf("Failed testList2.PopFront()\n... ");
		success = false;
	}

	for(auto it = testList2.begin(); it != testList2.end(); ++it)
	{
		if(it.Value() == 2)
		{
			testList2.EraseAfter(it);
			break;
		}
	}

	for(auto it = testList2.begin(); it != testList2.end(); ++it)
	{
		if(it.Value() == 3)
		{
			printf("Failed EraseAfter\n... ");
			success = false;
			break;
		}
	}

	return success;
}
