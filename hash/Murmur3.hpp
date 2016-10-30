#pragma once

#include <stddef.h>
#include <stdint.h>

namespace sprawl
{
	namespace murmur3
	{
		size_t HashInt32(uint32_t data);
		size_t HashInt64(uint64_t data);
		size_t HashPointer(intptr_t data);
		size_t Hash(const void* const data, const size_t size, const size_t seed);
		size_t Hash(const void* const data, const size_t size);
	}
}
