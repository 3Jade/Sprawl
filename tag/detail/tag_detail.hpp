#pragma once

#include "../tag_list_info.hpp"

namespace sprawl
{
	template<char... t_Chars>
	struct Tag;

	namespace detail
	{
		template<ssize_t t_Size>
		struct SizeChecker
		{
			static_assert(t_Size <= SPRAWL_CHR_MAX, "Size of tag exceeds SPRAWL_CHR_MAX; please increase SPRAWL_CHR_MAX to a larger power of 2.");
			static constexpr ssize_t value = t_Size <= SPRAWL_CHR_MAX ? t_Size : SPRAWL_CHR_MAX;
		};

		template<ssize_t t_Idx>
		struct IsPositive
		{
			static constexpr bool value = (t_Idx > 0);
		};

		template<char t_SearchChar, char t_FirstChar, char... t_MoreChars>
		struct CharIn
		{
			static constexpr bool value = (t_SearchChar == t_FirstChar ? true : CharIn<t_SearchChar, t_MoreChars...>::value);
		};

		template<char t_SearchChar, char t_LastChar>
		struct CharIn<t_SearchChar, t_LastChar>
		{
			static constexpr bool value = (t_SearchChar == t_LastChar);
		};

		template<typename t_ConditionType, char... t_AdditionalChars>
		struct MeetsCondition;

		template<typename t_ConditionType, char t_CurrentChar, char... t_AdditionalChars>
		struct MeetsCondition<t_ConditionType, t_CurrentChar, t_AdditionalChars...>
		{
			static constexpr bool value = t_ConditionType::template Check<t_CurrentChar>::value ? MeetsCondition<t_ConditionType, t_AdditionalChars...>::value : false;
		};

		template<typename t_ConditionType, char t_CurrentChar>
		struct MeetsCondition<t_ConditionType, t_CurrentChar>
		{
			static constexpr bool value = t_ConditionType::template Check<t_CurrentChar>::value;
		};

		template<typename t_ConditionType>
		struct MeetsCondition<t_ConditionType>
		{
			static constexpr bool value = false;
		};

		template<typename t_ConditionType, char t_PreviousChar, char... t_AdditionalChars>
		struct MeetsComplexCondition;

		template<typename t_ConditionType, char t_PreviousChar, char t_CurrentChar, char... t_AdditionalChars>
		struct MeetsComplexCondition<t_ConditionType, t_PreviousChar, t_CurrentChar, t_AdditionalChars...>
		{
			static constexpr bool value = t_ConditionType::template ComplexCheck<t_PreviousChar, t_CurrentChar>::value ? MeetsComplexCondition<t_ConditionType, t_CurrentChar, t_AdditionalChars...>::value : false;
		};

		template<typename t_ConditionType, char t_PreviousChar, char t_CurrentChar>
		struct MeetsComplexCondition<t_ConditionType, t_PreviousChar, t_CurrentChar>
		{
			static constexpr bool value = t_ConditionType::template ComplexCheck<t_PreviousChar, t_CurrentChar>::value;
		};

		template<typename t_ConditionType, char t_PreviousChar>
		struct MeetsComplexCondition<t_ConditionType, t_PreviousChar>
		{
			static constexpr bool value = false;
		};

		template<char... t_Chars>
		struct TagBuilder
		{
			template<char t_CharToAdd>
			using AppendChar = TagBuilder<t_Chars..., t_CharToAdd>;
		};
		
		template<typename t_Type>
		struct BuilderToTag;

		template<char... t_Chars>
		struct BuilderToTag<TagBuilder<t_Chars...>>
		{
			typedef ::sprawl::Tag<t_Chars...> type;
		};

		template<ssize_t t_Length, ssize_t t_Idx, typename t_CharsSoFarType, bool t_LengthIsPositive, bool t_IdxIsPositive, char... t_Chars>
		struct TagWrapper;

		template<ssize_t t_Idx, typename t_CharsSoFarType, char t_FirstChar, char... t_Chars>
		struct TagWrapper<0, t_Idx, t_CharsSoFarType, true, true, t_FirstChar, t_Chars...>
		{
			typedef typename BuilderToTag<t_CharsSoFarType>::type type;
		};

		template<ssize_t t_Length, typename t_CharsSoFarType, char t_FirstChar, char... t_Chars>
		struct TagWrapper<t_Length, t_Length, t_CharsSoFarType, true, true, t_FirstChar, t_Chars...>
		{
			typedef typename BuilderToTag<typename t_CharsSoFarType::template AppendChar<t_FirstChar>>::type type;
		};

