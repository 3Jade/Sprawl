#pragma once

#include "../../common/specialized.hpp"
#include "../../common/compat.hpp"

namespace sprawl
{
	template<typename A, typename B>
	class MapIterator;

	namespace collections
	{
		template< typename A, typename... B >
		class HashMap;

		namespace detail
		{
			template< typename A, typename B, size_t C, typename... D >
			class HashMap_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index, typename... AdditionalAccessors>
			class MapAccessorGroup_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index>
			class MapAccessorGroup_Impl<ValueType, MostInheritedType, index>
			{
			public:
				ValueType& Value()
				{
					return m_value;
				}

				ValueType const& Value() const
				{
					return m_value;
				}

				ValueType& operator*()
				{
					return m_value;
				}

				ValueType* operator->()
				{
					return &m_value;
				}


				ValueType const& operator*() const
				{
					return m_value;
				}

				ValueType const* operator->() const
				{
					return &m_value;
				}

				inline NullAccessor& Accessor(Specialized<index>)
				{
					static NullAccessor accessor;
					return accessor;
				}

				inline NullAccessor const& Accessor(Specialized<index>) const
				{
					static NullAccessor accessor;
					return accessor;
				}

			protected:
				template< typename A, typename... B >
				friend class sprawl::collections::HashMap;
				template< typename A, typename B, size_t C, typename... D >
				friend class sprawl::collections::detail::HashMap_Impl;
				template<typename A, typename B>
				friend class sprawl::MapIterator;

				MapAccessorGroup_Impl(ValueType const& value)
					: next(nullptr)
					, prev(nullptr)
					, m_value(value)
				{
					//
				}

				MapAccessorGroup_Impl(ValueType&& value)
					: next(nullptr)
					, prev(nullptr)
					, m_value(std::move(value))
				{
					//
				}

				MapAccessorGroup_Impl(MapAccessorGroup_Impl const& other)
					: next(nullptr)
					, prev(nullptr)
					, m_value(other.m_value)
				{
					//
				}

				inline MostInheritedType* Next(Specialized<index>)
				{
					return nullptr;
				}

				inline MostInheritedType* Prev(Specialized<index>)
				{
					return nullptr;
				}

				inline void SetNext(Specialized<index>, MostInheritedType*)
				{
					//
				}

				inline void SetPrev(Specialized<index>, MostInheritedType*)
				{
					//
				}

				inline size_t Idx(Specialized<index>)
				{
					return -1;
				}

				inline void SetIndex(Specialized<index>, size_t)
				{
					//
				}

				inline size_t GetHash(Specialized<index>)
				{
					return -1;
				}

				inline void SetHash(Specialized<index>, size_t)
				{
					//
				}

				MostInheritedType* next;
				MostInheritedType* prev;
				ValueType m_value;
			};

			template<typename ValueType, typename MostInheritedType, size_t index, typename AccessorType, typename... AdditionalAccessors>
			class MapAccessorGroup_Impl<ValueType, MostInheritedType, index, AccessorType, AdditionalAccessors...> : public MapAccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...>
			{
			public:
				typedef MapAccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...> Base;

				using Base::Accessor;
				inline AccessorType& Accessor(Specialized<index>)
				{
					return m_thisAccessor;
				}

				inline AccessorType const& Accessor(Specialized<index>) const
				{
					return m_thisAccessor;
				}

			protected:
				template< typename A, typename... B >
				friend class sprawl::collections::HashMap;
				template< typename A, typename B, size_t C, typename... D >
				friend class sprawl::collections::detail::HashMap_Impl;
				template<typename A, typename B>
				friend class sprawl::MapIterator;

				MapAccessorGroup_Impl(ValueType const& value)
					: Base(value)
					, m_thisAccessor(this->m_value)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				MapAccessorGroup_Impl(ValueType&& value)
					: Base(std::move(value))
					, m_thisAccessor(this->m_value)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				MapAccessorGroup_Impl(MapAccessorGroup_Impl const& other)
					: Base(other)
					, m_thisAccessor(other.m_thisAccessor)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				//Note: In all of these constructors, this->m_value isn't initialized yet.
				//But accessors hold a reference to it, and don't do anything with it during initialization.
				//So it's safe to give them the memory address even though we haven't initialized it yet.

