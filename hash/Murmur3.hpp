#pragma once

#include <stddef.h>

#ifndef _WIN64
#	define _WIN64 0
#endif
#ifndef __amd64__
#	define __amd64__ 0
#endif

#if _WIN64 || __amd64__
#	define SPRAWL_64_BIT 1
#	define SPRAWL_32_BIT 0
#else
#	define SPRAWL_64_BIT 0
#	define SPRAWL_32_BIT 1
#endif

namespace sprawl
{
	namespace murmur3
	{
		size_t Hash(const void* const data, const size_t size);
	}
}
