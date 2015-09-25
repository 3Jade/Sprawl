#define SPRAWL_LOG_LEVEL_TYPE LogLevel
#define SPRAWL_MINIMUM_LOG_LEVEL DEBUG

#include "../logging/Logging.hpp"
#include "../filesystem/filesystem.hpp"
#include "../filesystem/path.hpp"
#include "gtest_printers.hpp"
#include <gtest/gtest.h>

#ifdef _WIN32
	#include <Windows.h>
#endif

enum class LogLevel
{
	TRACE,
	DEBUG,
	INFO
};

TEST(LoggingTest, BasicLoggingToFileWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, ThreadedLoggingToFileWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile_Threaded("test.log"));
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, FormattingWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::SetFormat("{category} -- {level} -- {message} | {{{function} - {file}:{line}} {blah}");
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	EXPECT_EQ(
		sprawl::String("Test::Test -- INFO -- This is my message. | {") + sprawl::String(__FUNCTION__) + sprawl::String(" - ") + sprawl::path::Basename(__FILE__) + sprawl::String(":{}} {blah}{}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, PrintToStdoutWorks)
{
#ifdef _WIN32
	int oldStdoutid = _dup(1);
	FILE* newStdout = fopen("test.log", "w");
	_dup2(_fileno(newStdout), 1);
#else
	FILE* oldStdout = stdout;
	stdout = fopen("test.log", "w");
#endif

	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToStdout());
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();

#ifdef _WIN32
	fflush(stdout);
	fclose(newStdout);
	_dup2(oldStdoutid, 1);
	_close(oldStdoutid);
#else
	fclose(stdout);
	stdout = oldStdout;
#endif

	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, PrintToStderrWorks)
{
#ifdef _WIN32
	int oldStderrid = _dup(2);
	FILE* newStderr = fopen("test.log", "w");
	_dup2(_fileno(newStderr), 2);
#else
	FILE* oldStderr = stderr;
	stderr = fopen("test.log", "w");
#endif

	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToStderr());
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();

#ifdef _WIN32
	fflush(stderr);
	fclose(newStderr);
	_dup2(oldStderrid, 2);
	_close(oldStderrid);
#else
	fclose(stderr);
	stderr = oldStderr;
#endif

	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, CategoryHandlersWork)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::AddCategoryHandler(sprawl::logging::Category("Test"), sprawl::logging::PrintToFile("test2.log"));
	sprawl::logging::AddCategoryHandler(sprawl::logging::Category("Test", "Test"), sprawl::logging::PrintToFile("test3.log"), sprawl::logging::CategoryCombinationType::Exclusive);
	sprawl::logging::AddCategoryHandler(sprawl::logging::Category("Test", "Test3"), sprawl::logging::PrintToFile("test4.log"));
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	int line2 = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test2"), "This is my message.");

	int line3 = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test"), "This is my message.");

	int line4 = __LINE__;
	LOG(INFO, sprawl::logging::Category("NotTest"), "This is my message.");

	int line5 = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test3"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));
	ASSERT_TRUE(sprawl::path::Exists("test2.log"));
	ASSERT_TRUE(sprawl::path::Exists("test3.log"));
	ASSERT_TRUE(sprawl::path::Exists("test4.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (NotTest) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line4 + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	f = sprawl::filesystem::Open("test2.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test2) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line2 + 1, sprawl::filesystem::LineSeparator()),
		f.ReadLine()
	);
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::CurrentPosition);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line3 + 1, sprawl::filesystem::LineSeparator()),
		f.ReadLine()
	);
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::CurrentPosition);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test3) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line5 + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	f = sprawl::filesystem::Open("test3.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	f = sprawl::filesystem::Open("test4.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test3) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line5 + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
	EXPECT_TRUE(sprawl::filesystem::Remove("test2.log"));
	EXPECT_TRUE(sprawl::filesystem::Remove("test3.log"));
	EXPECT_TRUE(sprawl::filesystem::Remove("test4.log"));
}

