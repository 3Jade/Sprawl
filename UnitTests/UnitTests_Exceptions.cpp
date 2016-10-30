#include <gtest/gtest.h>
#include "../../common/compat.hpp"
#include "../../common/errors.hpp"
#include <type_traits>

#if SPRAWL_EXCEPTIONS_ENABLED
using sprawl::Exception;

#define TEST_EXCEPTION(ExceptionClass, BaseClass, id, name, type) \
	TEST(ExceptionTest, Test_##id) \
	{ \
		try \
		{ \
			throw ExceptionClass(); \
		} \
		catch(BaseClass const& e) \
		{ \
			ASSERT_EQ(sprawl::ExceptionId::id, e.GetId()); \
			ASSERT_EQ(0, strcmp(e.what(), name)); \
			ASSERT_EQ(e.what(), sprawl::ExceptionString(sprawl::ExceptionId::id)); \
		} \
	}

#define SPRAWL_EXCEPTION(baseId, id, name, type) TEST_EXCEPTION(Exception<sprawl::ExceptionId::id>, Exception<sprawl::ExceptionId::baseId>, id, name, type)
#include "../../common/exceptions.hpp"
#undef SPRAWL_EXCEPTION

#else

bool copied = false;
bool moved = false;

TEST(ErrorStateTest, ErrorStateReferenceDoesNotCopy)
{
	struct CopyTest
	{
		CopyTest() {}
		CopyTest(const CopyTest&) { copied = true; }
		CopyTest(CopyTest&&) { moved = true; }

		std::string data_member;
	};

	CopyTest c;

	sprawl::ErrorState<CopyTest&> copy = c;

	ASSERT_FALSE(copied);
	ASSERT_FALSE(moved);
}

#if SPRAWL_DEBUG

TEST(ErrorStateDeathTest, ErrorStateTerminatesOnDestructWithUncheckedError)
{
	EXPECT_DEATH(
		{
			sprawl::ErrorState<void> c = sprawl::GeneralException();
		},
		"Unknown or unspecified error"
	);
	EXPECT_DEATH(
		{
			sprawl::ErrorState<bool> c = sprawl::GeneralException();
		},
		"Unknown or unspecified error"
	);
	EXPECT_DEATH(
		{
			sprawl::ErrorState<bool&> c = sprawl::GeneralException();
		},
		"Unknown or unspecified error"
	);
}
TEST(ErrorStateDeathTest, ErrorStateDoesNotTerminateOnDestructWithCheckedError)
{
	EXPECT_EXIT(
		{
			sprawl::ErrorState<void> c = sprawl::GeneralException();
			ASSERT_TRUE(c.ErrorCode() == sprawl::ExceptionId::GENERAL_EXCEPTION);
			fprintf(stderr, "I'm alive!");
			exit(0);
		},
		testing::ExitedWithCode(0),
		"I'm alive!"
	);
	EXPECT_EXIT(
		{
			sprawl::ErrorState<bool> c = sprawl::GeneralException();
			ASSERT_TRUE(c.ErrorCode() == sprawl::ExceptionId::GENERAL_EXCEPTION);
			fprintf(stderr, "I'm alive!");
			exit(0);
		},
		testing::ExitedWithCode(0),
		"I'm alive!"
	);
	EXPECT_EXIT(
		{
			sprawl::ErrorState<bool&> c = sprawl::GeneralException();
			ASSERT_TRUE(c.ErrorCode() == sprawl::ExceptionId::GENERAL_EXCEPTION);
			fprintf(stderr, "I'm alive!");
			exit(0);
		},
		testing::ExitedWithCode(0),
		"I'm alive!"
	);
}
TEST(ErrorStateDeathTest, ErrorStateTerminatesOnGetWithError)
{
	EXPECT_DEATH(
		{
			sprawl::ErrorState<bool> c = sprawl::GeneralException();
			printf("%d\n", c.Get());
			ASSERT_TRUE(c.ErrorCode() == sprawl::ExceptionId::GENERAL_EXCEPTION);
		},
		"Unknown or unspecified error"
	);
	EXPECT_DEATH(
		{
			sprawl::ErrorState<bool&> c = sprawl::GeneralException();
			printf("%d\n", c.Get());
			ASSERT_TRUE(c.ErrorCode() == sprawl::ExceptionId::GENERAL_EXCEPTION);
		},
		"Unknown or unspecified error"
	);
}

#endif
#endif
