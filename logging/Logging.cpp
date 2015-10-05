#include "Logging.hpp"
#include <stdio.h>

#include "../threading/threadmanager.hpp"
#include "../collections/HashMap.hpp"
#include <map>
#include <set>
#include "../filesystem/filesystem.hpp"
#include "../filesystem/path.hpp"
#include <time.h>

namespace LoggingStatic
{
#ifdef _WIN32

	#include <io.h>

	static int colorArray[int(sprawl::logging::Color::DarkCyan) + 1] = {
		FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
		FOREGROUND_INTENSITY, 
		FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,
		0,
		FOREGROUND_RED,
		FOREGROUND_GREEN,
		FOREGROUND_GREEN | FOREGROUND_RED,
		FOREGROUND_BLUE,
		FOREGROUND_BLUE | FOREGROUND_RED,
		FOREGROUND_BLUE | FOREGROUND_GREEN
	};

	static void setColor_(sprawl::logging::Color color, FILE* stream)
	{
		if (!_isatty(_fileno(stream)))
		{
			return;
		}
		HANDLE handle = HANDLE(_get_osfhandle(_fileno(stream)));
		SetConsoleTextAttribute( handle, colorArray[int(color)] );
	}

	static void resetColor_(FILE* stream)
	{
		if (!_isatty(_fileno(stream)))
		{
			return;
		}
		HANDLE handle = HANDLE(_get_osfhandle(_fileno(stream)));
		SetConsoleTextAttribute( handle, colorArray[0] );
	}

#define snprintf _snprintf
#else
	#include <unistd.h>

	static char const* const colorArray[int(sprawl::logging::Color::DarkCyan) + 1] = {
		"\033[22;37m",
		"\033[1;30m",
		"\033[1;31m",
		"\033[1;32m",
		"\033[1;33m",
		"\033[1;34m",
		"\033[1;35m",
		"\033[1;36m",
		"\033[1;37m",
		"\033[22;30m",
		"\033[22;31m",
		"\033[22;32m",
		"\033[22;33m",
		"\033[22;34m",
		"\033[22;35m",
		"\033[22;36m"
	};

	static void setColor_(sprawl::logging::Color color, FILE* stream)
	{
		if(!isatty(fileno(stream)))
		{
			return;
		}
		fputs(colorArray[int(color)], stream);
	}

	static void resetColor_(FILE* stream)
	{
		if(!isatty(fileno(stream)))
		{
			return;
		}
		fputs("\033[0m", stream);
	}

#endif

	enum
	{
		NONE = 0,
		LOG_THREAD = 1
	};

	static sprawl::threading::ThreadManager logManager_;
	static bool useManager_ = false;

	struct FileData
	{
		FileData(sprawl::filesystem::File const& file_, sprawl::String const& filename_)
			: file(file_)
			, size(0)
			, filename(filename_)
			, mutex()
		{

		}

		FileData(FileData&& other)
			: file(std::move(other.file))
			, size(other.size)
			, filename(std::move(other.filename))
			, mutex(std::move(other.mutex))
		{
			other.size = 0;
		}

		sprawl::filesystem::File file;
		size_t size;
		sprawl::String filename;
		sprawl::threading::Mutex mutex;
	};

	static sprawl::collections::BasicHashMap<sprawl::String, FileData> fileMap_;

	static sprawl::logging::RenameMethod renameMethod_ = sprawl::logging::RenameMethod::Timestamp;
	static int renameArg_ = int(sprawl::time::Resolution::Milliseconds);

	static size_t maxFileSize_ = 500 * 1024 * 1024;

	static sprawl::String format_ = sprawl::String("{} {} [{}] {} [{}] ({}:{}:{})") + sprawl::filesystem::LineSeparator();

	static void addToThreadManager_(sprawl::logging::Handler const& handler, std::shared_ptr<sprawl::logging::Message> const& message, sprawl::threading::ThreadManager& manager, int64_t threadFlag)
	{
		manager.AddTask(std::bind(handler, message), threadFlag);
	}

