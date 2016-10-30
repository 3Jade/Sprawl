#pragma once

#include "hashmap/HashMap_Variadic.hpp"

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType>
		using HashSet = HashMap<ValueType, SelfAccessor<ValueType>>;

		template<typename KeyType, typename ValueType>
		using BasicHashMap = HashMap<ValueType, KeyAccessor<ValueType, KeyType>>;

		template<typename KeyType, typename ValueType, KeyType(ValueType::*function)()>
		using MemberHashMap = HashMap<ValueType, MemberAccessor<ValueType, KeyType, function>>;

		template<typename KeyType, typename ValueType, KeyType(ValueType::*function)() const>
		using ConstMemberHashMap = HashMap<ValueType, ConstMemberAccessor<ValueType, KeyType, function>>;

		namespace detail
		{
			template<typename T>
			struct UnderlyingType
			{
				typedef typename std::remove_reference<decltype(*(std::declval<T>()))>::type type;
			};

			template<typename KeyType, typename ValueType>
			struct MethodType
			{
				typedef typename UnderlyingType<ValueType>::type UType;
				typedef KeyType(UType::*type)(void);
				typedef KeyType(UType::*const_type)(void) const;
			};
		}

		template<typename KeyType, typename ValueType, typename detail::MethodType<KeyType, ValueType>::type function>
		using PtrMemberHashMap = HashMap<ValueType, PtrMemberAccessor<typename detail::UnderlyingType<ValueType>::type, KeyType, function, ValueType>>;

		template<typename KeyType, typename ValueType, typename detail::MethodType<KeyType, ValueType>::const_type function>
		using PtrConstMemberHashMap = HashMap<ValueType, PtrConstMemberAccessor<typename detail::UnderlyingType<ValueType>::type, KeyType, function, ValueType>>;

		template<typename KeyType, typename ValueType, KeyType(*function)(ValueType*)>
		using FunctionHashMap = HashMap<ValueType, FunctionAccessor<ValueType, KeyType, function>>;

		template<typename KeyType, typename ValueType, KeyType(*function)(typename detail::UnderlyingType<ValueType>::type*)>
		using PtrFunctionHashMap = HashMap<ValueType, PtrFunctionAccessor<typename detail::UnderlyingType<ValueType>::type, KeyType, function, ValueType>>;
	}
}
