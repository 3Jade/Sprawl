#include "CityHash.hpp"
#include <time.h>
#include <stdint.h>
#include <string.h>
#include "../common/compat.hpp"

#ifndef __BIG_ENDIAN__

#define INORDER32(x) (x)
#define INORDER64(x) (x)

#else

#ifdef _MSC_VER
#	include <stdlib.h>
#	define bswap_32(x) _byteswap_ulong(x)
#	define bswap_64(x) _byteswap_uint64(x)
#elif defined(__APPLE__)
#	include <libkern/OSByteOrder.h>
#	define bswap_32(x) OSSwapInt32(x)
#	define bswap_64(x) OSSwapInt64(x)
#else
#	include <byteswap.h>
#endif

#define INORDER32(x) (bswap_32(x))
#define INORDER64(x) (bswap_64(x))

#endif  // __BIG_ENDIAN__

namespace sprawl
{
	namespace cityhash
	{
		namespace CityHashStatic
		{
			struct uint128_t
			{
				uint64_t first;
				uint64_t second;
				uint128_t(uint64_t first_, uint64_t second_)
					: first(first_)
					, second(second_)
				{
					//
				}
			};

			static const uint64_t k0 = 0xc3a5c85c97cb3127ULL;
			static const uint64_t k1 = 0xb492b66fbe98f273ULL;
			static const uint64_t k2 = 0x9ae16a3b2f90404fULL;
			static const uint64_t k3 = 0xc949d7c7509e6557ULL;

			static inline uint64_t LoadUnaligned_32(const char *p)
			{
				uint64_t result;
				memcpy(&result, p, sizeof(result));
				return result;
			}

			static inline uint64_t LoadUnaligned_64(const char *p)
			{
				uint32_t result;
				memcpy(&result, p, sizeof(result));
				return result;
			}

			static inline uint64_t Fetch64(const char *p)
			{
				return INORDER64(LoadUnaligned_64(p));
			}

			static inline uint32_t Fetch32(const char *p)
			{
				return INORDER32(LoadUnaligned_32(p));
			}

			static inline uint64_t Rotate(uint64_t val, int shift)
			{
				return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
			}

			static inline uint64_t RotateByAtLeastOne(uint64_t val, int shift)
			{
				return (val >> shift) | (val << (64 - shift));
			}

			static inline uint64_t ShiftMix(uint64_t val)
			{
				return val ^ (val >> 47);
			}

			static inline uint64_t Hash128to64(uint128_t const& x)
			{
				const uint64_t kMul = 0x9ddfea08eb382d69ULL;
				uint64_t a = (x.first ^ x.second) * kMul;
				a ^= (a >> 47);
				uint64_t b = (x.second ^ a) * kMul;
				b ^= (b >> 47);
				b *= kMul;
				return b;
			}

			static inline uint64_t Hash16(uint64_t u, uint64_t v)
			{
				return Hash128to64(uint128_t(u, v));
			}

			static uint64_t Hash0to16(char const* const data, size_t length)
			{
				if(length > 8)
				{
					uint64_t a = Fetch64(data);
					uint64_t b = Fetch64(data + length - 8);
					return Hash16(a, RotateByAtLeastOne(b + length, length)) ^ b;
				}
				if(length >= 4)
				{
					uint64_t a = Fetch32(data);
					return Hash16(length + (a << 3), Fetch32(data + length - 4));
				}
				if(length > 0)
				{
					uint8_t a = data[0];
					uint8_t b = data[length >> 1];
					uint8_t c = data[length - 1];
					uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
					uint32_t z = length + (static_cast<uint32_t>(c) << 2);
					return ShiftMix(y * k2 ^ z * k3) * k2;
				}
				return k2;
			}

			static uint64_t Hash17to32(char const* const data, size_t length)
			{
				uint64_t a = Fetch64(data) * k1;
				uint64_t b = Fetch64(data + 8);
				uint64_t c = Fetch64(data + length - 8) * k2;
				uint64_t d = Fetch64(data + length - 16) * k0;
				return Hash16(Rotate(a - b, 43) + Rotate(c, 30) + d, a + Rotate(b ^ k3, 20) - c + length);
			}

