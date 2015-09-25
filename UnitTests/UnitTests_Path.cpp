#include "../filesystem/path.hpp"
#include <gtest/gtest.h>

TEST(PathTest, NormPathWorks)
{
	sprawl::String expectedResult1 = sprawl::Format("..{0}dir1", sprawl::path::Separator());
	sprawl::String expectedResult2 = "..";
	sprawl::String expectedResult3 =  sprawl::Format("..{0}..", sprawl::path::Separator());
	sprawl::String expectedResult4 =  sprawl::Format("..{0}..{0}..", sprawl::path::Separator());
	sprawl::String expectedResult5 =  sprawl::Format("..{0}..", sprawl::path::Separator());
	sprawl::String expectedResult6 =  sprawl::Format("..{0}dir1", sprawl::path::Separator());
	sprawl::String expectedResult7 = "..";
	sprawl::String expectedResult8 = sprawl::Format("..{0}dir1{0}dir2{0}dir3", sprawl::path::Separator());
	sprawl::String expectedResult9 = sprawl::Format("{}", sprawl::path::Separator());

	sprawl::String result1 = sprawl::path::NormPath("../dir1");
	sprawl::String result2 = sprawl::path::NormPath("../dir1/../");
	sprawl::String result3 = sprawl::path::NormPath("../dir1/../../");
	sprawl::String result4 = sprawl::path::NormPath("../../dir1/../../");
	sprawl::String result5 = sprawl::path::NormPath("../../dir1/../");
	sprawl::String result6 = sprawl::path::NormPath("../dir1/dir2/../");
	sprawl::String result7 = sprawl::path::NormPath("../dir1/dir2/../../");
	sprawl::String result8 = sprawl::path::NormPath("../dir1////dir2/./././dir3");
	sprawl::String result9 = sprawl::path::NormPath("/../../");

	EXPECT_EQ(expectedResult1, result1);
	EXPECT_EQ(expectedResult2, result2);
	EXPECT_EQ(expectedResult3, result3);
	EXPECT_EQ(expectedResult4, result4);
	EXPECT_EQ(expectedResult5, result5);
	EXPECT_EQ(expectedResult6, result6);
	EXPECT_EQ(expectedResult7, result7);
	EXPECT_EQ(expectedResult8, result8);
	EXPECT_EQ(expectedResult9, result9);
}

TEST(PathTest, JoinWorks)
{
	sprawl::String expectedResult1 = sprawl::Format("..{0}dir1", sprawl::path::Separator());
	sprawl::String expectedResult2 = "..";
	sprawl::String expectedResult3 =  sprawl::Format("..{0}..", sprawl::path::Separator());
	sprawl::String expectedResult4 =  sprawl::Format("..{0}..{0}..", sprawl::path::Separator());
	sprawl::String expectedResult5 =  sprawl::Format("..{0}dir1{0}dir2", sprawl::path::Separator());
	sprawl::String expectedResult6 = sprawl::Format("..{0}dir1{0}dir2{0}dir3", sprawl::path::Separator());

	sprawl::String result1 = sprawl::path::Join("..", "dir1");
	sprawl::String result2 = sprawl::path::Join("..");
	sprawl::String result3 = sprawl::path::Join("..", "..");
	sprawl::String result4 = sprawl::path::Join("..", "..", "..");
	sprawl::String result5 = sprawl::path::Join("..", "dir1", "dir2");
	sprawl::String result6 = sprawl::path::Join("..", "dir1", "dir2", "dir3");

	EXPECT_EQ(expectedResult1, result1);
	EXPECT_EQ(expectedResult2, result2);
	EXPECT_EQ(expectedResult3, result3);
	EXPECT_EQ(expectedResult4, result4);
	EXPECT_EQ(expectedResult5, result5);
	EXPECT_EQ(expectedResult6, result6);
}

TEST(PathTest, BaseNameWorks)
{
	sprawl::String baseName = sprawl::path::Basename("/some/path/to/some/file");
	EXPECT_EQ(sprawl::String("file"), baseName);
	sprawl::String baseName2 = sprawl::path::Basename("/some/path/to/some/dir/");
	EXPECT_EQ(sprawl::String(""), baseName2);
}

TEST(PathTest, DirNameWorks)
{
	sprawl::String dirName = sprawl::path::Dirname("/some/path/to/some/file");
	EXPECT_EQ(sprawl::String("/some/path/to/some/"), dirName);
	sprawl::String dirName2 = sprawl::path::Dirname("/some/path/to/some/dir/");
	EXPECT_EQ(sprawl::String("/some/path/to/some/dir/"), dirName2);
}


TEST(PathTest, SplitWorks)
{
	sprawl::path::SplitResult result = sprawl::path::Split("/some/path/to/some/file");
	EXPECT_EQ(sprawl::String("/some/path/to/some/"), result.dirname);
	EXPECT_EQ(sprawl::String("file"), result.basename);
	sprawl::path::SplitResult result2 = sprawl::path::Split("/some/path/to/some/dir/");
	EXPECT_EQ(sprawl::String("/some/path/to/some/dir/"), result2.dirname);
	EXPECT_EQ(sprawl::String(""), result2.basename);
}

TEST(PathTest, SplitExtWorks)
{
	sprawl::path::ExtResult result = sprawl::path::SplitExt("/some/path/to/some/file.ext");
	EXPECT_EQ(sprawl::String("/some/path/to/some/file"), result.path);
	EXPECT_EQ(sprawl::String("ext"), result.extension);
	sprawl::path::ExtResult result2 = sprawl::path::SplitExt("/some/path/to/some/file");
	EXPECT_EQ(sprawl::String("/some/path/to/some/file"), result2.path);
	EXPECT_EQ(sprawl::String(""), result2.extension);
}

TEST(PathTest, CommonPrefixWorks)
{
	sprawl::String str1 = "/dir1/dir2/dir3/file";
	sprawl::String str2 = "/dir1/dir2/dir3/otherfile";
	sprawl::String str3 = "/dir1/dir2/dir3";
	sprawl::String str4 = "/dir1/dir2/dir3/file";
	sprawl::String str5 = "/dir2/dir2/dir4";
	sprawl::String str6 = "/otherdir/otherdir2";
	sprawl::String str7 = "../somedir";

	EXPECT_EQ(sprawl::String("/dir1/dir2/dir3/"), sprawl::path::CommonPrefix(str1, str2));
	EXPECT_EQ(sprawl::String("/dir1/dir2/dir3"), sprawl::path::CommonPrefix(str1, str3));
	EXPECT_EQ(sprawl::String("/dir1/dir2/dir3/file"), sprawl::path::CommonPrefix(str1, str4));
	EXPECT_EQ(sprawl::String("/dir"), sprawl::path::CommonPrefix(str1, str5));
	EXPECT_EQ(sprawl::String("/"), sprawl::path::CommonPrefix(str1, str6));
	EXPECT_EQ(sprawl::String(""), sprawl::path::CommonPrefix(str1, str7));
}
