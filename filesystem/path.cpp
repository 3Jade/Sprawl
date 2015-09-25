#include "path.hpp"
#include "filesystem.hpp"
#ifdef _WIN32
	#include <windows.h>
	#define PATH_MAX MAX_PATH
	#define access _access
	#include <io.h>
	#define F_OK 0
	#define R_OK 2
	#define W_OK 4
	#define X_OK 0 //X_OK equivalent to F_OK on Windows
#else
	#include <limits.h>
	#include <unistd.h>
#endif

sprawl::String sprawl::path::AbsolutePath(String const& path)
{
	String cwd = filesystem::GetCwd();
	String joined = Join(cwd, path);
	return NormPath(joined);
}

sprawl::String sprawl::path::Basename(String const& path)
{
	int i = int(path.length() - 1);
	char const sep = Separator();
	char const altSep = AltSeparator();
	for( ; i >= 0; --i)
	{
		if(path[i] == sep || path[i] == altSep)
		{
			break;
		}
	}
	if(i < 0)
	{
		return path;
	}
	return String(path.c_str() + i + 1, path.length() - i - 1);
}

sprawl::String sprawl::path::Dirname(String const& path)
{
	int i = int(path.length() - 1);
	char const sep = Separator();
	char const altSep = AltSeparator();
	for( ; i >= 0; --i)
	{
		if(path[i] == sep || path[i] == altSep)
		{
			break;
		}
	}
	if(i < 0)
	{
		return "";
	}
	return String(path.c_str(), i + 1);
}


bool sprawl::path::Exists(String const& path)
{
	return access(path.GetOwned().c_str(), F_OK) == 0;
}


sprawl::path::SplitResult sprawl::path::Split(String const& path)
{
	int i = int(path.length() - 1);
	char const sep = Separator();
	char const altSep = AltSeparator();
	for( ; i >= 0; --i)
	{
		if(path[i] == sep || path[i] == altSep)
		{
			break;
		}
	}
	if(i < 0)
	{
		return { "", path };
	}
	return { String(path.c_str(), i + 1), String(path.c_str() + i + 1, path.length() - i - 1) };
}

sprawl::path::DriveResult sprawl::path::SplitDrive(String const& path)
{
	int i = int(path.length() - 1);
	char const sep = DriveSeparator();
	if(sep == '\0')
	{
		return { String(), path };
	}
	for( ; i >= 0; --i)
	{
		if(path[i] == sep)
		{
			break;
		}
	}
	if(i < 0)
	{
		return { "", path };
	}
	return { String(path.c_str(), i + 1), String(path.c_str() + i + 1, path.length() - i - 1) };
}

sprawl::path::ExtResult  sprawl::path::SplitExt(String const& path)
{
	int i = int(path.length() - 1);
	char const sep = ExtSeparator();
	for( ; i >= 0; --i)
	{
		if(path[i] == sep)
		{
			break;
		}
	}
	if(i < 0)
	{
		return { path, "" };
	}
	return { String(path.c_str(), i), String(path.c_str() + i + 1, path.length() - i - 1) };
}

sprawl::String sprawl::path::NormPath(String const& path)
{
	char out[PATH_MAX];
	char const sep = Separator();
	char const altSep = AltSeparator();
	size_t length = 0;
	for(size_t i = 0; i < path.length(); ++i)
	{
		char c = path[i];
		if (c == altSep)
		{
			c = sep;
		}
		if(c == sep)
		{
			//Collapse redundant separators
			if(i != 0)
			{
				if(path[i-1] == sep || path[i-1] == altSep)
				{
					continue;
				}
				if(path[i-1] == '.')
				{
					if(i > 2 && path[i-2] == '.' && (path[i-3] == sep || path[i-3] == altSep))
					{
						if(length < 4)
						{
							length = 1;
							continue;
						}
						length -= 4;
						if(out[length] == '.')
						{
							length += 4;
							out[length++] = c;
							continue;
						}
						while(length > 1 && out[length-1] != sep)
						{
							--length;
						}
						continue;
					}
					if(i > 1 && (path[i-2] == sep || path[i-2] == altSep))
					{
						--length;
						continue;
					}
				}
			}
		}

		out[length++] = c;
	}
	if (length > 1 && out[length - 1] == sep)
	{
		--length;
	}
	out[length] = '\0';
	return String(out, length);
}

bool sprawl::path::IsAbsolute(String const& path)
{
	if(path.empty())
	{
		return false;
	}
	DriveResult result = SplitDrive(path);
	char c = result.tail[0];
	return c == Separator() || (c != '\0' && c == AltSeparator());
}

sprawl::String sprawl::path::CommonPrefix(String const& path1, String const& path2)
{
	size_t maxLengthToIterate = path1.length() < path2.length() ? path1.length() : path2.length();
	char const* const path1ptr = path1.c_str();
	char const* const path2ptr = path2.c_str();
	for(size_t i = 0; i < maxLengthToIterate; ++i)
	{
		if(path1ptr[i] != path2ptr[i])
		{
			return sprawl::String(path1ptr, i);
		}
	}
	return sprawl::String(path1ptr, maxLengthToIterate);
}
