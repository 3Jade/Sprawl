#include "../filesystem/filesystem.hpp"
#include "../filesystem/path.hpp"
#include <gtest/gtest.h>

TEST(FilesystemTest, FilesWork)
{
	sprawl::filesystem::File f = sprawl::filesystem::Open("./test.txt", "w");
	f.Write("Hi there!");
	f.Flush();
	f.Sync();
	EXPECT_EQ(9, f.Tell());
	EXPECT_EQ(9, f.FileSize());
	EXPECT_FALSE(f.IsClosed());

	f.Seek(5, sprawl::filesystem::RelativeTo::Beginning);
	EXPECT_EQ(5, f.Tell());
	EXPECT_EQ(9, f.FileSize());
	f.Seek(2, sprawl::filesystem::RelativeTo::CurrentPosition);
	EXPECT_EQ(7, f.Tell());
	EXPECT_EQ(9, f.FileSize());
	f.Seek(-3, sprawl::filesystem::RelativeTo::End);
	EXPECT_EQ(6, f.Tell());
	EXPECT_EQ(9, f.FileSize());

	EXPECT_EQ(sprawl::String("w"), f.Mode());

	f.Close();

	EXPECT_TRUE(f.IsClosed());

	f = sprawl::filesystem::Open("./test.txt", "r+");

	EXPECT_FALSE(f.IsClosed());
	EXPECT_FALSE(f.IsATTY());
	EXPECT_EQ(9, f.FileSize());

	EXPECT_EQ(sprawl::String("r+"), f.Mode());

	sprawl::String str = f.Read(2);
	EXPECT_EQ(sprawl::String("Hi"), str);

	f.Seek(0, sprawl::filesystem::RelativeTo::Beginning);

	str = f.Read();
	EXPECT_EQ(sprawl::String("Hi there!"), str);

	f.Truncate(2);
	EXPECT_EQ(2, f.Tell());
	EXPECT_EQ(2, f.FileSize());
	f.Seek(0, sprawl::filesystem::RelativeTo::Beginning);

	str = f.Read();
	EXPECT_EQ(sprawl::String("Hi"), str);

	f.Close();

	f = sprawl::filesystem::Open("./test.txt", "r+");
	EXPECT_EQ(2, f.FileSize());

	str = f.Read();
	EXPECT_EQ(sprawl::String("Hi"), str);

	f.Close();

	f = sprawl::filesystem::Open("./test.txt", "w");
	f.Write("Hello");
	f.Write(sprawl::filesystem::LineSeparator());
	f.Write("World");
	f.Close();

	f = sprawl::filesystem::Open("./test.txt", "r");
	sprawl::String hello = f.ReadLine();
	sprawl::String world = f.ReadLine();
	EXPECT_EQ(sprawl::String("Hello{}").format(sprawl::filesystem::LineSeparator()), hello);
	EXPECT_EQ(sprawl::String("World"), world);
	f.Close();
	EXPECT_TRUE(sprawl::filesystem::Remove("test.txt")) << strerror(errno);
}

TEST(FilesystemTest, EnvironmentWorks)
{
	sprawl::filesystem::PutEnv("SprawlUnitTesting", "true");
	sprawl::String str1 = sprawl::filesystem::GetEnv("SprawlUnitTesting", "false");
	EXPECT_EQ(sprawl::String("true"), str1);
	sprawl::filesystem::UnsetEnv("SprawlUnitTesting");
	sprawl::String str2 = sprawl::filesystem::GetEnv("SprawlUnitTesting", "false");
	EXPECT_EQ(sprawl::String("false"), str2);
	sprawl::filesystem::PutEnv("SprawlUnitTesting", "");
	sprawl::String str3 = sprawl::filesystem::GetEnv("SprawlUnitTesting", "false");
	EXPECT_EQ(sprawl::String("false"), str3);
}

TEST(FilesystemTest, MkDirAndRmDirWork)
{
	sprawl::String path1 = "testdir";
	ASSERT_TRUE(sprawl::filesystem::MkDir(path1)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::Exists(path1)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::IsDirectory(path1)) << strerror(errno);

	ASSERT_TRUE(sprawl::filesystem::RmDir(path1)) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists(path1));
}

