#pragma once

#include <stdint.h>
#include <stddef.h>

#define SPRAWL_BACKTRACE_MAX_STACK 64
#define SPRAWL_BACKTRACE_STRING_MAX 512

namespace sprawl
{
	namespace logging
	{
		class Backtrace;
	}
}

class sprawl::logging::Backtrace
{
public:

	struct Frame
	{
		Frame()
			: address(0)
			, func()
			, file()
			, baseFile(nullptr)
			, module()
			, line(0)
			, offset(0)
			, text()
		{
			//
		}

		uintptr_t address;
		char func[SPRAWL_BACKTRACE_STRING_MAX];
		char file[SPRAWL_BACKTRACE_STRING_MAX];
		char const* baseFile; // Should be a pointer into file
		char module[SPRAWL_BACKTRACE_STRING_MAX];
		int line;
		uint64_t offset;
		char text[SPRAWL_BACKTRACE_STRING_MAX];
	};

	Backtrace()
		: m_stack()
		, m_size(0)
	{
		//
	}

	size_t Size() const
	{
		return m_size;
	}

	Frame GetFrame(size_t index) const;

	static void Init();
	static void ShutDown();
	static Backtrace Get(int skipFrames = 1);

protected:
	uintptr_t m_stack[SPRAWL_BACKTRACE_MAX_STACK];
	size_t m_size;
};
