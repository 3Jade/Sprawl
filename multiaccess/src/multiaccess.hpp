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
			const char *what() const throw(){ return "An entry with the given key already exists."; }
			~duplicate_entry() throw() {}
		};

		class no_such_element : public std::exception
		{
		public:
			no_such_element() throw(){}
			const char *what() const throw(){ return "No such entry in the table."; }
			~no_such_element() throw() {}
		};

		class access_not_supported : public std::exception
		{
		public:
			access_not_supported() throw(){}
			const char *what() const throw(){ return "The access method you tried to use is not supported on this table."; }
			~access_not_supported() throw() {}
		};

		template <typename T, typename IntFuncPtr, typename StrFuncPtr>
		struct dlist_t
		{
			typedef int(IntFunc)();
			typedef std::string(StrFunc)();
			dlist_t(const T &val_, IntFuncPtr IntGetter_, StrFuncPtr StrGetter_) : val(val_), next(nullptr), prev(nullptr), IntGetter(IntGetter_), StrGetter(StrGetter_)  {}
			T val;
			dlist_t<T, IntFuncPtr, StrFuncPtr> *next;
			dlist_t<T, IntFuncPtr, StrFuncPtr> *prev;
			IntFuncPtr IntGetter;
			StrFuncPtr StrGetter;
			auto GetInt() -> decltype(std::bind(IntGetter, &val))
			{
				return std::bind(IntGetter, &val);
			}
			auto GetStr() -> decltype(std::bind(StrGetter, &val))
			{
				return std::bind(StrGetter, &val);
			}
		};
	
		template<typename T, typename IntFuncPtr, typename StrFuncPtr>
		struct list_t
		{
			list_t() : p(nullptr), next(nullptr){}
			dlist_t<T, IntFuncPtr, StrFuncPtr> *p;
			list_t *next;
		};

		template<typename T>
		class ptrType
		{
		public:
			ptrType(const T &value) : val(value) {}
			T &operator->(){return val;}
			T &operator*(){return val;}
			T &operator&(){return val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T& returntype;
		private:
			T val;
		};

		template<typename T>
		class ptrType<T*>
		{
		public:
			ptrType(T *value) : val(value) {}
			T *operator->(){return val;}
			T *&operator*(){return val;}
			T *operator&(){return val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T* returntype;
		private:
			T *val;
		};

		template<typename T>
		class valType
		{
		public:
			valType(const T &value) : val(value) {}
			T *operator->(){return &val;}
			T &operator*(){return val;}
			T *operator&(){return &val;}
			typedef int(T::*IntFuncType)();
			typedef std::string(T::*StrFuncType)();
			typedef T* returntype;
		private:
			T val;
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
			multiaccess_iterator(dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *item) : current_item(item), next_item(current_item ? current_item->next : nullptr){}

			T& operator*() const { return *current_item->val; }
			typename ValueType::returntype operator->() const { return &current_item->val; }
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> & operator++()
			{
				current_item = next_item;
				next_item = current_item ? current_item->next : nullptr;
				return *this;
			}
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> operator++(int)
			{
				multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> tmp(*this);
				++(*this);
				return tmp;
			}
			bool operator==(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> &rhs) const
			{
				return current_item == rhs.current_item;
			}

			bool operator!=(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> &rhs) const
			{
				return !this->operator==(rhs);
			}

			operator bool() const { return current_item != nullptr; }
			bool operator!() const { return current_item != nullptr; }

		protected:
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *current_item;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *next_item;
		};	

		template<typename T, typename IntFuncPtr, typename StrFuncPtr>
		class multiaccess_const_iterator : public std::iterator<std::forward_iterator_tag, T, std::ptrdiff_t, T*, T&>
		{
		public:
			typedef typename Type<T>::value ValueType;
			typedef int(*IntFunc)();
			typedef std::string(*StrFunc)();
			multiaccess_const_iterator(dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *item) : current_item(item), next_item(current_item ? current_item->next : nullptr){}

			const T& operator*() const { return *current_item->val; }
			const typename ValueType::returntype operator->() const { return &current_item->val; }
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> & operator++()
			{
				current_item = next_item;
				next_item = current_item ? current_item->next : nullptr;
				return *this;
			}
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> operator++(int)
			{
				multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> tmp(*this);
				++(*this);
				return tmp;
			}
			bool operator==(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> &rhs) const
			{
				return current_item == rhs.current_item;
			}

			bool operator!=(const multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> &rhs) const
			{
				return !this->operator==(rhs);
			}

			operator bool() const { return current_item != nullptr; }
			bool operator!() const { return current_item != nullptr; }

		protected:
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *current_item;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *next_item;
		};	

		template<typename T, typename IntFuncPtr = typename Type<T>::value::IntFuncType, typename StrFuncPtr = typename Type<T>::value::StrFuncType>
		class multiaccess_map
		{
		public:
			typedef int(IntFunc)();
			typedef std::string(StrFunc)();
			typedef typename Type<T>::value ValueType;
			multiaccess_map(IntFuncPtr func, bool b=true) : first(nullptr), last(nullptr), size(0), total_size(89), ignore_case(b)
			{
				GetInt = func;
				GetStr = nullptr;
				idtable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
				nametable = nullptr;
				for(int i=0; i<total_size; i++)
				{
					idtable[i] = nullptr;
				}
			}

			multiaccess_map(StrFuncPtr func, bool b=true) : first(nullptr), last(nullptr), size(0), total_size(89), ignore_case(b)
			{
				GetInt = nullptr;
				GetStr = func;
				nametable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
				idtable = nullptr;
				for(int i=0; i<total_size; i++)
				{
					nametable[i] = nullptr;
				}
			}

			multiaccess_map(IntFuncPtr func1, StrFuncPtr func2, bool b=true) : first(nullptr), last(nullptr), size(0), total_size(89), ignore_case(b)
			{
				GetInt = func1;
				GetStr = func2;
				idtable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
				nametable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
				for(int i=0; i<total_size; i++)
				{
					idtable[i] = nullptr;
					nametable[i] = nullptr;
				}
			}

			virtual ~multiaccess_map()
			{
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l_del, *l_next;
				for(int i=0;i<total_size;i++)
				{
					if(idtable != nullptr)
					{
						for(l_del = idtable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->next;
							delete l_del;
						}
					}
					if(nametable != nullptr)
					{
						for(l_del = nametable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->next;
							delete l_del;
						}
					}
				}
				if(idtable != nullptr)
					delete[] idtable;
				if(nametable != nullptr)
					delete[] nametable;
			
			
				for( auto ptr = first; ptr; ptr = ptr->next )
				{
					delete ptr;
				}
			}

			void push(const T &val)
			{
				ValueType value(val);
				std::function<IntFunc> IntGetter = std::bind(GetInt, &value);
				std::function<StrFunc> StrGetter = std::bind(GetStr, &value);
				if((GetInt && find(IntGetter()) != end())
					|| (GetStr && find(StrGetter()) != end()))
				{
					throw duplicate_entry();
				}

				dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *list = new dlist_t<ValueType, IntFuncPtr, StrFuncPtr>(value, GetInt, GetStr);
				if(first == nullptr)
				{
					//assert(last == nullptr);
					first = list;
					last = list;
				}
				else
				{
					//assert(last != nullptr);
					last->next = list;
					list->prev = last;
					last = list;
				}
			
				push_int(list);
				push_str(list);
				size++;

				if(size > total_size*0.75)
				{
					total_size = (total_size * 2)+1;
					if(GetInt != nullptr)
					{
						delete [] idtable;
						idtable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
						for(int i=0; i<total_size; i++)
						{
							idtable[i] = nullptr;
						}
					}
					if(GetStr != nullptr)
					{
						delete [] nametable;
						nametable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
						for(int i=0; i<total_size; i++)
						{
							nametable[i] = nullptr;
						}
					}
				
					for( auto ptr = first; ptr; ptr = ptr->next )
					{
						push_int(ptr);
						push_str(ptr);
					}
				}
			}
		
			void pop(const std::string &str)
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
				for(int i=0;i<total_size;i++)
				{
					if(idtable != nullptr)
					{
						for(l_del = idtable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->next;
							delete l_del;
						}
					}
					if(nametable != nullptr)
					{
						for(l_del = nametable[i]; l_del; l_del = l_next)
						{
							l_next = l_del->next;
							delete l_del;
						}
					}
					idtable[i] = nullptr;
					nametable[i] = nullptr;
				}
				for( auto ptr = first; ptr; ptr = ptr->next )
				{
					push_int(ptr);
					push_str(ptr);
				}
			}

			const T &operator[](int searchKey) const
			{
				if(GetInt == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = idtable[index]; l; l = l->next )
					if(l->p->GetInt()() == searchKey)
						return *l->p->val;
				throw no_such_element();
			}

			const T &operator[](const std::string &searchKey) const
			{
				if(GetStr == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = nametable[index]; l; l = l->next )
					if(strToLower(l->p->GetStr()(), ignore_case) == strToLower(searchKey, ignore_case))
						return *l->p->val;
				throw no_such_element();
			}
		
			T &operator[](int searchKey)
			{
				if(GetInt == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = idtable[index]; l; l = l->next )
					if(l->p->GetInt()() == searchKey)
						return *l->p->val;
				throw no_such_element();
			}

			T &operator[](const std::string &searchKey)
			{
				if(GetStr == nullptr)
					throw access_not_supported();
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = nametable[index]; l; l = l->next )
					if(strToLower(l->p->GetStr()(), ignore_case) == strToLower(searchKey, ignore_case))
						return *l->p->val;
				throw no_such_element();
			}

			int getSize() const
			{ return size; }

			friend class multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>;
			typedef multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> iterator;
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> begin()
			{
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(first);
			}

			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> end()
			{
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> find(int searchKey)
			{
				if(GetInt == nullptr)
					return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = idtable[index]; l; l = l->next )
					if(l->p->GetInt()() == searchKey)
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->p);
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_iterator<T, IntFuncPtr, StrFuncPtr> find(const std::string &searchKey)
			{
				if(GetStr == nullptr)
					return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = nametable[index]; l; l = l->next )
					if(strToLower(l->p->GetStr()(), ignore_case) == strToLower(searchKey, ignore_case))
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->p);
				return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}

			friend class multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>;
			typedef multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> const_iterator;
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> begin() const
			{
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(first);
			}

			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> end() const
			{
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> find(int searchKey) const
			{
				if(GetInt == nullptr)
					return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = idtable[index]; l; l = l->next )
					if(l->p->GetInt()() == searchKey)
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->p);
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}
		
			multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr> find(const std::string &searchKey) const
			{
				if(GetStr == nullptr)
					return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
				int index = getIndex(searchKey);

				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
				for( l = nametable[index]; l; l = l->next )
					if(strToLower(l->p->GetStr()(), ignore_case) == strToLower(searchKey, ignore_case))
						return multiaccess_iterator<T, IntFuncPtr, StrFuncPtr>(l->p);
				return multiaccess_const_iterator<T, IntFuncPtr, StrFuncPtr>(nullptr);
			}

		private:
			unsigned long _hash(int i)
			{
				return abs(i)%total_size;
			}

			unsigned long _hash(const char *str)
			{
				unsigned int hash = 0;
				int c;

				while((c = *str++))
				{
					/* hash = hash * 33 ^ c */
					hash = ((hash << 5) + hash) ^ c;
				}

			   return hash%total_size;
			}
			unsigned long getIndex(int searchKey)
			{
				return _hash(searchKey);
			}

			unsigned long getIndex(std::string searchKey)
			{
				return _hash(strToLower(searchKey, ignore_case).c_str());
			}

			void push_int(dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *p)
			{
				if(GetInt == nullptr)
					return;
				int index = getIndex(p->GetInt()());
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = new list_t<ValueType, IntFuncPtr, StrFuncPtr>();
				l->p = p;
				l->next = idtable[index];
				idtable[index] = l;
			}

			void push_str(dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *p)
			{
				if(GetStr == nullptr)
					return;
				int index = getIndex(p->GetStr()().c_str());
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = new list_t<ValueType, IntFuncPtr, StrFuncPtr>();
				l->p = p;
				l->next = nametable[index];
				nametable[index] = l;
			}
		
			void pop(T &p)
			{
				ValueType value(p);
				std::function<IntFunc> IntGetter = std::bind(GetInt, &value);
				std::function<StrFunc> StrGetter = std::bind(GetStr, &value);
				if((GetInt && find(IntGetter()) == end())
					|| (GetStr && find(StrGetter()) == end()))
				{
					throw no_such_element();
				}
			
				dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *ptr = nullptr;
				if(GetInt != nullptr)
				{
					int searchKey = IntGetter();
					int index = getIndex(searchKey);

					list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
					for( l = idtable[index]; l; l = l->next )
					{
						if(l->p->GetInt()() == searchKey)
						{
							ptr = l->p;
							break;
						}
					}
				}
				else
				{
					std::string searchKey = StrGetter();
					int index = getIndex(searchKey);

					list_t<ValueType, IntFuncPtr, StrFuncPtr> *l = nullptr;
					for( l = nametable[index]; l; l = l->next )
					{
						if(strToLower(l->p->GetStr()(), ignore_case) == strToLower(searchKey, ignore_case))
						{
							ptr = l->p;
							break;
						}
					}
				}
			
				if(GetInt != nullptr)
				{
					pop_int(IntGetter());
				}
				if(GetStr != nullptr)
				{
					pop_str(StrGetter());
				}
				size--;
			
				if(ptr != nullptr)
				{
					auto tmp = ptr->prev;
					if(tmp)
					{
						tmp->next = ptr->next;
					}
					if(ptr->next)
					{
						ptr->next->prev = tmp;
					}
					if(ptr == first)
					{
						first = ptr->next;
					}
					if(ptr == last)
					{
						last = ptr->prev;
					}
					delete ptr;
				}
			
				if(size < total_size*0.25 && total_size > 89)
				{
					total_size = (total_size - 1)/2;
					if(GetInt != nullptr)
					{
						delete [] idtable;
						idtable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
						for(int i=0; i<total_size; i++)
						{
							idtable[i] = nullptr;
						}
					}
					if(GetStr != nullptr)
					{
						delete [] nametable;
						nametable = new list_t<ValueType, IntFuncPtr, StrFuncPtr>*[total_size];
						for(int i=0; i<total_size; i++)
						{
							nametable[i] = nullptr;
						}
					}
				
					for( auto ptr = first; ptr; ptr = ptr->next )
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
				for( l = idtable[index]; l; l = l_next )
				{
					l_next = l->next;
					if(l->p->GetInt()() == searchKey)
					{
						idtable[index] = l->next;
						delete l;
						l = nullptr;
					}
					else if(l_next && l_next->p->GetInt()() == searchKey)
					{
						l->next = l_next->next;
						delete l_next;
						l_next = nullptr;
					}
				}
			}

			void pop_str(const std::string &searchKey)
			{
				int index = getIndex(searchKey.c_str());
				list_t<ValueType, IntFuncPtr, StrFuncPtr> *l, *l_next = nullptr;
				for( l = nametable[index]; l; l = l_next )
				{
					l_next = l->next;
					if(l->p->GetStr()() == searchKey)
					{
						nametable[index] = l->next;
						delete l;
						l = nullptr;
					}
					else if(l_next && l_next->p->GetStr()() == searchKey)
					{
						l->next = l_next->next;
						delete l_next;
						l_next = nullptr;
					}
				}
			}

			list_t<ValueType, IntFuncPtr, StrFuncPtr> **idtable;
			list_t<ValueType, IntFuncPtr, StrFuncPtr> **nametable;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *first;
			dlist_t<ValueType, IntFuncPtr, StrFuncPtr> *last;
			int size;
			int total_size;
			IntFuncPtr GetInt;
			StrFuncPtr GetStr;
			//RetFunc GetRet;
			bool ignore_case;
		};

		template<typename PtrType, typename UnderlyingType>
		struct SmartPtrHelper
		{
			typedef multiaccess_map<PtrType, int(UnderlyingType::*)(), std::string(UnderlyingType::*)()> MapType;
		};
	}
}
