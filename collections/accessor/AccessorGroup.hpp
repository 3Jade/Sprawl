#pragma once

#include "../../common/specialized.hpp"
#include <xtr1common>

using std::_Nil;

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template<typename ValueType, typename MostInheritedType, size_t index, typename ThisAccessor = _Nil, _MAX_CLASS_LIST>
			class AccessorGroup_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index>
			class AccessorGroup_Impl<ValueType, MostInheritedType, index, _Nil, _MAX_NIL_LIST>
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
			
#define _CLASS_ACCESSORGROUP_IMPL( \
			TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			template< typename ValueType, typename MostInheritedType, size_t index, typename AccessorType COMMA LIST(_CLASS_TYPEX)> \
			class AccessorGroup_Impl<ValueType, MostInheritedType, index, AccessorType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
				: public AccessorGroup_Impl< ValueType, MostInheritedType, index + 1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD) > \
			{ \
			public: \
				typedef AccessorGroup_Impl<ValueType, MostInheritedType, index+1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> Base; \
				 \
				AccessorGroup_Impl(ValueType const& value) \
					: Base(value) \
					, m_thisAccessor(this->m_value) \
					, m_nextThisAccessor(nullptr) \
					, m_prevThisAccessor(nullptr) \
					, m_thisIdx(0) \
				{ \
					\
				} \
				 \
				AccessorGroup_Impl(ValueType const& value, typename AccessorType::arg_type const& key) \
					: Base(value) \
					, m_thisAccessor(this->m_value, key) \
					, m_nextThisAccessor(nullptr) \
					, m_prevThisAccessor(nullptr) \
					, m_thisIdx(0) \
				{ \
					\
				} \
				 \
				_ACCESSORGROUP_IMPL_VARIADICS( TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
				 \
				using Base::Next; \
				inline MostInheritedType* Next(Specialized<index>) \
				{ \
					return m_nextThisAccessor; \
				} \
				 \
				using Base::Prev; \
				inline MostInheritedType* Prev(Specialized<index>) \
				{ \
					return m_prevThisAccessor; \
				} \
				 \
				using Base::SetNext; \
				inline void SetNext(Specialized<index>, MostInheritedType* next) \
				{ \
					m_nextThisAccessor = next; \
				} \
				 \
				using Base::SetPrev; \
				inline void SetPrev(Specialized<index>, MostInheritedType* prev) \
				{ \
					m_prevThisAccessor = prev; \
				} \
				 \
				using Base::Idx; \
				inline size_t Idx(Specialized<index>) \
				{ \
					return m_thisIdx; \
				} \
				 \
				using Base::SetIndex; \
				inline void SetIndex(Specialized<index>, size_t idx) \
				{ \
					m_thisIdx = idx; \
				} \
				 \
				using Base::GetHash; \
				inline size_t GetHash(Specialized<index>) \
				{ \
					return m_thisHash; \
				} \
				 \
				using Base::SetHash; \
				inline void SetHash(Specialized<index>, size_t hash) \
				{ \
					m_thisHash = hash; \
				} \
				 \
				using Base::Accessor; \
				inline AccessorType& Accessor(Specialized<index>) \
				{ \
					return m_thisAccessor; \
				} \
				 \
				AccessorType m_thisAccessor; \
				 \
				MostInheritedType* m_nextThisAccessor; \
				MostInheritedType* m_prevThisAccessor; \
				 \
				size_t m_thisIdx; \
				size_t m_thisHash; \
			};


#define _ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CLASS_TEMPLATE_LIST, CLASS_PADDING_LIST, CLASS_LIST, CLASS_COMMA) \
				template<typename FirstArg COMMA LIST(_CLASS_TYPE)> \
				AccessorGroup_Impl(ValueType const& value, typename AccessorType::arg_type const& key, FirstArg const& arg COMMA LIST(_TYPE_REFREF_ARG)) \
					: Base(value, arg, LIST(_FORWARD_ARG)) \
					, m_thisAccessor(this->m_value, key) \
					, m_nextThisAccessor(nullptr) \
					, m_prevThisAccessor(nullptr) \
					, m_thisIdx(0) \
				{ \
					\
				} \
				 \
				template<typename FirstArg COMMA LIST(_CLASS_TYPE)> \
				AccessorGroup_Impl(ValueType const& value, FirstArg const& arg COMMA LIST(_TYPE_REFREF_ARG)) \
					: Base(value, arg, LIST(_FORWARD_ARG)) \
					, m_thisAccessor(this->m_value) \
					, m_nextThisAccessor(nullptr) \
					, m_prevThisAccessor(nullptr) \
					, m_thisIdx(0) \
				{ \
					\
				} \
				
#define _ACCESSORGROUP_IMPL_VARIADICS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST0, _PAD_LIST0, _RAW_LIST0, , TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST1, _PAD_LIST1, _RAW_LIST1, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST2, _PAD_LIST2, _RAW_LIST2, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST3, _PAD_LIST3, _RAW_LIST3, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST4, _PAD_LIST4, _RAW_LIST4, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_IMPL_VARIADIC_VARIADICSS(_TEM_LIST5, _PAD_LIST5, _RAW_LIST5, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA)

			_VARIADIC_EXPAND_0X(_CLASS_ACCESSORGROUP_IMPL, , , , )
				
			template<typename ValueType, typename ThisAccessor = _Nil, _MAX_CLASS_LIST>
			class AccessorGroup;

#define _CLASS_ACCESSORGROUP( \
			TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			template<typename ValueType COMMA LIST(_CLASS_TYPEX)> \
			class AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
				: public AccessorGroup_Impl<ValueType, AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)>, 1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> \
			{ \
			public: \
				typedef AccessorGroup_Impl<ValueType, AccessorGroup<ValueType, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)>, 1, LIST(_TYPEX) COMMA PADDING_LIST(_NIL_PAD)> Base; \
				AccessorGroup(ValueType const& value) \
					: Base(value) \
				{ \
					\
				} \
				 \
				_ACCESSORGROUP_VARIADICS( TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
				 \
			private: \
				AccessorGroup(AccessorGroup& other); \
			};


#define _ACCESSORGROUP_VARIADIC_VARIADICSS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CLASS_TEMPLATE_LIST, CLASS_PADDING_LIST, CLASS_LIST, CLASS_COMMA) \
				template<typename FirstAccessor COMMA LIST(_CLASS_TYPE)> \
				AccessorGroup(ValueType const& value, FirstAccessor const& accessor COMMA LIST(_TYPE_REFREF_ARG)) \
					: Base(value, accessor COMMA LIST(_FORWARD_ARG)) \
				{ \
					\
				}
				
#define _ACCESSORGROUP_VARIADICS(TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST0, _PAD_LIST0, _RAW_LIST0, , TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST1, _PAD_LIST1, _RAW_LIST1, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST2, _PAD_LIST2, _RAW_LIST2, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST3, _PAD_LIST3, _RAW_LIST3, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST4, _PAD_LIST4, _RAW_LIST4, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA) \
			_ACCESSORGROUP_VARIADIC_VARIADICSS(_TEM_LIST5, _PAD_LIST5, _RAW_LIST5, _COMMA, TEMPLATE_LIST, PADDING_LIST, LIST, COMMA)

			_VARIADIC_EXPAND_0X(_CLASS_ACCESSORGROUP, , , , )
		}
	}
}
