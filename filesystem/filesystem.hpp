#pragma once

#include "../string/String.hpp"
#include <stdio.h>
#include "../collections/Vector.hpp"

namespace sprawl
{
	namespace filesystem
	{
		char const* LineSeparator();
		char const* NullDevice();

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
			File(FILE* file, sprawl::String const& mode)
				: m_file(file)
				, m_mode(mode)
				, m_refCount(file == nullptr ? nullptr : new std::atomic<int>(1))
			{
				//
			}

			File()
				: m_file(nullptr)
				, m_mode()
				, m_refCount(nullptr)
			{
				//
			}

			File(File const& other);

			void operator=(File const& other);

			~File();

			operator bool()
			{
				return Valid();
			}

			bool operator!()
			{
				return !Valid();
			}

			bool Valid()
			{
				return m_file != nullptr;
			}

			void Close();
			void Sync();
			void Flush();
			int FileNo();
			FILE* NativeHandle() { return m_file; }
			bool IsATTY();

			String Read(int numBytes = -1);
			String ReadLine(int numBytes = -1);

			void Seek(int offset, RelativeTo relativeTo);
			int Tell();

			void Truncate(int size = -1);

			void Write(String const& str);

			bool IsClosed();
			String const& Mode();

			int FileSize();
		private:
			FILE* m_file;
			String m_mode;
			std::atomic<int>* m_refCount;
		};

		File Open(String const& path, char const* const mode);

		struct PermissionType
		{
			typedef int Type;
			enum : Type
			{
				OwnerRead = 0400,
				OwnerWrite = 0200,
				OwnerExecute = 0100,
				GroupRead = 040,
				GroupWrite = 020,
				GroupExecute = 010,
				AllRead = 04,
				AllWrite = 02,
				AllExecute = 01,
			};
		};

		bool MkDir(String const& path, PermissionType::Type mode = 0777);
		bool MakeDirs(String const& path, PermissionType::Type mode = 0777);
		bool Remove(String const& path);
		bool RemoveDirs(String const& path);
		bool Rename(String const& path, String const& newName);
		bool Renames(String const& path, String const& newName);
		bool RmDir(String const& path);
		bool RmTree(String const& path);

		struct AccessType
		{
			typedef int Type;
			enum : Type
			{
				Exists = 0,
				Read = 1 << 1,
				Write = 1 << 2,
				Execute = 1 << 3,
			};
		};

		bool Access(String const& path, AccessType::Type type);

		void ChMod(String const& path, PermissionType::Type type);

		void PutEnv(String const& name, String const& value);
		sprawl::String GetEnv(String const& name, String const& defaultValue = "");
		void UnsetEnv(String const& name);

		collections::Vector<sprawl::String> ListDir(String const& directory);

		bool MakeSymlink(String const& target, String const& link);

		int GetPid();
	}
}
