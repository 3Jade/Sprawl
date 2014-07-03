#pragma once

#include <stddef.h>

namespace sprawl
{
	namespace cityhash
	{
		size_t Hash(const void* data, size_t length);
	}
}
