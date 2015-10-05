#pragma once

#include "../../common/specialized.hpp"
#include "../../common/compat.hpp"

namespace sprawl
{
	template<typename A, typename B, int Idx>
	class TreeIterator;

	namespace collections
	{
		template< typename A, typename... B >
		class BinaryTree;

		namespace detail
		{
			enum class RedBlackColor : bool
			{
				Red,
				Black,
			};

			template< typename A, typename B, size_t C, typename... D >
			class BinaryTree_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index, typename... AdditionalAccessors>
			class TreeAccessorGroup_Impl;

			template<typename ValueType, typename MostInheritedType, size_t index>
			class TreeAccessorGroup_Impl<ValueType, MostInheritedType, index>
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
				friend class sprawl::collections::BinaryTree;
				template< typename A, typename B, size_t C, typename... D >
				friend class sprawl::collections::detail::BinaryTree_Impl;
				template<typename A, typename B, int Idx>
				friend class sprawl::TreeIterator;

				TreeAccessorGroup_Impl(ValueType const& value)
					: m_value(value)
				{
					//
				}

				TreeAccessorGroup_Impl(ValueType&& value)
					: m_value(std::move(value))
				{
					//
				}

				TreeAccessorGroup_Impl(TreeAccessorGroup_Impl const& other)
					: m_value(other.m_value)
				{
					//
				}

				inline MostInheritedType* Left(Specialized<index>)
				{
					return nullptr;
				}

				inline MostInheritedType const* Left(Specialized<index>) const
				{
					return nullptr;
				}

				inline MostInheritedType* Right(Specialized<index>)
				{
					return nullptr;
				}

				inline MostInheritedType const* Right(Specialized<index>) const
				{
					return nullptr;
				}

				inline MostInheritedType* Parent(Specialized<index>)
				{
					return nullptr;
				}

				inline MostInheritedType const* Parent(Specialized<index>) const
				{
					return nullptr;
				}

				inline RedBlackColor Color(Specialized<index>)
				{
					return RedBlackColor::Red;
				}

				inline void SetLeft(Specialized<index>, MostInheritedType*)
				{
					//
				}

				inline void SetRight(Specialized<index>, MostInheritedType*)
				{
					//
				}

				inline void SetParent(Specialized<index>, MostInheritedType*)
				{
					//
				}

				inline void SetColor(Specialized<index>, RedBlackColor)
				{
					//
				}

				ValueType m_value;
			};

			template<typename ValueType, typename MostInheritedType, size_t index, typename AccessorType, typename... AdditionalAccessors>
			class TreeAccessorGroup_Impl<ValueType, MostInheritedType, index, AccessorType, AdditionalAccessors...> : public TreeAccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...>
			{
			public:
				typedef TreeAccessorGroup_Impl<ValueType, MostInheritedType, index+1, AdditionalAccessors...> Base;

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
				friend class sprawl::collections::BinaryTree;
				template< typename A, typename B, size_t C, typename... D >
				friend class sprawl::collections::detail::BinaryTree_Impl;
				template<typename A, typename B, int Idx>
				friend class sprawl::TreeIterator;

				TreeAccessorGroup_Impl(ValueType const& value)
					: Base(value)
					, m_thisAccessor(this->m_value)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				TreeAccessorGroup_Impl(ValueType&& value)
					: Base(std::move(value))
					, m_thisAccessor(this->m_value)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				TreeAccessorGroup_Impl(TreeAccessorGroup_Impl const& other)
					: Base(other)
					, m_thisAccessor(other.m_thisAccessor)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				//Note: In all of these constructors, this->m_value isn't initialized yet.
				//But accessors hold a reference to it, and don't do anything with it during initialization.
				//So it's safe to give them the memory address even though we haven't initialized it yet.

