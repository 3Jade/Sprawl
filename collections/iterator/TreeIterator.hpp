#pragma once
#include <iterator>
#include "../../common/specialized.hpp"

namespace sprawl
{
	template<typename ValueType, typename AccessorType, int Idx>
	class TreeIterator : public std::iterator<std::bidirectional_iterator_tag, ValueType, std::ptrdiff_t, ValueType*, ValueType&>
	{
	public:
		typedef AccessorType* accessor_type;
		TreeIterator(AccessorType* item)
			: m_currentItem(item)
		{
			//
		}

		AccessorType& operator*()
		{
			return *m_currentItem;
		}

		AccessorType* operator->()
		{
			return m_currentItem;
		}


		AccessorType const& operator*() const
		{
			return *m_currentItem;
		}

		AccessorType const* operator->() const
		{
			return m_currentItem;
		}

		template<int i>
		auto Key() const -> decltype(accessor_type(nullptr)->Accessor(Specialized<i>()).Key())
		{
			return m_currentItem->Accessor(Specialized<i>()).Key();
		}

		auto Key() const -> decltype(accessor_type(nullptr)->Accessor(Specialized<0>()).Key())
		{
			return m_currentItem->Accessor(Specialized<0>()).Key();
		}

		ValueType& Value()
		{
			return m_currentItem->m_value;
		}

		ValueType const& Value() const
		{
			return m_currentItem->m_value;
		}

		TreeIterator<ValueType, AccessorType, Idx>& operator++()
		{
			m_currentItem = nextItem_(m_currentItem);
			return *this;
		}

		TreeIterator<ValueType, AccessorType, Idx> operator++(int)
		{
			TreeIterator<ValueType, AccessorType, Idx> tmp(*this);
			++(*this);
			return tmp;
		}

		TreeIterator<ValueType, AccessorType, Idx> const& operator++() const
		{
			m_currentItem = nextItem_(m_currentItem);
			return *this;
		}

		TreeIterator<ValueType, AccessorType, Idx> const operator++(int) const
		{
			TreeIterator<ValueType, AccessorType, Idx> tmp(*this);
			++(*this);
			return tmp;
		}

		TreeIterator<ValueType, AccessorType, Idx> operator+(int steps)
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = nextItem_(m_currentItem);
			}
			return TreeIterator<ValueType, AccessorType, Idx>(item);
		}

		TreeIterator<ValueType, AccessorType, Idx> const operator+(int steps) const
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = nextItem_(m_currentItem);
			}
			return TreeIterator<ValueType, AccessorType, Idx>(item);
		}

		TreeIterator<ValueType, AccessorType, Idx>& operator--()
		{
			m_currentItem = previousItem_(m_currentItem);
			return *this;
		}

		TreeIterator<ValueType, AccessorType, Idx> operator--(int)
		{
			TreeIterator<ValueType, AccessorType, Idx> tmp(*this);
			--(*this);
			return tmp;
		}

		TreeIterator<ValueType, AccessorType, Idx> const& operator--() const
		{
			m_currentItem = previousItem_(m_currentItem);
			return *this;
		}

		TreeIterator<ValueType, AccessorType, Idx> const operator--(int) const
		{
			TreeIterator<ValueType, AccessorType, Idx> tmp(*this);
			--(*this);
			return tmp;
		}

		TreeIterator<ValueType, AccessorType, Idx> operator-(int steps)
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = previousItem_(m_currentItem);
			}
			return TreeIterator<ValueType, AccessorType, Idx>(item);
		}

		TreeIterator<ValueType, AccessorType, Idx> const operator-(int steps) const
		{
			AccessorType* item = m_currentItem;
			for(int i = 0; i < steps; ++i)
			{
				if(!item)
				{
					break;
				}
				item = previousItem_(m_currentItem);
			}
			return TreeIterator<ValueType, AccessorType, Idx>(item);
		}

		bool operator==(TreeIterator<ValueType, AccessorType, Idx> const& rhs) const
		{
			return m_currentItem == rhs.m_currentItem;
		}

		bool operator!=(TreeIterator<ValueType, AccessorType, Idx> const& rhs) const
		{
			return !this->operator==(rhs);
		}

		operator bool() const
		{
			return m_currentItem != nullptr;
		}

		bool operator!() const
		{
			return m_currentItem != nullptr;
		}

		bool Valid() const
		{
			return m_currentItem != nullptr;
		}

		bool More() const
		{
			return m_currentItem != nullptr && nextItem_(m_currentItem) != nullptr;
		}

		TreeIterator<ValueType, AccessorType, Idx> Next()
		{
			return TreeIterator<ValueType, AccessorType, Idx>(nextItem_(m_currentItem));
		}

		TreeIterator<ValueType, AccessorType, Idx> const Next() const
		{
			return TreeIterator<ValueType, AccessorType, Idx>(nextItem_(m_currentItem));
		}

		operator bool()
		{
			return m_currentItem != nullptr;
		}

	protected:
		AccessorType* previousItem_(AccessorType* item) const
		{
			AccessorType* left = item->Left(spec);
			if(left != nullptr)
			{
				while(left->Right(spec) != nullptr)
				{
					left = left->Right(spec);
				}
				return left;
			}
			AccessorType* parent = item->Parent(spec);
			if(parent == nullptr || item == parent->Right(spec))
			{
				return parent;
			}
			AccessorType* child = item;
			while(parent != nullptr && child == parent->Left(spec))
			{
				child = parent;
				parent = parent->Parent(spec);
			}
			return parent;
		}

		AccessorType* nextItem_(AccessorType* item) const
		{
			AccessorType* right = item->Right(spec);
			if(right != nullptr)
			{
				while(right->Left(spec) != nullptr)
				{
					right = right->Left(spec);
				}
				return right;
			}
			AccessorType* parent = item->Parent(spec);
			if(parent == nullptr || item == parent->Left(spec))
			{
				return parent;
			}
			AccessorType* child = item;
			while(parent != nullptr && child == parent->Right(spec))
			{
				child = parent;
				parent = parent->Parent(spec);
			}
			return parent;
		}

		mutable AccessorType* m_currentItem;
		Specialized<Idx> spec;
	};
}
