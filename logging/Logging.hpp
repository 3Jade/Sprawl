#pragma once
#include "../string/StringBuilder.hpp"
#include "../string/String.hpp"
#include "../time/time.hpp"
#include <functional>
#include <memory>

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
		void Log(int level, String const& levelStr, String const& message, String const& file, String const& function, int line);
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
		void Log(int level, String const& levelStr, Category const& category, sprawl::String const& message, String const& file, String const& function, int line);

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

		typedef std::function<void(std::shared_ptr<Message> const&)> Handler;

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
		 * @brief Print a message to the passed-in file. This is an immediate print operation.
		 *        This function is exposed to enable easier creation of new Handlers

		 * @param message   The message to print
		 * @param file      The file to print to
		 * @param filename  The base name of the file (without timestamps and/or .1/.2/etc) - this is used for file rotation to create a new file if needed.
		 */
		void PrintMessageToFile(std::shared_ptr<Message> const& message, filesystem::File& file, sprawl::String const& filename);

		/**
		 * @brief Prints a message directly to a stream (i.e., stdout/stderr).
		 *        If the stream is a TTY, then the message will be printed in color if the options indicate it should.
		 *
		 * @param message  The message to print
		 * @param stream   The stream to print to
		 */
		void PrintMessageToStream(const std::shared_ptr<Message>& message, FILE* stream);

		/**
		 * @brief   Returns a handler to print to a file of the given name (modified according to file naming rules)
		 *
		 * @param   filename  The base filename without any naming rules modifications
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToFile(sprawl::String const& filename);

		/**
		 * @brief   Returns a handler to print to a file of the given name (modified according to file naming rules).
		 *          In contrast with PrintToFile, PrintToFile_Threaded launches a dedicated thread and passes messages to it for printing.
		 *
		 * @param   filename  The base filename without any naming rules modifications
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToFile_Threaded(sprawl::String const& filename);

		/**
		 * @brief   Returns a handler to print to a file of the given name (modified according to file naming rules).
		 *          In contrast with PrintToFile_Threaded, PrintToFile_ThreadManager integrates with an existing sprawl::threading::ThreadManager instance
		 *          and pushes log operations to threads with the given flag. Note that if your manager is threaded, tasks will be added for the current stage.
		 *          If the stage advances after this point, the log printing may be delayed until the next time that stage is run.
		 *
		 * @param   filename    The base filename without any naming rules modifications
		 * @param   manager     The ThreadManager instance to use. This instance must not go out of scope or be destroyed before sprawl::logging::ShutDown() is called.
		 * @param   threadFlag  The flags to be passed into ThreadManager::AddTask()
		 * @return  A valid sprawl::logging::Handler instance
		 */
		Handler PrintToFile_ThreadManager(sprawl::String const& filename, threading::ThreadManager& manager, int64_t threadFlag);

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
		 * @brief Ensure all log messages are flushed to the OS buffers. Only useful for PrintToFile_Threaded handlers;
		 *        with PrintToFile handlers, output is flushed immediately, and with PrintToFile_ThreadManager handlers,
		 *        the logging system does not have sufficient control of the thread manager to ensure a proper flush.
		 */
		void Flush();

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
	Message(int64_t timestamp_, String const& level_, String const& category_, String const& message_, String const& file_, String const& function_, int line_, Options* options_)
		: timestamp(timestamp_)
		, level(level_)
		, category(category_)
		, message(message_)
		, file(file_)
		, function(function_)
		, line(line_)
		, messageOptions(options_)
	{

	}
	int64_t timestamp;
	String level;
	String category;
	String message;
	String file;
	String function;
	int line;

	Options* messageOptions;

	String ToString();
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

#define LOG(LEVEL, ...) SPRAWL_LOG_LEVEL_TYPE( SPRAWL_LOG_LEVEL_PREFIX SPRAWL_MINIMUM_LOG_LEVEL ) <= SPRAWL_LOG_LEVEL_PREFIX LEVEL ? ::sprawl::logging::Log(int(SPRAWL_LOG_LEVEL_PREFIX LEVEL), #LEVEL, __VA_ARGS__, __FILE__, __FUNCTION__, __LINE__) : ::sprawl::logging::Nop()

///@TODO: Backtraces
