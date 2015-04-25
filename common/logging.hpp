#pragma once

#include <cstdarg>
#include <cstdio>
#include <functional>

namespace sprawl
{
	namespace logging
	{
		enum class LogLevel
		{
			Trace,
			Debug,
			Info,
			Warning,
			Error,
		};

		inline char const* LogLevelToStr(LogLevel level)
		{
			switch(level)
			{
			case LogLevel::Trace: return "Trace";
			case LogLevel::Debug: return "Debug";
			case LogLevel::Info: return "Info";
			case LogLevel::Warning: return "Warning";
			case LogLevel::Error: return "Error";
			}
			return "";
		}

		namespace detail
		{
			//Templating this so that we don't have to instantiate ms_logFunction in a cpp file.
			template<bool b>
			class Logger
			{
			public:
				static void Log(LogLevel level, const char* const message, ...)
				{
					if(!ms_logFunction)
					{
						return;
					}
					if(level < ms_minLevel)
					{
						return;
					}
					va_list args;
					va_start(args, message);

					char msg[512];
					vsnprintf(msg, sizeof(msg), message, args);
					va_end(args);
					ms_logFunction(level, msg);
				}
				static LogLevel ms_minLevel;
				static std::function<void(LogLevel level, const char* const msg)> ms_logFunction;
			};
			template<bool b>
			LogLevel Logger<b>::ms_minLevel = LogLevel::Info;
			template<bool b>
			std::function<void(LogLevel level, const char* const msg)> Logger<b>::ms_logFunction = nullptr;
		}

		inline void RegisterLogFunction(std::function<void(LogLevel level, const char* const msg)> fn)
		{
			detail::Logger<true>::ms_logFunction = fn;
		}

		inline void SetMinimumLogLevel(LogLevel level)
		{
			detail::Logger<true>::ms_minLevel = level;
		}
	}
}

#define	SPRAWL_LOG_TRACE(...) ::sprawl::logging::detail::Logger<true>::Log(::sprawl::logging::LogLevel::Trace, __VA_ARGS__)
#define	SPRAWL_LOG_DEBUG(...) ::sprawl::logging::detail::Logger<true>::Log(::sprawl::logging::LogLevel::Debug, __VA_ARGS__)
#define	SPRAWL_LOG_INFO(...) ::sprawl::logging::detail::Logger<true>::Log(::sprawl::logging::LogLevel::Info, __VA_ARGS__)
#define	SPRAWL_LOG_WARNING(...) ::sprawl::logging::detail::Logger<true>::Log(::sprawl::logging::LogLevel::Warning, __VA_ARGS__)
#define	SPRAWL_LOG_ERROR(...) ::sprawl::logging::detail::Logger<true>::Log(::sprawl::logging::LogLevel::Error, __VA_ARGS__)
