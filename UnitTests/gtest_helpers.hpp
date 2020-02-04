#pragma once

#include "../string/String.hpp"
#include "../common/errors.hpp"
#include <iostream>

namespace sprawl
{
	inline void PrintTo(sprawl::String const& value, std::ostream* stream)
	{
		*stream << value.toStdString();
	}
}

#if SPRAWL_EXCEPTIONS_ENABLED
#	define ASSERT_NO_SPRAWL_EXCEPT(expression) do { try{ (expression); } catch(sprawl::ExceptionBase& e) { ASSERT_TRUE(false) << " Expression " #expression << " threw an exception: " << e.what(); } } while(false)
#	define ABORT_ON_SPRAWL_EXCEPT(expression) do { try{ (expression); } catch(sprawl::ExceptionBase& e) { std::cerr << " Expression " #expression << " threw an exception: " << e.what(); std::terminate(); } } while(false)
#else
#	define ASSERT_NO_SPRAWL_EXCEPT(expression) do { auto error__ = (expression); if(error__.Error()) { ASSERT_TRUE(false) << " Expression " #expression << " threw an exception: " << error__.ErrorString(); } } while(false)
#	define ABORT_ON_SPRAWL_EXCEPT(expression) do { auto error__ = (expression); if(error__.Error()) { std::cerr << " Expression " #expression << " threw an exception: " << error__.ErrorString(); std::terminate(); } } while(false)
#endif
