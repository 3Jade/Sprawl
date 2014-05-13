#pragma once
//Temporary to get hold of std::hash until this is fully implemented
#include <unordered_map>

namespace sprawl
{
	template<typename T>
	class Hash
	{
	public:
		size_t operator()(T const& value)
		{
			return std::hash<T>()(value);
		}
	};
}
