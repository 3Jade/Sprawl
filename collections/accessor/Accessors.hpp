#pragma once

#include "../../common/compat.hpp"

namespace sprawl
{
	template<typename ValueType, typename KeyType, KeyType(ValueType::*function)()>
	class MemberAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		MemberAccessor(ValueType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return (m_value->*function)();
		}

		inline ValueType& Value()
		{
			return *m_value;
		}

		inline ValueType const& Value() const
		{
			return *m_value;
		}

	private:
		ValueType* m_value;
	};

	template<typename ValueType, typename KeyType, KeyType(ValueType::*function)() const>
	class ConstMemberAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		ConstMemberAccessor(ValueType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return (m_value->*function)();
		}

		inline ValueType& Value()
		{
			return *m_value;
		}

		inline ValueType const& Value() const
		{
			return *m_value;
		}

	private:
		ValueType* m_value;
	};

	template<typename ValueType, typename KeyType, KeyType(ValueType::*function)(), typename PointerType = ValueType*>
	class PtrMemberAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		PtrMemberAccessor(PointerType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return ((*(*m_value)).*function)();
		}

		inline PointerType& Value()
		{
			return *m_value;
		}

		inline PointerType const& Value() const
		{
			return *m_value;
		}

	private:
		PointerType* m_value;
	};


	template<typename ValueType, typename KeyType, KeyType(ValueType::*function)() const, typename PointerType = ValueType*>
	class PtrConstMemberAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		PtrConstMemberAccessor(PointerType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return ((*m_value).*function)();
		}

		inline PointerType& Value()
		{
			return *m_value;
		}

		inline PointerType const& Value() const
		{
			return *m_value;
		}

	private:
		PointerType* m_value;
	};

	template<typename ValueType, typename KeyType, KeyType(*function)(ValueType*)>
	class FunctionAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		FunctionAccessor(ValueType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return (*function)(m_value);
		}

		inline ValueType& Value()
		{
			return *m_value;
		}

		inline ValueType const& Value() const
		{
			return *m_value;
		}

	private:
		ValueType* m_value;
	};

	template<typename ValueType, typename KeyType, KeyType(*function)(ValueType*), typename PointerType = ValueType*>
	class PtrFunctionAccessor
	{
	public:
		typedef KeyType key_type;
		struct arg_type {};

		PtrFunctionAccessor(PointerType& value)
			: m_value(&value)
		{
			//
		}

		inline KeyType const Key() const
		{
			return (*function)(&(*(*m_value)));
		}

		inline PointerType& Value()
		{
			return *m_value;
		}

		inline PointerType const& Value() const
		{
			return *m_value;
		}

	private:
		mutable PointerType* m_value;
	};

	template<typename ValueType, typename KeyType>
	class KeyAccessor
	{
	public:
		typedef KeyType key_type;
		typedef KeyType arg_type;

		KeyAccessor(ValueType& value, KeyType const& key)
			: m_key(key)
			, m_value(&value)
		{
			//
		}

		inline KeyType const& Key() const
		{
			return m_key;
		}

		inline ValueType& Value()
		{
			return *m_value;
		}

		inline ValueType const& Value() const
		{
			return *m_value;
		}

	private:
		KeyType m_key;
		ValueType* m_value;
	};

	template<typename ValueType>
	class SelfAccessor
	{
	public:
		typedef ValueType key_type;
		struct arg_type {};

		SelfAccessor(ValueType& value)
			: m_value(&value)
		{
			//
		}

		inline ValueType const& Key() const
		{
			return *m_value;
		}

		inline ValueType& Value()
		{
			return *m_value;
		}

		inline ValueType const& Value() const
		{
			return *m_value;
		}

	private:
		ValueType* m_value;
	};

	class NullAccessor
	{
	public:
		typedef void* key_type;
		struct arg_type {};

		inline void* Key()
		{
			return nullptr;
		}
	};
}