		template<ssize_t t_Length, ssize_t t_Idx, typename t_CharsSoFarType, char t_FirstChar, char... t_Chars>
		struct TagWrapper<t_Length, t_Idx, t_CharsSoFarType, true, true, t_FirstChar, t_Chars...>
		{
			typedef typename TagWrapper<t_Length, t_Idx + 1, typename t_CharsSoFarType::template AppendChar<t_FirstChar>, true, IsPositive<t_Idx + 1>::value, t_Chars...>::type type;
		};

		template<ssize_t t_Length, ssize_t t_Idx, typename t_TagType, bool t_IdxIsPositive, char... t_Chars>
		struct TagWrapper<t_Length, t_Idx, t_TagType, false, t_IdxIsPositive, t_Chars...>
		{
			typedef typename BuilderToTag<t_TagType>::type type;
		};


		template<ssize_t t_Length, typename t_CharsSoFarType, char t_FirstChar, char... t_Chars>
		struct TagWrapper<t_Length, t_Length, t_CharsSoFarType, true, false, t_FirstChar, t_Chars...>
		{
			typedef typename BuilderToTag<t_CharsSoFarType>::type type;
		};

		template<ssize_t t_Length, ssize_t t_Idx, typename t_CharsSoFarType, char t_FirstChar, char... t_Chars>
		struct TagWrapper<t_Length, t_Idx, t_CharsSoFarType, true, false, t_FirstChar, t_Chars...>
		{
			typedef typename TagWrapper<t_Length, t_Idx + 1, t_CharsSoFarType, true, IsPositive<t_Idx + 1>::value, t_Chars...>::type type;
		};

		template<template <char t_PreviousChar, char t_CurrentChar> class t_TransformType, typename t_CharsSoFarType, char t_PreviousChar, char... t_Chars>
		struct TransformTag
		{

		};

		template<template <char t_PreviousChar, char t_CurrentChar> class t_TransformType, typename t_CharsSoFarType, char t_PreviousChar>
		struct TransformTag<t_TransformType, t_CharsSoFarType, t_PreviousChar>
		{
			typedef t_CharsSoFarType type;
		};

		template<template <char t_PreviousChar, char t_CurrentChar> class t_TransformType, typename t_CharsSoFarType, char t_PreviousChar, char t_FirstChar, char... t_Chars>
		struct TransformTag<t_TransformType, t_CharsSoFarType, t_PreviousChar, t_FirstChar, t_Chars...>
			: public TransformTag<t_TransformType, typename t_CharsSoFarType::template AppendChar<t_TransformType<t_PreviousChar, t_FirstChar>::value>, t_TransformType<t_PreviousChar, t_FirstChar>::value, t_Chars...>
		{

		};

		template<typename t_TagToSearchType, typename t_TagToFindType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - (t_Idx - ssize_t(t_TagToFindType::length))>::value>
		struct FindTag
		{
			static constexpr ssize_t result = t_TagToSearchType::template Substring<t_Idx, t_TagToFindType::length>::template EqualTo<t_TagToFindType>() ? t_Idx : FindTag<t_TagToSearchType, t_TagToFindType, t_Idx + 1>::result;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, ssize_t t_Idx>
		struct FindTag<t_TagToSearchType, t_TagToFindType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - (t_Idx - ssize_t(t_TagToFindType::length))>::value>
		struct RFindTag
		{
			static constexpr ssize_t result = RFindTag<t_TagToSearchType, t_TagToFindType, t_Idx + 1>::result != -1 ? RFindTag<t_TagToSearchType, t_TagToFindType, t_Idx + 1>::result : t_TagToSearchType::template Substring<t_Idx, t_TagToFindType::length>::template EqualTo<t_TagToFindType>() ? t_Idx : -1;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, ssize_t t_Idx>
		struct RFindTag<t_TagToSearchType, t_TagToFindType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - t_Idx>::value>
		struct FindPred
		{
			static constexpr ssize_t result = t_PredType::template Check<t_TagToSearchType::template CharAt<t_Idx>()>::value ? t_Idx : FindPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx>
		struct FindPred<t_TagToSearchType, t_PredType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - t_Idx>::value>
		struct RFindPred
		{
			static constexpr ssize_t result = RFindPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result != -1 ? RFindPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result : t_PredType::template Check<t_TagToSearchType::template CharAt<t_Idx>()>::value ? t_Idx : -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx>
		struct RFindPred<t_TagToSearchType, t_PredType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - t_Idx>::value>
		struct FindNotPred
		{
			static constexpr ssize_t result = (!t_PredType::template Check<t_TagToSearchType::template CharAt<t_Idx>()>::value) ? t_Idx : FindNotPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx>
		struct FindNotPred<t_TagToSearchType, t_PredType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx, bool t_IdxIsPositive = IsPositive<ssize_t(t_TagToSearchType::length) - t_Idx>::value>
		struct RFindNotPred
		{
			static constexpr ssize_t result = RFindNotPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result != -1 ? RFindNotPred<t_TagToSearchType, t_PredType, t_Idx + 1>::result : (!t_PredType::template Check<t_TagToSearchType::template CharAt<t_Idx>()>::value) ? t_Idx : -1;
		};

