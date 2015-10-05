#include "Backtrace.hpp"
#include <libunwind.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <utility>
#include <string.h>
#include <stdlib.h>

namespace BacktraceStatic
{
	static bool demangle_(char const* input, char* outputBuffer, size_t outputLength)
	{
		///@TODO: Implement this directly so we can avoid heap allocations.
		int status;
		size_t length = outputLength;
		char* buffer = abi::__cxa_demangle(input, nullptr, &length, &status);
		if(buffer == nullptr)
		{
			return false;
		}
		strncpy(outputBuffer, buffer, outputLength);
		free(buffer);
		return true;
	}
}

void sprawl::logging::Backtrace::Init()
{
	// NOP
}

void sprawl::logging::Backtrace::ShutDown()
{
	// NOP
}

sprawl::logging::Backtrace sprawl::logging::Backtrace::Get(int skipFrames)
{
	Backtrace stack;

	unw_context_t context;
	unw_cursor_t cursor;

	unw_getcontext(&context);
	unw_init_local(&cursor, &context);

	unw_word_t ip;
	for(int i = 0; i < skipFrames + 1; ++i)
	{
		unw_step(&cursor);
	}

	int stackDepth = 0;

	do
	{
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		stack.m_stack[stackDepth] = ip;
		++stackDepth;
	} while (unw_step(&cursor) > 0 && stackDepth < SPRAWL_BACKTRACE_MAX_STACK);

	stack.m_size = stackDepth;

	return std::move(stack);
}

sprawl::logging::Backtrace::Frame sprawl::logging::Backtrace::GetFrame(size_t index) const
{
	Frame frame;

	uintptr_t addr = m_stack[index];
	frame.address = addr;

	Dl_info info;
	if(dladdr((void*)(addr), &info) != 0)
	{
		if(!BacktraceStatic::demangle_( info.dli_sname, frame.func, SPRAWL_BACKTRACE_STRING_MAX ))
		{
			strncpy(frame.func, info.dli_sname, SPRAWL_BACKTRACE_STRING_MAX);
		}
		strncpy(frame.module, info.dli_fname, SPRAWL_BACKTRACE_STRING_MAX);
		frame.offset = addr - intptr_t(info.dli_saddr);
	}

	return std::move(frame);
}