#pragma once
#include "../string/StringBuilder.hpp"
#include "../string/String.hpp"
#include "../time/time.hpp"
#include "../collections/Vector.hpp"
#include <functional>
#include <memory>
#include "Backtrace.hpp"

namespace sprawl
{
	namespace threading
	{
		class ThreadManager;
	}

	namespace filesystem
	{
		class File;
	}

	namespace logging
	{
		class Category;
		struct Options;
		struct Message;

		enum class Color
		{
			LightGrey,
			DarkGrey,
			Red,
			Green,
			Yellow,
			Blue,
			Magenta,
			Cyan,
			White,
			Black,
			DarkRed,
			DarkGreen,
			DarkYellow,
			DarkBlue,
			DarkMagenta,
			DarkCyan,
			Default = Color::LightGrey,
		};

#ifndef SPRAWL_LOG_ENFORCE_CATEGORIES
		/**
		 * @brief Log a message with no category
		 *
		 * @param level     Log level (to be compared against runtime maximum)
		 * @param levelStr  String representation of log level (to be printed)
		 * @param message   Message that was logged
		 * @param file      File containing the log statement
		 * @param function  Function containing the log statement
		 * @param line      Line on which the log statement appears
		 */
		void Log(int level, StringLiteral const& levelStr, String const& message, StringLiteral const& file, StringLiteral const& function, StringLiteral const& line);
#endif
		/**
		 * @brief Log a message with a filterable category
		 *
		 * @param level     Log level (to be compared against runtime maximum)
		 * @param levelStr  String representation of log level (to be printed)
		 * @param category  Category of the log message (to be compared against category filters and possibly printed)
		 * @param message   Message that was logged
		 * @param file      File containing the log statement
		 * @param function  Function containing the log statement
		 * @param line      Line on which the log statement appears
		 */
		void Log(int level, StringLiteral const& levelStr, Category const& category, sprawl::String const& message, StringLiteral const& file, StringLiteral const& function, StringLiteral const& line);

		/**
		 * @brief Set the global options that will apply to any log level without its own options set
		 *
		 * @param options  The options to apply
		 */
		void SetDefaultOptions(Options const& options);
		/**
		 * @brief Set the options for the given log level
		 *
		 * @param level   The level to set options on
		 * @param options The options to be set
		 */
		void SetLevelOptions(int level, Options const& options);

		template<typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
		void SetLevelOptions(T level, Options const& options)
		{
			SetLevelOptions(int(level), options);
		}

		/**
		 * @brief Sets the runtime minimum log level. Any log message with a level below 'level' will not be printed after this function has been called.
		 *
		 * @param level  The minimum level to allow logging for.
		 */
		void SetRuntimeMinimumLevel(int level);

		int GetRuntimeMinimumLevel();

		template<typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
		void SetRuntimeMinimumLevel(T level)
		{
			SetRuntimeMinimumLevel(int(level));
		}

		/**
		 * @brief Disable the given category. Any categories that contain this category as a prefix will not be printed after this call.
		 *
		 * @param category  The category to disable
		 */
		void DisableCategory(Category const& category);
		void DisableCategory(sprawl::String const& categoryStr);

		/**
		 * @brief Re-enables the given category and its children. No effect if the category was not previously disabled by DisableCategory.
		 *
		 * @param category  The category to enable
		 */
		void EnableCategory(Category const& category);
		void EnableCategory(sprawl::String const& categoryStr);

		/**
		 * @brief Set the format string to use for printing log messages.
		 *
		 * @details The format string accepts the following macro parameters:
		 *          {timestamp} - A timestamp whose format is specified by SetTimeFormat
		 *          {threadid}  - A numeric ID representing the thread that called LOG(). Not equivalent with gettid() on Linux, only guaranteed unique.
		 *          {level}     - A string representation of the level of this log message
		 *          {category}  - The category, if any, of this message. If logged without a category, this will be an empty string.
		 *          {message}   - The message that was printed
		 *          {file}      - The file in which the message appeared
		 *          {function}  - The function in which the message appeared
		 *          {line}      - The line on which the message appeared
		 *
		 * @param format  The format string to use
		 */
		void SetFormat(String const& format);

		/**
		 * @brief Set the format to be used to print timestamps
		 *
		 * @param strftimeFormat  The format of the seconds portion of the timestamp in stftime syntax
		 * @param maxResolution   The resolution below seconds to append to the timestamp; i.e., if this is sprawl::time::Resolution::Milliseconds,
		 *                        then a dot and three digits representing the millisecond portion of the timestamp will be appended to the strftime
		 *                        string. If this is set to sprawl::time::Resolution::Seconds or higher, nothing will be printed.
		 */
		void SetTimeFormat(String const& strftimeFormat, sprawl::time::Resolution maxResolution);

