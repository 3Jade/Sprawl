#include "filesystem.hpp"
#include "path.hpp"
#include <Windows.h>

char const* sprawl::filesystem::LineSeparator()
{
	return "\r\n";
}

char const* sprawl::filesystem::NullDevice()
{
	return "NUL";
}

sprawl::collections::Vector<sprawl::String> sprawl::filesystem::ListDir(sprawl::String const& directory)
{
	collections::Vector<String> out;

	WIN32_FIND_DATA ffd;
	HANDLE handle = INVALID_HANDLE_VALUE;

	sprawl::String dirLocal;
	char c = directory[directory.length() - 1];
	if (c != path::Separator() && c != path::AltSeparator())
	{
		dirLocal = Format("{}{}*", directory, path::Separator());
	}
	else
	{
		dirLocal = Format("{}*", directory);
	}
	handle = FindFirstFileA(dirLocal.c_str(), &ffd);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return std::move(out);
	}

	do
	{
		if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
		{
			out.PushBack(ffd.cFileName);
		}
	} while (FindNextFile(handle, &ffd) != 0);

	FindClose(handle);

	return std::move(out);
}

bool sprawl::filesystem::MakeSymlink(String const& target, String const& link)
{
	return CreateSymbolicLinkA(link.GetOwned().c_str(), target.GetOwned().c_str(), path::IsDirectory(target) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
}