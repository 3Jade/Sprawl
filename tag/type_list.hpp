#pragma once

#include "tag_list_info.hpp"

namespace sprawl
{

	template<typename... t_Types>
	struct TypeList
	{
	private:
		template<ssize_t t_GetIdx, ssize_t t_CurIdx, typename... t_CheckTypes>
		struct getType_
		{
			typedef std::false_type type;
			static_assert(t_CurIdx < 0, "TypeList::Get - out of bounds");
		};

		template<ssize_t t_GetIdx, ssize_t t_CurIdx, typename t_FirstType, typename... t_MoreTypes>
		struct getType_<t_GetIdx, t_CurIdx, t_FirstType, t_MoreTypes...>
		{
			typedef typename getType_<t_GetIdx, t_CurIdx + 1, t_MoreTypes...>::type type;
		};

		template<ssize_t t_GetIdx, typename t_FirstType, typename... t_MoreTypes>
		struct getType_<t_GetIdx, t_GetIdx, t_FirstType, t_MoreTypes...>
		{
			typedef t_FirstType type;
		};

		template<typename... t_AppendTypes>
		struct append_
		{
			typedef TypeList<t_Types..., t_AppendTypes...> type;
		};

		template<typename t_ExtendType>
		struct extend_;

		template<typename... t_ExtendTypes>
		struct extend_<TypeList<t_ExtendTypes...>>
		{
			typedef TypeList<t_Types..., t_ExtendTypes...> type;
		};

	public:
		static constexpr size_t size = sizeof...(t_Types);

		template<size_t t_Idx>
		using Get = typename getType_<t_Idx, 0, t_Types...>::type;

		template<typename... t_AppendTypes>
		using Append = typename append_<t_AppendTypes...>::type;

		template<typename t_ExtendListType>
		using Extend = typename extend_<t_ExtendListType>::type;
	};

	template<typename... t_Types>
	struct TagListInfo<TypeList<t_Types...>>
	{
		static constexpr size_t numElements = TypeList<t_Types...>::size;

		template<size_t t_Idx>
		using Element = typename TypeList<t_Types...>::template Get<t_Idx>;
	};
}