		/**
		 * @brief A handler, or 'sink', to direct log messages to a destination.
		 */
		struct Handler
		{
			std::function<void(std::shared_ptr<Message> const&)> Log; /**< Function to parse and print the log message to its destination */
			std::function<void()> Flush; /**< Function to flush the log - to ensure it has fully reached its destination */
			void* uniqueId; /**<
							 * A unique identifier to ensure the same log sink is only flushed once per call to Flush()
							 * Usually this is a pointer to the underlying object (i.e., pointer to a file) but it doesn't
							 * have to point to anything valid. This could just store a random integer value if you can guarantee
							 * the uniqueness of that value; it's only used to guard multiple calls to the same Flush() function,
							 * since std::function has no comparison operator.
							 */
		};

		enum class CategoryCombinationType
		{
			Exclusive,
			Combined,
		};

		/**
		 * @brief Set the handler for the categories that have not been explicitly bound to another handler.
		 *        By default, if this is not set, logs will print to stdout.
		 *
		 * @param handler  The Handler to use to default-handle messages
		 */
		void SetDefaultHandler(Handler const& handler);

		/**
		 * @brief Add a handler for the given category.
		 *
		 * @details  If 'type' is CategoryCombinationType::Combined (the default), then if this message matches any other criteria,
		 *           it will be handled by both this handler and the other one. For example, if a handler is set for category
		 *           'Sprawl' and another is added for category 'Sprawl::Logging', then a message of category 'Sprawl::Logging' will be handled
		 *		     by both the 'Sprawl::Logging' handler AND the 'Sprawl' handler. If the type is CategoryCombinationType::Exclusive,
		 *           then it will only be handled by the 'Sprawl::Logging' handler.
		 *
		 *           Node that for performance reasons, the 'Sprawl' handler in this example must be registered before the 'Sprawl::Logging' handler,
		 *           otherwise only the 'Sprawl::Logging' handler will handle those messages.
		 *
		 * @param category  The category to handle
		 * @param handler   The handler to handle it with
		 * @param type      The combination type
		 */
		void AddCategoryHandler(Category const& category, Handler const& handler, CategoryCombinationType type = CategoryCombinationType::Combined);

		/**
		 * @brief   Returns a handler to print to a file of the given name (modified according to file naming rules)
		 *
		 * @param   filename  The base filename without any naming rules modifications
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToFile(sprawl::String const& filename);

		/**
		 * @brief   Returns a handler that runs another handler on a dedicated internal thread.
		 *
		 * @param   handler  The handler to execute on the internal thread
		 * @return  A new valid sprawl::logging::Handler instance
		 */
		Handler RunHandler_Threaded(Handler const& handler);

		/**
		 * @brief   Returns a handler to run another handler on one or more external threads
		 *          In contrast with RunHandler_Threaded, RunHandler_ThreadManager integrates with an existing sprawl::threading::ThreadManager instance
		 *          and pushes log operations to threads with the given flag. Note that if your manager is staged, tasks will be added for the current stage.
		 *          If the stage advances after this point, the log printing may be delayed until the next time that stage is run.
		 *
		 * @param   handler  The handler to execute on the supplied thread manager
		 * @param   manager     The ThreadManager instance to use. This instance must not go out of scope or be destroyed before sprawl::logging::ShutDown() is called.
		 * @param   threadFlag  The flags to be passed into ThreadManager::AddTask()
		 * @return  A new, valid sprawl::logging::Handler instance
		 */
		Handler RunHandler_ThreadManager(Handler const& handler, threading::ThreadManager& manager, int64_t threadFlag);

		/**
		 * @brief Returns the File object associated with the given filename
		 *
		 * @detail Note that if the file gets rotated, this File object will point at the old file, not the new.
		 */
		filesystem::File GetHandleForFile(sprawl::String const& filename);

		/**
		 * @brief   Returns a simple handler that prints all messages directly to stdout
		 *
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToStdout();

		/**
		 * @brief   Returns a simple handler that prints all messages directly to stderr
		 *
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToStderr();

		enum class RenameMethod
		{
			Timestamp,
			Counter,
		};

		/**
		 * @brief    Sets the method of renaming files for file rotation.
		 *
		 * @details  The 'arg' parameter is context-sensitive.
		 *           If method is RenameMethod::Timestamp, 'arg' should be a sprawl::time::Resolution value (cast to int); any other value is undefined behavior.
		 *           If method is RenameMethod::Counter, 'arg' specifies the maximum number of files to keep around before deleting old ones.
		 *           i.e., if arg is 5, it will rename files to .log.1, .log.2, .log.3, .log.4, and .log.5, and delete any existing .log.5 files instead of renaming to .log.6.
		 *
		 * @param method  Method by which to cycle files
		 * @param arg     Context-sensitive arg (see details)
		 */
		void SetRenameMethod(RenameMethod method, int arg);

