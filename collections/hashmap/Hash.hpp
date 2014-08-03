#pragma once
//Temporary to get hold of std::hash until this is fully implemented
#include <unordered_map>
#include <type_traits>

namespace sprawl
{
	template<typename T>
	class Hash
	{
	public:
		typedef typename std::remove_const<typename std::remove_reference<T>::type>::type adjusted_type;

		inline static size_t Compute(T const& value)
		{
			return std::hash<adjusted_type>()(value);
		}
	};
}
