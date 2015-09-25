#pragma once

#include "binarytree/BinaryTree_Variadic.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		using BinarySet = BinaryTree<ValueType, SelfAccessor<ValueType>>;

		template<typename KeyType, typename ValueType>
		using BasicBinaryTree = BinaryTree<ValueType, KeyAccessor<ValueType, KeyType>>;

		template<typename KeyType, typename ValueType, KeyType(ValueType::*function)()>
		using MemberBinaryTree = BinaryTree<ValueType, MemberAccessor<ValueType, KeyType, function>>;

		template<typename KeyType, typename ValueType, KeyType(ValueType::*function)() const>
		using ConstMemberBinaryTree = BinaryTree<ValueType, ConstMemberAccessor<ValueType, KeyType, function>>;

		namespace detail
		{
			template<typename T>
			struct TreeUnderlyingType
			{
				typedef typename std::remove_reference<decltype(*(std::declval<T>()))>::type type;
			};

			template<typename KeyType, typename ValueType>
			struct TreeMethodType
			{
				typedef typename TreeUnderlyingType<ValueType>::type UType;
				typedef KeyType(UType::*type)(void);
				typedef KeyType(UType::*const_type)(void) const;
			};
		}

		template<typename KeyType, typename ValueType, typename detail::TreeMethodType<KeyType, ValueType>::type function>
		using PtrMemberBinaryTree = BinaryTree<ValueType, PtrMemberAccessor<typename detail::TreeUnderlyingType<ValueType>::type, KeyType, function, ValueType>>;

		template<typename KeyType, typename ValueType, typename detail::TreeMethodType<KeyType, ValueType>::const_type function>
		using PtrConstMemberBinaryTree = BinaryTree<ValueType, PtrConstMemberAccessor<typename detail::TreeUnderlyingType<ValueType>::type, KeyType, function, ValueType>>;

		template<typename KeyType, typename ValueType, KeyType(*function)(ValueType*)>
		using FunctionBinaryTree = BinaryTree<ValueType, FunctionAccessor<ValueType, KeyType, function>>;

		template<typename KeyType, typename ValueType, KeyType(*function)(typename detail::TreeUnderlyingType<ValueType>::type*)>
		using PtrFunctionBinaryTree = BinaryTree<ValueType, PtrFunctionAccessor<typename detail::TreeUnderlyingType<ValueType>::type, KeyType, function, ValueType>>;
	}
}