		/**
		 * @brief Set the maximum allowable filesize before closing the file and opening a new one.
		 *
		 * @param maxSizeBytes  Max size in bytes
		 */
		void SetMaxFilesize(size_t maxSizeBytes);

		/**
		 * @brief Initialize the logging system.
		 *        Currently only needed if using a PrintToFile_Threaded handler, but may be necessary for other reasons in the future;
		 *        therefore future-proof code should call this regardless of handlers used.
		 */
		void Init();

		/**
		* @brief Ensure all log messages are flushed
		*/
		void Flush();

		/**
		* @brief Ensure all log messages for the given category are flushed
		*/
		void Flush(Category const& category);

		/**
		 * @brief Stop the logging system. For PrintToFile_Threaded handlers, no more logs will be written, but they will be queued.
		 *        Init() can be called again to resume logging.
		 *        For other handler types, this has no effect.
		 */
		void Stop();

		/**
		 * @brief Terminate the logging system entirely. This resets all parameters to their defaults and joins any internal threads that have been started.
		 */
		void ShutDown();

		/**
		 * @brief Formats the given stack frame into the given string builder to be printed as part of a stack trace.
		 *        Made publically accessible so that it can be used by applications with scripting systems to provide
		 *        script backtraces, etc.
		 *
		 * @param frame            A Backtrace::Frame object containing the data to print.
		 * @param printMemoryData  If true, the "address" and "offset" values will be printed. If false, only the function name will be printed.
		 * @param builder          String builder to output the results to. Each frame automatically inserts a blank line after it.
		 */
		void FormatStackFrame(Backtrace::Frame const& frame, bool printMemoryData, sprawl::StringBuilder& builder);

		/**
		 * @brief Format an entire backtrace to a string.
		 *
		 * @param backtrace  Backtrace to format
		 * @param builder    String builder to contain the resulting data
		 */
		void BacktraceToString(Backtrace const& backtrace, sprawl::StringBuilder& builder);

		/**
		 * @brief Registers a callback to be called with log messages of the given level. The callback appends to the passed-in string builder
		 *        with extra data to be printed to the log (such as a script backtrace or a print of the global error state).
		 *        Callbacks will be called in the order they are added.
		 *
		 * @param logLevel  The log level that triggers this callback.
		 * @param callback  The callback to be triggered.
		 */
		void AddExtraInfoCallback(int logLevel, std::function<void*()> collect, std::function<void(void* data, sprawl::StringBuilder& builder)> print);

		template<typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
		void  AddExtraInfoCallback(T logLevel, std::function<void*()> collect, std::function<void(void* data, sprawl::StringBuilder& builder)> print)
		{
			AddExtraInfoCallback(int(logLevel), collect, print);
		}

		inline void Nop() {}
	}
}

class sprawl::logging::Category
{
public:
	template<typename... Params>
	Category(Params... params)
		: m_str()
	{
		init_(params...);
	}

	Category()
		: m_str()
	{
		//
	}

	String const& Str() const
	{
		return m_str;
	}

private:
	void init_(char const* const param)
	{
		m_str = param;
	}

	void init_(StringBuilder& builder, char const* const param)
	{
		builder << param;
		m_str = builder.Str();
	}

	template<typename... Params>
	void init_(StringBuilder& builder, char const* const param1, Params... params)
	{
		builder << param1 << "::";
		init_(builder, params...);
	}

	template<typename... Params>
	void init_(char const* const param1, Params... params)
	{
		StringBuilder builder;
		builder << param1 << "::";
		init_(builder, params...);
	}

	String m_str;
};

struct sprawl::logging::Options
{
	Options()
		: color(Color::Default)
		, logToStdout(false)
		, logToStderr(false)
		, includeBacktrace(false)
		, nameOverride(nullptr)
	{
		//
	}

	Color color;
	bool logToStdout;
	bool logToStderr;
	bool includeBacktrace;
	char const* nameOverride;
};

struct sprawl::logging::Message
{
	Message(int64_t timestamp_, int64_t threadid_, int levelInt_, StringLiteral const& level_, String const& category_, String const& message_, StringLiteral const& file_, StringLiteral const& function_, StringLiteral const& line_, Options* options_)
		: timestamp(timestamp_)
		, threadid(threadid_)
		, levelInt(levelInt_)
		, level(level_)
		, category(category_)
		, message(message_)
		, file(file_)
		, function(function_)
		, line(line_)
		, messageOptions(options_)
		, backtrace()
		, extraInfo(sprawl::collections::Capacity(0))
	{

	}