		template<typename t_TagToSearchType, typename t_PredType, ssize_t t_Idx>
		struct RFindNotPred<t_TagToSearchType, t_PredType, t_Idx, false>
		{
			static constexpr ssize_t result = -1;
		};

		template<typename t_TagToSearchType, typename t_TagToCountType, ssize_t t_Start, ssize_t t_End, bool t_WasFound = (t_TagToSearchType::template Find<t_TagToCountType, t_Start, t_End>() != -1)>
		struct CountTag
		{
			static constexpr ssize_t value = 1 + CountTag<t_TagToSearchType, t_TagToCountType, t_TagToSearchType::template Find<t_TagToCountType, t_Start, t_End>() + 1, t_End>::value;
		};

		template<typename t_TagToSearchType, typename t_TagToCountType, ssize_t t_Start, ssize_t t_End>
		struct CountTag<t_TagToSearchType, t_TagToCountType, t_Start, t_End, false>
		{
			static constexpr ssize_t value = 0;
		};

		template<size_t t_Idx>
		struct HasGetTestHelper
		{
			template<typename t_Type, typename = typename std::enable_if<(t_Idx < TagListInfo<t_Type>::numElements)>::type>
				static std::true_type HasGetTest(const t_Type&);

				static std::false_type HasGetTest(...);
		};

		template<class t_Type, size_t t_Idx>
		using HasGet = decltype(HasGetTestHelper<t_Idx>::HasGetTest(std::declval<t_Type>()));

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, size_t t_Idx, typename t_NextIndexExistsType>
		struct FindTagInList
		{
			static constexpr ssize_t thisTagLoc = t_TagToSearchType::template Find<typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type>();
			static constexpr ssize_t nextTagLoc = FindTagInList<t_TagToSearchType, t_AccessibleByGetType, t_Idx + 1, HasGet<t_AccessibleByGetType, t_Idx + 2>>::value;
			static constexpr ssize_t value = thisTagLoc < nextTagLoc ? thisTagLoc == -1 ? nextTagLoc : thisTagLoc : nextTagLoc == -1 ? thisTagLoc : nextTagLoc;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, size_t t_Idx>
		struct FindTagInList<t_TagToSearchType, t_AccessibleByGetType, t_Idx, std::false_type>
		{
			static constexpr ssize_t value = t_TagToSearchType::template Find<typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type>();
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, size_t t_Idx, typename t_NextIndexExistsType>
		struct RFindTagInList
		{
			static constexpr ssize_t thisTagLoc = t_TagToSearchType::template RFind<typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type>();
			static constexpr ssize_t nextTagLoc = RFindTagInList<t_TagToSearchType, t_AccessibleByGetType, t_Idx + 1, HasGet<t_AccessibleByGetType, t_Idx + 2>>::value;
			static constexpr ssize_t value = thisTagLoc > nextTagLoc ? thisTagLoc : nextTagLoc;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, size_t t_Idx>
		struct RFindTagInList<t_TagToSearchType, t_AccessibleByGetType, t_Idx, std::false_type>
		{
			static constexpr ssize_t value = t_TagToSearchType::template RFind<typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type>();
		};

		template<typename t_TagToJoinType, typename t_AccessibleByGetType, size_t t_Idx, typename t_NextIndexExistsType>
		struct JoinTags
		{
			typedef typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type
				::template Append<t_TagToJoinType>
				::template Append<typename JoinTags<t_TagToJoinType, t_AccessibleByGetType, t_Idx + 1, HasGet<t_AccessibleByGetType, t_Idx + 2>>::type> type;
		};

		template<typename t_TagToJoinType, typename t_AccessibleByGetType, size_t t_Idx>
		struct JoinTags<t_TagToJoinType, t_AccessibleByGetType, t_Idx, std::false_type>
		{
			typedef typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type type;
		};

