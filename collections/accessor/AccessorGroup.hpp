#pragma once

namespace sprawl
{
	namespace collections
	{
		template<typename ValueType, typename Accessor1, typename Accessor2 = NullAccessor, typename Accessor3 = NullAccessor>
		class AccessorGroup
		{
		public:
			AccessorGroup(ValueType const& value)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value)
				, m_accessor3(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			struct VerifyFirstKeyUnique {};
			struct VerifySecondKeyUnique {};
			struct VerifyThirdKeyUnique {};

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key, VerifyFirstKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value, key)
				, m_accessor2(m_value)
				, m_accessor3(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor2::arg_type const& key, VerifySecondKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value, key)
				, m_accessor3(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor3::arg_type const& key, VerifyThirdKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value)
				, m_accessor3(m_value, key)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key1, typename Accessor2::arg_type const& key2, VerifyFirstKeyUnique* = 0, VerifySecondKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value, key1)
				, m_accessor2(m_value, key2)
				, m_accessor3(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key1, typename Accessor3::arg_type const& key2, VerifyFirstKeyUnique* = 0, VerifyThirdKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value, key1)
				, m_accessor2(m_value)
				, m_accessor3(m_value, key2)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor2::arg_type const& key1, typename Accessor3::arg_type const& key2, VerifySecondKeyUnique* = 0, VerifyThirdKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value, key1)
				, m_accessor3(m_value, key2)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key1, typename Accessor2::arg_type const& key2, typename Accessor3::arg_type const& key3)
				: m_value(value)
				, m_accessor1(m_value, key1)
				, m_accessor2(m_value, key2)
				, m_accessor3(m_value, key3)
				, next1(nullptr)
				, next2(nullptr)
				, next3(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, prev3(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
				, idx3(0)
			{
				//
			}

			ValueType m_value;
			Accessor1 m_accessor1;
			Accessor2 m_accessor2;
			Accessor3 m_accessor3;

			AccessorGroup* next1;
			AccessorGroup* next2;
			AccessorGroup* next3;

			AccessorGroup* prev1;
			AccessorGroup* prev2;
			AccessorGroup* prev3;

			AccessorGroup* next;
			AccessorGroup* prev;

			size_t idx1;
			size_t idx2;
			size_t idx3;
		private:
			AccessorGroup(AccessorGroup& other);
		};

		template<typename ValueType, typename Accessor1, typename Accessor2>
		class AccessorGroup<ValueType, Accessor1, Accessor2, NullAccessor>
		{
		public:
			AccessorGroup(ValueType const& value)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
			{
				//
			}

			struct VerifyFirstKeyUnique {};
			struct VerifySecondKeyUnique {};

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key, VerifyFirstKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value, key)
				, m_accessor2(m_value)
				, next1(nullptr)
				, next2(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor2::arg_type const& key, VerifySecondKeyUnique* = 0)
				: m_value(value)
				, m_accessor1(m_value)
				, m_accessor2(m_value, key)
				, next1(nullptr)
				, next2(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key1, typename Accessor2::arg_type const& key2)
				: m_value(value)
				, m_accessor1(m_value, key1)
				, m_accessor2(m_value, key2)
				, next1(nullptr)
				, next2(nullptr)
				, prev1(nullptr)
				, prev2(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
				, idx2(0)
			{
				//
			}

			ValueType m_value;
			Accessor1 m_accessor1;
			Accessor2 m_accessor2;

			AccessorGroup* next1;
			AccessorGroup* next2;

			AccessorGroup* prev1;
			AccessorGroup* prev2;

			AccessorGroup* next;
			AccessorGroup* prev;

			size_t idx1;
			size_t idx2;
		private:
			AccessorGroup(AccessorGroup& other);
		};
		template<typename ValueType, typename Accessor1>
		class AccessorGroup<ValueType, Accessor1, NullAccessor, NullAccessor>
		{
		public:
			AccessorGroup(ValueType const& value)
				: m_value(value)
				, m_accessor1(m_value)
				, next1(nullptr)
				, prev1(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
			{
				//
			}

			AccessorGroup(ValueType const& value, typename Accessor1::arg_type const& key)
				: m_value(value)
				, m_accessor1(m_value, key)
				, next1(nullptr)
				, prev1(nullptr)
				, next(nullptr)
				, prev(nullptr)
				, idx1(0)
			{
				//
			}

			ValueType m_value;
			Accessor1 m_accessor1;

			AccessorGroup* next1;
			AccessorGroup* prev1;

			AccessorGroup* next;
			AccessorGroup* prev;

			size_t idx1;
		private:
			AccessorGroup(AccessorGroup& other);
		};
	}
}
