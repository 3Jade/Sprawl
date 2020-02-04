#pragma once

#if defined(_M_X64) || defined(__x86_64__)
#	define SPRAWL_64_BIT 1
#	define SPRAWL_32_BIT 0
#else
#	define SPRAWL_64_BIT 0
#	define SPRAWL_32_BIT 1
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#	define SPRAWL_COMPILER_MSVC 1
#	define SPRAWL_COMPILER_CLANG 0
#	define SPRAWL_COMPILER_GCC 0
#	define SPRAWL_COMPILER_INTEL 0
#	define SPRAWL_COMPILER_GNU_COMPAT 0
#elif defined(__INTEL_COMPILER_BUILD_DATE)
#	define SPRAWL_COMPILER_MSVC 0
#	define SPRAWL_COMPILER_CLANG 0
#	define SPRAWL_COMPILER_GCC 0
#	define SPRAWL_COMPILER_INTEL 1
#	define SPRAWL_COMPILER_GNU_COMPAT 1
#elif defined(__clang__)
#	define SPRAWL_COMPILER_MSVC 0
#	define SPRAWL_COMPILER_CLANG 1
#	define SPRAWL_COMPILER_GCC 0
#	define SPRAWL_COMPILER_INTEL 0
#	define SPRAWL_COMPILER_GNU_COMPAT 1
#elif defined(__GNUC__)
#	define SPRAWL_COMPILER_MSVC 0
#	define SPRAWL_COMPILER_CLANG 0
#	define SPRAWL_COMPILER_GCC 1
#	define SPRAWL_COMPILER_INTEL 0
#	define SPRAWL_COMPILER_GNU_COMPAT 1
#endif

#if SPRAWL_COMPILER_GNU_COMPAT
#	define SPRAWL_LIKELY(x) (__builtin_expect(!!(x), 1))
#	define SPRAWL_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#	define SPRAWL_LIKELY(x) (x)
#	define SPRAWL_UNLIKELY(x) (x)
#endif

#if SPRAWL_COMPILER_GNU_COMPAT
#	define SPRAWL_MEMCMP __builtin_memcmp
#	ifndef SPRAWL_MULTITHREADED
#		ifdef _REENTRANT
#			define SPRAWL_MULTITHREADED 1
#		else
#			define SPRAWL_MULTITHREADED 0
#		endif
#	endif
#	define SPRAWL_FORCE_INLINE inline __attribute__((always_inline))
#	define SPRAWL_FORCE_NO_INLINE __attribute__((noinline))
#else
#	define SPRAWL_MEMCMP memcmp
#	ifndef SPRAWL_MULTITHREADED
#		ifdef _MT
#			define SPRAWL_MULTITHREADED 1
#		else
#			define SPRAWL_MULTITHREADED 0
#		endif
#	endif
#	define SPRAWL_FORCE_INLINE inline __forceinline
#	define SPRAWL_FORCE_NO_INLINE __declspec(noinline)
#endif

#define SPRAWL_CONCAT_2(left, right) left ## right
#define SPRAWL_CONCAT(left, right) SPRAWL_CONCAT_2(left, right)

#if defined(_WIN32)
#	include <BaseTsd.h>
	typedef SSIZE_T ssize_t;
#endif

#if defined(_WIN32)
	#define SPRAWL_I64FMT "ll"
#elif defined(__APPLE__)
	#define SPRAWL_I64FMT "ll"
#else
	#define SPRAWL_I64FMT "l"
#endif

//Detecting debug settings works as follows:
// 1) If __OPTIMIZE__ is defined, DEBUG is 0 regardless of system
// 2) If _DEBUG is defined, DEBUG is 1 regardless of system
// 3) If neither of those is defined, DEBUG is 0 for Windows (which makes a define for when something IS debug)
//    or 1 otherwise (where a define is made when something IS NOT debug)
#if defined(__OPTIMIZE__)
#	define SPRAWL_DEBUG 0
#elif defined(_DEBUG)
#	define SPRAWL_DEBUG 1
#else
#	ifdef _WIN32
#		define SPRAWL_DEBUG 0
#	else
#		define SPRAWL_DEBUG 1
#	endif
#endif

#define SPRAWL_CONSTEXPR_INCLASS_INIT(...) = __VA_ARGS__
#define SPRAWL_CONSTEXPR_OUT_OF_CLASS_INIT(...)
#define CONSTEXPR_ARRAY constexpr

#define SPRAWL_DEFINE_COMPILE_ERROR(type, text) template<bool t_Check> struct type { static_assert(t_Check, text); }

#if !defined(SPRAWL_BIG_ENDIAN) || !defined(SPRAWL_LITTLE_ENDIAN)
#	if (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || \
    (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIPSEB) || \
    defined(__MIPSEB) || \
    defined(__MIPSEB__) || \
	(defined(_MSC_VER) && defined(_M_PPC))
#		define SPRAWL_BIG_ENDIAN 1
#		define SPRAWL_LITTLE_ENDIAN 0
#	elif (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || \
    (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || \
    defined(__MIPSEL) || \
    defined(__MIPSEL__) || \
	(defined(_MSC_VER) && !defined(_M_PPC))
#		define SPRAWL_BIG_ENDIAN 0
#		define SPRAWL_LITTLE_ENDIAN 1
#	else
#		error "Could not detect endianness - please define both SPRAWL_BIG_ENDIAN and SPRAWL_LITTLE_ENDIAN"
#	endif
#endif

#if SPRAWL_BIG_ENDIAN && SPRAWL_LITTLE_ENDIAN
#	error "SPRAWL_BIG_ENDIAN and SPRAWL_LITTLE_ENDIAN are both true! One must be false!"
#elif !SPRAWL_BIG_ENDIAN && !SPRAWL_LITTLE_ENDIAN
#	error "SPRAWL_BIG_ENDIAN and SPRAWL_LITTLE_ENDIAN are both false! One must be true!"
#endif