		template<typename t_TagToJoinType, ssize_t t_Start, typename t_AccessibleByGetType, size_t t_Idx, typename t_NextIndexExistsType>
		struct MatchLength
		{
			typedef typename std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type thisType;
			static constexpr ssize_t length = thisType::length;
			static constexpr bool match = t_TagToJoinType::template Substring<t_Start, length>::template EqualTo<thisType>();
			static constexpr ssize_t value = match ? length : MatchLength<t_TagToJoinType, t_Start, t_AccessibleByGetType, t_Idx + 1, HasGet<t_AccessibleByGetType, t_Idx + 2>>::value;
		};

		template<typename t_TagToJoinType, ssize_t t_Start, typename t_AccessibleByGetType, size_t t_Idx>
		struct MatchLength<t_TagToJoinType, t_Start, t_AccessibleByGetType, t_Idx, std::false_type>
		{
			static constexpr ssize_t value = std::remove_reference<typename TagListInfo<t_AccessibleByGetType>::template Element<t_Idx>>::type::length;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt = t_TagToSearchType::template Find<t_TagToFindType>()>
		struct ReplaceTags
		{
			typedef typename t_TagToSearchType::template Substring<0, t_ReplaceAt>::template Append<t_ReplacementType>::template Append<typename ReplaceTags<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + t_TagToFindType::length>, t_TagToFindType, t_ReplacementType, t_Count - 1>::type> type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_Count>
		struct ReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct ReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType>
		struct ReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt = t_TagToSearchType::template FindFirstInList<t_AccessibleByGetType>()>
		struct ReplaceTagsInList
		{
			typedef typename t_TagToSearchType::template Substring<0, t_ReplaceAt>::template Append<t_ReplacementType>::template Append<typename ReplaceTagsInList<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + MatchLength<t_TagToSearchType, t_ReplaceAt, t_AccessibleByGetType, 0, HasGet<t_AccessibleByGetType, 1>>::value>, t_AccessibleByGetType, t_ReplacementType, t_Count - 1>::type> type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_Count>
		struct ReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct ReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType>
		struct ReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt>
		struct ReplaceTagsAnyOf
		{
			typedef typename t_TagToSearchType::
				template Substring<0, t_ReplaceAt>::
				template Append<t_ReplacementType>::
				template Append<
				typename ReplaceTagsAnyOf<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + 1>,
				t_PredType,
				t_ReplacementType,
				t_Count - 1,
				t_TagToSearchType::template Substring<t_ReplaceAt + 1>::template FindFirstOf<t_PredType>()
				>::type
				> type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count>
		struct ReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct ReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType>
		struct ReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt>
		struct ReplaceTagsAnyNotOf
		{
			typedef typename t_TagToSearchType::
				template Substring<0, t_ReplaceAt>::
				template Append<t_ReplacementType>::
				template Append<
				typename ReplaceTagsAnyNotOf<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + 1>,
				t_PredType,
				t_ReplacementType,
				t_Count - 1,
				t_TagToSearchType::template Substring<t_ReplaceAt + 1>::template FindFirstNotOf<t_PredType>()
				>::type
				> type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count>
		struct ReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct ReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType>
		struct ReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt = t_TagToSearchType::template RFind<t_TagToFindType>()>
		struct RReplaceTags
		{
			typedef typename RReplaceTags<
				typename t_TagToSearchType::template Substring<0, t_ReplaceAt>, t_TagToFindType, t_ReplacementType, t_Count - 1
			>::type::template Append<t_ReplacementType>::template Append<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + t_TagToFindType::length>
			> type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_Count>
		struct RReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct RReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_TagToFindType, typename t_ReplacementType>
		struct RReplaceTags<t_TagToSearchType, t_TagToFindType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};


		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt = t_TagToSearchType::template FindLastInList<t_AccessibleByGetType>()>
		struct RReplaceTagsInList
		{
			typedef typename RReplaceTagsInList<
				typename t_TagToSearchType::template Substring<0, t_ReplaceAt>, t_AccessibleByGetType, t_ReplacementType, t_Count - 1
			>::type::template Append<t_ReplacementType>::template Append<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + MatchLength<t_TagToSearchType, t_ReplaceAt, t_AccessibleByGetType, 0, HasGet<t_AccessibleByGetType, 1>>::value>
			> type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_Count>
		struct RReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct RReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_AccessibleByGetType, typename t_ReplacementType>
		struct RReplaceTagsInList<t_TagToSearchType, t_AccessibleByGetType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt>
		struct RReplaceTagsAnyOf
		{
			typedef typename RReplaceTagsAnyOf<
				typename t_TagToSearchType::template Substring<0, t_ReplaceAt>,
				t_PredType,
				t_ReplacementType,
				t_Count - 1,
				t_TagToSearchType::template Substring<0, t_ReplaceAt>::template FindLastOf<t_PredType>()
			>::type
				::template Append<t_ReplacementType>
				::template Append<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + 1>
				> type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count>
		struct RReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct RReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType>
		struct RReplaceTagsAnyOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count, ssize_t t_ReplaceAt>
		struct RReplaceTagsAnyNotOf
		{
			typedef typename RReplaceTagsAnyNotOf<
				typename t_TagToSearchType::template Substring<0, t_ReplaceAt>,
				t_PredType,
				t_ReplacementType,
				t_Count - 1,
				t_TagToSearchType::template Substring<0, t_ReplaceAt>::template FindLastNotOf<t_PredType>()
			>::type
				::template Append<t_ReplacementType>
				::template Append<
				typename t_TagToSearchType::template Substring<t_ReplaceAt + 1>
				> type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_Count>
		struct RReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, t_Count, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType, ssize_t t_ReplaceAt>
		struct RReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, t_ReplaceAt>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSearchType, typename t_PredType, typename t_ReplacementType>
		struct RReplaceTagsAnyNotOf<t_TagToSearchType, t_PredType, t_ReplacementType, 0, -1>
		{
			typedef t_TagToSearchType type;
		};

