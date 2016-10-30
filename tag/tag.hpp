#pragma once

#include <type_traits>
#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

#include "../common/compat.hpp"
#include "../if/if.hpp"

#ifndef SPRAWL_CHR_MAX
#define SPRAWL_CHR_MAX 64
#endif

static_assert(SPRAWL_CHR_MAX >= 2 && (SPRAWL_CHR_MAX & (SPRAWL_CHR_MAX - 1)) == 0 && SPRAWL_CHR_MAX <= 32768, "SPRAWL_CHR_MAX must be a power of 2 between 2 and 32768");

#define SPRAWL_CHR(str, size, idx) (idx < size ? str[idx] : '\0')

#define SPRAWL_CHR_2(str, size, idx) SPRAWL_CHR(str, size, idx), SPRAWL_CHR(str, size, idx+1)

#if SPRAWL_CHR_MAX >= 4
#define SPRAWL_CHR_4(str, size, idx) SPRAWL_CHR_2(str, size, idx), SPRAWL_CHR_2(str, size, idx+2)
#endif
#if SPRAWL_CHR_MAX >= 8
#define SPRAWL_CHR_8(str, size, idx) SPRAWL_CHR_4(str, size, idx), SPRAWL_CHR_4(str, size, idx+4)
#endif
#if SPRAWL_CHR_MAX >= 16
#define SPRAWL_CHR_16(str, size, idx) SPRAWL_CHR_8(str, size, idx), SPRAWL_CHR_8(str, size, idx+8)
#endif
#if SPRAWL_CHR_MAX >= 32
#define SPRAWL_CHR_32(str, size, idx) SPRAWL_CHR_16(str, size, idx), SPRAWL_CHR_16(str, size, idx+16)
#endif
#if SPRAWL_CHR_MAX >= 64
#define SPRAWL_CHR_64(str, size, idx) SPRAWL_CHR_32(str, size, idx), SPRAWL_CHR_32(str, size, idx+32)
#endif
#if SPRAWL_CHR_MAX >= 128
#define SPRAWL_CHR_128(str, size, idx) SPRAWL_CHR_64(str, size, idx), SPRAWL_CHR_64(str, size, idx+64)
#endif
#if SPRAWL_CHR_MAX >= 256
#define SPRAWL_CHR_256(str, size, idx) SPRAWL_CHR_128(str, size, idx), SPRAWL_CHR_128(str, size, idx+128)
#endif
#if SPRAWL_CHR_MAX >= 512
#define SPRAWL_CHR_512(str, size, idx) SPRAWL_CHR_256(str, size, idx), SPRAWL_CHR_256(str, size, idx+256)
#endif
#if SPRAWL_CHR_MAX >= 1024
#define SPRAWL_CHR_1024(str, size, idx) SPRAWL_CHR_512(str, size, idx), SPRAWL_CHR_512(str, size, idx+512)
#endif
#if SPRAWL_CHR_MAX >= 2048
#define SPRAWL_CHR_2048(str, size, idx) SPRAWL_CHR_1024(str, size, idx), SPRAWL_CHR_1024(str, size, idx+1024)
#endif
#if SPRAWL_CHR_MAX >= 4096
#define SPRAWL_CHR_4096(str, size, idx) SPRAWL_CHR_2048(str, size, idx), SPRAWL_CHR_2048(str, size, idx+2048)
#endif
#if SPRAWL_CHR_MAX >= 8192
#define SPRAWL_CHR_8192(str, size, idx) SPRAWL_CHR_4096(str, size, idx), SPRAWL_CHR_4096(str, size, idx+4096)
#endif
#if SPRAWL_CHR_MAX >= 16384
#define SPRAWL_CHR_16384(str, size, idx) SPRAWL_CHR_8192(str, size, idx), SPRAWL_CHR_8192(str, size, idx+8192)
#endif
#if SPRAWL_CHR_MAX >= 32768
#define SPRAWL_CHR_32768(str, size, idx) SPRAWL_CHR_16384(str, size, idx), SPRAWL_CHR_16384(str, size, idx+16384)
#endif

#define SPRAWL_CHR_MACRO_2(val) SPRAWL_CHR_ ## val
#define SPRAWL_CHR_MACRO(val) SPRAWL_CHR_MACRO_2(val)
#define SPRAWL_CHR_MAX_MACRO SPRAWL_CHR_MACRO(SPRAWL_CHR_MAX)

#define SPRAWL_TAG(s) ::sprawl::detail::TagWrapper< ::sprawl::detail::SizeChecker<sizeof(s) - 1>::value, 1, ::sprawl::Tag<0>, ::sprawl::detail::IsPositive<sizeof(s) - 1>::value, true, SPRAWL_CHR_MAX_MACRO(s, sizeof(s), 0)>::type

#include "detail/tag_detail.hpp"
#include "type_list.hpp"

namespace sprawl
{
	//MSVC likes to complain about integer constant overflow here, but hash algorithms use that intentionally so this is being unilaterally disabled for this section.
#if SPRAWL_COMPILER_MSVC
#	pragma warning(push)
#	pragma warning(disable: 4307)
#endif
#if SPRAWL_64_BIT
	template<size_t t_Seed>
	struct Murmur3
	{
		template<char... t_Chars>
		struct Hash
		{
			static constexpr size_t outputHash1 = detail::murmur3::Murmur3<t_Seed, t_Chars...>::value;
			static constexpr size_t outputHash2 = outputHash1 ^ sizeof...(t_Chars);
			static constexpr size_t outputHash3 = outputHash2 ^ (outputHash2 >> 33);
			static constexpr size_t outputHash4 = outputHash3 * 0xff51afd7ed558ccdULL;
			static constexpr size_t outputHash5 = outputHash4 ^ (outputHash4 >> 33);
			static constexpr size_t outputHash6 = outputHash5 * 0xc4ceb9fe1a85ec53ULL;
			static constexpr size_t value = outputHash6 ^ (outputHash6 >> 33);
		};
	};
#else
	template<size_t t_Seed>
	struct Murmur3
	{
		template<char... t_Chars>
		struct Hash
		{
			static constexpr size_t outputHash1 = detail::murmur3::Murmur3<t_Seed, t_Chars...>::value;
			static constexpr size_t outputHash2 = outputHash1 ^ sizeof...(t_Chars);
			static constexpr size_t outputHash3 = outputHash2 ^ (outputHash2 >> 16);
			static constexpr size_t outputHash4 = outputHash3 * 0x85ebca6b;
			static constexpr size_t outputHash5 = outputHash4 ^ (outputHash4 >> 13);
			static constexpr size_t outputHash6 = outputHash5 * 0xc2b2ae35;
			static constexpr size_t value = outputHash6 ^ (outputHash6 >> 16);
		};
	};
#endif
#if SPRAWL_COMPILER_MSVC
#	pragma warning(pop)
#endif

