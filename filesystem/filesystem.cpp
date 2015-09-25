#include "filesystem.hpp"
#include "path.hpp"
#ifdef _WIN32
	#include <windows.h>
	#include <direct.h>
	#include <io.h>
	#define PATH_MAX MAX_PATH

	#define chdir _chdir
	#define getcwd _getcwd
	#define fileno _fileno
	#define isatty _isatty

	#define access _access
	#define F_OK 0
	#define R_OK 2
	#define W_OK 4
	#define X_OK 0 //X_OK equivalent to F_OK on Windows

	#define chmod _chmod
	#define putenv _putenv
	#define rmdir _rmdir
	#define tempnam _tempnam
	#include <process.h>
	#define getpid _getpid
#else
	#include <limits.h>
	#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

//Lot of string.GetOwned() in use here...
//If there's an external buffer being referenced and string.length() != strlen(buffer) these things won't behave as expected.
//Thus if it's referencing an external buffer, we have to get an owned version of it so we can safely add the \0 in the right place.
//We technically COULD modify the buffer directly to add that \0, but that isn't thread safe, so we won't.

void sprawl::filesystem::Chdir(String const& path)
{
	chdir(path.GetOwned().c_str());
}

sprawl::String sprawl::filesystem::GetCwd()
{
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	return sprawl::String(cwd);
}

sprawl::filesystem::File sprawl::filesystem::Open(String const& path, char const* const mode)
{
	return File( fopen( path.GetOwned().c_str(), mode ), sprawl::String(mode) );
}


sprawl::filesystem::File::File(sprawl::filesystem::File const& other)
	: m_file(other.m_file)
	, m_mode(other.m_mode)
	, m_refCount(other.m_refCount)
{
	if(m_refCount)
	{
		++(*m_refCount);
	}
}

void sprawl::filesystem::File::operator=(sprawl::filesystem::File const& other)
{
	if(m_file)
	{
		if(--(*m_refCount) == 0)
		{
			fclose(m_file);
		}
	}
	m_file = other.m_file;
	m_mode = other.m_mode;
	m_refCount = other.m_refCount;
	if(m_refCount)
	{
		++(*m_refCount);
	}
}

sprawl::filesystem::File::~File()
{
	if(m_file)
	{
		if(--(*m_refCount) == 0)
		{
			fclose(m_file);
			delete m_refCount;
		}
	}
}

void sprawl::filesystem::File::Close()
{
	*m_refCount = 0;
	fclose(m_file);
	m_file = nullptr;
}

void sprawl::filesystem::File::Sync()
{
#ifdef _WIN32
	_commit(FileNo());
#else
	fsync(FileNo());
#endif
}

void sprawl::filesystem::File::Flush()
{
	fflush(m_file);
}

int sprawl::filesystem::File::FileNo()
{
	return fileno(m_file);
}

bool sprawl::filesystem::File::IsATTY()
{
	return isatty(FileNo());
}

sprawl::String sprawl::filesystem::File::Read(int numBytes)
{
	if(numBytes == -1)
	{
		numBytes = FileSize() - Tell();
	}
	char* buffer = (char*)malloc(numBytes);
	size_t size = fread(buffer, 1, numBytes, m_file);
	sprawl::String ret(buffer, size);
	free(buffer);
	return std::move(ret);
}

sprawl::String sprawl::filesystem::File::ReadLine(int numBytes)
{
	if(numBytes == -1)
	{
		numBytes = FileSize() - Tell();
	}
	numBytes += 1;
	char* buffer = (char*)malloc(numBytes);
	fgets(buffer, numBytes, m_file);
	sprawl::String ret(buffer);
	free(buffer);
	return std::move(ret);
}

void sprawl::filesystem::File::Seek(int offset, RelativeTo relativeTo)
{
	switch(relativeTo)
	{
		case RelativeTo::Beginning:
		{
			fseek(m_file, offset, SEEK_SET);
			return;
		}
		case RelativeTo::CurrentPosition:
		{
			fseek(m_file, offset, SEEK_CUR);
			return;
		}
		case RelativeTo::End:
		{
			fseek(m_file, offset, SEEK_END);
			return;
		}
	}
}

