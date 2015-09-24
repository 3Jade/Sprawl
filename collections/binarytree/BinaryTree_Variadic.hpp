#pragma once

#include "../iterator/TreeIterator.hpp"
#include "../accessor/Accessors.hpp"
#include "../accessor/AccessorGroup_BinaryTree.hpp"
#include "../../memory/PoolAllocator.hpp"
#include "../../common/specialized.hpp"

#include "../../string/String.hpp"
#include "../../string/StringBuilder.hpp"

namespace sprawl
{
	namespace collections
	{
		namespace detail
		{
			template< typename ValueType, typename mapped_type, size_t Idx, typename... AdditionalAccessors >
			class BinaryTree_Impl;

			template< typename ValueType, typename mapped_type, size_t Idx >
			class BinaryTree_Impl<ValueType, mapped_type, Idx>
			{
			public:
				typedef ValueType value_type;

				template<int i = 0>
				using iterator = TreeIterator<ValueType, mapped_type, i>;

				template<int i = 0>
				using const_iterator = TreeIterator<ValueType, mapped_type, i>;

				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

				template<typename RequestedKeyType>
				inline void Get(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void UpperBound(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void LowerBound(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline void Print(Specialized<Idx>)
				{
					ValueType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void GetOrInsert(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void find(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template< typename RequestedKeyType>
				inline void find(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void cfind(RequestedKeyType const&, Specialized<Idx>) const
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template< typename RequestedKeyType>
				inline void Erase(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				template<typename RequestedKeyType>
				inline void Has(RequestedKeyType const&, Specialized<Idx>)
				{
					RequestedKeyType::error_invalid_key_index_combination();
				}

				inline void begin(Specialized<Idx>)
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline void begin(Specialized<Idx>) const
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline void cbegin(Specialized<Idx>)
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline void end(Specialized<Idx>)
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline void end(Specialized<Idx>) const
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline void cend(Specialized<Idx>)
				{
					ValueType::error_invalid_key_index_combination();
				}

				inline size_t Size()
				{
					return m_size;
				}

				inline bool Empty()
				{
					return m_size == 0;
				}

				void Clear()
				{
					m_size = 0;
				}

				virtual ~BinaryTree_Impl()
				{
					Clear();
				}

			protected:

				inline bool checkAndInsert_(mapped_type* newItem)
				{
					insert_(newItem);
					return true;
				}

				inline void reinsert_(mapped_type*)
				{
					//
				}

				inline void insert_(mapped_type*)
				{
					++m_size;
				}

				void erase_(mapped_type* item)
				{
					--m_size;
					item->~mapped_type();
					allocator::free(item);
				}

				BinaryTree_Impl()
					: m_size(0)
				{
					//
				}

				BinaryTree_Impl(BinaryTree_Impl const& other)
					: m_size(other.m_size)
				{
					//
				}

				BinaryTree_Impl(BinaryTree_Impl&& other)
					: m_size(other.m_size)
				{
					other.m_size = 0;
				}

				size_t m_size;
			};

			template< typename ValueType, typename mapped_type, size_t Idx, typename Accessor, typename... AdditionalAccessors >
			class BinaryTree_Impl<ValueType, mapped_type, Idx, Accessor, AdditionalAccessors...> : public BinaryTree_Impl< ValueType, mapped_type, Idx + 1, AdditionalAccessors... >
			{
			public:
				typedef BinaryTree_Impl<ValueType, mapped_type, Idx + 1, AdditionalAccessors...> Base;

				typedef ValueType value_type;

				template<int i = 0>
				using iterator = TreeIterator<ValueType, mapped_type, i>;

				template<int i = 0>
				using const_iterator = TreeIterator<ValueType, mapped_type, i>;

				typedef sprawl::memory::DynamicPoolAllocator<sizeof(mapped_type)> allocator;

				using Base::Get;
				inline ValueType& Get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key)->Value();
				}

				inline ValueType const& Get(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return get_(key)->Value();
				}

				using Base::Print;
				inline void Print(Specialized<Idx>)
				{
					if(!m_thisKeyRootNode)
					{
						return;
					}
					printf("\n\n");
					if(m_thisKeyRootNode->Right(spec) != nullptr)
					{
						printNode_(m_thisKeyRootNode->Right(spec), true, "");
					}

					sprawl::StringBuilder builder;
					typename Accessor::key_type key = m_thisKeyRootNode->Accessor(spec).Key();
					builder << key;
					printf("%s\n", builder.Str().c_str());

					if(m_thisKeyRootNode->Left(spec) != nullptr)
					{
						printNode_(m_thisKeyRootNode->Left(spec), false, "");
					}
					printf("\n\n");
				}

				using Base::GetOrInsert;
				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, ValueType const& defaultValue, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, defaultValue).Value();
				}

				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, ValueType&& defaultValue, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, std::move(defaultValue)).Value();
				}

				inline ValueType& GetOrInsert(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					auto it = find(key, spec);
					if(it.Valid())
					{
						return it.Value();
					}
					return this->Insert(key, ValueType()).Value();
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, ValueType const& defaultValue, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<BinaryTree_Impl*>(this)->GetOrInsert(key, defaultValue, spec);
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, ValueType&& defaultValue, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<BinaryTree_Impl*>(this)->GetOrInsert(key, std::move(defaultValue), spec);
				}

				inline ValueType const& GetOrInsert(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<BinaryTree_Impl*>(this)->GetOrInsert(key, spec);
				}

				using Base::UpperBound;
				using Base::LowerBound;
				inline iterator<Idx> UpperBound(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* ret = getUpperBound_(key);
					return iterator<Idx>(ret);
				}

				inline const_iterator<Idx> UpperBound(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					mapped_type* ret = getUpperBound_(key);
					return iterator<Idx>(ret);
				}

				inline iterator<Idx> LowerBound(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* ret = getLowerBound_(key);
					return iterator<Idx>(ret);
				}

				inline const_iterator<Idx> LowerBound(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					mapped_type* ret = getLowerBound_(key);
					return iterator<Idx>(ret);
				}

				using Base::find;
				using Base::cfind;
				inline iterator<Idx> find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* ret = get_(key);
					return iterator<Idx>(ret);
				}

				inline const_iterator<Idx> find(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					return cfind(key);
				}

				inline const_iterator<Idx> cfind(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>()) const
				{
					mapped_type* ret = const_cast<mapped_type*>(get_(key));
					return const_iterator<Idx>(ret);
				}

				using Base::begin;
				using Base::cbegin;
				using Base::end;
				using Base::cend;
				inline iterator<Idx> begin(Specialized<Idx> = Specialized<Idx>())
				{
					mapped_type* left = m_thisKeyRootNode;
					while(left->Left(spec) != nullptr)
					{
						left = left->Left(spec);
					}
					return iterator<Idx>(left);
				}

				inline const_iterator<Idx> begin(Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_cast<BinaryTree_Impl*>(this)->begin();
				}

				inline const_iterator<Idx> cbegin(Specialized<Idx> = Specialized<Idx>()) const
				{
					return begin();
				}

				inline iterator<Idx> end(Specialized<Idx> = Specialized<Idx>())
				{
					return iterator<Idx>(nullptr);
				}

				inline const_iterator<Idx> end(Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_iterator<Idx>(nullptr);
				}

				inline const_iterator<Idx> cend(Specialized<Idx> = Specialized<Idx>()) const
				{
					return const_iterator<Idx>(nullptr);
				}

				using Base::Has;
				inline bool Has(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					return get_(key) != nullptr;
				}

				inline void Clear()
				{
					if(Idx == 0)
					{
						clear_(m_thisKeyRootNode);
					}
					m_thisKeyRootNode = nullptr;
					Base::Clear();
				}

				template<typename... Params>
				inline iterator<Idx> Insert(Params&&... keysAndValue)
				{
					mapped_type* newItem = (mapped_type*)allocator::alloc();
					::new((void*)newItem) mapped_type(std::forward<Params>(keysAndValue)...);

					if(!checkAndInsert_(newItem))
					{
						newItem->~mapped_type();
						allocator::free(newItem);
						return iterator<Idx>(nullptr);
					}

					return iterator<Idx>(newItem);
				}

				using Base::Erase;
				inline void Erase(typename Accessor::key_type const& key, Specialized<Idx> = Specialized<Idx>())
				{
					erase_(get_(key));
				}

				virtual ~BinaryTree_Impl()
				{
					Clear();
				}

			protected:
				BinaryTree_Impl()
					: Base()
					, m_thisKeyRootNode(nullptr)
				{
					//
				}

				BinaryTree_Impl(BinaryTree_Impl const& other)
					: Base(other)
					, m_thisKeyRootNode(nullptr)
				{
					//
				}

				BinaryTree_Impl(BinaryTree_Impl&& other)
					: Base(std::move(other))
					, m_thisKeyRootNode(other.m_thisKeyRootNode)
				{
					other.m_thisKeyRootNode = nullptr;
				}

				inline mapped_type* sibling_(mapped_type* node)
				{
					if(node == node->Parent(spec)->Left(spec))
					{
						return node->Parent(spec)->Right(spec);
					}
					return node->Parent(spec)->Left(spec);
				}

				inline mapped_type* grandparent_(mapped_type* node)
				{
					return node && node->Parent(spec) ? node->Parent(spec)->Parent(spec) : nullptr;
				}

				inline mapped_type* uncle_(mapped_type* node)
				{
					mapped_type* grandparent = grandparent_(node);
					if(!grandparent)
					{
						return nullptr;
					}
					if(node->Parent(spec) == grandparent->Left(spec))
					{
						return grandparent->Right(spec);
					}
					return grandparent->Left(spec);
				}

				inline void clear_(mapped_type* item)
				{
					if(item == nullptr)
					{
						return;
					}

					clear_(item->Left(spec));
					clear_(item->Right(spec));
					item->~mapped_type();
					allocator::free(item);
				}

				inline void reinsert_(mapped_type* newItem)
				{
					insertHere_(newItem);
					Base::reinsert_(newItem);
				}

				inline void insert_(mapped_type* newItem)
				{
					insertHere_(newItem);
					Base::insert_(newItem);
				}

				bool checkAndInsert_(mapped_type* newItem)
				{
					typename Accessor::key_type const& key = newItem->Accessor(spec).Key();

					mapped_type* node = m_thisKeyRootNode;
					while(node)
					{
						if(key == node->Accessor(spec).Key())
						{
							return false;
						}
						if(key < node->Accessor(spec).Key())
						{
							if(node->Left(spec))
							{
								node = node->Left(spec);
							}
							else
							{
								break;
							}
						}
						else
						{
							if(node->Right(spec))
							{
								node = node->Right(spec);
							}
							else
							{
								break;
							}
						}
					}

					if(!Base::checkAndInsert_(newItem))
					{
						return false;
					}

					insertHere_(newItem, node);
					return true;
				}

				void erase_(mapped_type* item)
				{
					if(item->Right(spec) != nullptr && item->Left(spec) != nullptr)
					{
						mapped_type* node = item;
						while(node->Right(spec) != nullptr)
						{
							node = node->Right(spec);
						}

						item->SetValue(std::move(node->Value()));
						item = node;
					}

					mapped_type* child = item->Right(spec) ? item->Right(spec) : item->Left(spec);

					if(item->Color(spec) == detail::RedBlackColor::Black)
					{
						item->SetColor(spec, child->Color(spec));
						while(item != nullptr)
						{
							if(item->Parent(spec) == nullptr)
							{
								break;
							}

							mapped_type* sibling = sibling_(item);
							mapped_type* parent = item->Parent(spec);
							if(sibling->Color(spec) == detail::RedBlackColor::Red)
							{
								parent->SetColor(spec, detail::RedBlackColor::Red);
								sibling->SetColor(spec, detail::RedBlackColor::Black);
								if(item == parent->Left(spec))
								{
									rotateLeft_(parent);
								}
								else
								{
									rotateRight_(parent);
								}
							}

							if(
								parent->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Left(spec)->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Right(spec)->Color(spec) == detail::RedBlackColor::Black
							)
							{
								sibling->SetColor(spec, detail::RedBlackColor::Red);
								item = item->Parent(spec);
								continue;
							}

							if(
								parent->Color(spec) == detail::RedBlackColor::Red
								&& sibling->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Left(spec)->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Right(spec)->Color(spec) == detail::RedBlackColor::Black
							)
							{
								sibling->SetColor(spec, detail::RedBlackColor::Red);
								parent->SetColor(spec, detail::RedBlackColor::Black);
								break;
							}

							if(
								item == parent->Left(spec)
								&& sibling->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Left(spec)->Color(spec) == detail::RedBlackColor::Red
								&& sibling->Right(spec)->Color(spec) == detail::RedBlackColor::Black
							)
							{
								sibling->SetColor(spec, detail::RedBlackColor::Red);
								sibling->Left(spec)->SetColor(spec, detail::RedBlackColor::Black);
								rotateRight_(sibling);
							}
							else if(
								item == parent->Right(spec)
								&& sibling->Color(spec) == detail::RedBlackColor::Black
								&& sibling->Right(spec)->Color(spec) == detail::RedBlackColor::Red
								&& sibling->Left(spec)->Color(spec) == detail::RedBlackColor::Black
							)
							{
								sibling->SetColor(spec, detail::RedBlackColor::Red);
								sibling->Right(spec)->SetColor(spec, detail::RedBlackColor::Black);
								rotateLeft_(sibling);
							}

							sibling->SetColor(spec, parent->Color(spec));
							parent->SetColor(spec, detail::RedBlackColor::Black);
							if(item == parent->Left(spec))
							{
								sibling->Right(spec)->SetColor(spec, detail::RedBlackColor::Black);
								rotateLeft_(parent);
								break;
							}

							sibling->Left(spec)->SetColor(spec, detail::RedBlackColor::Black);
							rotateRight_(parent);
							break;
						}
					}

					replaceNode_(item, child);
					Base::erase_(item);
				}

				mapped_type* m_thisKeyRootNode;

			private:
				inline void printNode_(mapped_type* node, bool right = false, sprawl::String indent = "")
				{
					if(node->Right(spec) != nullptr)
					{
						printNode_(node->Right(spec), true, indent + (right ? "        " : " |      "));
					}

					printf("%s", indent.c_str());

					if(right)
					{
						printf(" /");
					}
					else
					{
						printf(" \\");
					}

					sprawl::StringBuilder builder;
					typename Accessor::key_type key = node->Accessor(spec).Key();
					builder << key;
					printf("----- %s\n", builder.Str().c_str());

					if(node->Left(spec) != nullptr)
					{
						printNode_(node->Left(spec), false, indent + (right ? " |      " : "        "));
					}
				}

				inline void insertHere_(mapped_type* newItem)
				{
					mapped_type* node = m_thisKeyRootNode;
					while(node)
					{
						if(newItem->Accessor(spec).Key() < node->Accessor(spec).Key())
						{
							if(node->Left(spec))
							{
								node = node->Left(spec);
							}
							else
							{
								insertHere_(newItem, node);
								return;
							}
						}
						else //Assuming not equal because that should be checked before this is called.
						{
							if(node->Right(spec))
							{
								node = node->Right(spec);
							}
							else
							{
								insertHere_(newItem, node);
								return;
							}
						}
					}
					insertHere_(newItem, nullptr);
				}

				inline void insertHere_(mapped_type* newItem, mapped_type* parent)
				{
					if(!parent)
					{
						m_thisKeyRootNode = newItem;
					}
					else if(newItem->Accessor(spec).Key() < parent->Accessor(spec).Key())
					{
						parent->SetLeft(spec, newItem);
					}
					else
					{
						parent->SetRight(spec, newItem);
					}
					newItem->SetParent(spec, parent);

					while(newItem)
					{
						if(newItem->Parent(spec) == nullptr)
						{
							newItem->SetColor(spec, detail::RedBlackColor::Black);
							return;
						}

						if(newItem->Parent(spec)->Color(spec) == detail::RedBlackColor::Black)
						{
							return;
						}

						mapped_type* uncle = uncle_(newItem);

						if(uncle != nullptr && uncle->Color(spec) == detail::RedBlackColor::Red)
						{
							newItem->Parent(spec)->SetColor(spec, detail::RedBlackColor::Black);
							uncle->SetColor(spec, detail::RedBlackColor::Black);
							newItem = grandparent_(newItem);
							newItem->SetColor(spec, detail::RedBlackColor::Red);
							continue;
						}

						mapped_type* gdad = grandparent_(newItem);
						if(newItem == newItem->Parent(spec)->Right(spec) && newItem->Parent(spec) == gdad->Left(spec))
						{
							rotateLeft_(newItem->Parent(spec));
							newItem = newItem->Left(spec);
						}
						else if(newItem == newItem->Parent(spec)->Left(spec) && newItem->Parent(spec) == gdad->Right(spec))
						{
							rotateRight_(newItem->Parent(spec));
							newItem = newItem->Right(spec);
						}

						gdad = grandparent_(newItem);

						newItem->Parent(spec)->SetColor(spec, detail::RedBlackColor::Black);
						gdad->SetColor(spec, detail::RedBlackColor::Red);
						if(newItem == newItem->Parent(spec)->Left(spec) && newItem->Parent(spec) == gdad->Left(spec))
						{
							rotateRight_(gdad);
							continue;
						}
						rotateLeft_(gdad);
					}
				}

				inline mapped_type* get_(typename Accessor::key_type const& key)
				{
					mapped_type* node = m_thisKeyRootNode;
					while(node)
					{
						if(key == node->Accessor(spec).Key())
						{
							return node;
						}

						if(key < node->Accessor(spec).Key())
						{
							node = node->Left(spec);
						}
						else
						{
							node = node->Right(spec);
						}
					}
					return nullptr;
				}

				inline mapped_type* getUpperBound_(typename Accessor::key_type const& key)
				{
					mapped_type* node = m_thisKeyRootNode;
					mapped_type* foundNode = nullptr;
					while(node)
					{
						if(key < node->Accessor(spec).Key())
						{
							foundNode = node;
							node = node->Left(spec);
						}
						else
						{
							node = node->Right(spec);
						}
					}
					return foundNode;
				}

				inline mapped_type* getLowerBound_(typename Accessor::key_type const& key)
				{
					mapped_type* node = m_thisKeyRootNode;
					mapped_type* foundNode = nullptr;
					while(node)
					{
						if(key == node->Accessor(spec).Key())
						{
							return node;
						}

						if(key < node->Accessor(spec).Key())
						{
							node = node->Left(spec);
						}
						else
						{
							foundNode = node;
							node = node->Right(spec);
						}
					}

					return foundNode;
				}

				inline mapped_type const* get_(typename Accessor::key_type const& key) const
				{
					return const_cast<BinaryTree_Impl*>(this)->get_(key);
				}

				void replaceNode_(mapped_type* node, mapped_type* replaceWith)
				{
					if(node->Parent(spec) == nullptr)
					{
						m_thisKeyRootNode = replaceWith;
					}
					else
					{
						mapped_type* parent = node->Parent(spec);
						if(parent->Left(spec) == node)
						{
							parent->SetLeft(spec, replaceWith);
						}
						else
						{
							parent->SetRight(spec, replaceWith);
						}
					}
					if(replaceWith != nullptr)
					{
						replaceWith->SetParent(spec, node->Parent(spec));
					}
				}

				void rotateLeft_(mapped_type* node)
				{
					mapped_type* right = node->Right(spec);
					replaceNode_(node, right);
					node->SetRight(spec, right->Left(spec));
					if(right->Left(spec) != nullptr)
					{
						right->Left(spec)->SetParent(spec, node);
					}
					right->SetLeft(spec, node);
					node->SetParent(spec, right);
				}

				void rotateRight_(mapped_type* node)
				{
					mapped_type* left = node->Left(spec);
					replaceNode_(node, left);
					node->SetLeft(spec, left->Right(spec));
					if(left->Right(spec) != nullptr)
					{
						left->Right(spec)->SetParent(spec, node);
					}
					left->SetRight(spec, node);
					node->SetParent(spec, left);
				}

				static Specialized<Idx> spec;
			};