TEST(FilesystemTest, MakeDirsAndRmTreeWork)
{
	sprawl::String path2 = "testdir2/nested/directory/structure";
	ASSERT_TRUE(sprawl::filesystem::MakeDirs(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::Exists(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::IsDirectory(path2)) << strerror(errno);

	EXPECT_FALSE(sprawl::filesystem::RmDir("testdir2"));
	ASSERT_TRUE(sprawl::path::Exists(path2)) << strerror(errno);

	ASSERT_TRUE(sprawl::filesystem::RmTree("testdir2")) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists(path2));
	EXPECT_FALSE(sprawl::path::Exists("testdir"));
	ASSERT_TRUE(sprawl::path::Exists(".")) << strerror(errno);
}

TEST(FilesystemTest, RemoveWorks)
{
	sprawl::filesystem::File f = sprawl::filesystem::Open("test.txt", "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::path::Exists("test.txt")) << strerror(errno);
	ASSERT_TRUE(sprawl::filesystem::Remove("test.txt")) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists("test.txt"));
}

TEST(FilesystemTest, RemoveDirsWorks)
{
	sprawl::String path2 = "testdir2/nested/directory/structure";
	ASSERT_TRUE(sprawl::filesystem::MakeDirs(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::Exists(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::IsDirectory(path2)) << strerror(errno);

	ASSERT_TRUE(sprawl::filesystem::RemoveDirs(path2)) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists(path2));
	EXPECT_FALSE(sprawl::path::Exists("testdir2"));
	ASSERT_TRUE(sprawl::path::Exists(".")) << strerror(errno);
}

TEST(FilesystemTest, RmTreeWorksWithFilesInTree)
{
	sprawl::String path2 = "testdir2/nested/directory/structure";
	ASSERT_TRUE(sprawl::filesystem::MakeDirs(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::Exists(path2)) << strerror(errno);
	ASSERT_TRUE(sprawl::path::IsDirectory(path2)) << strerror(errno);

	sprawl::filesystem::File f = sprawl::filesystem::Open(sprawl::path::Join(path2, "test.txt"), "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::filesystem::RmTree("testdir2")) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists(path2));
	EXPECT_FALSE(sprawl::path::Exists("testdir"));
	ASSERT_TRUE(sprawl::path::Exists(".")) << strerror(errno);
}

TEST(FilesystemTest, RenameWorks)
{
	sprawl::filesystem::File f = sprawl::filesystem::Open("test.txt", "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::path::Exists("test.txt")) << strerror(errno);
	ASSERT_TRUE(sprawl::filesystem::Rename("test.txt", "test2.txt")) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists("test.txt"));
	ASSERT_TRUE(sprawl::path::Exists("test2.txt")) << strerror(errno);
	ASSERT_TRUE(sprawl::filesystem::Remove("test2.txt")) << strerror(errno);
}

TEST(FilesystemTest, RenamesWorks)
{
	sprawl::String path2 = "testdir2/nested/directory/structure";
	sprawl::filesystem::File f = sprawl::filesystem::Open("test.txt", "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::path::Exists("test.txt")) << strerror(errno);
	sprawl::String newName = sprawl::path::Join(path2, "test2.txt");
	ASSERT_TRUE(sprawl::filesystem::Renames("test.txt", newName)) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists("test.txt"));
	ASSERT_TRUE(sprawl::path::Exists(newName)) << strerror(errno);
	ASSERT_TRUE(sprawl::filesystem::RmTree("testdir2")) << strerror(errno);
}

TEST(FilesystemTest, FileSymlinkWorks)
{
	sprawl::filesystem::File f = sprawl::filesystem::Open("test.txt", "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::path::Exists("test.txt"));
	sprawl::filesystem::MakeSymlink("test.txt", "test2.txt");

	ASSERT_TRUE(sprawl::path::Exists("test2.txt"));
	EXPECT_TRUE(sprawl::path::IsLink("test2.txt"));
	EXPECT_TRUE(sprawl::path::IsFile("test2.txt"));

	f = sprawl::filesystem::Open("test2.txt", "r");
	EXPECT_EQ(sprawl::String("Hello, world!"), f.Read());
	f.Close();

	ASSERT_TRUE(sprawl::filesystem::Remove("test.txt")) << strerror(errno);
	ASSERT_TRUE(sprawl::filesystem::Remove("test2.txt")) << strerror(errno);
}

TEST(FilesystemTest, DirectorySymlinkWorks)
{
	sprawl::filesystem::MkDir("test");

	sprawl::String file1 = "test/test.txt";
	sprawl::String file2 = "test2/test.txt";

	sprawl::filesystem::File f = sprawl::filesystem::Open(file1, "w");
	f.Write("Hello, world!");
	f.Close();

	ASSERT_TRUE(sprawl::path::Exists(file1));
	sprawl::filesystem::MakeSymlink("test", "test2");

	ASSERT_TRUE(sprawl::path::Exists(file2));
	EXPECT_FALSE(sprawl::path::IsLink(file2));
	EXPECT_TRUE(sprawl::path::IsFile(file2));

	EXPECT_TRUE(sprawl::path::IsLink("test2"));
	EXPECT_TRUE(sprawl::path::IsDirectory("test2"));

	f = sprawl::filesystem::Open(file2, "r");
	EXPECT_EQ(sprawl::String("Hello, world!"), f.Read());
	f.Close();

	EXPECT_TRUE(sprawl::filesystem::Remove(file2)) << strerror(errno);
	EXPECT_FALSE(sprawl::path::Exists(file1));
	EXPECT_FALSE(sprawl::filesystem::RmDir("test2"));
	EXPECT_TRUE(sprawl::filesystem::Remove("test2"));
	EXPECT_FALSE(sprawl::path::Exists("test2"));
	EXPECT_TRUE(sprawl::path::Exists("test"));
	EXPECT_TRUE(sprawl::filesystem::RmDir("test"));
}
