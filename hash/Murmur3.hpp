#pragma once

#include <stddef.h>

namespace sprawl
{
	namespace murmur3
	{
		size_t Hash(const void* const data, const size_t size);
	}
}