			template< typename ValueType, typename mapped_type, size_t Idx, typename Accessor, typename... AdditionalAccessors >
			/*static*/ Specialized<Idx> BinaryTree_Impl<ValueType, mapped_type, Idx, Accessor, AdditionalAccessors...>::spec;
		}

		template< typename ValueType, typename... Accessors >
		class BinaryTree : public detail::BinaryTree_Impl<ValueType, detail::TreeAccessorGroup<ValueType, Accessors...>, 0, Accessors...>
		{
		public:
			typedef detail::BinaryTree_Impl<ValueType, detail::TreeAccessorGroup<ValueType, Accessors...>, 0, Accessors...> Base;
			typedef detail::TreeAccessorGroup<ValueType, Accessors...> mapped_type;

			template<int i = 0>
			using iterator = TreeIterator<ValueType, mapped_type, i>;
			template<int i = 0>
			using const_iterator = TreeIterator<ValueType, mapped_type, i>;

			using Base::Get;
			template<int i, typename T2>
			inline ValueType& Get(T2 const& key)
			{
				return Get(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& Get(T2 const& key) const
			{
				return Get(key, Specialized<i>());
			}

			using Base::LowerBound;
			template<int i, typename T2>
			inline typename Base::template iterator<i> LowerBound(T2 const& key)
			{
				return LowerBound(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline typename Base::template const_iterator<i> LowerBound(T2 const& key) const
			{
				return LowerBound(key, Specialized<i>());
			}

			using Base::UpperBound;
			template<int i, typename T2>
			inline typename Base::template iterator<i> UpperBound(T2 const& key)
			{
				return UpperBound(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline typename Base::template const_iterator<i> UpperBound(T2 const& key) const
			{
				return UpperBound(key, Specialized<i>());
			}

			using Base::GetOrInsert;
			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key, ValueType const& defaultValue)
			{
				return GetOrInsert(key, defaultValue, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key, ValueType const& defaultValue) const
			{
				return GetOrInsert(key, defaultValue, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key, ValueType&& defaultValue)
			{
				return GetOrInsert(key, std::move(defaultValue), Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key, ValueType&& defaultValue) const
			{
				return GetOrInsert(key, std::move(defaultValue), Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType& GetOrInsert(T2 const& key)
			{
				return GetOrInsert(key, Specialized<i>());
			}

			template<int i, typename T2>
			inline ValueType const& GetOrInsert(T2 const& key) const
			{
				return GetOrInsert(key, Specialized<i>());
			}

			template<typename T2>
			inline ValueType& operator[](T2 const& key)
			{
				return GetOrInsert(key);
			}

			template<typename T2>
			inline ValueType const& operator[](T2 const& key) const
			{
				return GetOrInsert(key);
			}

			using Base::find;
			template<int i, typename T2>
			inline typename Base::template iterator<i> find(T2 const& val)
			{
				return find(val, Specialized<i>());
			}

			template<int i, typename T2>
			inline typename Base::template iterator<i> find(T2 const& val) const
			{
				return find(val, Specialized<i>());
			}

			using Base::cfind;
			template<int i, typename T2>
			inline typename Base::template iterator<i> cfind(T2 const& val) const
			{
				return cfind(val, Specialized<i>());
			}

			using Base::Has;
			template<int i, typename T2>
			inline bool Has(T2 const& val)
			{
				return Has(val, Specialized<i>());
			}

			using Base::Erase;
			template<int i, typename T2>
			inline void Erase(T2 const& val)
			{
				return Erase(val, Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template iterator<i> begin()
			{
				return Base::begin(Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template const_iterator<i> begin() const
			{
				return Base::begin(Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template const_iterator<i> cbegin() const
			{
				return Base::cbegin(Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template iterator<i> end()
			{
				return Base::end(Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template const_iterator<i> end() const
			{
				return Base::end(Specialized<i>());
			}

			template<int i = 0>
			inline typename Base::template const_iterator<i> cend() const
			{
				return Base::cend(Specialized<i>());
			}

			using Base::Print;
			template<int i = 0>
			inline void Print()
			{
				Base::Print(Specialized<i>());
			}

			BinaryTree()
				: Base()
			{
				//
			}

			BinaryTree(BinaryTree const& other)
				: Base(other)
			{
				//Will get Idx == 0
				insertNodeAndChildren_(other.m_thisKeyRootNode);
			}

			BinaryTree(BinaryTree&& other)
				: Base(std::move(other))
			{
				//
			}

			BinaryTree& operator=(BinaryTree const& other)
			{
				Base::Clear();
				this->m_size = other.m_size;
				//Will get Idx == 0
				insertNodeAndChildren_(other.m_thisKeyRootNode);
				return *this;
			}
		private:
			void insertNodeAndChildren_(mapped_type const* node)
			{
				if(node == nullptr)
				{
					return;
				}
				Specialized<0> spec;
				mapped_type* newNode = (mapped_type*)Base::allocator::alloc();
				new (newNode) mapped_type(*node);
				Base::insert_(newNode);
				insertNodeAndChildren_(node->Left(spec));
				insertNodeAndChildren_(node->Right(spec));
			}
		};
	}
}
