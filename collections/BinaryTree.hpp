#pragma once

//Support for legacy compilers lacking variadic template support
#if (defined(_WIN32) && _MSC_VER < 1800)
/*TODO: Support these too
|| (defined(__clang__) && (__clang_major__ < 2 || (__clang_major__ == 2 && __clang_minor__ < 9))) \
	|| (!defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))) \
	|| defined(SPRAWL_NO_VARIADIC_TEMPLATES)*/

#include "binarytree/BinaryTree_Windows.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		class BinarySet : public BinaryTree<ValueType, SelfAccessor<ValueType>, _MAX_NIL_LIST>
		{
			//
		};
	}
}
#else
#include "binarytree/BinaryTree_Variadic.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		using BinarySet = BinaryTree<ValueType, SelfAccessor<ValueType>>;
	}
}
#endif
