#pragma once

#include "../../common/specialized.hpp"
#include "../../common/compat.hpp"

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename ValueType, typename MostInheritedType, size_t index, typename... AdditionalAccessors>
			class AccessorGroup_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index>
			class AccessorGroup_Impl<ValueType, MostInheritedType, index>
			{
			public:
				AccessorGroup_Impl(ValueType const& value)
					: next(nullptr)
					, prev(nullptr)
					, m_value(value)
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

				inline NullAccessor& Accessor(Specialized<index>)
				{
					static NullAccessor accessor;
					return accessor;
				}

				MostInheritedType* next;
				MostInheritedType* prev;
				ValueType m_value;
			private:
				AccessorGroup_Impl(AccessorGroup_Impl& other);
				AccessorGroup_Impl(MostInheritedType& other);
			};

			template<typename ValueType, typename MostInheritedType, size_t index, typename AccessorType, typename... AdditionalAccessors>
			class AccessorGroup_Impl<ValueType, MostInheritedType, index, AccessorType, AdditionalAccessors...> : public AccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...>
			{
			public:
				typedef AccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...> Base;

				AccessorGroup_Impl(ValueType const& value)
					: Base(value)
					, m_thisAccessor(this->m_value)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				template<typename... Params>
				AccessorGroup_Impl(ValueType const& value, typename AccessorType::arg_type const& key, Params... moreKeys)
					: Base(value, moreKeys...)
					, m_thisAccessor(this->m_value, key)
					, m_nextThisAccessor(nullptr)
					, m_prevThisAccessor(nullptr)
					, m_thisIdx(0)
				{
					//
				}

				template<typename... Params>
				AccessorGroup_Impl(ValueType const& value, Params... moreKeys)
					: Base(value, moreKeys...)
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

				using Base::Accessor;
				inline AccessorType& Accessor(Specialized<index>)
				{
					return m_thisAccessor;
				}

				AccessorType m_thisAccessor;

				MostInheritedType* m_nextThisAccessor;
				MostInheritedType* m_prevThisAccessor;

				size_t m_thisIdx;
				size_t m_thisHash;
			};

			template<typename ValueType, typename... Accessors>
			class AccessorGroup : public AccessorGroup_Impl<ValueType, AccessorGroup<ValueType, Accessors...>, 1, Accessors...>
			{
			public:
				typedef AccessorGroup_Impl<ValueType, AccessorGroup<ValueType, Accessors...>, 1, Accessors...> Base;
				AccessorGroup(ValueType const& value)
					: Base(value)
				{
					//
				}

				template<typename... Params>
				AccessorGroup(ValueType const& value, Params... keys)
					: Base(value, keys...)
				{
					//
				}
			private:
				AccessorGroup(AccessorGroup& other);
			};
		}
	}
}