	template<char... t_Chars>
	struct CharIn
	{
		template<char t_CheckChar>
		using Check = detail::CharIn<t_CheckChar, t_Chars...>;
	};

	struct IsWhitespace
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = detail::CharIn<t_CheckChar, ' ', '\n', '\t', '\v', '\f', '\r'>::value;
		};
	};

	struct IsLineEnding
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = detail::CharIn<t_CheckChar, '\n', '\r'>::value;
		};
	};

	struct IsDigit
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = detail::CharIn<t_CheckChar, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'>::value;
		};
	};

	struct IsUpper
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = ('A' <= t_CheckChar && t_CheckChar <= 'Z');
		};
	};

	struct IsLower
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = ('a' <= t_CheckChar && t_CheckChar <= 'z');
		};
	};

	struct IsAlpha
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = IsUpper::Check<t_CheckChar>::value || IsLower::Check<t_CheckChar>::value;
		};
	};

	struct IsAlnum
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = IsAlpha::Check<t_CheckChar>::value || IsDigit::Check<t_CheckChar>::value;
		};
	};

	struct IsPrintable
	{
		template<char t_CheckChar>
		struct Check
		{
			static constexpr bool value = (32 <= t_CheckChar && t_CheckChar <= 127) || t_CheckChar < 0;
		};
	};

	struct IsTitle
	{
		template<char t_PreviousChar, char t_CheckChar>
		struct ComplexCheck
		{
			static constexpr bool value = IsUpper::Check<t_CheckChar>::value ? (IsWhitespace::Check<t_PreviousChar>::value || t_PreviousChar == -1) : !(IsWhitespace::Check<t_PreviousChar>::value || t_PreviousChar == -1);
		};
	};

	template<char t_PreviousChar, char t_CurrentChar>
	struct Upper
	{
		static constexpr char value = (('a' <= t_CurrentChar && t_CurrentChar <= 'z') ? ((t_CurrentChar - 'a') + 'A') : t_CurrentChar);
	};

	template<char t_PreviousChar, char t_CurrentChar>
	struct Lower
	{
		static constexpr char value = (('A' <= t_CurrentChar && t_CurrentChar <= 'Z') ? ((t_CurrentChar - 'A') + 'a') : t_CurrentChar);
	};

	template<char t_PreviousChar, char t_CurrentChar>
	struct SwapCase
	{
		static constexpr char value = (('A' <= t_CurrentChar && t_CurrentChar <= 'Z') ? ((t_CurrentChar - 'A') + 'a') : ('a' <= t_CurrentChar && t_CurrentChar <= 'z') ? ((t_CurrentChar - 'a') + 'A') : t_CurrentChar);
	};

	template<char t_PreviousChar, char t_CurrentChar>
	struct Title
	{
		static constexpr char value = t_PreviousChar == -1 || IsWhitespace::Check<t_PreviousChar>::value ? Upper<t_PreviousChar, t_CurrentChar>::value : Lower<t_PreviousChar, t_CurrentChar>::value;
	};

	template<char t_PreviousChar, char t_CurrentChar>
	struct Capitalize
	{
		static constexpr char value = t_PreviousChar == -1 ? Upper<t_PreviousChar, t_CurrentChar>::value : Lower<t_PreviousChar, t_CurrentChar>::value;
	};

	template<ssize_t t_Len, char... t_Chars>
	struct Tag
	{
	public:
		static constexpr ssize_t length = t_Len;
		static CONSTEXPR_ARRAY char name[t_Len + 1] SPRAWL_CONSTEXPR_INCLASS_INIT({ t_Chars..., '\0' });
		typedef ssize_t length_type;
		typedef char char_type;

		constexpr Tag(){}

		template<ssize_t t_Idx, ssize_t t_Length = t_Len - t_Idx>
		using Substring = typename detail::TagWrapper<(t_Idx < 0 ? 0 : (t_Length >(t_Len - t_Idx)) ? (t_Len - t_Idx) : t_Length), t_Idx < 0 ? 0 : (1 - t_Idx), Tag<0>, detail::IsPositive<t_Idx < 0 ? 0 : ((t_Length >(t_Len - t_Idx)) ? (t_Len - t_Idx) : t_Length)>::value, detail::IsPositive<t_Idx < 0 ? 0 : (1 - t_Idx)>::value, t_Chars...>::type;

		template<typename t_OtherTagType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t Find()
		{
			return detail::FindTag<Substring<t_Start, t_End - t_Start>, t_OtherTagType, 0>::result != -1 ? detail::FindTag<Substring<t_Start, t_End - t_Start>, t_OtherTagType, 0>::result + t_Start : -1;
		}


		template<ssize_t t_Length>
		static constexpr ssize_t Find(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return t_Length - 1 <= Tag::length && t_Length - 1 <= (end - start) ? find_(0, start, end, str, t_Chars...) : -1;
		}

		template<typename t_OtherTagType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t RFind()
		{
			return detail::RFindTag<Substring<t_Start, t_End - t_Start>, t_OtherTagType, 0>::result != -1 ? detail::RFindTag<Substring<t_Start, t_End - t_Start>, t_OtherTagType, 0>::result + t_Start : -1;
		}

		template<ssize_t t_Length>
		static constexpr ssize_t RFind(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return t_Length - 1 <= Tag::length && t_Length - 1 <= (end - start) ? rfind_(0, start, end, str, t_Chars...) : -1;
		}

		template<typename t_PredType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindFirstOf()
		{
			return detail::FindPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result != -1 ? detail::FindPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result + t_Start : -1;
		}

		template<ssize_t t_Length>
		static constexpr ssize_t FindFirstOf(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return findFirstOf_(0, start, end, str, t_Chars...);
		}

		template<typename t_PredType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindLastOf()
		{
			return detail::RFindPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result != -1 ? detail::RFindPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result + t_Start : -1;
		}

		template<ssize_t t_Length>
		static constexpr ssize_t FindLastOf(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return findLastOf_(0, start, end, str, t_Chars...);
		}

		template<typename t_PredType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindFirstNotOf()
		{
			return detail::FindNotPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result != -1 ? detail::FindNotPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result + t_Start : -1;
		}

		template<ssize_t t_Length>
		static constexpr ssize_t FindFirstNotOf(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return findFirstNotOf_(0, start, end, str, t_Chars...);
		}

		template<typename t_PredType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindLastNotOf()
		{
			return detail::RFindNotPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result != -1 ? detail::RFindNotPred<Substring<t_Start, t_End - t_Start>, t_PredType, 0>::result + t_Start : -1;
		}

		template<ssize_t t_Length>
		static constexpr ssize_t FindLastNotOf(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return findLastNotOf_(0, start, end, str, t_Chars...);
		}

		template<template <char t_PreviousChar, char t_CurrentChar> class t_TransformType>
		using Transform = typename detail::TransformTag<t_TransformType, Tag<0>, -1, t_Chars...>::type;

		//Using as a namespace so following namespace naming conventions despite being a struct.
		struct tag_detail
		{
			template<typename t_OtherTagType>
			struct AppendTagHelper
			{

			};

			template<ssize_t t_OtherLength, char... t_OtherChars>
			struct AppendTagHelper<Tag<t_OtherLength, t_OtherChars...>>
			{
				typedef Tag<t_Len + t_OtherLength, t_Chars..., t_OtherChars...> type;
			};

#if SPRAWL_COMPILER_MSVC
			template<ssize_t t_TargetIdx, bool t_CharWithinBounds, char... t_AdditionalChars>
			struct CharAt;

			template<ssize_t t_TargetIdx, bool t_CharWithinBounds, char t_FirstChar, char... t_AdditionalChars>
			struct CharAt<t_TargetIdx, t_CharWithinBounds, t_FirstChar, t_AdditionalChars...>
			{
				static constexpr char value = t_TargetIdx < 0 ? -1 :
					t_TargetIdx == 0 ? t_FirstChar :
					CharAt<t_TargetIdx - 1, t_CharWithinBounds, t_AdditionalChars...>::value;
			};

			template<ssize_t t_TargetIdx, bool t_CharWithinBounds, char t_FirstChar>
			struct CharAt<t_TargetIdx, t_CharWithinBounds, t_FirstChar>
			{
				static constexpr char value = t_TargetIdx < 0 ? -1 : t_TargetIdx == 0 ? t_FirstChar : -1;
			};

			template<ssize_t t_TargetIdx, bool t_CharWithinBounds>
			struct CharAt<t_TargetIdx, t_CharWithinBounds>
			{
				static constexpr char value = -1;
			};

			template<char t_NextChar, bool t_CharWithinBounds>
			struct CharAt<-1, t_CharWithinBounds, t_NextChar>
			{
				static constexpr char value = -1;
			};
#else
			template<ssize_t t_TargetIdx, bool t_CharWithinBounds, char... t_Ignored>
			struct CharAt
			{
				static constexpr char value = name[t_TargetIdx];
			};

			template<ssize_t t_TargetIdx, char... t_Ignored>
			struct CharAt<t_TargetIdx, false, t_Ignored...>
			{
				static constexpr char value = -1;
			};
#endif

			template<typename t_PredicateType, ssize_t t_Idx = 0, bool t_ShouldStrip = t_PredicateType::template Check<CharAt<t_Idx, t_Idx >= 0 && t_Idx <= t_Len, t_Chars...>::value>::value>
			struct LStripHelper
			{
				typedef typename LStripHelper<t_PredicateType, t_Idx + 1>::type type;
			};

			template<typename t_PredicateType, ssize_t t_Idx>
			struct LStripHelper<t_PredicateType, t_Idx, false>
			{
				typedef Substring<t_Idx> type;
			};

			template<typename t_PredicateType, ssize_t t_Idx = 0, bool t_ShouldStrip = t_PredicateType::template Check<CharAt<t_Len - t_Idx - 1, (t_Len - t_Idx - 1) >= 0 && (t_Len - t_Idx - 1) <= t_Len, t_Chars...>::value>::value>
			struct RStripHelper
			{
				typedef typename RStripHelper<t_PredicateType, t_Idx + 1>::type type;
			};

			template<typename t_PredicateType, ssize_t t_Idx>
			struct RStripHelper<t_PredicateType, t_Idx, false>
			{
				typedef Substring<0, t_Len - t_Idx> type;
			};

			template<typename t_SearchStringType, ssize_t t_SplitLocation>
			struct PartitionHelper
			{
				typedef TypeList<Substring<0, t_SplitLocation>, t_SearchStringType, Substring<t_SplitLocation + t_SearchStringType::length>> type;
			};
			template<typename t_SearchStringType>
			struct PartitionHelper<t_SearchStringType, -1>
			{
				typedef TypeList<Tag, SPRAWL_TAG(""), SPRAWL_TAG("")> type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelper
			{
				typedef typename SplitHelper<
					typename t_TagToSearchType::template Substring<t_SplitLocation + t_SearchStringType::length>,
					t_SearchStringType,
					typename t_TypeSoFar::template Append<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>
					>,
					t_TagToSearchType::template Substring<t_SplitLocation + t_SearchStringType::length>::template Find<t_SearchStringType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct SplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct SplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperInList
			{
				typedef typename SplitHelperInList<
					typename t_TagToSearchType::template Substring<
					t_SplitLocation + detail::MatchLength<
					t_TagToSearchType, t_SplitLocation, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>
					>::value
					>,
					t_AccessibleByGetType,
					typename t_TypeSoFar::template Append<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>
					>,
					t_TagToSearchType::template Substring<
					t_SplitLocation + detail::MatchLength<
					t_TagToSearchType, t_SplitLocation, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>
					>::value
					>::template FindFirstInList<t_AccessibleByGetType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct SplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct SplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};


			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperAnyOf
			{
				typedef typename SplitHelperAnyOf<
					typename t_TagToSearchType::template Substring<t_SplitLocation + 1>,
					t_PredType,
					typename t_TypeSoFar::template Append<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>
					>,
					t_TagToSearchType::template Substring<t_SplitLocation + 1>::template FindFirstOf<t_PredType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct SplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct SplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperAnyNotOf
			{
				typedef typename SplitHelperAnyNotOf<
					typename t_TagToSearchType::template Substring<t_SplitLocation + 1>,
					t_PredType,
					typename t_TypeSoFar::template Append<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>
					>,
					t_TagToSearchType::template Substring<t_SplitLocation + 1>::template FindFirstNotOf<t_PredType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct SplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct SplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct SplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};


			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelper
			{
				typedef typename RSplitHelper<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>,
					t_SearchStringType,
					typename TypeList<typename t_TagToSearchType::template Substring<t_SplitLocation + t_SearchStringType::length>>::template Extend<t_TypeSoFar>,
					t_TagToSearchType::template Substring<0, t_SplitLocation>::template RFind<t_SearchStringType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct RSplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_SearchStringType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct RSplitHelper<t_TagToSearchType, t_SearchStringType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};


			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperInList
			{
				typedef typename RSplitHelperInList<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>,
					t_AccessibleByGetType,
					typename TypeList<typename t_TagToSearchType::template Substring<t_SplitLocation + detail::MatchLength<t_TagToSearchType, t_SplitLocation, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>>::value>>::template Extend<t_TypeSoFar>,
					t_TagToSearchType::template Substring<0, t_SplitLocation>::template FindLastInList<t_AccessibleByGetType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct RSplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct RSplitHelperInList<t_TagToSearchType, t_AccessibleByGetType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};


			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperAnyOf
			{
				typedef typename RSplitHelperAnyOf<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>,
					t_PredType,
					typename TypeList<typename t_TagToSearchType::template Substring<t_SplitLocation + 1>>::template Extend<t_TypeSoFar>,
					t_TagToSearchType::template Substring<0, t_SplitLocation>::template FindLastOf<t_PredType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct RSplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct RSplitHelperAnyOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};


			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperAnyNotOf
			{
				typedef typename RSplitHelperAnyNotOf<
					typename t_TagToSearchType::template Substring<0, t_SplitLocation>,
					t_PredType,
					typename TypeList<typename t_TagToSearchType::template Substring<t_SplitLocation + 1>>::template Extend<t_TypeSoFar>,
					t_TagToSearchType::template Substring<0, t_SplitLocation>::template FindLastNotOf<t_PredType>(),
					t_CurrentSplit + 1,
					t_MaxSplit
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_CurrentSplit, ssize_t t_MaxSplit>
			struct RSplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_CurrentSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_MaxSplit>
			struct RSplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, t_SplitLocation, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};

			template<typename t_TagToSearchType, typename t_PredType, typename t_TypeSoFar, ssize_t t_MaxSplit>
			struct RSplitHelperAnyNotOf<t_TagToSearchType, t_PredType, t_TypeSoFar, -1, t_MaxSplit, t_MaxSplit>
			{
				typedef typename TypeList<t_TagToSearchType>::template Extend<t_TypeSoFar> type;
			};



			template<typename t_TagToSearchType, typename t_TypeSoFar, ssize_t t_SplitLocation, ssize_t t_EndOfSplit, bool t_KeepEnds>
			struct SplitLinesHelper
			{
				typedef typename SplitLinesHelper<
					typename t_TagToSearchType::template Substring<t_EndOfSplit + 1>,
					typename t_TypeSoFar::template Append<
					typename t_TagToSearchType::template Substring<0, t_KeepEnds ? t_EndOfSplit + 1 : t_SplitLocation>
					>,
					t_TagToSearchType::template Substring<t_EndOfSplit + 1>::template FindFirstOf<sprawl::IsLineEnding>(),
					t_TagToSearchType::template Substring<t_EndOfSplit + 1>
					::template Substring<t_TagToSearchType::template Substring<t_EndOfSplit + 1>::template FindFirstOf<sprawl::IsLineEnding>()>
					::template FindFirstNotOf<sprawl::IsLineEnding>(),
					t_KeepEnds
				>::type type;
			};

			template<typename t_TagToSearchType, typename t_TypeSoFar, ssize_t t_SplitLocation, bool t_KeepEnds>
			struct SplitLinesHelper<t_TagToSearchType, t_TypeSoFar, t_SplitLocation, -1, t_KeepEnds>
			{
				typedef typename t_TypeSoFar::template Append<typename t_TagToSearchType::template Substring<0, t_KeepEnds ? t_TagToSearchType::length : t_SplitLocation>> type;
			};

			template<typename t_TagToSearchType, typename t_TypeSoFar, bool t_KeepEnds>
			struct SplitLinesHelper<t_TagToSearchType, t_TypeSoFar, -1, -1, t_KeepEnds>
			{
				typedef typename t_TypeSoFar::template Append<t_TagToSearchType> type;
			};

			template<typename t_Type, bool integer, bool floating, bool boolean>
			struct As;

			template<typename t_Type>
			struct As<t_Type, true, false, false>
			{
				static constexpr t_Type value = t_Type(detail::TagToInt<t_Chars...>::value);
			};

			template<typename t_Type>
			struct As<t_Type, true, false, true>
			{
				static constexpr bool value = detail::TagToBool<Tag::Transform<sprawl::Lower>>::value;
			};

			template<typename t_Type>
			struct As<t_Type, false, true, false>
			{
				static constexpr t_Type value = t_Type(detail::TagToDouble<t_Chars...>::value);
			};
		};

		template<typename t_PredicateType = IsWhitespace>
		using LStrip = typename tag_detail::template LStripHelper<t_PredicateType>::type;

		template<typename t_PredicateType = IsWhitespace>
		using RStrip = typename tag_detail::template RStripHelper<t_PredicateType>::type;

		template<typename t_PredicateType = IsWhitespace>
		using Strip = typename LStrip<t_PredicateType>::template RStrip<t_PredicateType>;

		template<char t_CharToAdd>
		using AppendChar = Tag<t_Len + 1, t_Chars..., t_CharToAdd>;

		template<typename t_OtherTagType>
		using Append = typename tag_detail::template AppendTagHelper<t_OtherTagType>::type;

		template<ssize_t t_Start, ssize_t t_Length = t_Len - t_Start>
		using Erase = typename Substring<0, t_Start>::template Append<Substring<t_Start + t_Length, t_Len - (t_Start + t_Length)>>;

		template<typename t_AccessibleByGetType>
		using Join = typename detail::JoinTags<Tag, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>>::type;

		template<typename t_AccessibleByGetType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindFirstInList()
		{
			return detail::FindTagInList<Substring<t_Start, t_End - t_Start>, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>>::value;
		}

		template<typename t_AccessibleByGetType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t FindLastInList()
		{
			return detail::RFindTagInList<Substring<t_Start, t_End - t_Start>, t_AccessibleByGetType, 0, detail::HasGet<t_AccessibleByGetType, 1>>::value;
		}

		static constexpr bool IsAlnum()
		{
			return detail::MeetsCondition<sprawl::IsAlnum, t_Chars...>::value;
		}

		static constexpr bool IsAlpha()
		{
			return detail::MeetsCondition<sprawl::IsAlpha, t_Chars...>::value;
		}

		static constexpr bool IsDigit()
		{
			return detail::MeetsCondition<sprawl::IsDigit, t_Chars...>::value;
		}

		static constexpr bool IsLower()
		{
			return detail::MeetsCondition<sprawl::IsLower, t_Chars...>::value;
		}

		static constexpr bool IsSpace()
		{
			return detail::MeetsCondition<sprawl::IsWhitespace, t_Chars...>::value;
		}

		static constexpr bool IsUpper()
		{
			return detail::MeetsCondition<sprawl::IsUpper, t_Chars...>::value;
		}

		static constexpr bool IsPrintable()
		{
			return detail::MeetsCondition<sprawl::IsPrintable, t_Chars...>::value;
		}

		static constexpr bool IsTitle()
		{
			return detail::MeetsComplexCondition<sprawl::IsTitle, -1, t_Chars...>::value;
		}

		template<typename t_OtherTagType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr bool StartsWith()
		{
			return t_OtherTagType::length <= length && t_OtherTagType::length <= (t_End - t_Start) && Substring<t_Start, t_End - t_Start>::template Substring<0, t_OtherTagType::length>::template EqualTo<t_OtherTagType>();
		}

		template<typename t_OtherTagType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr bool EndsWith()
		{
			return t_OtherTagType::length <= length && t_OtherTagType::length <= (t_End - t_Start) && Substring<t_Start, t_End - t_Start>::template Substring<Substring<t_Start, t_End - t_Start>::length - t_OtherTagType::length, t_OtherTagType::length>::template EqualTo<t_OtherTagType>();
		}

		template<ssize_t t_Length>
		static constexpr bool StartsWith(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
#if SPRAWL_COMPILER_MSVC
			(void)(str);
			(void)(start);
			(void)(end);
#endif
			return t_Length - 1 <= Tag::length && t_Length - 1 <= (end - start) && equalTo_(0, t_Length - 1, start, str, t_Chars...);
		}

		template<ssize_t t_Length>
		static constexpr bool EndsWith(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
#if SPRAWL_COMPILER_MSVC
			(void)(str);
			(void)(start);
			(void)(end);
#endif
			return t_Length - 1 <= Tag::length && t_Length - 1 <= (end - start) && equalTo_(0, t_Length - 1, end - (t_Length - 1), str, t_Chars...);
		}

		template<typename t_OtherTagType>
		static constexpr bool Contains()
		{
			return detail::FindTag<Tag, t_OtherTagType, 0>::result != -1;
		}

		template<size_t t_Length>
		static constexpr bool Contains(char const (&str)[t_Length])
		{
			return Find(str) != -1;
		}

		template<typename t_OtherTagType, ssize_t t_Start = 0, ssize_t t_End = t_Len>
		static constexpr ssize_t Count()
		{
			return detail::CountTag<Tag, t_OtherTagType, t_Start, t_End>::value;
		}

		template<size_t t_Length>
		static constexpr ssize_t Count(char const (&str)[t_Length], ssize_t start = 0, ssize_t end = t_Len)
		{
			return Find(str, start, end) != -1 ? 1 + Count(str, Find(str, start, end) + 1, end) : 0;
		}

		template<ssize_t t_Idx>
		static constexpr char CharAt()
		{
			return tag_detail::template CharAt<t_Idx, t_Idx >= 0 && t_Idx <= t_Len, t_Chars...>::value;
		}

		template<typename t_Type>
		static constexpr t_Type As()
		{
			return tag_detail::template As<t_Type, std::is_integral<t_Type>::value, std::is_floating_point<t_Type>::value, std::is_same<t_Type, bool>::value>::value;
		}

		template<typename t_FindTagType, typename t_ReplaceWithType, ssize_t count = -1>
		using Replace = typename detail::ReplaceTags<Tag, t_FindTagType, t_ReplaceWithType, count>::type;

		template<typename t_FindTagType, typename t_ReplaceWithType, ssize_t count = -1>
		using RReplace = typename detail::RReplaceTags<Tag, t_FindTagType, t_ReplaceWithType, count>::type;

		template<typename t_TagListType, typename t_ReplaceWithType, ssize_t count = -1>
		using ReplaceAnyInList = typename detail::ReplaceTagsInList<Tag, t_TagListType, t_ReplaceWithType, count>::type;

		template<typename t_TagListType, typename t_ReplaceWithType, ssize_t count = -1>
		using RReplaceAnyInList = typename detail::RReplaceTagsInList<Tag, t_TagListType, t_ReplaceWithType, count>::type;

		template<typename t_PredType, typename t_ReplaceWithType, ssize_t count = -1>
		using ReplaceAnyOf = typename detail::ReplaceTagsAnyOf<Tag, t_PredType, t_ReplaceWithType, count, FindFirstOf<t_PredType>()>::type;

		template<typename t_PredType, typename t_ReplaceWithType, ssize_t count = -1>
		using ReplaceAnyNotOf = typename detail::ReplaceTagsAnyNotOf<Tag, t_PredType, t_ReplaceWithType, count, FindFirstNotOf<t_PredType>()>::type;

		template<typename t_PredType, typename t_ReplaceWithType, ssize_t count = -1>
		using RReplaceAnyOf = typename detail::RReplaceTagsAnyOf<Tag, t_PredType, t_ReplaceWithType, count, FindLastOf<t_PredType>()>::type;

		template<typename t_PredType, typename t_ReplaceWithType, ssize_t count = -1>
		using RReplaceAnyNotOf = typename detail::RReplaceTagsAnyNotOf<Tag, t_PredType, t_ReplaceWithType, count, FindLastNotOf<t_PredType>()>::type;

		template<ssize_t t_Start, ssize_t t_Length, typename t_OtherTagType>
		using ReplaceAt = typename Substring<0, t_Start>::template Append<t_OtherTagType>::template Append<Substring<t_Start + t_Length>>;

		template<typename t_TagToSplitOnType>
		using Partition = typename tag_detail::template PartitionHelper<t_TagToSplitOnType, Find<t_TagToSplitOnType>()>::type;

		template<typename t_TagToSplitOnType>
		using RPartition = typename tag_detail::template PartitionHelper<t_TagToSplitOnType, RFind<t_TagToSplitOnType>()>::type;

		template<typename t_TagToSplitOnType, ssize_t t_MaxSplit = -1>
		using Split = typename tag_detail::template SplitHelper<Tag, t_TagToSplitOnType, TypeList<>, Find<t_TagToSplitOnType>(), 0, t_MaxSplit>::type;

		template<typename t_AccessibleByGetType, ssize_t t_MaxSplit = -1>
		using SplitAnyInList = typename tag_detail::template SplitHelperInList<Tag, t_AccessibleByGetType, TypeList<>, FindFirstInList<t_AccessibleByGetType>(), 0, t_MaxSplit>::type;

		template<typename t_PredType, ssize_t t_MaxSplit = -1>
		using SplitAnyOf = typename tag_detail::template SplitHelperAnyOf<Tag, t_PredType, TypeList<>, FindFirstOf<t_PredType>(), 0, t_MaxSplit>::type;

		template<typename t_PredType, ssize_t t_MaxSplit = -1>
		using SplitAnyNotOf = typename tag_detail::template SplitHelperAnyNotOf<Tag, t_PredType, TypeList<>, FindFirstNotOf<t_PredType>(), 0, t_MaxSplit>::type;

		template<typename t_TagToSplitOnType, ssize_t t_MaxSplit = -1>
		using RSplit = typename tag_detail::template RSplitHelper<Tag, t_TagToSplitOnType, TypeList<>, RFind<t_TagToSplitOnType>(), 0, t_MaxSplit>::type;

		template<typename t_AccessibleByGetType, ssize_t t_MaxSplit = -1>
		using RSplitAnyInList = typename tag_detail::template RSplitHelperInList<Tag, t_AccessibleByGetType, TypeList<>, FindLastInList<t_AccessibleByGetType>(), 0, t_MaxSplit>::type;

		template<typename t_PredType, ssize_t t_MaxSplit = -1>
		using RSplitAnyOf = typename tag_detail::template RSplitHelperAnyOf<Tag, t_PredType, TypeList<>, FindLastOf<t_PredType>(), 0, t_MaxSplit>::type;

		template<typename t_PredType, ssize_t t_MaxSplit = -1>
		using RSplitAnyNotOf = typename tag_detail::template RSplitHelperAnyNotOf<Tag, t_PredType, TypeList<>, FindLastNotOf<t_PredType>(), 0, t_MaxSplit>::type;

		template<bool t_KeepEnds = false>
		using SplitLines = typename tag_detail::template SplitLinesHelper<Tag, TypeList<>, FindFirstOf<sprawl::IsLineEnding>(), Substring<FindFirstOf<sprawl::IsLineEnding>()>::template FindFirstNotOf<sprawl::IsLineEnding>(), t_KeepEnds>::type;

		template<ssize_t t_Idx, typename t_OtherTagType>
		using Insert = typename Substring<0, t_Idx>::template Append<t_OtherTagType>::template Append<Substring<t_Idx>>;

		template<ssize_t t_Length>
		static constexpr bool EqualTo(char const (&str)[t_Length])
		{
			return t_Length - 1 == Tag::length ? equalTo_(0, length, 0, str, t_Chars...) : false;
		}

		template<typename t_OtherTagType>
		static constexpr bool EqualTo()
		{
			return std::is_same<Tag, t_OtherTagType>::value;
		}

		template<typename t_HashType = Murmur3<0>>
		static constexpr auto Hash() -> decltype(t_HashType::template Hash<t_Chars...>::value)
		{
			return t_HashType::template Hash<t_Chars...>::value;
		}

		template<ssize_t t_Start, ssize_t t_End = t_Len, ssize_t t_Interval = 1>
		using Slice = typename detail::SliceTag<
			Tag, 
			Tag<0>, 
			(t_Start >= t_Len ? t_Len - 1 : t_Start < 0 ? 0 : t_Start), 
			(t_End > t_Len ? t_Len : t_End < -1 ? -1 : t_End), 
			t_Interval, 
			(t_Start >= t_Len ? t_Len - 1 : t_Start < 0 ? 0 : t_Start),
			((t_Start < t_End ? t_Start < t_End : t_Start > t_End) && ((t_End - t_Start)/t_Interval) > 0)
		>::type;
	private:
		template<size_t t_Length>
		static constexpr bool equalTo_(ssize_t /*idx*/, ssize_t /*length_*/, ssize_t /*offset*/, char const (&/*str*/)[t_Length])
		{
			return t_Length == 1 && Tag::length == 0;
		}

		template<size_t t_Length>
		static constexpr bool equalTo_(ssize_t idx, ssize_t length_, ssize_t offset, char const (&str)[t_Length], char compareChar)
		{
			return offset > 0 ? false : idx >= length_ ? true : str[idx] == compareChar;
		}
		template<size_t t_Length, typename t_FirstCharType, typename... t_MorCharTypes>
		static constexpr bool equalTo_(ssize_t idx, ssize_t length_, ssize_t offset, char const (&str)[t_Length], t_FirstCharType compareChar, t_MorCharTypes... moreChars)
		{
			return
				offset > 0
				?
				equalTo_(idx, length_, offset - 1, str, moreChars...)
				:
				idx >= length_
				?
				true
				:
				str[idx] == compareChar ? equalTo_(idx + 1, length_, offset, str, moreChars...) : false;
		}

		template<size_t t_Length>
		static constexpr ssize_t find_(ssize_t idx, ssize_t start, size_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx + t_Length - 1 > end
				?
				-1
				:
				idx >= start
				?
				equalTo_(0, t_Length - 1, 0, str, compareChar)
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MorCharTypes>
		static constexpr ssize_t find_(ssize_t idx, ssize_t start, size_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MorCharTypes... moreChars)
		{
			return
				idx + t_Length - 1 > end
				?
				-1
				:
				idx >= start
				?
				equalTo_(0, t_Length - 1, 0, str, compareChar, moreChars...)
				?
				idx
				:
				find_(idx + 1, start, end, str, moreChars...)
				:
				find_(idx + 1, start, end, str, moreChars...);
		}

		template<size_t t_Length>
		static constexpr ssize_t rfind_(ssize_t idx, ssize_t start, size_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx + t_Length - 1 > end
				?
				-1
				:
				idx >= start
				?
				equalTo_(0, t_Length - 1, 0, str, compareChar)
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MoreCharTypes>
		static constexpr ssize_t rfind_(ssize_t idx, ssize_t start, size_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MoreCharTypes... moreChars)
		{
			return
				idx + t_Length - 1 > end
				?
				-1
				:
				idx >= start
				?
				rfind_(idx + 1, start, end, str, moreChars...) != -1
				?
				rfind_(idx + 1, start, end, str, moreChars...)
				:
				equalTo_(0, t_Length - 1, 0, str, compareChar, moreChars...)
				?
				idx
				:
				-1
				:
				rfind_(idx + 1, start, end, str, moreChars...);
		}

		template<size_t t_Length>
		static constexpr bool isOneOf_(char const (&str)[t_Length], char compareChar, size_t idx)
		{
			return idx == t_Length ? false : compareChar == str[idx] ? true : isOneOf_(str, compareChar, idx + 1);
		}

		template<size_t t_Length>
		static constexpr ssize_t findFirstOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				isOneOf_(str, compareChar, 0)
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MorCharTypes>
		static constexpr ssize_t findFirstOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MorCharTypes... moreChars)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				isOneOf_(str, compareChar, 0)
				?
				idx
				:
				findFirstOf_(idx + 1, start, end, str, moreChars...)
				:
				findFirstOf_(idx + 1, start, end, str, moreChars...);
		}

		template<size_t t_Length>
		static constexpr ssize_t findLastOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				isOneOf_(str, compareChar, 0)
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MoreCharTypes>
		static constexpr ssize_t findLastOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MoreCharTypes... moreChars)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				findLastOf_(idx + 1, start, end, str, moreChars...) != -1
				?
				findLastOf_(idx + 1, start, end, str, moreChars...)
				:
				isOneOf_(str, compareChar, 0)
				?
				idx
				:
				-1
				:
				findLastOf_(idx + 1, start, end, str, moreChars...);
		}

		template<size_t t_Length>
		static constexpr ssize_t findFirstNotOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				(!isOneOf_(str, compareChar, 0))
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MorCharTypes>
		static constexpr ssize_t findFirstNotOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MorCharTypes... moreChars)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				(!isOneOf_(str, compareChar, 0))
				?
				idx
				:
				findFirstNotOf_(idx + 1, start, end, str, moreChars...)
				:
				findFirstNotOf_(idx + 1, start, end, str, moreChars...);
		}

		template<size_t t_Length>
		static constexpr ssize_t findLastNotOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], char compareChar)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				(!isOneOf_(str, compareChar, 0))
				?
				idx
				:
				-1
				:
				-1;
		}

		template<size_t t_Length, typename t_FirstCharType, typename... t_MoreCharTypes>
		static constexpr ssize_t findLastNotOf_(ssize_t idx, ssize_t start, ssize_t end, char const (&str)[t_Length], t_FirstCharType compareChar, t_MoreCharTypes... moreChars)
		{
			return
				idx >= end
				?
				-1
				:
				idx >= start
				?
				findLastNotOf_(idx + 1, start, end, str, moreChars...) != -1
				?
				findLastNotOf_(idx + 1, start, end, str, moreChars...)
				:
				(!isOneOf_(str, compareChar, 0))
				?
				idx
				:
				-1
				:
				findLastNotOf_(idx + 1, start, end, str, moreChars...);
		}
	};

	template<ssize_t t_Len, char... t_Chars>
	CONSTEXPR_ARRAY char Tag<t_Len, t_Chars...>::name[t_Len + 1] SPRAWL_CONSTEXPR_OUT_OF_CLASS_INIT({ t_Chars..., '\0' });

	namespace detail
	{
		template<typename t_Type>
		struct TypeToString
		{
		private:
			struct NameSize
			{
				char const* name;
				size_t size;
				constexpr NameSize(char const* name_, size_t size_) : name(name_), size(size_) {}
			};

#if defined(_MSC_VER) && !defined(__clang__)
			static constexpr size_t prefix_size = sizeof("sprawl::detail::TypeToString<") - 1;
			static constexpr size_t postfix_size = sizeof(">::f") - 1;
#else
			static constexpr size_t prefix_size = sizeof("static sprawl::detail::TypeToString::NameSize sprawl::detail::TypeToString<") - 1;
			static constexpr size_t postfix_size = sizeof(">::f() [t_Type = ]") - 1;
#endif

			static constexpr NameSize f()
			{
#if defined(_MSC_VER) && !defined(__clang__)
				return{ __FUNCTION__ + prefix_size, sizeof(__FUNCTION__) - prefix_size - postfix_size };
#else
				return{ __PRETTY_FUNCTION__ + prefix_size, sizeof(__PRETTY_FUNCTION__) - prefix_size - postfix_size - ((sizeof(__PRETTY_FUNCTION__) - prefix_size - postfix_size) / 2) };
#endif
			}

			static constexpr NameSize nameSize = f();
		public:
			//Gotta do this the hard way since we can't sizeof()...
			typedef typename ::sprawl::detail::TagWrapper<
				::sprawl::detail::SizeChecker<nameSize.size - 1>::value, 1,
				::sprawl::Tag<0>,
				::sprawl::detail::IsPositive<nameSize.size - 1>::value,
				true,
				SPRAWL_CHR_MAX_MACRO(nameSize.name, nameSize.size, 0)
			>::type nakedType;

#if defined(_MSC_VER) && !defined(__clang__)
			// Consistency - msvc adds 'struct' or 'class' to the typename, clang/gcc do not.
			// Also, msvc doesn't include spaces after commas, clang/gcc do.
			// Also, msvc will add a space after every >, while clang/gcc will only put it between them - for simplicity, we're removing all spaces that follow >
			typedef typename If<nakedType::StartsWith("struct "), typename nakedType::template ReplaceAt<0, 7, SPRAWL_TAG("")>>
				::template ElseIf<nakedType::StartsWith("class "), typename nakedType::template ReplaceAt<0, 6, SPRAWL_TAG("")>>
				::template Else<nakedType>
				::template type<::sprawl::UnmatchedIfSequence>
				::template Replace<SPRAWL_TAG(","), SPRAWL_TAG(", ")>
				::template Replace<SPRAWL_TAG("> "), SPRAWL_TAG(">")>
				::template Replace<SPRAWL_TAG(" class "), SPRAWL_TAG(" ")>
				::template Replace<SPRAWL_TAG(" struct "), SPRAWL_TAG(" ")>
				::template Replace<SPRAWL_TAG("<class "), SPRAWL_TAG("<")>
				::template Replace<SPRAWL_TAG("<struct "), SPRAWL_TAG("<")>
				type;
#else
			typedef typename nakedType::template Replace<SPRAWL_TAG("> "), SPRAWL_TAG(">")> type;
#endif
		};

		constexpr char intBaseValues[] = {
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
			'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
			'u', 'v', 'w', 'x', 'y', 'z'
		};

		constexpr char intBaseValuesUpper[] = {
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
			'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
			'U', 'V', 'W', 'X', 'Y', 'Z'
		};

		template<typename t_Type, t_Type t_Value, size_t t_Base, bool t_Capitalize, bool t_IsSingleDigit = (t_Value >= 0 && t_Value < t_Base), bool t_IsNegative = (t_Value < 0)>
		struct TagFromInt
		{
			typedef typename TagFromInt<t_Type, t_Value / t_Base, t_Base, t_Capitalize>::type::template Append<Tag<1, t_Capitalize ? intBaseValuesUpper[t_Value % t_Base] : intBaseValues[t_Value % t_Base]>> type;
		};

		template<typename t_Type, t_Type t_Value, size_t t_Base, bool t_Capitalize>
		struct TagFromInt<t_Type, t_Value, t_Base, t_Capitalize, true, false>
		{
			typedef Tag<1, t_Capitalize ? intBaseValuesUpper[t_Value] : intBaseValues[t_Value]> type;
		};

		template<typename t_Type, t_Type t_Value, size_t t_Base, bool t_Capitalize>
		struct TagFromInt<t_Type, t_Value, t_Base, t_Capitalize, false, true>
		{
			typedef typename Tag<1, '-'>::Append<typename TagFromInt<t_Type, 0 - t_Value, t_Base, t_Capitalize>::type> type;
		};

		template<typename t_Type, t_Type t_Value, size_t t_Base, bool t_Capitalize, bool t_IsSingleDigit = (t_Value >= 0 && t_Value < t_Base)>
		struct TagFromUInt
		{
			typedef typename TagFromUInt<t_Type, t_Value / t_Base, t_Base, t_Capitalize>::type::template Append<Tag<1, t_Capitalize ? intBaseValuesUpper[t_Value % t_Base] : intBaseValues[t_Value % t_Base]>> type;
		};

		template<typename t_Type, t_Type t_Value, size_t t_Base, bool t_Capitalize>
		struct TagFromUInt<t_Type, t_Value, t_Base, t_Capitalize, true>
		{
			typedef Tag<1, t_Capitalize ? intBaseValuesUpper[t_Value] : intBaseValues[t_Value]> type;
		};

		template<bool t_Value>
		struct TagFromBool
		{
			typedef Tag<4, 't', 'r', 'u', 'e'> type;
		};

		template<>
		struct TagFromBool<false>
		{
			typedef Tag<5, 'f', 'a', 'l', 's', 'e'> type;
		};
	}
	template<typename t_Type>
	using TypeName = typename detail::TypeToString<t_Type>::type;

	template<typename t_Type>
	struct TagFrom;

	template<>
	struct TagFrom<char>
	{
		template<char t_Char>
		using FromValue = Tag<1, t_Char>;
	};

	template<> struct TagFrom<signed char> { template<signed char t_Value, size_t t_Base = 10, bool t_Capitalize = true> using FromValue = typename detail::TagFromInt<signed char, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<short      > { template<short t_Value, size_t t_Base = 10, bool t_Capitalize = true      > using FromValue = typename detail::TagFromInt<short, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<int        > { template<int t_Value, size_t t_Base = 10, bool t_Capitalize = true        > using FromValue = typename detail::TagFromInt<int, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<long       > { template<long t_Value, size_t t_Base = 10, bool t_Capitalize = true       > using FromValue = typename detail::TagFromInt<long, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<long long  > { template<long long t_Value, size_t t_Base = 10, bool t_Capitalize = true  > using FromValue = typename detail::TagFromInt<long long, t_Value, t_Base, t_Capitalize>::type; };

	template<> struct TagFrom<unsigned char     > { template<unsigned char t_Value, size_t t_Base = 10, bool t_Capitalize = true     > using FromValue = typename detail::TagFromUInt<unsigned char, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<unsigned short    > { template<unsigned short t_Value, size_t t_Base = 10, bool t_Capitalize = true    > using FromValue = typename detail::TagFromUInt<unsigned short, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<unsigned int      > { template<unsigned int t_Value, size_t t_Base = 10, bool t_Capitalize = true      > using FromValue = typename detail::TagFromUInt<unsigned int, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<unsigned long     > { template<unsigned long t_Value, size_t t_Base = 10, bool t_Capitalize = true     > using FromValue = typename detail::TagFromUInt<unsigned long, t_Value, t_Base, t_Capitalize>::type; };
	template<> struct TagFrom<unsigned long long> { template<unsigned long long t_Value, size_t t_Base = 10, bool t_Capitalize = true> using FromValue = typename detail::TagFromUInt<unsigned long long, t_Value, t_Base, t_Capitalize>::type; };

	template<> struct TagFrom<bool> { template<bool t_Value> using FromValue = typename detail::TagFromBool<t_Value>::type; };

	template<>
	struct TagFrom<std::nullptr_t>
	{
		template<std::nullptr_t value>
		using FromValue = Tag<6, '(', 'n', 'u', 'l', 'l', ')'>;
	};
}