	Message(Message&& other) = delete;
	Message(Message const& other) = delete;
	Message& operator=(Message const& other) = delete;
	Message& operator=(Message&& other) = delete;

	int64_t timestamp;
	int64_t threadid;
	int levelInt;
	StringLiteral level;
	String category;
	String message;
	StringLiteral file;
	StringLiteral function;
	StringLiteral line;

	Options* messageOptions;

	Backtrace backtrace;

	sprawl::collections::Vector<void*> extraInfo;

	String ToString();
	String CollectExtraData();
};

#ifndef SPRAWL_LOG_LEVEL_TYPE
	#define SPRAWL_LOG_LEVEL_TYPE int
	#define SPRAWL_LOG_LEVEL_PREFIX
#else
	#define SPRAWL_LOG_LEVEL_PREFIX SPRAWL_LOG_LEVEL_TYPE ::
#endif

#ifndef SPRAWL_MINIMUM_LOG_LEVEL
	#define SPRAWL_MINIMUM_LOG_LEVEL 0
#endif

#define SPRAWL_LOG_GENERATE_VARIABLE_NAME_CONCAT(base, line) base ## line
#define SPRAWL_LOG_GENERATE_VARIABLE_NAME(base, line) SPRAWL_LOG_GENERATE_VARIABLE_NAME_CONCAT(base, line)
#define SPRAWL_LOG_STATIC_INT SPRAWL_LOG_GENERATE_VARIABLE_NAME(sprawl_log_internal_int_, __LINE__)

#define SPRAWL_STRINGIFY_2(input) #input
#define SPRAWL_STRINGIFY(input) SPRAWL_STRINGIFY_2(input)

#define LOG(LEVEL, ...) SPRAWL_LOG_LEVEL_TYPE( SPRAWL_LOG_LEVEL_PREFIX SPRAWL_MINIMUM_LOG_LEVEL ) <= SPRAWL_LOG_LEVEL_PREFIX LEVEL && ::sprawl::logging::GetRuntimeMinimumLevel() <= int(SPRAWL_LOG_LEVEL_PREFIX LEVEL) ? ::sprawl::logging::Log(int(SPRAWL_LOG_LEVEL_PREFIX LEVEL), sprawl::StringLiteral(#LEVEL), __VA_ARGS__, sprawl::StringLiteral(__FILE__), sprawl::StringLiteral(__FUNCTION__), sprawl::StringLiteral(SPRAWL_STRINGIFY(__LINE__))) : ::sprawl::logging::Nop()

#define LOG_IF(condition, LEVEL, ...) (!(!(condition))) ? LOG(LEVEL, __VA_ARGS__) : ::sprawl::logging::Nop()

#define LOG_EVERY_N(count, LEVEL, ...) do \
	{ \
		static int SPRAWL_LOG_STATIC_INT = count - 1; \
		++SPRAWL_LOG_STATIC_INT; \
		if(SPRAWL_LOG_STATIC_INT == count) \
		{ \
			LOG(LEVEL, __VA_ARGS__); \
			SPRAWL_LOG_STATIC_INT = 0; \
		} \
	} while(false)

#define LOG_IF_EVERY_N(condition, count, LEVEL, ...) do \
	{ \
		static int SPRAWL_LOG_STATIC_INT = count - 1; \
		++SPRAWL_LOG_STATIC_INT; \
		if(SPRAWL_LOG_STATIC_INT == count) \
		{ \
			LOG_IF(condition, LEVEL, __VA_ARGS__); \
			SPRAWL_LOG_STATIC_INT = 0; \
		} \
	} while(false)

#define LOG_FIRST_N(count, LEVEL, ...) do \
	{ \
		static int SPRAWL_LOG_STATIC_INT = 0; \
		if(SPRAWL_LOG_STATIC_INT != count) \
		{ \
			LOG(LEVEL, __VA_ARGS__); \
			++SPRAWL_LOG_STATIC_INT; \
		} \
	} while(false)

#define LOG_ONCE(...) LOG_FIRST_N(1, LEVEL, __VA_ARGS__)

#define LOG_IF_FIRST_N(condition, count, LEVEL, ...) do \
	{ \
		static int SPRAWL_LOG_STATIC_INT = 0; \
		if((!(!(condition))) && SPRAWL_LOG_STATIC_INT != count) \
		{ \
			LOG(LEVEL, __VA_ARGS__); \
			++SPRAWL_LOG_STATIC_INT; \
		} \
	} while(false)

#define LOG_ASSERT(condition, LEVEL, ...) \
	if(!(condition)) \
	{ \
		LOG(LEVEL, "Assertion failed: " #condition); \
		LOG(LEVEL, __VA_ARGS__); \
		::sprawl::logging::Flush(); \
		std::terminate(); \
	}