		template<typename t_TagToSplit, typename t_TypeSoFarType, ssize_t t_Start, ssize_t t_End, ssize_t t_Interval, ssize_t t_Idx, bool t_WithinInterval>
		struct SliceTag
		{
			typedef typename t_TypeSoFarType::template AppendChar<t_TagToSplit::template CharAt<t_Idx>()> interimType;
			static constexpr ssize_t nextIdx = t_Idx + t_Interval;
			typedef typename SliceTag<
				t_TagToSplit,
				interimType,
				t_Start,
				t_End,
				t_Interval,
				nextIdx,
				(t_Start < t_End ? (nextIdx >= t_Start && nextIdx < t_End) : (nextIdx <= t_Start && nextIdx > t_End))
				>::type type;
		};

		template<typename t_TagToSplit, typename t_TypeSoFarType, ssize_t t_Start, ssize_t t_End, ssize_t t_Interval, ssize_t t_Idx>
		struct SliceTag<t_TagToSplit, t_TypeSoFarType, t_Start, t_End, t_Interval, t_Idx, false>
		{
			typedef t_TypeSoFarType type;
		};

		template<char... t_AllChars>
		struct tagToInt_;

		template<bool t_IsDigit, char t_FirstChar, char... t_MoreChars>
		struct TagToIntRecurseIf
		{
			static constexpr uint64_t value = (t_FirstChar - '0') * tagToInt_<t_MoreChars...>::order + tagToInt_<t_MoreChars...>::value;
			static constexpr uint64_t order = tagToInt_<t_MoreChars...>::order * 10;
		};

		template<char t_FirstChar, char... t_MoreChars>
		struct TagToIntRecurseIf<false, t_FirstChar, t_MoreChars...>
		{
			static constexpr uint64_t value = 0;
			static constexpr uint64_t order = 1;
		};

		template<char t_FirstChar, char... t_MoreChars>
		struct tagToInt_<t_FirstChar, t_MoreChars...>
		{
			static constexpr uint64_t value = TagToIntRecurseIf<(t_FirstChar >= '0' && t_FirstChar <= '9'), t_FirstChar, t_MoreChars...>::value;
			static constexpr uint64_t order = TagToIntRecurseIf<(t_FirstChar >= '0' && t_FirstChar <= '9'), t_FirstChar, t_MoreChars...>::order;
		};

		template<char t_FirstChar>
		struct tagToInt_<t_FirstChar>
		{
			static constexpr uint64_t value = (t_FirstChar >= '0' && t_FirstChar <= '9') ? (t_FirstChar - '0') : 0;
			static constexpr uint64_t order = (t_FirstChar >= '0' && t_FirstChar <= '9') ? 10 : 1;
		};

		template<char... t_Chars>
		struct TagToInt;

		template<char t_Char, char... t_Chars>
		struct TagToInt<t_Char, t_Chars...>
		{
			static constexpr int64_t value = int64_t(tagToInt_<t_Char, t_Chars...>::value);
		};

