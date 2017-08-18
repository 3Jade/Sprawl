#pragma once

#include <type_traits>
#include <exception>
#include <stddef.h>
#include <utility>
#include <stdio.h>
#include "compat.hpp"

#if defined(SPRAWL_NO_EXCEPTIONS)
#	define SPRAWL_EXCEPTIONS_ENABLED 0
#elif defined(__clang__)
#	define SPRAWL_EXCEPTIONS_ENABLED __has_feature(cxx_exceptions)
#elif defined(__GNUC__)
#	ifdef __EXCEPTIONS
#		define SPRAWL_EXCEPTIONS_ENABLED 1
#	else
#		define SPRAWL_EXCEPTIONS_ENABLED 0
#	endif
#elif defined(_WIN32)
#	ifdef _CPPUNWIND
#		define SPRAWL_EXCEPTIONS_ENABLED 1
#	else
#		define SPRAWL_EXCEPTIONS_ENABLED 0
#	endif
#else
#	define SPRAWL_EXCEPTIONS_ENABLED 0
#endif

#if !SPRAWL_EXCEPTIONS_ENABLED
#	if defined(__GNUC__)
#		define SPRAWL_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#	elif defined _WIN32
#		define SPRAWL_WARN_UNUSED_RESULT _Check_return_
#	endif
#else
#	define SPRAWL_WARN_UNUSED_RESULT
#endif

#if SPRAWL_EXCEPTIONS_ENABLED
#	define SPRAWL_THROW_EXCEPTION_OR_ABORT(exception) throw exception
#else
#	define SPRAWL_THROW_EXCEPTION_OR_ABORT(exception) fprintf(stderr, exception.what()); fflush(stderr); std::terminate()
#endif

#define SPRAWL_ABORT_MSG_F(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__); fputs("\n", stderr); fflush(stderr); std::terminate();
#define SPRAWL_ABORT_MSG(msg, ...) fputs(msg, stderr); fputs("\n", stderr); fflush(stderr); std::terminate();

#define SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD SPRAWL_THROW_EXCEPTION(sprawl::UnimplementedVirtualMethod())
#define SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD_ABORT SPRAWL_ABORT_MSG(sprawl::ExceptionString(sprawl::ExceptionId::UNIMPLEMENTED_VIRTUAL_METHOD))

namespace sprawl
{
#define SPRAWL_EXCEPTION(baseId, id, name, type) id,
	enum class ExceptionId
	{
		NONE,
		GENERAL_EXCEPTION,
		#include "exceptions.hpp"
	};
#undef SPRAWL_EXCEPTION


	class ExceptionBase : public std::exception
	{
	public:
		~ExceptionBase() throw()
		{

		}

		virtual ExceptionId GetId() const throw() = 0;
	};

	inline char const* ExceptionString(ExceptionId id)
	{
		#define SPRAWL_EXCEPTION(baseId, id, name, type) name,
		static char const* const exception_table[] = {
			"",
			"Unknown or unspecified error",
			#include "exceptions.hpp"
		};
		#undef SPRAWL_EXCEPTION

		return exception_table[size_t(id)];
	}

	template<ExceptionId id>
	struct ExceptionBaseClass
	{
	};

	template<>
	struct ExceptionBaseClass<ExceptionId::GENERAL_EXCEPTION>
	{
		typedef ExceptionBase base;
	};

	template<ExceptionId error_code>
	class Exception;

#define SPRAWL_EXCEPTION(baseId, id, name, type) \
	template<> \
	struct ExceptionBaseClass<ExceptionId::id> \
	{ \
		typedef Exception<ExceptionId::baseId> base; \
	};
#include "exceptions.hpp"
#undef SPRAWL_EXCEPTION

	template<ExceptionId error_code>
	class Exception : public ExceptionBaseClass<error_code>::base
	{
	public:
		virtual char const* what() const throw () override
		{
			return ExceptionString(error_code);
		}

		~Exception() throw ()
		{

		}

		virtual ExceptionId GetId() const throw() override
		{
			return error_code;
		}
	};

	typedef Exception<ExceptionId::GENERAL_EXCEPTION> GeneralException;
#define SPRAWL_EXCEPTION(baseId, id, name, type) typedef Exception<ExceptionId::id> type;
#include "exceptions.hpp"
#undef SPRAWL_EXCEPTION
}

#if SPRAWL_EXCEPTIONS_ENABLED
namespace sprawl
{
	template<typename T>
	using ErrorState = T;
}
#	define SPRAWL_THROW_EXCEPTION(exception) throw (exception)
#	define SPRAWL_RETHROW(expression) expression
#	define SPRAWL_RETHROW_OR_GET(expression, value) value = expression
#	define SPRAWL_ACTION_ON_ERROR(expression, action) try{ (expression); } catch(sprawl::ExceptionBase& e) { (action); }
#else
namespace sprawl
{
	template<typename T>
	class ErrorState;

	class ErrorStateTypeDetector {};
}

template<typename T>
class sprawl::ErrorState : public ErrorStateTypeDetector
{
public:
	template<typename... Params>
	ErrorState(Params&&... params)
		: element()
		, error_code(ExceptionId::NONE)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{
		new(&element) T(std::forward<Params>(params)...);
	}

	ErrorState()
		: element()
		, error_code(ExceptionId::NONE)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{
		new(&element) T();
	}

	ErrorState(ExceptionId rethrow_error_code)
		: element()
		, error_code(rethrow_error_code)
	#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
	#endif
	{

	}

	template<sprawl::ExceptionId error_code_>
	ErrorState(Exception<error_code_> const&)
		: element()
		, error_code(error_code_)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

