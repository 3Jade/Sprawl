#include "time.hpp"
#include <type_traits>

namespace sprawl
{
	namespace time
	{
		int64_t Convert(int64_t time, Resolution fromResolution, Resolution toResolution)
		{
			return (time * std::underlying_type<Resolution>::type(fromResolution)) / std::underlying_type<Resolution>::type(toResolution);
		}
	}
}