		template<char... t_Chars>
		struct TagToInt<'-', t_Chars...>
		{
			static constexpr int64_t value = -int64_t(tagToInt_<t_Chars...>::value);
		};

		template<char... t_Chars>
		struct TagToInt<'+', t_Chars...>
		{
			static constexpr int64_t value = int64_t(tagToInt_<t_Chars...>::value);
		};

		template<char t_Char>
		struct TagToInt<t_Char>
		{
			static constexpr int64_t value = (t_Char >= '0' && t_Char <= '9') ? (t_Char - '0') : 0;
		};

		template<>
		struct TagToInt<>
		{
			static constexpr int64_t value = 0;
		};

		template<char... t_AllChars>
		struct tagToDouble_;

		template<bool t_IsDigit, char t_FirstChar, char... t_MoreChars>
		struct TagToDoubleRecurseIf
		{
			static constexpr long double value = (t_FirstChar - '0') * tagToDouble_<t_MoreChars...>::order + tagToDouble_<t_MoreChars...>::value;
			static constexpr long double order = tagToInt_<t_MoreChars...>::order * 10;
		};

		template<char t_FirstChar, char... t_MoreChars>
		struct TagToDoubleRecurseIf<false, t_FirstChar, t_MoreChars...>
		{
			static constexpr long double value = 0;
			static constexpr long double order = 1;
		};

		template<char t_FirstChar, char... t_MoreChars>
		struct tagToDouble_<t_FirstChar, t_MoreChars...>
		{
			static constexpr long double value = TagToDoubleRecurseIf<(t_FirstChar >= '0' && t_FirstChar <= '9'), t_FirstChar, t_MoreChars...>::value;
			static constexpr long double order = TagToDoubleRecurseIf<(t_FirstChar >= '0' && t_FirstChar <= '9'), t_FirstChar, t_MoreChars...>::order;
		};

		template<char... t_MoreChars>
		struct tagToDouble_<'.', t_MoreChars...>
		{
			static constexpr long double value = ((long double)(tagToInt_<t_MoreChars...>::value) / (long double)(tagToInt_<t_MoreChars...>::order));
			static constexpr long double order = 1;
		};

		template<char t_FirstChar>
		struct tagToDouble_<t_FirstChar>
		{
			static constexpr long double value = (t_FirstChar >= '0' && t_FirstChar <= '9') ? (t_FirstChar - '0') : 0;
			static constexpr long double order = (t_FirstChar >= '0' && t_FirstChar <= '9') ? 10 : 1;
		};

		template<>
		struct tagToDouble_<'.'>
		{
			static constexpr long double value = 0;
			static constexpr long double order = 1;
		};

		template<char... t_Chars>
		struct TagToDouble;

		template<char t_Char, char... t_Chars>
		struct TagToDouble<t_Char, t_Chars...>
		{
			static constexpr long double value = tagToDouble_<t_Char, t_Chars...>::value;
		};

		template<char... t_Chars>
		struct TagToDouble<'-', t_Chars...>
		{
			static constexpr long double value = -tagToDouble_<t_Chars...>::value;
		};

		template<char... t_Chars>
		struct TagToDouble<'+', t_Chars...>
		{
			static constexpr long double value = tagToDouble_<t_Chars...>::value;
		};

		template<char t_Char>
		struct TagToDouble<t_Char>
		{
			static constexpr long double value = (t_Char >= '0' && t_Char <= '9') ? (t_Char - '0') : 0;
		};

		template<typename t_TagType>
		struct TagToBool;

		template<char... t_Chars>
		struct TagToBool<Tag<t_Chars...>>
		{
			static constexpr bool value = TagToDouble<t_Chars...>::value != 0.0;
		};

		template<>
		struct TagToBool<Tag<'t', 'r', 'u', 'e'>>
		{
			static constexpr bool value = true;
		};

		template<>
		struct TagToBool<Tag<'f', 'a', 'l', 's', 'e'>>
		{
			static constexpr bool value = false;
		};

		//MSVC likes to complain about integer constant overflow here, but hash algorithms use that intentionally so this is being unilaterally disabled for this section.
#if SPRAWL_COMPILER_MSVC
#	pragma warning(push)
#	pragma warning(disable: 4307)
#endif
		namespace murmur3
		{
#if SPRAWL_32_BIT
			constexpr size_t c1 = 0xcc9e2d51;
			constexpr size_t c2 = 0x1b873593;
			constexpr uint8_t r1 = 15;
			constexpr uint8_t r2 = 13;
			constexpr size_t m = 5;
			constexpr size_t n = 0xe6546b64;