				//Case one: Exactly two parameters, first one is key type, second one is value type.
				//Initialize with key, pass value up.
				template<typename Param1, typename = typename std::enable_if<std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				TreeAccessorGroup_Impl(Param1 const& key, ValueType const& value)
					: Base(value)
					, m_thisAccessor(this->m_value, key)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				//Case two: Three or more parameters, first one is key type. Second one can't be value because value is last, so if we have a third parameter, second isn't value.
				//Initialize with key, pass remaining parameters up.
				template<typename Param1, typename Param2, typename... Params, typename = typename std::enable_if<std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				TreeAccessorGroup_Impl(Param1 const& key, Param2&& leftParam, Params&&... moreParams)
					: Base(std::forward<Param2>(leftParam), std::forward<Params>(moreParams)...)
					, m_thisAccessor(this->m_value, key)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				//Case three: Exactly two parameters, first one is not key type, second one's type doesn't matter.
				//Pass both parameters up.
				template<typename Param1, typename Param2, typename = typename std::enable_if<!std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				TreeAccessorGroup_Impl(Param1&& firstParam, Param2&& leftParam)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(leftParam))
					, m_thisAccessor(this->m_value)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				//Case four: More than two parameters, first one is not key type.
				//Pass all parameters up.
				template<typename Param1, typename Param2, typename... Params, typename = typename std::enable_if<!std::is_constructible<typename AccessorType::arg_type const&, Param1>::value>::type>
				TreeAccessorGroup_Impl(Param1&& firstParam, Param2&& secondParam, Params&&... moreKeys)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(secondParam), std::forward<Params>(moreKeys)...)
					, m_thisAccessor(this->m_value)
					, m_leftThisAccessor(nullptr)
					, m_rightThisAccessor(nullptr)
					, m_parentThisAccessor(nullptr)
					, m_colorThisAccessor(RedBlackColor::Red)
				{
					//
				}

				using Base::Left;
				inline MostInheritedType* Left(Specialized<index>)
				{
					return m_leftThisAccessor;
				}

				inline MostInheritedType const* Left(Specialized<index>) const
				{
					return m_leftThisAccessor;
				}

				using Base::Right;
				inline MostInheritedType* Right(Specialized<index>)
				{
					return m_rightThisAccessor;
				}

				inline MostInheritedType const* Right(Specialized<index>) const
				{
					return m_rightThisAccessor;
				}

				using Base::Parent;
				inline MostInheritedType* Parent(Specialized<index>)
				{
					return m_parentThisAccessor;
				}

				inline MostInheritedType const* Parent(Specialized<index>) const
				{
					return m_parentThisAccessor;
				}

				using Base::Color;
				inline RedBlackColor Color(Specialized<index>) const
				{
					return m_colorThisAccessor;
				}


				using Base::SetLeft;
				inline void SetLeft(Specialized<index>, MostInheritedType* left)
				{
					m_leftThisAccessor = left;
				}

				using Base::SetRight;
				inline void SetRight(Specialized<index>, MostInheritedType* right)
				{
					m_rightThisAccessor = right;
				}

				using Base::SetParent;
				inline void SetParent(Specialized<index>, MostInheritedType* right)
				{
					m_parentThisAccessor = right;
				}

				using Base::SetColor;
				inline void SetColor(Specialized<index>, RedBlackColor color)
				{
					m_colorThisAccessor = color;
				}

				AccessorType m_thisAccessor;

				MostInheritedType* m_leftThisAccessor;
				MostInheritedType* m_rightThisAccessor;
				MostInheritedType* m_parentThisAccessor;
				RedBlackColor m_colorThisAccessor;
			};

			template<typename ValueType, typename... Accessors>
			class TreeAccessorGroup : public TreeAccessorGroup_Impl<ValueType, TreeAccessorGroup<ValueType, Accessors...>, 0, Accessors...>
			{
			public:
				typedef TreeAccessorGroup_Impl<ValueType, TreeAccessorGroup<ValueType, Accessors...>, 0, Accessors...> Base;
				typedef Base* BasePtr;

				template<typename Param1, typename Param2, typename... Params>
				TreeAccessorGroup(Param1&& firstParam, Param2&& secondParam, Params&&... params)
					: Base(std::forward<Param1>(firstParam), std::forward<Param2>(secondParam), std::forward<Params>(params)...)
				{
					//
				}

				TreeAccessorGroup(ValueType const& value)
					: Base(value)
				{
					//
				}

				TreeAccessorGroup(ValueType&& value)
					: Base(std::move(value))
				{
					//
				}

				TreeAccessorGroup(TreeAccessorGroup const& other)
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
