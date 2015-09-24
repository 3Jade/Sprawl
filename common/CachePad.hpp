#pragma once


#ifndef SPRAWL_CACHELINE_SIZE
#	define SPRAWL_CACHELINE_SIZE 64
#endif

namespace sprawl
{
	typedef unsigned char CachelinePad[SPRAWL_CACHELINE_SIZE];
}

#define SPRAWL_PAD_CACHELINE CachelinePad SPRAWL_CONCAT( SPRAWL_CONCAT(pad ## _, __LINE__), __ )