int sprawl::filesystem::File::Tell()
{
	return int(ftell(m_file));
}

void sprawl::filesystem::File::Truncate(int size)
{
	if(size == -1)
	{
		size = Tell();
	}
	if(Tell() > size)
	{
		Seek(size, RelativeTo::Beginning);
	}
#ifdef _WIN32
	_chsize(FileNo(), size);
#else
	ftruncate(FileNo(), size);
#endif
}

void sprawl::filesystem::File::Write(String const& str)
{
	fwrite(str.c_str(), 1, str.length(), m_file);
}

bool sprawl::filesystem::File::IsClosed()
{
	return m_refCount == nullptr || m_file == nullptr || *m_refCount <= 0;
}

sprawl::String const& sprawl::filesystem::File::Mode()
{
	return m_mode;
}

int sprawl::filesystem::File::FileSize()
{
	int current = Tell();
	Seek(0, RelativeTo::End);
	int size = Tell();
	Seek(current, RelativeTo::Beginning);
	return size;
}

bool sprawl::filesystem::Access(String const& path, sprawl::filesystem::AccessType::Type type)
{
	int flags = F_OK;
	if(type & AccessType::Read)
	{
		flags |= R_OK;
	}
	if(type & AccessType::Write)
	{
		flags |= W_OK;
	}
	if(type & AccessType::Execute)
	{
		flags |= X_OK;
	}
	return access(path.GetOwned().c_str(), flags) == 0;
}

static int TranslateMode(sprawl::filesystem::PermissionType::Type type)
{
	int mode = 0;
#ifdef _WIN32
	if(type & sprawl::filesystem::PermissionType::OwnerRead)
	{
		mode |= _S_IREAD;
	}
	if(type & sprawl::filesystem::PermissionType::OwnerWrite)
	{
		mode |= _S_IWRITE;
	}
#else
	if(type & sprawl::filesystem::PermissionType::OwnerRead)
	{
		mode |= S_IRUSR;
	}
	if(type & sprawl::filesystem::PermissionType::OwnerWrite)
	{
		mode |= S_IWUSR;
	}
	if(type & sprawl::filesystem::PermissionType::OwnerExecute)
	{
		mode |= S_IXUSR;
	}

	if(type & sprawl::filesystem::PermissionType::GroupRead)
	{
		mode |= S_IRGRP;
	}
	if(type & sprawl::filesystem::PermissionType::GroupWrite)
	{
		mode |= S_IWGRP;
	}
	if(type & sprawl::filesystem::PermissionType::GroupExecute)
	{
		mode |= S_IXGRP;
	}

	if(type & sprawl::filesystem::PermissionType::AllRead)
	{
		mode |= S_IROTH;
	}
	if(type & sprawl::filesystem::PermissionType::AllWrite)
	{
		mode |= S_IWOTH;
	}
	if(type & sprawl::filesystem::PermissionType::AllExecute)
	{
		mode |= S_IXOTH;
	}
#endif
	return mode;
}

void sprawl::filesystem::ChMod(sprawl::String const& path, sprawl::filesystem::PermissionType::Type type)
{
	chmod(path.GetOwned().c_str(), TranslateMode(type));
}


void sprawl::filesystem::PutEnv(sprawl::String const& name, sprawl::String const& value)
{
#ifdef _WIN32
	sprawl::String envstr = sprawl::Format("{}={}", name, value);
	putenv(envstr.GetOwned().c_str());
#else
	if(value.empty())
	{
		unsetenv(name.GetOwned().c_str());
	}
	else
	{
		setenv(name.GetOwned().c_str(), value.GetOwned().c_str(), true);
	}
#endif
}


sprawl::String sprawl::filesystem::GetEnv(sprawl::String const& name, sprawl::String const& defaultValue)
{
	char const* result = getenv(name.GetOwned().c_str());
	if(result)
	{
		return sprawl::String(result);
	}
	return defaultValue;
}


void sprawl::filesystem::UnsetEnv(sprawl::String const& name)
{
#ifdef _WIN32
	sprawl::String envstr = sprawl::Format("{}=", name);
	putenv(envstr.GetOwned().c_str());
#else
	unsetenv(name.GetOwned().c_str());
#endif
}

