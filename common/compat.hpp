#pragma once

#if defined(_M_X64) || defined(__x86_64__)
#	define SPRAWL_64_BIT 1
#	define SPRAWL_32_BIT 0
#else
#	define SPRAWL_64_BIT 0
#	define SPRAWL_32_BIT 1
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#	define SPRAWL_LIKELY(x) (__builtin_expect(!!(x), 1))
#	define SPRAWL_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#	define SPRAWL_LIKELY(x) (x)
#	define SPRAWL_UNLIKELY(x) (x)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#	define SPRAWL_MEMCMP __builtin_memcmp
#	ifndef SPRAWL_MULTITHREADED
#		ifdef _REENTRANT
#			define SPRAWL_MULTITHREADED 1
#		else
#			define SPRAWL_MULTITHREADED 0
#		endif
#	endif
#	define SPRAWL_FORCEINLINE inline __attribute__((always_inline))
#	define SPRAWL_CONSTEXPR constexpr
#else
#	define SPRAWL_MEMCMP memcmp
#	ifndef SPRAWL_MULTITHREADED
#		ifdef _MT
#			define SPRAWL_MULTITHREADED 1
#		else
#			define SPRAWL_MULTITHREADED 0
#		endif
#	endif
#	define SPRAWL_FORCEINLINE inline __forceinline
#	define SPRAWL_CONSTEXPR const
#endif
