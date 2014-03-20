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

#include <exception>
#include <sstream>
#include <typeinfo>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace sprawl
{
	namespace multitype
	{
		class ex_no_such_variable: public std::exception
		{
		public:
			const char* what() const throw ()
			{
				return m_ret.c_str();
			}
			~ex_no_such_variable() throw ()
			{
			}
			ex_no_such_variable(std::string str)
			{
				m_ret = "Variable \"" + str + "\" has not been declared.";
			}
		private:
			std::string m_ret;
		};

		class ex_invalid_variable_type: public std::exception
		{
		public:
			const char* what() const throw ()
			{
				return m_ret.c_str();
			}
			~ex_invalid_variable_type() throw ()
			{
			}
			ex_invalid_variable_type(int i, std::string str,
					std::string str2)
			{
				std::stringstream ss;
				ss << "Attempting to get vector member #" << i
						<< ", which is of type \"" << str2 << "\", as type \"" << str
						<< ".\"";
				m_ret = ss.str();
			}
			ex_invalid_variable_type(std::string name, std::string str,
					std::string str2)
			{
				std::stringstream ss;
				ss << "Attempting to get vector member named \"" << name
						<< "\", which is of type \"" << str2 << "\", as type \"" << str
						<< ".\"";
				m_ret = ss.str();
			}
		private:
			std::string m_ret;
		};

		class ex_out_of_bounds_error: public std::exception
		{
		public:
			const char* what() const throw ()
			{
				return m_ret.c_str();
			}
			~ex_out_of_bounds_error() throw ()
			{
			}
			ex_out_of_bounds_error(int i, int size)
			{
				std::stringstream ss;
				ss << "Attempting to get vector member #" << i + 1
						<< " in vector of size " << size << ".";
				m_ret = ss.str();
			}
		private:
			std::string m_ret;
		};

		class BaseVariable
		{
		public:
			template<typename T>
			BaseVariable(T value, std::string name)
			{
				T* ptr = new T(value);
				m_Value = new void*;
				*m_Value = ptr;
				m_Type = typeid(T).name();
				m_bDynamicPtr = true;
				m_Name = name;
			}
			BaseVariable(std::string type, void** value, std::string name,
					bool DynamicPtr = false)
			{
				m_Type = type;
				m_Value = value;
				m_Name = name;
				m_bDynamicPtr = DynamicPtr;
			}
			virtual ~BaseVariable()
			{
			}
			;
			std::string type()
			{
				return m_Type;
			}
			void** value()
			{
				return m_Value;
			}
			std::string name()
			{
				return m_Name;
			}

			virtual void complete_class() = 0;

			template<typename T>
			T& as()
			{
				std::string type_name = typeid(T).name();
				if (m_Type != type_name)
					throw ex_invalid_variable_type(m_Name, type_name, m_Type);
				return *static_cast<T*>(*m_Value);
			}
		protected:
			std::string m_Type;
			void** m_Value; //This can't be a void *, because an array or vector of void* isn't possible.  An array or vector of void** is, though.
			std::string m_Name;
			bool m_bDynamicPtr;
		};

		template<typename T>
		class Variable: public BaseVariable
		{
		public:
			template<typename T1>
			Variable(T1 value, std::string name) :
					BaseVariable(value, name)
			{
			}
			Variable(std::string type, void** value, std::string name, bool DynamicPtr =
					false) :
					BaseVariable(type, value, name, DynamicPtr)
			{
			}
			virtual void complete_class()
			{
			}
			virtual ~Variable()
			{
				T* DelVal = static_cast<T*>(*m_Value);
				if (m_bDynamicPtr)
					delete DelVal;
				delete m_Value;
			}
		};

		typedef void* VARIABLE;
		typedef void* TYPE;

		class multitype_list // uses std::vector
		{
		public:
			multitype_list() : m_variables() {}
			template<typename T>
			void m_multitype_push(const T& val, const std::string& name) //multitype_push is much more friendly to use than m_multitype_push, just so ya'know.
			{
				T* ptr = new T;
				*ptr = val;
				void** value = new void*;
				*value = ptr;
				std::string type = typeid(*ptr).name();
				Variable<T>* v = new Variable<T>(type, value, name, true);
				m_variables.push_back(v);
			}

			template<typename T>
			T& get(const std::string& str)
			{
				std::string type_name = typeid(T).name();
				for (unsigned int i = 0; i < m_variables.size(); i++)
				{
					if (m_variables[i]->name() == str)
					{
						if (m_variables[i]->type() == type_name)
							return *static_cast<T*>(*m_variables[i]->value());
						throw ex_invalid_variable_type(str, type_name,
								m_variables[i]->type());
					}
				}
				throw ex_no_such_variable(str);
			}

			BaseVariable& operator[](unsigned int i)
			{
				if (i >= m_variables.size())
					throw ex_out_of_bounds_error(i, m_variables.size());
				return *m_variables[i];
			}

			template<typename T>
			T& get(unsigned int i)
			{
				std::string type_name = typeid(T).name();
				if (i >= m_variables.size())
					throw ex_out_of_bounds_error(i, m_variables.size());
				if (m_variables[i]->type() != type_name)
					throw ex_invalid_variable_type(i, type_name,
							m_variables[i]->type());
				return *static_cast<T*>(*m_variables[i]->value());
			}

			//These are not actually used. They're here for the sake of things like intellisense so that uv_push and uv_get shown in the class member list.
			void multitype_push(VARIABLE _var_)
			{
			}
			void multitype_get(TYPE _type_, VARIABLE _var_)
			{
			}
		private:
			std::vector<BaseVariable*> m_variables;
		};

		class multitype_map // uses std::map
		{
		public:
			multitype_map() : m_variables() {}
			template<typename T>
			void m_multitype_push(T val, const std::string& name) //multitype_push is much more friendly to use than m_multitype_push, just so ya'know.
			{
				T* ptr = new T;
				*ptr = val;
				void** value = new void*;
				*value = ptr;
				std::string type = typeid(*ptr).name();
				Variable<T>* v = new Variable<T>(type, value, name, true);
				m_variables[name] = v;
			}

			template<typename T>
			T& get(const std::string& str)
			{
				std::string type_name = typeid(T).name();
				try
				{
					if (m_variables.at(str)->type() == type_name)
						return *static_cast<T*>(*m_variables[str]->value());
					throw ex_invalid_variable_type(str, type_name,
							m_variables[str]->type());
				}
				catch (std::out_of_range& e)
				{
					throw ex_no_such_variable(str);
				}
			}

			BaseVariable& operator[](const std::string& str)
			{
				try
				{
					return *m_variables.at(str);
				}
				catch (std::out_of_range& e)
				{
					throw ex_no_such_variable(str);
				}
			}
			//These are not actually used. They're here for the sake of things like intellisense so that jv_push and jv_get shown in the class member list.
			void multitype_push(VARIABLE _var_)
			{
			}
			void multitype_get(TYPE _type_, VARIABLE _var_)
			{
			}
		private:
			std::unordered_map<std::string, BaseVariable*> m_variables;
		};

		#define multitype_push(_var_) m_multitype_push(_var_, #_var_)
		#define multitype_get(_type_, _var_) get<_type_>(#_var_)
	}
}
