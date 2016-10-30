#pragma once

#include <tuple>

namespace sprawl
{
	template<typename t_Type>
	struct TagListInfo
	{
		static constexpr size_t numElements = std::tuple_size<t_Type>::value;

		template<size_t t_Idx>
		using Element = typename std::tuple_element<t_Idx, t_Type>::type;
	};
}