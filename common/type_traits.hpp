#pragma once

#include <type_traits>

namespace sprawl
{
	namespace type_traits
	{
#define SPRAWL_COMMA ,

#define CREATE_OPERATOR_TEST(NAME, OPERATOR) \
		namespace detail { \
			template<class T, class = decltype(std::declval<T>()OPERATOR)> \
			std::true_type Has ## NAME ## Test(const T&); \
			std::false_type Has ## NAME ## Test(...); \
		} \
		template<class T> using Has ## NAME = decltype(detail::Has ## NAME ## Test(std::declval<T>()))

#define CREATE_PRE_OPERATOR_TEST(NAME, OPERATOR) \
		namespace detail { \
			template<class T, class = decltype(OPERATOR std::declval<T>())> \
			std::true_type Has ## NAME ## Test(const T&); \
			std::false_type Has ## NAME ## Test(...); \
		} \
		template<class T> using Has ## NAME = decltype(detail::Has ## NAME ## Test(std::declval<T>()))

#define CREATE_FUNCTION_TEST(NAME, FUNCTION) \
		namespace detail { \
			template<class T, class = decltype(std::declval<T>().FUNCTION)> \
			std::true_type Has ## NAME ## Test(const T&); \
			std::false_type Has ## NAME ## Test(...); \
		} \
		template<class T> using Has ## NAME = decltype(detail::Has ## NAME ## Test(std::declval<T>()))

		CREATE_OPERATOR_TEST(EmptyOperatorParens, ());
		CREATE_OPERATOR_TEST(IndexOperator, [0]);
		CREATE_OPERATOR_TEST(PostIncrement, ++);
		CREATE_OPERATOR_TEST(PostDecrement, --);
		CREATE_PRE_OPERATOR_TEST(PreIncrement, ++);
		CREATE_PRE_OPERATOR_TEST(PreDecrement, --);
		CREATE_PRE_OPERATOR_TEST(Dereference, *);
		CREATE_PRE_OPERATOR_TEST(AddressOf, &);
		CREATE_PRE_OPERATOR_TEST(UnaryPlus, +);
		CREATE_PRE_OPERATOR_TEST(UneryMinus, -);

		CREATE_OPERATOR_TEST(SameTypeAdditionOperator, +std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeSubtractionOperator, - std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeMultiplicationOperator, * std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeDivisionOperator, / std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeModuloOperator, % std::declval<T>());

		CREATE_OPERATOR_TEST(SameTypeAdditionAssignmentOperator, += std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeSubtractionAssignmentOperator, -= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeMultiplicationAssignmentOperator, *= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeDivisionAssignmentOperator, /= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeModuloAssignmentOperator, %= std::declval<T>());

		CREATE_OPERATOR_TEST(SameTypeEqualityOperator, == std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeInequalityOperator, != std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeGreaterThanOperator, > std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeLessThanOperator, < std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeGreaterOrEqualOperator, >= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeLessOrEqualOperator, <= std::declval<T>());

		CREATE_PRE_OPERATOR_TEST(LogicalNotOperator, !);
		CREATE_OPERATOR_TEST(SameTypeLogicalAnd, && std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeLogicalOr, || std::declval<T>());

		CREATE_PRE_OPERATOR_TEST(BitwiseNotOperator, ~);
		CREATE_OPERATOR_TEST(SameTypeBitwiseAnd, &std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeBitwiseOr, | std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeBitwiseXor, ^ std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeLeftShiftOperator, << std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeRightShiftOperator, >> std::declval<T>());

		CREATE_OPERATOR_TEST(SameTypeBitwiseAndAssignment, &= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeBitwiseOrAssignment, |= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeBitwiseXorAssignment, ^= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeLeftShiftOperatorAssignment, <<= std::declval<T>());
		CREATE_OPERATOR_TEST(SameTypeRightShiftOperatorAssignment, >>= std::declval<T>());

		CREATE_OPERATOR_TEST(SameTypeCommaOperator, SPRAWL_COMMA std::declval<T>());

		CREATE_OPERATOR_TEST(CopyAssignmentOperator, = std::declval<T>());
	}
}