bool sprawl::filesystem::MkDir(sprawl::String const& path, sprawl::filesystem::PermissionType::Type mode)
{
#ifdef _WIN32
	if(_mkdir(path.GetOwned().c_str()) == 0)
	{
		ChMod(path.GetOwned().c_str(), mode);
		return true;
	}
	return false;
#else
	return (mkdir(path.GetOwned().c_str(), TranslateMode(mode)) == 0);
#endif
}

bool sprawl::filesystem::MakeDirs(sprawl::String const& path, sprawl::filesystem::PermissionType::Type mode)
{
	char const* const start = path.c_str();
	char const* ptr = start;
	char const* const end = start + path.length();
	char const sep = path::Separator();
	char const altSep = path::AltSeparator();
	while(ptr < end)
	{
		if(*ptr == sep || *ptr == altSep)
		{
			sprawl::String str(start, ptr - start);
			if(!path::Exists(str))
			{
				if(!MkDir(str, mode))
				{
					return false;
				}
			}
			else if(!path::IsDirectory(str))
			{
				return false;
			}
		}
		++ptr;
	}

	sprawl::String str(start, ptr - start);
	if(!path::Exists(str))
	{
		if(!MkDir(str, mode))
		{
			return false;
		}
	}
	else if(!path::IsDirectory(str))
	{
		return false;
	}

	return true;
}

bool sprawl::filesystem::Remove(sprawl::String const& path)
{
#ifdef _WIN32
	//Consistent behavior between windows and posix... symlinks should always be removed with Remove() and not RmDir()
	//Though Windows wants it differently, choosing Posix because they've been doing this longer.
	if (path::IsLink(path) && (GetFileAttributesA(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0)
	{
		return (rmdir(path.GetOwned().c_str()) == 0);
	}
#endif
	return (remove(path.GetOwned().c_str()) == 0);
}

bool sprawl::filesystem::RemoveDirs(sprawl::String const& path)
{
	char const* const start = path.c_str();
	char const* end = start + path.length() - 1;
	char const sep = path::Separator();
	char const altSep = path::AltSeparator();
	bool retcode = false;

	while (*end == sep || *end == altSep)
	{
		--end;
	}

	while(end > start)
	{
		if(!RmDir(sprawl::String(start, end - start + 1)))
		{
			return retcode;
		}
		retcode = true;
		//Backtrack to last separator
		while (*end != sep && *end != altSep)
		{
			--end;
		}
		//And beyond it
		while(*end == sep || *end == altSep)
		{
			--end;
		}
	}
	return true;
}

bool sprawl::filesystem::Rename(sprawl::String const& path, String const& newName)
{
	return (rename(path.GetOwned().c_str(), newName.GetOwned().c_str()) == 0);
}

bool sprawl::filesystem::Renames(sprawl::String const& path, String const& newName)
{
	if(!MakeDirs(path::Dirname(newName), 0777))
	{
		return false;
	}
	return Rename(path, newName);
}

bool sprawl::filesystem::RmDir(sprawl::String const& path)
{
#ifdef _WIN32
	//Consistent behavior between windows and posix... symlinks should always be removed with Remove() and not RmDir()
	//Though Windows wants it differently, choosing Posix because they've been doing this longer.
	if (path::IsLink(path))
	{
		return false;
	}
#endif
	return (rmdir(path.GetOwned().c_str()) == 0);
}

bool sprawl::filesystem::RmTree(sprawl::String const& path)
{
	if(!path::IsDirectory(path))
	{
		return Remove(path);
	}

	auto dirList = ListDir(path);

	for(auto& entry : dirList)
	{
		entry = path::Join(path, entry);
		if(path::IsDirectory(entry) && !path::IsLink(entry))
		{
			if(!RmTree(entry))
			{
				return false;
			}
		}
		else
		{
			if(!Remove(entry))
			{
				return false;
			}
		}
	}

	if(!RmDir(path))
	{
		return false;
	}

	return true;
}

int sprawl::filesystem::GetPid()
{
	return getpid();
}
