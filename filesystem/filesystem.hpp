#pragma once

#include "../string/String.hpp"

namespace sprawl
{
	namespace path
	{
		char Separator();

		String AbsolutePath(String const& path);
		String Basename(String const& path);
		String Dirname(String const& path);

		bool Exists(String const& path);
		bool LExists(String const& path);

		String ExpandUser(String const& path);
		String ExpandVars(String const& path);

		int64_t GetAccessTime(String const& path);
		int64_t GetModificationTime(String const& path);
		int64_t GetCreationTime(String const& path);
		int64_t GetSize(String const& path);

		bool IsAbsolute(String const& path);
		bool IsFile(String const& path);
		bool IsDirectory(String const& path);
		bool IsLink(String const& path);
		bool IsMountPoint(String const& path);

		template<typename T>
		String Join(StringBuilder builder, T const& lastItem);

		template<typename T, typename... Params>
		String Join(StringBuilder builder, T const& nextItem, Params const&... params);

		template<typename T, typename... Params>
		String Join(T const& begin, Params const&... params);

		String NormCase(String const& path);
		String NormPath(String const& path);
		String RealPath(String const& path);
		String RelativePath(String const& path, String const& relativeTo = StringLiteral("./"));

		bool SameFile(String const& path1, String const& path2);

		///@todo - Split, walk, commonprefix
	}
	namespace filesystem
	{
		void Chdir(String const& path);
		String GetCwd();

		enum class RelativeTo
		{
			Beginning,
			CurrentPosition,
			End,
		};

		class File
		{
		public:
			void Close();
			void Sync();
			void Flush();
			int FileNo();
			bool IsATTY();

			String const& Read(int numBytes = -1);
			String const& ReadLine(int numBytes = -1);

			void Seek(int offset, RelativeTo relativeTo);
			int Tell();

			void Truncate(int size = -1);

			void Write(String const& str);

			bool IsClosed();
			String const& Mode();
		};

		File Open(String const& path);

		/// @todo - access(), chflags

		bool Chmod(String const& path, int mode);
		bool Mkdir(String const& path, int mode);
		bool MakeDirs(String const& path, int mode);
		bool Remove(String const& path);
		bool RemoveDirs(String const& path);
		bool Rename(String const& path, String const& newName);
		bool Renames(String const& path, String const& newName);
		bool RmDir(String const& path);
	}
}
