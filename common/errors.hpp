#pragma once

#if defined(SPRAWL_NO_EXCEPTIONS)
#	define SPRAWL_EXCEPTIONS_ENABLED 0
#elif defined(__clang__)
#	define SPRAWL_EXCEPTIONS_ENABLED __has_feature(cxx_exceptions)
#elif defined(__GNUC__)
#	define SPRAWL_EXCEPTIONS_ENABLED defined(__EXCEPTIONS)
#elif defined(_WIN32)
#	define SPRAWL_EXCEPTIONS_ENABLED defined(_CPPUNWIND)
#else
#	define SPRAWL_EXCEPTIONS_ENABLED 0
#endif

#if SPRAWL_EXCEPTIONS_ENABLED
#	define SPRAWL_THROW_EXCEPTION(exception) throw exception
#else
	namespace sprawl { void throw_exception(std::exception const& exception); }
#	define SPRAWL_THROW_EXCEPTION(exception) sprawl::throw_exception(exception);
#endif

#define SPRAWL_ABORT_MSG(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__); fputs("\n", stderr); abort();

#define SPRAWL_UNIMPLEMENTED_BASE_CLASS_METHOD SPRAWL_ABORT_MSG("This method called is not implemented on this object")
