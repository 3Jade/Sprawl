#include "Logging.hpp"
#include <stdio.h>

#include "../threading/threadmanager.hpp"
#include "../collections/HashMap.hpp"
#include "../collections/BinaryTree.hpp"
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
	static sprawl::collections::BasicHashMap<sprawl::String, sprawl::filesystem::File> fileMap_;

	static sprawl::logging::RenameMethod renameMethod_ = sprawl::logging::RenameMethod::Timestamp;
	static int renameArg_ = int(sprawl::time::Resolution::Milliseconds);

	static size_t maxFileSize_ = 500000;

	static sprawl::String format_ = sprawl::String("{} [{}] ({}) {} ({}:{}:{})") + sprawl::filesystem::LineSeparator();

	static void addToThreadManager_(std::shared_ptr<sprawl::logging::Message> const& message, sprawl::filesystem::File const& file, sprawl::threading::ThreadManager& manager, int64_t threadFlag, sprawl::String const& filename)
	{
		manager.AddTask(std::bind(sprawl::logging::PrintMessageToFile, message, file, filename), threadFlag);
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

	static sprawl::collections::BasicHashMap<sprawl::String, sprawl::threading::Mutex> fileMutexes_;

	static int resolutionSize_ = 6;

	static sprawl::collections::BasicHashMap<FILE*, sprawl::threading::Mutex> streamMutexes_;

	static sprawl::logging::Handler defaultHandler_ = sprawl::logging::PrintToStdout();
	static sprawl::collections::BasicBinaryTree<sprawl::String, sprawl::collections::Vector<sprawl::logging::Handler>> handlers_;

	static sprawl::logging::Options defaultOptions_;
	static sprawl::collections::BasicHashMap<int, sprawl::logging::Options> options_;

	static sprawl::collections::BinarySet<sprawl::String> disabledCategories_;

	static int minLevel_ = 0;

	template<typename T>
	static typename T::template const_iterator<0> getClosestParentCategory_(T& map, sprawl::String const& categoryStr)
	{
		auto it = map.find(categoryStr);
		if(!it.Valid())
		{
			it = map.LowerBound(categoryStr);
		}
		if(it.Valid())
		{
			sprawl::String checkStr = it.Key();
			while(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
			{
				if(categoryStr == checkStr)
				{
					return it;
					break;
				}
				else if(categoryStr.length() > checkStr.length())
				{
					if(categoryStr[checkStr.length()] == ':')
					{
						if(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
						{
							return it;
							break;
						}
					}
				}
				--it;
				if(!it.Valid())
				{
					break;
				}
				checkStr = it.Key();
			}
		}
		return map.end();
	}
}


void sprawl::logging::Log(int level, sprawl::String const& levelStr, sprawl::String const& message, sprawl::String const& file, sprawl::String const& function, int line)
{
	if(level < LoggingStatic::minLevel_)
	{
		return;
	}

	Options* options;
	if(LoggingStatic::options_.Has(level))
	{
		options = &LoggingStatic::options_.Get(level);
	}
	else
	{
		options = &LoggingStatic::defaultOptions_;
	}
	LoggingStatic::defaultHandler_(std::make_shared<Message>(sprawl::time::Now(sprawl::time::Resolution::Nanoseconds), levelStr, "", message, file, function, line, options));
}

void sprawl::logging::Log(int level, sprawl::String const& levelStr, sprawl::logging::Category const& category, sprawl::String const& message, sprawl::String const& file, sprawl::String const&  function, int line)
{
	if(level < LoggingStatic::minLevel_)
	{
		return;
	}

	Options* options;
	if(LoggingStatic::options_.Has(level))
	{
		options = &LoggingStatic::options_.Get(level);
	}
	else
	{
		options = &LoggingStatic::defaultOptions_;
	}

	sprawl::String categoryStr = category.Str();

	auto disableIt = LoggingStatic::getClosestParentCategory_(LoggingStatic::disabledCategories_, categoryStr);
	if(disableIt.Valid())
	{
		return;
	}

	auto it = LoggingStatic::getClosestParentCategory_(LoggingStatic::handlers_, categoryStr);
	if(it.Valid())
	{
		for(auto& handler : it.Value())
		{
			handler(std::make_shared<Message>(sprawl::time::Now(sprawl::time::Resolution::Nanoseconds), levelStr, categoryStr, message, file, function, line, options));
		}
	}
	else
	{
		LoggingStatic::defaultHandler_(std::make_shared<Message>(sprawl::time::Now(sprawl::time::Resolution::Nanoseconds), levelStr, categoryStr, message, file, function, line, options));
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
	LoggingStatic::disabledCategories_.Insert(categoryStr);
}

void sprawl::logging::EnableCategory(Category const& category)
{
	EnableCategory(category.Str());
}

void sprawl::logging::EnableCategory(sprawl::String const& categoryStr)
{
	LoggingStatic::disabledCategories_.Erase(categoryStr);
}

void sprawl::logging::SetFormat(sprawl::String const& format)
{
	static bool idxPopulated = false;
	if(!idxPopulated)
	{
		int i = 0;
		LoggingStatic::idx_.Insert("timestamp", i++);
		LoggingStatic::idx_.Insert("level", i++);
		LoggingStatic::idx_.Insert("category", i++);
		LoggingStatic::idx_.Insert("message", i++);
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

	if(type == CategoryCombinationType::Combined)
	{
		auto it = LoggingStatic::handlers_.LowerBound(categoryStr);
		if(it.Valid())
		{
			sprawl::String checkStr = it.Key();
			while(sprawl::String(sprawl::StringRef(categoryStr.c_str(), checkStr.length())) == checkStr)
			{
				if(categoryStr == checkStr)
				{
					for(auto& absorbedHandler : it.Value())
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
							for(auto& absorbedHandler : it.Value())
							{
								handlers.PushBack(absorbedHandler);
							}
						}
					}
				}
				--it;
				if(!it.Valid())
				{
					break;
				}
				checkStr = it.Key();
			}
		}
	}
	LoggingStatic::handlers_.Insert(categoryStr, handlers);
}

void sprawl::logging::PrintMessageToFile(std::shared_ptr<sprawl::logging::Message> const& message, sprawl::filesystem::File& file, sprawl::String const& filename)
{
	sprawl::String strMessage = message->ToString();

	{
		sprawl::threading::ScopedLock(LoggingStatic::fileMutexes_.GetOrInsert(filename));
		if(file.FileSize() + strMessage.length() > LoggingStatic::maxFileSize_)
		{
			file.Close();
			file = sprawl::filesystem::Open(LoggingStatic::modifyFilename_(filename), "w");
		}
		file.Write(strMessage);
	}

	file.Flush();

	if(message->messageOptions->logToStdout)
	{
		LoggingStatic::setColor_(message->messageOptions->color, stdout);
		fwrite(strMessage.c_str(), 1, strMessage.length(), stdout);
		LoggingStatic::resetColor_(stdout);
		fflush(stdout);
	}
	if(message->messageOptions->logToStderr)
	{
		LoggingStatic::setColor_(message->messageOptions->color, stderr);
		fwrite(strMessage.c_str(), 1, strMessage.length(), stderr);
		LoggingStatic::resetColor_(stderr);
		fflush(stderr);
	}
}

void sprawl::logging::PrintMessageToStream(std::shared_ptr<sprawl::logging::Message> const& message, FILE* stream)
{
	sprawl::String strMessage = message->ToString();
	{
		sprawl::threading::ScopedLock(LoggingStatic::streamMutexes_.GetOrInsert(stream));
		LoggingStatic::setColor_(message->messageOptions->color, stream);
		fwrite(strMessage.c_str(), 1, strMessage.length(), stream);
		LoggingStatic::resetColor_(stream);
		fflush(stream);
	}
}

sprawl::logging::Handler sprawl::logging::PrintToFile(sprawl::String const& filename)
{
	sprawl::filesystem::File file;
	if(!LoggingStatic::fileMap_.Has(filename))
	{
		sprawl::String modifiedFilename = LoggingStatic::modifyFilename_(filename);
		file = sprawl::filesystem::Open(modifiedFilename, "w");
		LoggingStatic::fileMap_.Insert(filename, file);
	}
	return std::bind(PrintMessageToFile, std::placeholders::_1, std::ref(LoggingStatic::fileMap_.Get(filename)), filename);
}

sprawl::logging::Handler sprawl::logging::PrintToFile_Threaded(sprawl::String const& filename)
{
	static bool managerInitialized_ = false;
	if(!managerInitialized_)
	{
		LoggingStatic::logManager_.AddThread(LoggingStatic::LOG_THREAD);
		managerInitialized_ = true;
	}
	LoggingStatic::useManager_ = true;
	return PrintToFile_ThreadManager(filename, LoggingStatic::logManager_, LoggingStatic::LOG_THREAD);
}

sprawl::logging::Handler sprawl::logging::PrintToFile_ThreadManager(sprawl::String const& filename, sprawl::threading::ThreadManager& manager, int64_t threadFlag)
{
	sprawl::filesystem::File file;
	if(!LoggingStatic::fileMap_.Has(filename))
	{
		sprawl::String modifiedFilename = LoggingStatic::modifyFilename_(filename);
		file = sprawl::filesystem::Open(modifiedFilename, "w");
		LoggingStatic::fileMap_.Insert(filename, file);
	}
	return std::bind(LoggingStatic::addToThreadManager_, std::placeholders::_1, std::ref(LoggingStatic::fileMap_.Get(filename)), std::ref(manager), threadFlag, filename);
}

sprawl::logging::Handler sprawl::logging::PrintToStdout()
{
	return std::bind(PrintMessageToStream, std::placeholders::_1, stdout);
}

sprawl::logging::Handler sprawl::logging::PrintToStderr()
{
	return std::bind(PrintMessageToStream, std::placeholders::_1, stderr);
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
	LoggingStatic::handlers_.Clear();
	LoggingStatic::streamMutexes_.Clear();
	LoggingStatic::fileMutexes_.Clear();

	LoggingStatic::renameMethod_ = sprawl::logging::RenameMethod::Timestamp;
	LoggingStatic::renameArg_ = int(sprawl::time::Resolution::Milliseconds);

	LoggingStatic::maxFileSize_ = 500000;

	LoggingStatic::format_ = sprawl::String("{} [{}] ({}) {} ({}:{}:{})") + sprawl::filesystem::LineSeparator();

	LoggingStatic::fileMap_.Clear();

	LoggingStatic::useManager_ = false;
}

sprawl::String sprawl::logging::Message::ToString()
{
	int64_t seconds = sprawl::time::Convert(timestamp, sprawl::time::Resolution::Nanoseconds, sprawl::time::Resolution::Seconds);

	sprawl::String timestampStr;

	if (!LoggingStatic::strftimeFormat_.empty())
	{
		char buffer[128];

		struct tm* tmInfo;
		time_t asTimeT = time_t(seconds);

		tmInfo = localtime(&asTimeT);

		strftime(buffer, sizeof(buffer), LoggingStatic::strftimeFormat_.c_str(), tmInfo);

		if(LoggingStatic::maxResolution_ < sprawl::time::Resolution::Seconds)
		{
			int64_t secsAsMaxRes = sprawl::time::Convert(seconds, sprawl::time::Resolution::Seconds, LoggingStatic::maxResolution_);
			int64_t nanosecsAsMaxRes = sprawl::time::Convert(timestamp, sprawl::time::Resolution::Nanoseconds, LoggingStatic::maxResolution_);
			sprawl::String formatStr = sprawl::Format("{{}.{{:0{}}", LoggingStatic::resolutionSize_);
			timestampStr = formatStr.format(buffer, nanosecsAsMaxRes - secsAsMaxRes);
		}
		else
		{
			timestampStr = buffer;
		}
	}

	return LoggingStatic::format_.format(timestampStr, messageOptions->nameOverride != nullptr ? messageOptions->nameOverride : level, category, message, sprawl::path::Basename(file), function, line);
}
