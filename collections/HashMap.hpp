#pragma once

//Support for legacy compilers lacking variadic template support
#if (defined(_WIN32) && _MSC_VER < 1800)
/*TODO: Support these too
|| (defined(__clang__) && (__clang_major__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 9))) \
	|| (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))) \
	|| defined(SPRAWL_NO_VARIADIC_TEMPLATES)*/

#include "hashmap/HashMap_Windows.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		class HashSet : public HashMap<ValueType, SelfAccessor<ValueType>, _MAX_NIL_LIST>
		{
			//
		};
	}
}

/*#undef SPRAWL_GETTER
#undef SPRAWL_HASCHECK
#undef SPRAWL_ERASER
#undef SPRAWL_FINDER
#undef SPRAWL_INDEXED_RESERVE
#undef SPRAWL_INDEXED_CLEAR
#undef SPRAWL_INDEXED_REHASH
#undef SPRAWL_INDEX_INSERTER
#undef SPRAWL_INDEX_GETTER*/
#else
#include "hashmap/HashMap_Variadic.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		using HashSet = HashMap<ValueType, SelfAccessor<ValueType>>;
	}
}
#endif