	static sprawl::String modifyFilename_(sprawl::String const& filename)
	{
		if(renameMethod_ == sprawl::logging::RenameMethod::Timestamp)
		{
			sprawl::path::ExtResult result = sprawl::path::SplitExt(filename);
			return sprawl::Format("{}_{}.{}", result.path, sprawl::time::Now(sprawl::time::Resolution(renameArg_)), result.extension);
		}

		if(sprawl::path::Exists(filename))
		{
			int max = renameArg_;
			sprawl::String maxPath = sprawl::Format("{}.{}", filename, max);
			if(sprawl::path::Exists(maxPath))
			{
				sprawl::filesystem::Remove(maxPath);
			}
			for(int i = max - 1; i > 0; --i)
			{
				sprawl::String path = sprawl::Format("{}.{}", filename, i);
				if(sprawl::path::Exists(path))
				{
					sprawl::filesystem::Rename(path, Format("{}.{}", filename, i + 1));
				}
			}
			sprawl::filesystem::Rename(filename, Format("{}.1", filename));
		}
		return filename;
	}

	static sprawl::collections::BasicHashMap<sprawl::String, int> idx_;

	static sprawl::String strftimeFormat_ = "%Y-%m-%dT%H:%M:%S";
	static sprawl::time::Resolution maxResolution_ = sprawl::time::Resolution::Microseconds;

	static int resolutionSize_ = 6;

	static sprawl::collections::BasicHashMap<FILE*, sprawl::threading::Mutex> streamMutexes_;

	static sprawl::logging::Handler defaultHandler_ = sprawl::logging::PrintToStdout();
	static std::map<sprawl::String, sprawl::collections::Vector<sprawl::logging::Handler>> handlers_;

	static sprawl::logging::Options defaultOptions_;
	static sprawl::collections::BasicHashMap<int, sprawl::logging::Options> options_;

	static std::set<sprawl::String> disabledCategories_;

	static int minLevel_ = 0;