			static uint64_t Hash33to64(char const* const data, size_t length)
			{
				uint64_t z = Fetch64(data + 24);
				uint64_t a = Fetch64(data) + (length + Fetch64(data + length - 16)) * k0;
				uint64_t b = Rotate(a + z, 52);
				uint64_t c = Rotate(a, 37);
				a += Fetch64(data + 8);
				c += Rotate(a, 7);
				a += Fetch64(data + 16);
				uint64_t vf = a + z;
				uint64_t vs = b + Rotate(a, 31) + c;
				a = Fetch64(data + 16) + Fetch64(data + length - 32);
				z = Fetch64(data + length - 8);
				b = Rotate(a + z, 52);
				c = Rotate(a, 37);
				a += Fetch64(data + length - 24);
				c += Rotate(a, 7);
				a += Fetch64(data + length - 16);
				uint64_t wf = a + z;
				uint64_t ws = b + Rotate(a, 31) + c;
				uint64_t r = ShiftMix((vf + ws) * k2 + (wf + vs) * k0);
				return ShiftMix(r * k0 + vs) * k2;
			}

			static inline uint128_t WeakHashLen32WithSeeds(uint64_t w, uint64_t x, uint64_t y, uint64_t z, uint64_t a, uint64_t b)
			{
				a += w;
				b = Rotate(b + a + z, 21);
				uint64_t c = a;
				a += x;
				a += y;
				b += Rotate(a, 44);
				return uint128_t(a + z, b + c);
			}

			static inline uint128_t WeakHashLen32WithSeeds(char const* const data, uint64_t a, uint64_t b)
			{
				return WeakHashLen32WithSeeds(
					Fetch64(data),
					Fetch64(data + 8),
					Fetch64(data + 16),
					Fetch64(data + 24),
					a,
					b
				);
			}
		}

		size_t Hash(void const* data, size_t length)
		{
			const char* data_ = static_cast<char const*>(data);
			if(length <= 32)
			{
				if(length <= 16)
				{
					return CityHashStatic::Hash0to16(data_, length);
				}
				return CityHashStatic::Hash17to32(data_, length);
			}
			else if(length <= 64)
			{
				return CityHashStatic::Hash33to64(data_, length);
			}

			uint64_t x = CityHashStatic::Fetch64(data_ + length - 40);
			uint64_t y = CityHashStatic::Fetch64(data_ + length - 16) + CityHashStatic::Fetch64(data_ + length - 56);
			uint64_t z = CityHashStatic::Hash16(CityHashStatic::Fetch64(data_ + length - 48) + length, CityHashStatic::Fetch64(data_ + length - 24));
			CityHashStatic::uint128_t v = CityHashStatic::WeakHashLen32WithSeeds(data_ + length - 64, length, z);
			CityHashStatic::uint128_t w = CityHashStatic::WeakHashLen32WithSeeds(data_ + length - 32, y + CityHashStatic::k1, x);
			x = x * CityHashStatic::k1 + CityHashStatic::Fetch64(data_);

			length = (length - 1) & ~static_cast<size_t>(63);

			do
			{
				x = CityHashStatic::Rotate(x + y + v.first + CityHashStatic::Fetch64(data_ + 8), 37) * CityHashStatic::k1;
				y = CityHashStatic::Rotate(y + v.second + CityHashStatic::Fetch64(data_ + 48), 42) * CityHashStatic::k1;
				x ^= w.second;
				y += v.first + CityHashStatic::Fetch64(data_ + 40);
				z = CityHashStatic::Rotate(z + w.first, 33) * CityHashStatic::k1;
				v = CityHashStatic::WeakHashLen32WithSeeds(data_, v.second * CityHashStatic::k1, x + w.first);
				w = CityHashStatic::WeakHashLen32WithSeeds(data_ + 32, z + w.second, y + CityHashStatic::Fetch64(data_ + 16));

				uint64_t temp = z;
				z = x;
				x = temp;

				data_ += 64;
				length -= 64;
			  } while (length != 0);
			  return CityHashStatic::Hash16(CityHashStatic::Hash16(v.first, w.first) + CityHashStatic::ShiftMix(y) * CityHashStatic::k1 + z, CityHashStatic::Hash16(v.second, w.second) + x);
		}
	}
}
