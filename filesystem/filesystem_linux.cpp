#include "filesystem.hpp"
#include <dirent.h>
#include <unistd.h>

char const* sprawl::filesystem::LineSeparator()
{
	return "\n";
}

char const* sprawl::filesystem::NullDevice()
{
	return "/dev/null";
}

sprawl::collections::Vector<sprawl::String> sprawl::filesystem::ListDir(sprawl::String const& directory)
{
	sprawl::collections::Vector<sprawl::String> out;
	struct dirent const* entry;
	DIR* dir;
	dir = opendir(directory.GetOwned().c_str());
	if(dir != nullptr)
	{
		entry = readdir(dir);
		while(entry != nullptr)
		{
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
			{
				out.PushBack(entry->d_name);
			}
			entry = readdir(dir);
		}
		closedir(dir);
	}
	return std::move(out);
}

bool sprawl::filesystem::MakeSymlink(String const& target, String const& link)
{
	return (symlink(target.GetOwned().c_str(), link.GetOwned().c_str()) == 0);
}
