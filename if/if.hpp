#pragma once

#include "../common/compat.hpp"

namespace sprawl
{
	namespace detail
	{

		template<bool t_Val, typename t_IfTrueType>
		struct IfHelper
		{
			template<template <bool t_IsError> class t_ErrorType>
			using type = t_IfTrueType;

			typedef t_IfTrueType if_you_see_an_error_here_nothing_matched_your_if_sequence___consider_adding_an_else;

			template<bool, typename t_ElseIfType>
			using ElseIf = IfHelper<true, t_IfTrueType>;

			template<typename t_ElseType>
			using Else = IfHelper<true, t_IfTrueType>;
		};

		template<typename t_IfTrueType>
		struct IfHelper<false, t_IfTrueType>
		{
			template<template <bool t_IsError> class t_ErrorType>
			using type = t_ErrorType<false>;

			template<bool t_Val, typename t_ElseType>
			using ElseIf = IfHelper<t_Val, t_ElseType>;

			template<typename t_ElseType>
			using Else = IfHelper<true, t_ElseType>;
		};
	}

	template<bool t_Val, typename t_IfTryeType>
	using If = detail::IfHelper<t_Val, t_IfTryeType>;

	SPRAWL_DEFINE_COMPILE_ERROR(UnmatchedIfSequence, "If you see an error here, nothing matched your 'if' sequence, and no custom error message was installed. Consider adding an 'Else' or correct your invocation.");
}

#define IF(val, IfTrue) typename sprawl::If<(val), IfTrue>
#define ELSEIF(val, IfTrue) ::template ElseIf<(val), IfTrue>
#define ELSE(T) ::template Else< T >
#define ENDIF_ERR(ErrorType) ::template type<ErrorType>
#define ENDIF ::template type<::sprawl::UnmatchedIfSequence>