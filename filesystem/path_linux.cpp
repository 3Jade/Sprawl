#include "path.hpp"
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <wordexp.h>
#include "../time/time.hpp"

char sprawl::path::Separator()
{
	return '/';
}

char sprawl::path::AltSeparator()
{
	return '\0';
}

char sprawl::path::ExtSeparator()
{
	return '.';
}

char sprawl::path::PathSeparator()
{
	return ':';
}

char sprawl::path::DriveSeparator()
{
	return '\0';
}

bool sprawl::path::LinkExists(String const& path)
{
	struct stat ignored;
	return lstat(path.GetOwned().c_str(), &ignored) == 0;
}

sprawl::String sprawl::path::ExpandPath(String const& path)
{
	wordexp_t exp_result;
	wordexp(path.GetOwned().c_str(), &exp_result, WRDE_NOCMD);
	sprawl::String ret(exp_result.we_wordv[0]);
	wordfree(&exp_result);
	return ret;
}

sprawl::String sprawl::path::RealPath(String const& path)
{
	char abspath[PATH_MAX];
	realpath(path.GetOwned().c_str(), abspath);
	return String(abspath);
}

int64_t sprawl::path::GetAccessTime(String const& path)
{
	struct stat attrib;
	stat(path.GetOwned().c_str(), &attrib);

#ifdef __APPLE__
	return sprawl::time::Convert(attrib.st_atime, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);
#else
	int64_t nanosecondResult = attrib.st_atim.tv_sec;
	nanosecondResult *= 1000000000;
	nanosecondResult += attrib.st_atim.tv_nsec;
	return nanosecondResult;
#endif
}

int64_t sprawl::path::GetModificationTime(String const& path)
{
	struct stat attrib;
	stat(path.GetOwned().c_str(), &attrib);

#ifdef __APPLE__
	return sprawl::time::Convert(attrib.st_mtime, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds);
#else
	int64_t nanosecondResult = attrib.st_mtim.tv_sec;
	nanosecondResult *= 1000000000;
	nanosecondResult += attrib.st_mtim.tv_nsec;
	return nanosecondResult;
#endif
}

int64_t sprawl::path::GetSize(String const& path)
{
	struct stat attrib;
	stat(path.GetOwned().c_str(), &attrib);
	return attrib.st_size;
}

bool sprawl::path::IsFile(String const& path)
{
	struct stat attrib;
	stat(path.GetOwned().c_str(), &attrib);
	return S_ISREG(attrib.st_mode);
}

bool sprawl::path::IsDirectory(String const& path)
{
	struct stat attrib;
	stat(path.GetOwned().c_str(), &attrib);
	return S_ISDIR(attrib.st_mode);
}

bool sprawl::path::IsLink(String const& path)
{
	struct stat attrib;
	lstat(path.GetOwned().c_str(), &attrib);
	return S_ISLNK(attrib.st_mode);
}

bool sprawl::path::IsMountPoint(String const& path)
{
	struct stat attrib;
	if(lstat(path.GetOwned().c_str(), &attrib) != 0)
	{
		return false;
	}

	if(S_ISLNK(attrib.st_mode))
	{
		return false;
	}

	struct stat attrib2;
	if(lstat(Join(path, "..").c_str(), &attrib2) != 0)
	{
		return false;
	}

	if(attrib.st_dev != attrib2.st_dev)
	{
		return true;
	}

	if(attrib.st_ino == attrib2.st_ino)
	{
		return true;
	}

	return false;
}

bool sprawl::path::SameFile(String const& path1, String const& path2)
{
	struct stat attrib;
	stat(path1.GetOwned().c_str(), &attrib);

	struct stat attrib2;
	stat(path2.GetOwned().c_str(), &attrib2);

	return attrib.st_ino == attrib2.st_ino && attrib.st_dev == attrib2.st_dev;
}