			constexpr size_t rotateLeft_(const size_t x, const uint8_t r)
			{
				return (x << r) | (x >> (32 - r));
			}

			template<char t_FirstChar, char t_SecondChar = '\0', char t_ThirdChar = '\0', char t_FourthChar = '\0'>
			struct charsToInt_
			{
#if SPRAWL_LITTLE_ENDIAN
				static constexpr size_t i = t_FourthChar;
				static constexpr size_t i2 = (i << 8) | t_ThirdChar;
				static constexpr size_t i3 = (i2 << 8) | t_SecondChar;
				static constexpr size_t value = (i3 << 8) | t_FirstChar;
#else
				static constexpr size_t i1 = t_FirstChar;
				static constexpr size_t i2 = (i1 << 8) | t_SecondChar;
				static constexpr size_t i3 = (i2 << 8) | t_ThirdChar;
				static constexpr size_t value = (i3 << 8) | t_FourthChar;
#endif
			};
#else
			constexpr size_t c1 = 0x87c37b91114253d5ULL;
			constexpr size_t c2 = 0x4cf5ad432745937fULL;
			constexpr uint8_t r1 = 15;
			constexpr uint8_t r2 = 13;
			constexpr size_t m = 5;
			constexpr size_t n = 0x52dce72938495ab5ULL;

			constexpr size_t rotateLeft_(const size_t x, const uint8_t r)
			{
				return (x << r) | (x >> (64 - r));
			}

			template<char t_FirstChar, char t_SecondChar = '\0', char t_ThirdChar = '\0', char t_FourthChar = '\0', char t_FifthChar = '\0', char t_SixthChar = '\0', char t_SeventhChar = '\0', char t_EighthChar = '\0'>
			struct charsToInt_
			{
#if SPRAWL_LITTLE_ENDIAN
				static constexpr size_t i1 = t_EighthChar;
				static constexpr size_t i2 = (i1 << 8) | t_SeventhChar;
				static constexpr size_t i3 = (i2 << 8) | t_SixthChar;
				static constexpr size_t i4 = (i3 << 8) | t_FifthChar;
				static constexpr size_t i5 = (i4 << 8) | t_FourthChar;
				static constexpr size_t i6 = (i5 << 8) | t_ThirdChar;
				static constexpr size_t i7 = (i6 << 8) | t_SecondChar;
				static constexpr size_t value = (i7 << 8) | t_FirstChar;
#else
				static constexpr size_t i1 = t_FirstChar;
				static constexpr size_t i2 = (i1 << 8) | t_SecondChar;
				static constexpr size_t i3 = (i2 << 8) | t_ThirdChar;
				static constexpr size_t i4 = (i3 << 8) | t_FourthChar;
				static constexpr size_t i5 = (i4 << 8) | t_FifthChar;
				static constexpr size_t i6 = (i5 << 8) | t_SixthChar;
				static constexpr size_t i7 = (i6 << 8) | t_SeventhChar;
				static constexpr size_t value = (i7 << 8) | t_EighthChar;
#endif
			};
#endif

			template<size_t t_FinalChunk, char... t_Chars>
			struct getFinalChunk_;

			template<size_t t_FinalChunk, char t_C1>
			struct getFinalChunk_<t_FinalChunk, t_C1>
			{
				static constexpr size_t value = t_FinalChunk | charsToInt_<t_C1>::value;
			};

			template<size_t t_FinalChunk, char t_C1, char t_C2>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2>::value;
			};

			template<size_t t_FinalChunk, char t_C1, char t_C2, char t_C3>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2, t_C3>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2, t_C3>::value;
			};

#if SPRAWL_64_BIT
			template<size_t t_FinalChunk, char t_C1, char t_C2, char t_C3, char t_C4>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2, t_C3, t_C4>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2, t_C3, t_C4>::value;
			};

			template<size_t t_FinalChunk, char t_C1, char t_C2, char t_C3, char t_C4, char t_C5>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2, t_C3, t_C4, t_C5>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2, t_C3, t_C4, t_C5>::value;
			};

			template<size_t t_FinalChunk, char t_C1, char t_C2, char t_C3, char t_C4, char t_C5, char t_C6>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2, t_C3, t_C4, t_C5, t_C6>::value;
			};

