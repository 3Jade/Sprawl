#pragma once
#include "../string/String.hpp"

namespace sprawl
{
	inline void PrintTo(sprawl::String const& value, std::ostream* stream)
	{
		*stream << value.toStdString();
	}
}