	template<typename Key, typename Value>
	static typename std::map<Key, Value>::const_iterator getClosestParentCategory_(std::map<Key, Value>& map, sprawl::String const& categoryStr)
	{
		if(map.empty())
		{
			return map.end();
		}
		auto it = map.find(categoryStr);
		if(it == map.end())
		{
			it = map.upper_bound(categoryStr);
			if(it == map.begin())
			{
				return map.end();
			}
			else if(it == map.end())
			{
				it = (++map.rbegin()).base();
			}
			else
			{
				--it;
			}
		}

		sprawl::String checkStr = it->first;
		while(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
		{
			if(categoryStr == checkStr)
			{
				return it;
			}
			else if(categoryStr.length() > checkStr.length())
			{
				if(categoryStr[checkStr.length()] == ':')
				{
					if(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
					{
						return it;
					}
				}
			}
			if(it == map.begin())
			{
				return map.end();
			}
			--it;
			checkStr = it->first;
		}
		return map.end();
	}

	template<typename T>
	static typename std::set<T>::const_iterator getClosestParentCategory_(std::set<T>& map, sprawl::String const& categoryStr)
	{
		if(map.empty())
		{
			return map.end();
		}
		auto it = map.find(categoryStr);
		if(it == map.end())
		{
			it = map.upper_bound(categoryStr);
			if(it == map.begin())
			{
				return map.end();
			}
			else if(it == map.end())
			{
				it = (++map.rbegin()).base();
			}
			else
			{
				--it;
			}
		}
		sprawl::String checkStr = *it;
		while(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
		{
			if(categoryStr == checkStr)
			{
				return it;
			}
			else if(categoryStr.length() > checkStr.length())
			{
				if(categoryStr[checkStr.length()] == ':')
				{
					if(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
					{
						return it;
					}
				}
			}
			if(it == map.begin())
			{
				return map.end();
			}
			--it;
			checkStr = *it;
		}
		return map.end();
	}

	struct ExtraInfoCallbackPair
	{
		std::function<void*()> Get;
		std::function<void(void*, sprawl::StringBuilder&)> Print;
	};

	static sprawl::collections::BasicHashMap<int, sprawl::collections::Vector<ExtraInfoCallbackPair>> extraInfoCallbacks_;


	static void printMessageToFile_(std::shared_ptr<sprawl::logging::Message> const& message, LoggingStatic::FileData& fileData)
	{
		sprawl::String strMessage = message->ToString();
		sprawl::String extraData = message->CollectExtraData();

		{
			sprawl::threading::ScopedLock(fileData.mutex);
			if(fileData.size + strMessage.length() + extraData.length() > LoggingStatic::maxFileSize_)
			{
				fileData.file.Close();
				fileData.file = sprawl::filesystem::Open(LoggingStatic::modifyFilename_(fileData.filename), "w");
			}
			fileData.file.Write(strMessage);

			if(!extraData.empty())
			{
				fileData.file.Write(extraData);
			}

			fileData.size += strMessage.length() + extraData.length();
		}

		if(message->messageOptions->logToStdout)
		{
			LoggingStatic::setColor_(message->messageOptions->color, stdout);
			fwrite(strMessage.c_str(), 1, strMessage.length(), stdout);
			LoggingStatic::resetColor_(stdout);
		}
		if(message->messageOptions->logToStderr)
		{
			LoggingStatic::setColor_(message->messageOptions->color, stderr);
			fwrite(strMessage.c_str(), 1, strMessage.length(), stderr);
			LoggingStatic::resetColor_(stderr);
		}
	}

	static void printMessageToStream_(std::shared_ptr<sprawl::logging::Message> const& message, FILE* stream)
	{
		sprawl::String strMessage = message->ToString();
		sprawl::String extraData = message->CollectExtraData();

		{
			sprawl::threading::ScopedLock(LoggingStatic::streamMutexes_.GetOrInsert(stream));
			LoggingStatic::setColor_(message->messageOptions->color, stream);
			fwrite(strMessage.c_str(), 1, strMessage.length(), stream);
			LoggingStatic::resetColor_(stream);


			if(!extraData.empty())
			{
				fwrite(extraData.c_str(), 1, extraData.length(), stream);
			}
		}
	}
}


void sprawl::logging::Log(int level, sprawl::String const& levelStr, sprawl::String const& message, sprawl::String const& file, sprawl::String const& function, String const& line)
{
	if(level < LoggingStatic::minLevel_)
	{
		return;
	}

	Options* options;
	auto it = LoggingStatic::options_.find(level);
	if(it.Valid())
	{
		options = &it.Value();
	}
	else
	{
		options = &LoggingStatic::defaultOptions_;
	}

	std::shared_ptr<Message> messagePtr = std::make_shared<Message>(sprawl::time::Now(sprawl::time::Resolution::Nanoseconds), sprawl::this_thread::GetHandle().GetUniqueId(), level, levelStr, "", message, file, function, line, &LoggingStatic::defaultOptions_);
	if(options->includeBacktrace)
	{
		messagePtr->backtrace = Backtrace::Get();
	}

	auto it2 = LoggingStatic::extraInfoCallbacks_.find(level);
	if(it2.Valid())
	{
		auto& callbacks = it2.Value();
		for(auto& callbackPair : callbacks)
		{
			messagePtr->extraInfo.PushBack(callbackPair.Get());
		}
	}

	LoggingStatic::defaultHandler_(messagePtr);
}

void sprawl::logging::Log(int level, sprawl::String const& levelStr, sprawl::logging::Category const& category, sprawl::String const& message, sprawl::String const& file, sprawl::String const& function, String const& line)
{
	if(level < LoggingStatic::minLevel_)
	{
		return;
	}

	Options* options;
	auto it = LoggingStatic::options_.find(level);
	if(it.Valid())
	{
		options = &it.Value();
	}
	else
	{
		options = &LoggingStatic::defaultOptions_;
	}

	sprawl::String categoryStr = category.Str();

	auto disableIt = LoggingStatic::getClosestParentCategory_(LoggingStatic::disabledCategories_, categoryStr);
	if(disableIt != LoggingStatic::disabledCategories_.end())
	{
		return;
	}
	std::shared_ptr<Message> messagePtr = std::make_shared<Message>(sprawl::time::Now(sprawl::time::Resolution::Nanoseconds), sprawl::this_thread::GetHandle().GetUniqueId(), level, levelStr, categoryStr, message, file, function, line, options);
	if(options->includeBacktrace)
	{
		messagePtr->backtrace = Backtrace::Get();
	}


	auto it2 = LoggingStatic::extraInfoCallbacks_.find(level);
	if(it2.Valid())
	{
		auto& callbacks = it2.Value();
		for(auto& callbackPair : callbacks)
		{
			messagePtr->extraInfo.PushBack(callbackPair.Get());
		}
	}

	auto it3 = LoggingStatic::getClosestParentCategory_(LoggingStatic::handlers_, categoryStr);
	if(it3 != LoggingStatic::handlers_.end())
	{
		for(auto& handler : it3->second)
		{
			handler(messagePtr);
		}
	}
	else
	{
		LoggingStatic::defaultHandler_(messagePtr);
	}
}

void sprawl::logging::SetDefaultOptions(Options const& options)
{
	LoggingStatic::defaultOptions_ = options;
}

void sprawl::logging::SetLevelOptions(int level, Options const& options)
{
	LoggingStatic::options_[level] = options;
}

void sprawl::logging::SetRuntimeMinimumLevel(int level)
{
	LoggingStatic::minLevel_ = level;
}

void sprawl::logging::DisableCategory(Category const& category)
{
	DisableCategory(category.Str());
}

void sprawl::logging::DisableCategory(sprawl::String const& categoryStr)
{
	LoggingStatic::disabledCategories_.insert(categoryStr);
}

void sprawl::logging::EnableCategory(Category const& category)
{
	EnableCategory(category.Str());
}

void sprawl::logging::EnableCategory(sprawl::String const& categoryStr)
{
	LoggingStatic::disabledCategories_.erase(categoryStr);
}

void sprawl::logging::SetFormat(sprawl::String const& format)
{
	static bool idxPopulated = false;
	if(!idxPopulated)
	{
		int i = 0;
		LoggingStatic::idx_.Insert("timestamp", i++);
		LoggingStatic::idx_.Insert("threadid", i++);
		LoggingStatic::idx_.Insert("level", i++);
		LoggingStatic::idx_.Insert("message", i++);
		LoggingStatic::idx_.Insert("category", i++);
		LoggingStatic::idx_.Insert("file", i++);
		LoggingStatic::idx_.Insert("function", i++);
		LoggingStatic::idx_.Insert("line", i++);
	}
	sprawl::StringBuilder builder;
	sprawl::StringBuilder name;

	size_t const formatLength = format.length();
	char const* const data = format.c_str();

	bool inBracket = false;

	for(size_t i = 0; i < formatLength; ++i)
	{
		const char c = data[i];
		if(c == '{')
		{
			if(inBracket)
			{
				builder << '{' << '{';
			}
			inBracket = !inBracket;
			continue;
		}

		if(inBracket)
		{
			if(c == '}')
			{
				sprawl::String str = name.TempStr();
				if(LoggingStatic::idx_.Has(str))
				{
					builder << '{' << LoggingStatic::idx_.Get(str) << '}';
				}
				else
				{
					builder << '{' << str << '}';
				}
				name.Reset();
				inBracket = false;
			}
			else
			{
				name << c;
			}

			continue;
		}

		builder << c;
	}
	builder << sprawl::filesystem::LineSeparator();
	LoggingStatic::format_ = builder.Str();
}

void sprawl::logging::SetTimeFormat(const sprawl::String& strftimeFormat, sprawl::time::Resolution maxResolution)
{
	LoggingStatic::strftimeFormat_ = strftimeFormat.GetOwned();
	LoggingStatic::maxResolution_ = maxResolution;
	switch(maxResolution)
	{
		case sprawl::time::Resolution::Nanoseconds: LoggingStatic::resolutionSize_ = 9; return;
		case sprawl::time::Resolution::Microseconds: LoggingStatic::resolutionSize_ = 6; return;
		case sprawl::time::Resolution::Milliseconds: LoggingStatic::resolutionSize_ = 3; return;
		default: LoggingStatic::resolutionSize_ = 0; return;
	}
}

void sprawl::logging::SetDefaultHandler(Handler const& handler)
{
	LoggingStatic::defaultHandler_ = handler;
}

void sprawl::logging::AddCategoryHandler(Category const& category, Handler const& handler, CategoryCombinationType type)
{
	sprawl::String categoryStr = category.Str();
	sprawl::collections::Vector<Handler> handlers;
	handlers.PushBack(handler);

	if(type == CategoryCombinationType::Combined && !LoggingStatic::handlers_.empty())
	{
		auto it = LoggingStatic::handlers_.upper_bound(categoryStr);

		if(it == LoggingStatic::handlers_.begin())
		{
			it = LoggingStatic::handlers_.end();
		}
		else if(it == LoggingStatic::handlers_.end())
		{
			it = (++LoggingStatic::handlers_.rbegin()).base();
		}
		else
		{
			--it;
		}
		if(it != LoggingStatic::handlers_.end())
		{
			sprawl::String checkStr = it->first;
			while(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
			{
				if(categoryStr == checkStr)
				{
					for(auto& absorbedHandler : it->second)
					{
						handlers.PushBack(absorbedHandler);
					}
				}
				else if(categoryStr.length() > checkStr.length())
				{
					if(categoryStr[checkStr.length()] == ':')
					{
						if(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
						{
							for(auto& absorbedHandler : it->second)
							{
								handlers.PushBack(absorbedHandler);
							}
						}
					}
				}
				if(it == LoggingStatic::handlers_.begin())
				{
					break;
				}
				--it;
				checkStr = it->first;
			}
		}
	}
	LoggingStatic::handlers_.emplace(categoryStr, handlers);
}

sprawl::logging::Handler sprawl::logging::PrintToFile(sprawl::String const& filename)
{
	sprawl::filesystem::File file;
	if(!LoggingStatic::fileMap_.Has(filename))
	{
		sprawl::String modifiedFilename = LoggingStatic::modifyFilename_(filename);
		file = sprawl::filesystem::Open(modifiedFilename, "w");
		LoggingStatic::fileMap_.Insert(filename, LoggingStatic::FileData(file, filename));
	}
	return std::bind(LoggingStatic::printMessageToFile_, std::placeholders::_1, std::ref(LoggingStatic::fileMap_.Get(filename)));
}

sprawl::logging::Handler sprawl::logging::RunHandler_Threaded(Handler const& handler)
{
	static bool managerInitialized_ = false;
	if(!managerInitialized_)
	{
		LoggingStatic::logManager_.AddThread(LoggingStatic::LOG_THREAD);
		managerInitialized_ = true;
	}
	LoggingStatic::useManager_ = true;
	return RunHandler_ThreadManager(handler, LoggingStatic::logManager_, LoggingStatic::LOG_THREAD);
}

sprawl::logging::Handler sprawl::logging::RunHandler_ThreadManager(Handler const& handler, sprawl::threading::ThreadManager& manager, int64_t threadFlag)
{
	return std::bind(LoggingStatic::addToThreadManager_, handler, std::placeholders::_1, std::ref(manager), threadFlag);
}

sprawl::logging::Handler sprawl::logging::PrintToStdout()
{
	return std::bind(LoggingStatic::printMessageToStream_, std::placeholders::_1, stdout);
}

sprawl::logging::Handler sprawl::logging::PrintToStderr()
{
	return std::bind(LoggingStatic::printMessageToStream_, std::placeholders::_1, stderr);
}

void sprawl::logging::SetRenameMethod(RenameMethod method, int arg)
{
	LoggingStatic::renameMethod_ = method;
	LoggingStatic::renameArg_ = arg;
}

void sprawl::logging::SetMaxFilesize(size_t maxSizeBytes)
{
	LoggingStatic::maxFileSize_ = maxSizeBytes;
}

void sprawl::logging::Init()
{
	Backtrace::Init();
	if(LoggingStatic::useManager_)
	{
		LoggingStatic::logManager_.Start(0);
	}
}

void sprawl::logging::Flush()
{
	if(LoggingStatic::useManager_)
	{
		LoggingStatic::logManager_.Sync();
	}
	for(auto& kvp : LoggingStatic::fileMap_)
	{
		kvp.Value().file.Flush();
	}
	fflush(stdout);
	fflush(stderr);
}

void sprawl::logging::Stop()
{
	if(LoggingStatic::useManager_)
	{
		LoggingStatic::logManager_.Stop();
	}
}

void sprawl::logging::ShutDown()
{
	Flush();
	if(LoggingStatic::useManager_)
	{
		LoggingStatic::logManager_.ShutDown();
	}
	LoggingStatic::options_.Clear();
	LoggingStatic::handlers_.clear();
	LoggingStatic::streamMutexes_.Clear();

	LoggingStatic::renameMethod_ = sprawl::logging::RenameMethod::Timestamp;
	LoggingStatic::renameArg_ = int(sprawl::time::Resolution::Milliseconds);

	LoggingStatic::maxFileSize_ = 500 * 1024 * 1024;

	LoggingStatic::format_ = sprawl::String("{} {} [{}] {} [{}] ({}:{}:{})") + sprawl::filesystem::LineSeparator();

	LoggingStatic::fileMap_.Clear();

	LoggingStatic::useManager_ = false;

	LoggingStatic::disabledCategories_.clear();
	LoggingStatic::extraInfoCallbacks_.Clear();
	Backtrace::ShutDown();
}

sprawl::String sprawl::logging::Message::ToString()
{
	sprawl::String timestampStr;

	if (!LoggingStatic::strftimeFormat_.empty())
	{
		int64_t seconds = sprawl::time::Convert(timestamp, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);

		char buffer[128];

		struct tm tmInfo;
		time_t asTimeT = time_t(seconds);

#ifdef _WIN32
		localtime_s(&tmInfo, &asTimeT);
#else
		localtime_r(&asTimeT, &tmInfo);
#endif

		strftime(buffer, sizeof(buffer), LoggingStatic::strftimeFormat_.c_str(), &tmInfo);

		if(LoggingStatic::maxResolution_ < sprawl::time::Resolution::Seconds)
		{
			int64_t secsAsMaxRes = sprawl::time::Convert(seconds, sprawl::time::Resolution::Seconds, LoggingStatic::maxResolution_);
			int64_t nanosecsAsMaxRes = sprawl::time::Convert(timestamp, sprawl::time::Resolution::Nanoseconds, LoggingStatic::maxResolution_);
			char buf2[128];
			snprintf(buf2, 128, "%s.%0*" SPRAWL_I64FMT "d", buffer, LoggingStatic::resolutionSize_, nanosecsAsMaxRes - secsAsMaxRes);
			timestampStr = buf2;
		}
		else
		{
			timestampStr = buffer;
		}
	}

	return LoggingStatic::format_.format(timestampStr, threadid, messageOptions->nameOverride != nullptr ? messageOptions->nameOverride : level, message, category, sprawl::path::Basename(file), function, line);
}

void sprawl::logging::BacktraceToString(Backtrace const& backtrace, sprawl::StringBuilder& builder)
{
	for(size_t i = 0; i < backtrace.Size(); ++i)
	{
		FormatStackFrame(backtrace.GetFrame(i), true, builder);
	}
}

void sprawl::logging::FormatStackFrame(Backtrace::Frame const& frame, bool printMemoryData, sprawl::StringBuilder& builder)
{
	if(printMemoryData)
	{
		builder << "        [0x";
		builder.AppendElementToBuffer(frame.address, "x");
		builder << "] " << frame.func << " (+0x";
		builder.AppendElementToBuffer(frame.offset, "x");
		builder << ")";
	}
	else
	{
		builder << "        " << frame.func;
	}

	builder << sprawl::filesystem::LineSeparator();

	builder << "        Line: " << frame.line << " (+/-1), File: " << frame.baseFile << ", in module: " << frame.module << sprawl::filesystem::LineSeparator();

	if(frame.text[0] != '\0')
	{
		builder << "            " << frame.text;
	}
	else if(sprawl::path::Exists(frame.file))
	{
		sprawl::filesystem::File f = sprawl::filesystem::Open(frame.file, "r");

		if (f.Valid())
		{
			for (int i = 1; i <= frame.line + 1; i++)
			{
				sprawl::String str = f.ReadLine();

				if (i >= frame.line - 1)
				{
					builder << "            <" << i << ">" << str;
				}
			}
		}
		else
		{
			builder << "            Code unavailable." << sprawl::filesystem::LineSeparator();
		}
	}
	else
	{
		builder << "            Code unavailable." << sprawl::filesystem::LineSeparator();
	}

	builder << "            " << sprawl::filesystem::LineSeparator();
}

void sprawl::logging::AddExtraInfoCallback(int logLevel, std::function<void*()> collect, std::function<void(void* data, sprawl::StringBuilder& builder)> print)
{
	LoggingStatic::extraInfoCallbacks_[logLevel].PushBack({collect, print});
}

sprawl::String sprawl::logging::Message::CollectExtraData()
{
	sprawl::StringBuilder builder;
	if(messageOptions->includeBacktrace)
	{
		BacktraceToString(backtrace, builder);
	}
	auto it = LoggingStatic::extraInfoCallbacks_.find(levelInt);
	if(it.Valid())
	{
		auto& callbacks = it.Value();
		for(size_t i = 0, size = extraInfo.Size(); i < size; ++i)
		{
			callbacks[i].Print(extraInfo[i], builder);
			builder << sprawl::filesystem::LineSeparator();
		}
	}
	return builder.Str();
}