	template<sprawl::ExceptionId error_code_>
	ErrorState(Exception<error_code_>&&)
		: element()
		, error_code(error_code_)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	~ErrorState()
	{
		if (error_code != ExceptionId::NONE && !error_code_checked)
		{
			fprintf(stderr, "ErrorState object destroyed with unhandled exception state!\n"
				"(To disable this check on debug builds, define SPRAWL_ERRORSTATE_PERMISSIVE; to enable on release builds, define SPRAWL_ERRORSTATE_STRICT)\n"
				"The error was: ");
			SPRAWL_ABORT_MSG(ErrorString());
		}
	}
#endif

	inline operator T&() { return Get(); }

	inline T& Get()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		if (error_code != ExceptionId::NONE)
		{
			SPRAWL_ABORT_MSG(sprawl::ExceptionString(error_code));
		}
		#endif
		return reinterpret_cast<T&>(element);
	}

	bool Error()
	{
		return error_code != ExceptionId::NONE;
	}

	sprawl::ExceptionId ErrorCode()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return error_code;
	}

	char const* ErrorString()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return ExceptionString(error_code);
	}

private:
	ErrorState(ErrorState const& other);
	ErrorState& operator=(ErrorState const& other);

	typename std::aligned_storage<sizeof(T), alignof(T)>::type element;
	sprawl::ExceptionId error_code;
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	bool error_code_checked;
#endif
};



template<typename T>
class sprawl::ErrorState<T&> : public ErrorStateTypeDetector
{
public:
	ErrorState(T& elem)
		: element(&elem)
		, error_code(ExceptionId::NONE)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

	template<sprawl::ExceptionId error_code_>
	ErrorState(Exception<error_code_> const&)
		: element()
		, error_code(error_code_)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

	ErrorState(ExceptionId rethrow_error_code)
		: element()
		, error_code(rethrow_error_code)
	#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
	#endif
	{

	}

	template<sprawl::ExceptionId error_code_>
	ErrorState(Exception<error_code_>&&)
		: element()
		, error_code(error_code_)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	~ErrorState()
	{
		if (error_code != ExceptionId::NONE && !error_code_checked)
		{
			fprintf(stderr, "ErrorState object destroyed with unhandled exception state!\n"
				"(To disable this check on debug builds, define SPRAWL_ERRORSTATE_PERMISSIVE; to enable on release builds, define SPRAWL_ERRORSTATE_STRICT)\n"
				"The error was: ");
			SPRAWL_ABORT_MSG(ErrorString());
		}
	}
#endif

	inline operator T&() { return Get(); }

	inline T& Get()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		if (error_code != ExceptionId::NONE)
		{
			SPRAWL_ABORT_MSG(sprawl::ExceptionString(error_code));
		}
#endif
		return *element;
	}

	bool Error()
	{
		return error_code != ExceptionId::NONE;
	}

	sprawl::ExceptionId ErrorCode()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return error_code;
	}

	char const* ErrorString()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return ExceptionString(error_code);
	}

	template<typename U>
	auto operator=(U&& value) -> decltype(std::declval<T>() = std::forward<U>(value))
	{
		return *element = std::forward<T>(value);
	}

private:
	ErrorState& operator=(ErrorState const& other);

	T* element;
	sprawl::ExceptionId error_code;
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	bool error_code_checked;
#endif
};




template<>
class sprawl::ErrorState<void> : public ErrorStateTypeDetector
{
public:
	ErrorState()
		: error_code(ExceptionId::NONE)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

	ErrorState(ExceptionId rethrow_error_code)
		: error_code(rethrow_error_code)
	#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
	#endif
	{

	}

	template<sprawl::ExceptionId error_code_>
	ErrorState(Exception<error_code_> const&)
		: error_code(error_code_)
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		, error_code_checked(false)
#endif
	{

	}

#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	~ErrorState()
	{
		if (error_code != ExceptionId::NONE && !error_code_checked)
		{
			fprintf(stderr, "ErrorState object destroyed with unhandled exception state!\n"
				"(To disable this check on debug builds, define SPRAWL_ERRORSTATE_PERMISSIVE; to enable on release builds, define SPRAWL_ERRORSTATE_STRICT)\n"
				"The error was: ");
			SPRAWL_ABORT_MSG(ErrorString());
		}
	}
#endif

	bool Error()
	{
		return error_code != ExceptionId::NONE;
	}

	sprawl::ExceptionId ErrorCode()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return error_code;
	}

	char const* ErrorString()
	{
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
		error_code_checked = true;
#endif
		return ExceptionString(error_code);
	}

private:
	ErrorState& operator=(ErrorState const& other);

	sprawl::ExceptionId error_code;
#if (SPRAWL_DEBUG || defined(SPRAWL_ERRORSTATE_STRICT)) && !defined(SPRAWL_ERRORSTATE_PERMISSIVE)
	bool error_code_checked;
#endif
};

#	define SPRAWL_THROW_EXCEPTION(exception) return exception;
#   define SPRAWL_RETHROW(expression) auto SPRAWL_CONCAT(error__, __LINE__) = (expression); if(SPRAWL_CONCAT(error__, __LINE__).Error()) { return SPRAWL_CONCAT(error__, __LINE__).ErrorCode(); }
#   define SPRAWL_RETHROW_OR_GET(expression, value) auto SPRAWL_CONCAT(error__, __LINE__) = (expression); if(SPRAWL_CONCAT(error__, __LINE__).Error()) { return SPRAWL_CONCAT(error__, __LINE__).ErrorCode(); } value = SPRAWL_CONCAT(error__, __LINE__).Get()
#	define SPRAWL_ACTION_ON_ERROR(expression, action) auto SPRAWL_CONCAT(error__, __LINE__) = (expression); if(SPRAWL_CONCAT(error__, __LINE__).Error()) { action; }
#endif
