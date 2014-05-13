#pragma once

#include "../../common/specialized.hpp"

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

				MostInheritedType* Next(Specialized<index>)
				{
					return nullptr;
				}

				MostInheritedType* Prev(Specialized<index>)
				{
					return nullptr;
				}

				void SetNext(Specialized<index>, MostInheritedType*)
				{
					//
				}

				void SetPrev(Specialized<index>, MostInheritedType*)
				{
					//
				}

				size_t Idx(Specialized<index>)
				{
					return -1;
				}

				void SetIndex(Specialized<index>, size_t)
				{
					//
				}

				NullAccessor& Accessor(Specialized<index>)
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
				MostInheritedType* Next(Specialized<index>)
				{
					return m_nextThisAccessor;
				}

				using Base::Prev;
				MostInheritedType* Prev(Specialized<index>)
				{
					return m_prevThisAccessor;
				}

				using Base::SetNext;
				void SetNext(Specialized<index>, MostInheritedType* next)
				{
					m_nextThisAccessor = next;
				}

				using Base::SetPrev;
				void SetPrev(Specialized<index>, MostInheritedType* prev)
				{
					m_prevThisAccessor = prev;
				}

				using Base::Idx;
				size_t Idx(Specialized<index>)
				{
					return m_thisIdx;
				}

				using Base::SetIndex;
				void SetIndex(Specialized<index>, size_t idx)
				{
					m_thisIdx = idx;
				}

				using Base::Accessor;
				AccessorType& Accessor(Specialized<index>)
				{
					return m_thisAccessor;
				}

				AccessorType m_thisAccessor;

				MostInheritedType* m_nextThisAccessor;
				MostInheritedType* m_prevThisAccessor;

				size_t m_thisIdx;
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
