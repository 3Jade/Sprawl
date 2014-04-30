#pragma once

#ifdef _WIN32
#	define SPRAWL_CONSTEXPR const
#else
#	define SPRAWL_CONSTEXPR constexpr
#endif

#if defined(_M_X64) || defined(__x86_64__)
#	define SPRAWL_64_BIT 1
#	define SPRAWL_32_BIT 0
#else
#	define SPRAWL_64_BIT 0
#	define SPRAWL_32_BIT 1
#endif
