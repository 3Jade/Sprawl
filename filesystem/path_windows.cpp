
#include "path.hpp"
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>

char sprawl::path::Separator()
{
	return '\\';
}

char sprawl::path::AltSeparator()
{
	return '/';
}

char sprawl::path::ExtSeparator()
{
	return '.';
}

char sprawl::path::PathSeparator()
{
	return ';';
}

char sprawl::path::DriveSeparator()
{
	return ':';
}

bool sprawl::path::LinkExists(String const& path)
{
	return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

sprawl::String sprawl::path::ExpandPath(String const& path)
{
	char out[MAX_PATH];
	ExpandEnvironmentStringsA(path.c_str(), out, MAX_PATH);
	return sprawl::String(out);
}

sprawl::String sprawl::path::RealPath(String const& path)
{
	HANDLE fileHandle = CreateFileA(
		path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		return "";
	}
	char buf[MAX_PATH];
	GetFinalPathNameByHandleA(fileHandle, buf, MAX_PATH, FILE_NAME_NORMALIZED);
	CloseHandle(fileHandle);
	return sprawl::String(buf + 4);
}

int64_t sprawl::path::GetAccessTime(String const& path)
{
	HANDLE fileHandle = CreateFileA(
		path.GetOwned().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
		);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	FILETIME out;
	GetFileTime(fileHandle, nullptr, &out, nullptr);
	CloseHandle(fileHandle);

	uint64_t tt = out.dwHighDateTime;
	tt <<= 32;
	tt |= out.dwLowDateTime;
	tt -= 116444736000000000ULL;
	tt *= 100;
	return int64_t(tt);
}

int64_t sprawl::path::GetModificationTime(String const& path)
{
	HANDLE fileHandle = CreateFileA(
		path.GetOwned().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
		);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	FILETIME out;
	GetFileTime(fileHandle, nullptr, nullptr, &out);
	CloseHandle(fileHandle);

	uint64_t tt = out.dwHighDateTime;
	tt <<= 32;
	tt |= out.dwLowDateTime;
	tt -= 116444736000000000ULL;
	tt *= 100;
	return int64_t(tt);
}

int64_t sprawl::path::GetSize(String const& path)
{
	struct _stat attrib;
	_stat(path.GetOwned().c_str(), &attrib);
	return attrib.st_size;
}

bool sprawl::path::IsFile(String const& path)
{
	return !IsDirectory(path);
}

bool sprawl::path::IsDirectory(String const& path)
{
	return (GetFileAttributesA(path.GetOwned().c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool sprawl::path::IsLink(String const& path)
{
	return (GetFileAttributesA(path.GetOwned().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool sprawl::path::IsMountPoint(String const& path)
{
	if((GetFileAttributesA(path.GetOwned().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
	{
		return false;
	}

	struct _stat attrib;
	if(_stat(path.GetOwned().c_str(), &attrib) != 0)
	{
		return false;
	}

	struct _stat attrib2;
	if(_stat(Join(path, "..").c_str(), &attrib2) != 0)
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
	struct _stat attrib;
	_stat(path1.GetOwned().c_str(), &attrib);

	struct _stat attrib2;
	_stat(path2.GetOwned().c_str(), &attrib2);

	return attrib.st_ino == attrib2.st_ino && attrib.st_dev == attrib2.st_dev;
}
