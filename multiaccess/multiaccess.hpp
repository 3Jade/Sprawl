#pragma once

/*
 * This module is included as a part of libSprawl
 *
 * Copyright (C) 2013 Jaedyn K. Draper
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string>
#include <vector>
#include <iostream>
#include <boost/type_traits/has_dereference.hpp>
#include <functional>

namespace sprawl
{
	namespace multiaccess
	{
		std::string strToLower(std::string, bool);

		class duplicate_entry : public std::exception
		{
		public:
			duplicate_entry() throw(){ }
			const char* what() const throw(){ return "An entry with the given key already exists."; }
			~duplicate_entry() throw() {}
		};

		class no_such_element : public std::exception
		{
		public:
			no_such_element() throw(){}
			const char* what() const throw(){ return "No such entry in the table."; }
			~no_such_element() throw() {}
		};

		class access_not_supported : public std::exception
		{
		public:
			access_not_supported() throw(){}
			const char* what() const throw(){ return "The access method you tried to use is not supported on this table."; }
			~access_not_supported() throw() {}
		};

		template <typename T, typename IntFuncPtr, typename StrFuncPtr>
		struct dlist_t
		{
			typedef int(IntFunc)();
			typedef std::string(StrFunc)();
			dlist_t(const T& val_, IntFuncPtr IntGetter_, StrFuncPtr StrGetter_) : m_val(val_), m_next(nullptr), m_prev(nullptr), m_intGetter(IntGetter_), m_strGetter(StrGetter_)  {}
			T m_val;
			dlist_t<T, IntFuncPtr, StrFuncPtr>* m_next;
			dlist_t<T, IntFuncPtr, StrFuncPtr>* m_prev;
			IntFuncPtr m_intGetter;
			StrFuncPtr m_strGetter;
			auto GetInt() -> decltype(std::bind(m_intGetter, &m_val))
			{
				return std::bind(m_intGetter, &m_val);
			}
			auto GetStr() -> decltype(std::bind(m_strGetter, &m_val))
			{
				return std::bind(m_strGetter, &m_val);
			}
		};
	
		template<typename T, typename IntFuncPtr, typename StrFuncPtr>
		struct list_t
		{
			list_t() : m_ptr(nullptr), m_next(nullptr){}
			dlist_t<T, IntFuncPtr, StrFuncPtr>* m_ptr;
			list_t* m_next;
		};

		template<typename T>
		class ptrType
		{
		public:
			ptrType(const T& value) : m_val(value) {}
			T& operator->(){return m_val;}
			T& operator*(){return m_val;}
			T& operator&(){return m_val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T& returntype;
		private:
			T m_val;
		};

		template<typename T>
		class ptrType<T*>
		{
		public:
			ptrType(T* value) : m_val(value) {}
			T* operator->(){return m_val;}
			T*& operator*(){return m_val;}
			T* operator&(){return m_val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T* returntype;
		private:
			T* m_val;
		};

		template<typename T>
		class valType
		{
		public:
			valType(const T& value) : m_val(value) {}
			T* operator->(){return& m_val;}
			T& operator*(){return m_val;}
			T* operator&(){return& m_val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T* returntype;
		private:
			T m_val;
		};

		template<typename T, bool b = boost::has_dereference<T>::value>
		struct Type
		{
			typedef ptrType<T> value;
		};

		template<typename T>
		struct Type<T, false>
		{
			typedef valType<T> value;
		};

		template<typename T, typename IntFuncPtr, typename StrFuncPtr>
		class multiaccess_iterator : public std::iterator<std::forward_iterator_tag, T, std::ptrdiff_t, T*, T&>
		{
		public:
			typedef typename Type<T>::value ValueType;
			typedef int(*IntFunc)();
			typedef std::string(*StrFunc)();
			multiaccess_iterator(dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* item) : m_currentItem(item), m_nextItem(m_currentItem ? m_currentItem->m_next : nullptr){}

			T& operator*() const { return *m_currentItem->m_val; }
			typename ValueType::returntype operator->() const { return& m_currentItem->m_val; }
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> & operator++()
			{
				m_currentItem = m_nextItem;
				m_nextItem = m_currentItem ? m_currentItem->m_next : nullptr;
				return *this;
			}
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> operator++(int)
			{
				multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> tmp(*this);
				++(*this);
				return tmp;
			}
			bool operator==(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>& rhs) const
			{
				return m_currentItem == rhs.m_currentItem;
			}

			bool operator!=(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>& rhs) const
			{
				return !this->operator==(rhs);
			}

			operator bool() const { return m_currentItem != nullptr; }
			bool operator!() const { return m_currentItem != nullptr; }

		protected:
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_currentItem;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_nextItem;
		};	

		template<typename T, typename IntFuncPtr, typename StrFuncPtr>
		class multiaccess_const_iterator : public std::iterator<std::forward_iterator_tag, T, std::ptrdiff_t, T*, T&>
		{
		public:
			typedef typename Type<T>::value ValueType;
			typedef int(*IntFunc)();
			typedef std::string(*StrFunc)();
			multiaccess_const_iterator(dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* item) : m_currentItem(item), m_nextItem(m_currentItem ? m_currentItem->m_next : nullptr){}

			const T& operator*() const { return *m_currentItem->m_val; }
			const typename ValueType::returntype operator->() const { return& m_currentItem->m_val; }
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> & operator++()
			{
				m_currentItem = m_nextItem;
				m_nextItem = m_currentItem ? m_currentItem->m_next : nullptr;
				return *this;
			}
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> operator++(int)
			{
				multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> tmp(*this);
				++(*this);
				return tmp;
			}
			bool operator==(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>& rhs) const
			{
				return m_currentItem == rhs.m_currentItem;
			}

			bool operator!=(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>& rhs) const
			{
				return !this->operator==(rhs);
			}

			operator bool() const { return m_currentItem != nullptr; }
			bool operator!() const { return m_currentItem != nullptr; }

		protected:
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_currentItem;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_nextItem;
		};	

		template<typename T, typename IntFuncPtr = typename Type<T>::value::IntFuncType, typename StrFuncPtr = typename Type<T>::value::StrFuncType>
		class multiaccess_map
		{
		public:
			typedef int(IntFunc)();
			typedef std::string(StrFunc)();
			typedef typename Type<T>::value ValueType;
			multiaccess_map(IntFuncPtr func, bool b=true) : m_first(nullptr), m_last(nullptr), m_size(0), m_totalSize(89), m_ignoreCase(b)
			{
				m_getIntFunc = func;
				m_getStrFunc = nullptr;
				m_idTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
				m_nameTable = nullptr;
				for(int i=0; i<m_totalSize; i++)
				{
					m_idTable[i] = nullptr;
				}
			}

			multiaccess_map(StrFuncPtr func, bool b=true) : m_first(nullptr), m_last(nullptr), m_size(0), m_totalSize(89), m_ignoreCase(b)
			{
				m_getIntFunc = nullptr;
				m_getStrFunc = func;
				m_nameTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
				m_idTable = nullptr;
				for(int i=0; i<m_totalSize; i++)
				{
					m_nameTable[i] = nullptr;
				}
			}

			multiaccess_map(IntFuncPtr func1, StrFuncPtr func2, bool b=true) : m_first(nullptr), m_last(nullptr), m_size(0), m_totalSize(89), m_ignoreCase(b)
			{
				m_getIntFunc = func1;
				m_getStrFunc = func2;
				m_idTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
				m_nameTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
				for(int i=0; i<m_totalSize; i++)
				{
					m_idTable[i] = nullptr;
					m_nameTable[i] = nullptr;
				}
			}

			virtual ~multiaccess_map()
			{
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l_del, *l_next;
				for(int i=0;i<m_totalSize;i++)
				{
					if(m_idTable != nullptr)
					{
						for(l_del = m_idTable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->m_next;
							delete l_del;
						}
					}
					if(m_nameTable != nullptr)
					{
						for(l_del = m_nameTable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->m_next;
							delete l_del;
						}
					}
				}
				if(m_idTable != nullptr)
					delete[] m_idTable;
				if(m_nameTable != nullptr)
					delete[] m_nameTable;
			
			
				for( auto ptr = m_first; ptr; ptr = ptr->m_next )
				{
					delete ptr;
				}
			}

			void push(const T& val)
			{
				ValueType value(val);
				std::function<IntFunc> IntGetter = std::bind(m_getIntFunc, &value);
				std::function<StrFunc> StrGetter = std::bind(m_getStrFunc, &value);
				if((m_getIntFunc && find(IntGetter()) != end())
					|| (m_getStrFunc && find(StrGetter()) != end()))
				{
					throw duplicate_entry();
				}

				dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* list = new dlist_t<ValueType, IntFuncPtr, StrFuncPtr>(value, m_getIntFunc, m_getStrFunc);
				if(m_first == nullptr)
				{
					//assert(last == nullptr);
					m_first = list;
					m_last = list;
				}
				else
				{
					//assert(last != nullptr);
					m_last->m_next = list;
					list->m_prev = m_last;
					m_last = list;
				}
			
				push_int(list);
				push_str(list);
				m_size++;

				if(m_size > m_totalSize*0.75)
				{
					m_totalSize = (m_totalSize * 2)+1;
					if(m_getIntFunc != nullptr)
					{
						delete [] m_idTable;
						m_idTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
						for(int i=0; i<m_totalSize; i++)
						{
							m_idTable[i] = nullptr;
						}
					}
					if(m_getStrFunc != nullptr)
					{
						delete [] m_nameTable;
						m_nameTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
						for(int i=0; i<m_totalSize; i++)
						{
							m_nameTable[i] = nullptr;
						}
					}
				
					for( auto ptr = m_first; ptr; ptr = ptr->m_next )
					{
						push_int(ptr);
						push_str(ptr);
					}
				}
			}
		
			void pop(const std::string& str)
			{
				pop(this->operator[](str));
			}
		
			void pop(int i)
			{
				pop(this->operator[](i));
			}
		
			void resort()
			{
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l_del, *l_next;
				for(int i=0;i<m_totalSize;i++)
				{
					if(m_idTable != nullptr)
					{
						for(l_del = m_idTable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->m_next;
							delete l_del;
						}
					}
					if(m_nameTable != nullptr)
					{
						for(l_del = m_nameTable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->m_next;
							delete l_del;
						}
					}
					m_idTable[i] = nullptr;
					m_nameTable[i] = nullptr;
				}
				for( auto ptr = m_first; ptr; ptr = ptr->m_next )
				{
					push_int(ptr);
					push_str(ptr);
				}
			}

			const T& operator[](int searchKey) const
			{
				if(m_getIntFunc == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_idTable[index]; l; l = l->m_next )
					if(l->m_ptr->GetInt()() == searchKey)
						return *l->m_ptr->m_val;
				throw no_such_element();
			}

			const T& operator[](const std::string& searchKey) const
			{
				if(m_getStrFunc == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_nameTable[index]; l; l = l->m_next )
					if(strToLower(l->m_ptr->GetStr()(), m_ignoreCase) == strToLower(searchKey, m_ignoreCase))
						return *l->m_ptr->m_val;
				throw no_such_element();
			}
		
			T& operator[](int searchKey)
			{
				if(m_getIntFunc == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_idTable[index]; l; l = l->m_next )
					if(l->m_ptr->GetInt()() == searchKey)
						return *l->m_ptr->m_val;
				throw no_such_element();
			}

			T& operator[](const std::string& searchKey)
			{
				if(m_getStrFunc == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_nameTable[index]; l; l = l->m_next )
					if(strToLower(l->m_ptr->GetStr()(), m_ignoreCase) == strToLower(searchKey, m_ignoreCase))
						return *l->m_ptr->m_val;
				throw no_such_element();
			}

			int getSize() const
			{ return m_size; }

			friend class multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>;
			typedef multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> iterator;
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> begin()
			{
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(m_first);
			}

			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> end()
			{
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> find(int searchKey)
			{
				if(m_getIntFunc == nullptr)
					return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_idTable[index]; l; l = l->m_next )
					if(l->m_ptr->GetInt()() == searchKey)
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->m_ptr);
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> find(const std::string& searchKey)
			{
				if(m_getStrFunc == nullptr)
					return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_nameTable[index]; l; l = l->m_next )
					if(strToLower(l->m_ptr->GetStr()(), m_ignoreCase) == strToLower(searchKey, m_ignoreCase))
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->m_ptr);
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}

			friend class multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>;
			typedef multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> const_iterator;
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> begin() const
			{
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(m_first);
			}

			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> end() const
			{
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> find(int searchKey) const
			{
				if(m_getIntFunc == nullptr)
					return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_idTable[index]; l; l = l->m_next )
					if(l->m_ptr->GetInt()() == searchKey)
						return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(l->m_ptr);
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> find(const std::string& searchKey) const
			{
				if(m_getStrFunc == nullptr)
					return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
				for( l = m_nameTable[index]; l; l = l->m_next )
					if(strToLower(l->m_ptr->GetStr()(), m_ignoreCase) == strToLower(searchKey, m_ignoreCase))
						return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(l->m_ptr);
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}

		private:
			unsigned long _hash(int i) const
			{
				return abs(i)%m_totalSize;
			}

			unsigned long _hash(const char* str) const
			{
				unsigned int hash = 0;
				int c;

				while((c = *str++))
				{
					/* hash = hash * 33 ^ c */
					hash = ((hash << 5) + hash) ^ c;
				}

			   return hash%m_totalSize;
			}
			unsigned long getIndex(int searchKey) const
			{
				return _hash(searchKey);
			}

			unsigned long getIndex(std::string searchKey) const
			{
				return _hash(strToLower(searchKey, m_ignoreCase).c_str());
			}

			void push_int(dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* p)
			{
				if(m_getIntFunc == nullptr)
					return;
				int index = getIndex(p->GetInt()());
				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = new list_t<ValueType, IntFuncPtr, StrFuncPtr>();
				l->m_ptr = p;
				l->m_next = m_idTable[index];
				m_idTable[index] = l;
			}

			void push_str(dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* p)
			{
				if(m_getStrFunc == nullptr)
					return;
				int index = getIndex(p->GetStr()().c_str());
				list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = new list_t<ValueType, IntFuncPtr, StrFuncPtr>();
				l->m_ptr = p;
				l->m_next = m_nameTable[index];
				m_nameTable[index] = l;
			}
		
			void pop(T& p)
			{
				ValueType value(p);
				std::function<IntFunc> IntGetter = std::bind(m_getIntFunc, &value);
				std::function<StrFunc> StrGetter = std::bind(m_getStrFunc, &value);
				if((m_getIntFunc && find(IntGetter()) == end())
					|| (m_getStrFunc && find(StrGetter()) == end()))
				{
					throw no_such_element();
				}
			
				dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* ptr = nullptr;
				if(m_getIntFunc != nullptr)
				{
					int searchKey = IntGetter();
					int index = getIndex(searchKey);

					list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
					for( l = m_idTable[index]; l; l = l->m_next )
					{
						if(l->m_ptr->GetInt()() == searchKey)
						{
							ptr = l->m_ptr;
							break;
						}
					}
				}
				else
				{
					std::string searchKey = StrGetter();
					int index = getIndex(searchKey);

					list_t<ValueType, IntFuncPtr, StrFuncPtr>* l = nullptr;
					for( l = m_nameTable[index]; l; l = l->m_next )
					{
						if(strToLower(l->m_ptr->GetStr()(), m_ignoreCase) == strToLower(searchKey, m_ignoreCase))
						{
							ptr = l->m_ptr;
							break;
						}
					}
				}
			
				if(m_getIntFunc != nullptr)
				{
					pop_int(IntGetter());
				}
				if(m_getStrFunc != nullptr)
				{
					pop_str(StrGetter());
				}
				m_size--;
			
				if(ptr != nullptr)
				{
					auto tmp = ptr->m_prev;
					if(tmp)
					{
						tmp->m_next = ptr->m_next;
					}
					if(ptr->m_next)
					{
						ptr->m_next->m_prev = tmp;
					}
					if(ptr == m_first)
					{
						m_first = ptr->m_next;
					}
					if(ptr == m_last)
					{
						m_last = ptr->m_prev;
					}
					delete ptr;
				}
			
				if(m_size < m_totalSize*0.25 && m_totalSize > 89)
				{
					m_totalSize = (m_totalSize - 1)/2;
					if(m_getIntFunc != nullptr)
					{
						delete [] m_idTable;
						m_idTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
						for(int i=0; i<m_totalSize; i++)
						{
							m_idTable[i] = nullptr;
						}
					}
					if(m_getStrFunc != nullptr)
					{
						delete [] m_nameTable;
						m_nameTable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[m_totalSize];
						for(int i=0; i<m_totalSize; i++)
						{
							m_nameTable[i] = nullptr;
						}
					}
				
					for( auto ptr = m_first; ptr; ptr = ptr->m_next )
					{
						push_int(ptr);
						push_str(ptr);
					}
				}
			}

			void pop_int(int searchKey)
			{
				int index = getIndex(searchKey);
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l, *l_next = nullptr;
				for( l = m_idTable[index]; l; l = l_next )
				{
					l_next = l->m_next;
					if(l->m_ptr->GetInt()() == searchKey)
					{
						m_idTable[index] = l->m_next;
						delete l;
						l = nullptr;
					}
					else if(l_next && l_next->m_ptr->GetInt()() == searchKey)
					{
						l->m_next = l_next->m_next;
						delete l_next;
						l_next = nullptr;
					}
				}
			}

			void pop_str(const std::string& searchKey)
			{
				int index = getIndex(searchKey.c_str());
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l, *l_next = nullptr;
				for( l = m_nameTable[index]; l; l = l_next )
				{
					l_next = l->m_next;
					if(l->m_ptr->GetStr()() == searchKey)
					{
						m_nameTable[index] = l->m_next;
						delete l;
						l = nullptr;
					}
					else if(l_next && l_next->m_ptr->GetStr()() == searchKey)
					{
						l->m_next = l_next->m_next;
						delete l_next;
						l_next = nullptr;
					}
				}
			}

			list_t<ValueType, IntFuncPtr, StrFuncPtr>** m_idTable;
			list_t<ValueType, IntFuncPtr, StrFuncPtr>** m_nameTable;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_first;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr>* m_last;
			int m_size;
			int m_totalSize;
			IntFuncPtr m_getIntFunc;
			StrFuncPtr m_getStrFunc;
			//RetFunc GetRet;
			bool m_ignoreCase;
		};

		template<typename PtrType, typename UnderlyingType>
		struct SmartPtrHelper
		{
			typedef multiaccess_map<PtrType, int(UnderlyingType::*)(), std::string(UnderlyingType::*)()> MapType;
		};
	}
}