			template<size_t t_FinalChunk, char t_C1, char t_C2, char t_C3, char t_C4, char t_C5, char t_C6, char t_C7>
			struct getFinalChunk_<t_FinalChunk, t_C1, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7>
			{
				static constexpr size_t finalChunk2 = t_FinalChunk | size_t(t_C1);
				static constexpr size_t value = getFinalChunk_<finalChunk2 << 8, t_C2, t_C3, t_C4, t_C5, t_C6, t_C7>::value;
			};
#endif

			template<size_t t_OutputHash, char... t_Chars>
			struct Murmur3
			{
				static constexpr size_t finalChunk = 0;
				static constexpr size_t finalChunk2 = getFinalChunk_<finalChunk, t_Chars...>::value;
				static constexpr size_t finalChunk3 = finalChunk2 * c1;
				static constexpr size_t finalChunk4 = rotateLeft_(finalChunk3, r1);
				static constexpr size_t finalChunk5 = finalChunk4 * c2;
				static constexpr size_t value = t_OutputHash ^ finalChunk5;
			};

#if SPRAWL_64_BIT
			template<size_t t_OutputHash, char t_FirstChar, char t_SecondChar, char t_ThirdChar, char t_FourthChar, char t_FifthChar, char t_SixthChar, char t_SeventhChar, char t_EighthChar, char... t_MoreChars>
			struct Murmur3<t_OutputHash, t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar, t_FifthChar, t_SixthChar, t_SeventhChar, t_EighthChar, t_MoreChars...>
			{
				static constexpr size_t k = charsToInt_<t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar, t_FifthChar, t_SixthChar, t_SeventhChar, t_EighthChar>::value;
				static constexpr size_t k2 = k * c1;
				static constexpr size_t k3 = rotateLeft_(k2, r1);
				static constexpr size_t k4 = k3 * c2;
				static constexpr size_t output1 = t_OutputHash ^ k4;
				static constexpr size_t output2 = rotateLeft_(output1, r2);
				static constexpr size_t output3 = (output2 * m) + n;
				static constexpr size_t value = Murmur3<output3, t_MoreChars...>::value;
			};

			template<size_t t_OutputHash, char t_FirstChar, char t_SecondChar, char t_ThirdChar, char t_FourthChar, char t_FifthChar, char t_SixthChar, char t_SeventhChar, char t_EighthChar>
			struct Murmur3<t_OutputHash, t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar, t_FifthChar, t_SixthChar, t_SeventhChar, t_EighthChar>
			{
				static constexpr size_t k = charsToInt_<t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar, t_FifthChar, t_SixthChar, t_SeventhChar, t_EighthChar>::value;
				static constexpr size_t k2 = k * c1;
				static constexpr size_t k3 = rotateLeft_(k2, r1);
				static constexpr size_t k4 = k3 * c2;
				static constexpr size_t output1 = t_OutputHash ^ k4;
				static constexpr size_t output2 = rotateLeft_(output1, r2);
				static constexpr size_t value = (output2 * m) + n;
			};
		}
#else
			template<size_t t_OutputHash, char t_FirstChar, char t_SecondChar, char t_ThirdChar, char t_FourthChar, char... t_MoreChars>
			struct Murmur3<t_OutputHash, t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar, t_MoreChars...>
			{
				static constexpr size_t k = charsToInt_<t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar>::value;
				static constexpr size_t k2 = k * c1;
				static constexpr size_t k3 = rotateLeft_(k2, r1);
				static constexpr size_t k4 = k3 * c2;
				static constexpr size_t output1 = t_OutputHash ^ k4;
				static constexpr size_t output2 = rotateLeft_(output1, r2);
				static constexpr size_t output3 = (output2 * m) + n;
				static constexpr size_t value = Murmur3<output3, t_MoreChars...>::value;
			};

			template<size_t t_OutputHash, char t_FirstChar, char t_SecondChar, char t_ThirdChar, char t_FourthChar>
			struct Murmur3<t_OutputHash, t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar>
			{
				static constexpr size_t k = charsToInt_<t_FirstChar, t_SecondChar, t_ThirdChar, t_FourthChar>::value;
				static constexpr size_t k2 = k * c1;
				static constexpr size_t k3 = rotateLeft_(k2, r1);
				static constexpr size_t k4 = k3 * c2;
				static constexpr size_t output1 = t_OutputHash ^ k4;
				static constexpr size_t output2 = rotateLeft_(output1, r2);
				static constexpr size_t value = (output2 * m) + n;
			};
	}
#endif
#if SPRAWL_COMPILER_MSVC
#	pragma warning(pop)
#endif
}
}

//#include "tag_wrapper_opt.hpp"