#include "Backtrace.hpp"
#include <Windows.h>
#include <DbgHelp.h>
#include "../filesystem/filesystem.hpp"
#include "../filesystem/path.hpp"

namespace BacktraceStatic
{
	bool initialized_ = false;
}

void sprawl::logging::Backtrace::Init()
{
	if(BacktraceStatic::initialized_)
		return;

	BacktraceStatic::initialized_ = true;

	HANDLE process = GetCurrentProcess();
	char path[MAX_PATH];
	char symbolPath[MAX_PATH];
	char altPath[MAX_PATH];
	GetEnvironmentVariableA("_NT_SYMBOL_PATH", symbolPath, sizeof(symbolPath));
	GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", altPath, sizeof(altPath));
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST | SYMOPT_LOAD_ANYTHING);

	_snprintf(path, sizeof(path), "%s;%s;%s", sprawl::filesystem::GetCwd().c_str(), symbolPath, altPath);

	SymInitialize( process, path, TRUE );
}

void sprawl::logging::Backtrace::ShutDown()
{
	HANDLE process = GetCurrentProcess();
	SymCleanup(process);
	BacktraceStatic::initialized_ = false;
}

sprawl::logging::Backtrace sprawl::logging::Backtrace::Get(int skipFrames)
{
	Backtrace backtrace;

	void** stack = reinterpret_cast<void**>(backtrace.m_stack);
	backtrace.m_size = CaptureStackBackTrace(skipFrames + 1, SPRAWL_BACKTRACE_MAX_STACK, stack, nullptr);

	return std::move(backtrace);
}


sprawl::logging::Backtrace::Frame sprawl::logging::Backtrace::GetFrame(size_t index) const
{
	Frame frame;

	uintptr_t addr = m_stack[index];

	HANDLE process = GetCurrentProcess();

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(buffer);

	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = MAX_SYM_NAME;

	if(SymFromAddr( process, addr, 0, symbol ))
	{
		strncpy(frame.func, symbol->Name, SPRAWL_BACKTRACE_STRING_MAX);
		frame.offset = (addr - symbol->Address);
	}
	else
	{
		strncpy(frame.func, "Unknown", SPRAWL_BACKTRACE_STRING_MAX);
	}

	IMAGEHLP_MODULE64 module;
	memset(&module, 0, sizeof(IMAGEHLP_MODULE64) );
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

	if(SymGetModuleInfo64(process, addr, &module))
	{
		strncpy(frame.module, module.ModuleName, SPRAWL_BACKTRACE_STRING_MAX);
	}
	else
	{
		strncpy(frame.module, "Unknown", SPRAWL_BACKTRACE_STRING_MAX);
	}

	DWORD displacement;

	IMAGEHLP_LINE64 line;
	memset(&line, 0, sizeof(IMAGEHLP_LINE64));
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	if (SymGetLineFromAddr64(process, addr, &displacement, &line))
	{
		strncpy(frame.file, line.FileName, SPRAWL_BACKTRACE_STRING_MAX);

		char const* ptr = frame.file;
		char sep = sprawl::path::Separator();
		char altSep = sprawl::path::AltSeparator();
		while(*ptr != '\0')
		{
			if(*ptr == sep || *ptr == altSep)
			{
				frame.baseFile = ptr + 1;
			}
			++ptr;
		}

		frame.line = line.LineNumber;
	}
	else
	{
		strncpy(frame.file, "Unknown", sizeof(frame.file));
		frame.baseFile = frame.file;
		frame.line = -1;
	}

	return std::move(frame);
}

