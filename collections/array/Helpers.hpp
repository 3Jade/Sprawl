#pragma once

#include <stddef.h>

namespace sprawl
{
	namespace collections
	{
		struct Fill;
		struct Capacity;
	}
}

struct sprawl::collections::Fill
{
	explicit Fill(size_t count_)
		: count(count_)
	{
		//
	}

	size_t count;
};

struct sprawl::collections::Capacity
{
	explicit Capacity(size_t count_)
		: count(count_)
	{
		//
	}

	size_t count;
};