				//Case one: Exactly two parameters, first one is key type, second one is value type.
				//Initialize with key, pass value up.
				template<typename Param1, typename = typename std::enable_if<std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				MapAccessorGroup_Impl(Param1 const& key, ValueType const& value)
					: Base(value)
					, m_thisAccessor(this->m_value, key)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				//Case two: Three or more parameters, first one is key type. Second one can't be value because value is last, so if we have a third parameter, second isn't value.
				//Initialize with key, pass remaining parameters up.
				template<typename Param1, typename Param2, typename... Params, typename = typename std::enable_if<std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				MapAccessorGroup_Impl(Param1 const& key, Param2&& nextParam, Params&&... moreParams)
					: Base(std::forward<Param2>(nextParam), std::forward<Params>(moreParams)...)
					, m_thisAccessor(this->m_value, key)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				//Case three: Exactly two parameters, first one is not key type, second one's type doesn't matter.
				//Pass both parameters up.
				template<typename Param1, typename Param2, typename = typename std::enable_if<!std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				MapAccessorGroup_Impl(Param1&& firstParam, Param2&& nextParam)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(nextParam))
					, m_thisAccessor(this->m_value)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				//Case four: More than two parameters, first one is not key type.
				//Pass all parameters up.
				template<typename Param1, typename Param2, typename... Params, typename = typename std::enable_if<!std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				MapAccessorGroup_Impl(Param1&& firstParam, Param2&& secondParam, Params&&... moreKeys)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(secondParam), std::forward<Params>(moreKeys)...)
					, m_thisAccessor(this->m_value)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				using Base::Next;
				inline MostInheritedType* Next(Specialized<index>)
				{
					return m_nextThisAccessor;
				}

				using Base::Prev;
				inline MostInheritedType* Prev(Specialized<index>)
				{
					return m_prevThisAccessor;
				}

				using Base::SetNext;
				inline void SetNext(Specialized<index>, MostInheritedType* next)
				{
					m_nextThisAccessor = next;
				}

				using Base::SetPrev;
				inline void SetPrev(Specialized<index>, MostInheritedType* prev)
				{
					m_prevThisAccessor = prev;
				}

				using Base::Idx;
				inline size_t Idx(Specialized<index>)
				{
					return m_thisIdx;
				}

				using Base::SetIndex;
				inline void SetIndex(Specialized<index>, size_t idx)
				{
					m_thisIdx = idx;
				}

				using Base::GetHash;
				inline size_t GetHash(Specialized<index>)
				{
					return m_thisHash;
				}

				using Base::SetHash;
				inline void SetHash(Specialized<index>, size_t hash)
				{
					m_thisHash = hash;
				}

				AccessorType m_thisAccessor;

				MostInheritedType* m_nextThisAccessor;
				MostInheritedType* m_prevThisAccessor;

				size_t m_thisIdx;
				size_t m_thisHash;
			};

			template<typename ValueType, typename... Accessors>
			class MapAccessorGroup : public MapAccessorGroup_Impl<ValueType, MapAccessorGroup<ValueType, Accessors...>, 0, Accessors...>
			{
			public:
				typedef MapAccessorGroup_Impl<ValueType, MapAccessorGroup<ValueType, Accessors...>, 0, Accessors...> Base;
				typedef Base* BasePtr;

				template<typename Param1, typename Param2, typename... Params>
				MapAccessorGroup(Param1&& firstParam, Param2&& secondParam, Params&&... params)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(secondParam), std::forward<Params>(params)...)
				{
					//
				}

				MapAccessorGroup(ValueType const& value)
					: Base(value)
				{
					//
				}

				MapAccessorGroup(ValueType&& value)
					: Base(std::move(value))
				{
					//
				}

				MapAccessorGroup(MapAccessorGroup const& other)
					: Base(other)
				{
					//
				}

				template<int i>
				auto Key() const -> decltype(BasePtr(nullptr)->Accessor(Specialized<i>()).Key())
				{
					return this->Accessor(Specialized<i>()).Key();
				}

				auto Key() const -> decltype(BasePtr(nullptr)->Accessor(Specialized<0>()).Key())
				{
					return this->Accessor(Specialized<0>()).Key();
				}
			};
		}
	}
}