TEST(LoggingTest, OptionsWork)
{
#ifdef _WIN32
	int oldStdoutid = _dup(1);
	FILE* newStdout = fopen("stdout.log", "w");
	_dup2(_fileno(newStdout), 1);
#else
	FILE* oldStdout = stdout;
	stdout = fopen("stdout.log", "w");
#endif

	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));

	sprawl::logging::Options infoOptions;
	infoOptions.logToStdout = true;
	infoOptions.nameOverride = "Information!";
	sprawl::logging::SetLevelOptions(LogLevel::INFO, infoOptions);

	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");
	LOG(DEBUG, sprawl::logging::Category("Test", "Test"), "This is also my message.");

#ifdef _WIN32
	fflush(stdout);
	fclose(newStdout);
	_dup2(oldStdoutid, 1);
	_close(oldStdoutid);
#else
	fclose(stdout);
	stdout = oldStdout;
#endif

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));
	ASSERT_TRUE(sprawl::path::Exists("stdout.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[Information!] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.ReadLine()
	);

	f.Seek(27, sprawl::filesystem::RelativeTo::CurrentPosition);
	EXPECT_EQ(
		sprawl::String("[DEBUG] (Test::Test) This is also my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 2, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	f = sprawl::filesystem::Open("stdout.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[Information!] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
	EXPECT_TRUE(sprawl::filesystem::Remove("stdout.log"));
}

TEST(LoggingTest, FileRecyclingWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("recycle.log"));
	//Win32 prints more info in __FUNCTION__ so it needs a larger buffer to get the same result
#ifdef _WIN32
	sprawl::logging::SetMaxFilesize(350);
#else
	sprawl::logging::SetMaxFilesize(250);
#endif
	sprawl::logging::Init();

	int line = __LINE__;
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");
	LOG(INFO, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("recycle.log"));
	ASSERT_TRUE(sprawl::path::Exists("recycle.log.1"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("recycle.log.1", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 1, sprawl::filesystem::LineSeparator()),
		f.ReadLine()
	);
	f.Seek(27, sprawl::filesystem::RelativeTo::CurrentPosition);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 2, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	f = sprawl::filesystem::Open("recycle.log", "r");
	//Skip timestamp.
	f.Seek(27, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(
		sprawl::String("[INFO] (Test::Test) This is my message. (") + sprawl::path::Basename(__FILE__) + sprawl::String(":") + sprawl::String(__FUNCTION__) + sprawl::String(":{}){}").format(line + 3, sprawl::filesystem::LineSeparator()),
		f.Read()
	);
	f.Close();

	EXPECT_TRUE(sprawl::filesystem::Remove("recycle.log"));
	EXPECT_TRUE(sprawl::filesystem::Remove("recycle.log.1"));
}

TEST(LoggingTest, CompileTimeLevelFilterWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::Init();

	LOG(TRACE, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	EXPECT_EQ(0, f.FileSize());
	EXPECT_EQ(sprawl::String(""), f.Read());
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, RunTimeLevelFilterWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::SetRuntimeMinimumLevel(LogLevel::INFO);
	sprawl::logging::Init();

	LOG(DEBUG, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	EXPECT_EQ(0, f.FileSize());
	EXPECT_EQ(sprawl::String(""), f.Read());
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, ExactCategoryFilterWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::DisableCategory(sprawl::logging::Category("Test", "Test"));
	sprawl::logging::Init();

	LOG(DEBUG, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	EXPECT_EQ(0, f.FileSize());
	EXPECT_EQ(sprawl::String(""), f.Read());
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}

TEST(LoggingTest, ParentCategoryFilterWorks)
{
	sprawl::logging::SetRenameMethod(sprawl::logging::RenameMethod::Counter, 5);
	sprawl::logging::SetDefaultHandler(sprawl::logging::PrintToFile("test.log"));
	sprawl::logging::DisableCategory(sprawl::logging::Category("Test"));
	sprawl::logging::Init();

	LOG(DEBUG, sprawl::logging::Category("Test", "Test"), "This is my message.");

	sprawl::logging::ShutDown();
	ASSERT_TRUE(sprawl::path::Exists("test.log"));

	sprawl::filesystem::File f = sprawl::filesystem::Open("test.log", "r");
	EXPECT_EQ(0, f.FileSize());
	EXPECT_EQ(sprawl::String(""), f.Read());
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.log"));
}
