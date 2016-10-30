#pragma once

#include "../string/String.hpp"

namespace sprawl
{
	namespace path
	{
		char Separator();
		char AltSeparator();
		char ExtSeparator();
		char PathSeparator();
		char DriveSeparator();

		String AbsolutePath(String const& path);
		String Basename(String const& path);
		String Dirname(String const& path);

		bool Exists(String const& path);
		bool LinkExists(String const& path);

		String ExpandPath(String const& path);

		int64_t GetAccessTime(String const& path);
		int64_t GetModificationTime(String const& path);
		int64_t GetSize(String const& path);

		bool IsAbsolute(String const& path);
		bool IsFile(String const& path);
		bool IsDirectory(String const& path);
		bool IsLink(String const& path);
		bool IsMountPoint(String const& path);

		namespace detail
		{
			template<typename T>
			void Join(StringBuilder& builder, T const& lastItem)
			{
				builder << lastItem;
			}

			template<typename T, typename... Params>
			void Join(StringBuilder& builder, T const& nextItem, Params const&... params)
			{
				builder << nextItem << Separator();
				Join(builder, params...);
			}
		}

		template<typename T, typename... Params>
		String Join(T const& begin, Params const&... params)
		{
			sprawl::StringBuilder b;
			detail::Join(b, begin, params...);
			return b.Str();
		}

		String NormCase(String const& path);
		String NormPath(String const& path);
		String RealPath(String const& path);
		String RelativePath(String const& path, String const& relativeTo = StringLiteral("./"));

		bool SameFile(String const& path1, String const& path2);

		struct SplitResult
		{
			String dirname;
			String basename;
		};

		SplitResult Split(String const& path);

		struct DriveResult
		{
			String drive;
			String tail;
		};

		DriveResult SplitDrive(String const& path);

		struct ExtResult
		{
			String path;
			String extension;
		};

		ExtResult SplitExt(String const& path);

		String CommonPrefix(String const& path1, String const& path2);
	}
}
