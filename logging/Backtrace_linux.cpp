#include "Backtrace.hpp"
#include "../filesystem/path.hpp"
#include "../collections/Array.hpp"
#include <execinfo.h>

#include <unistd.h>

#include <cxxabi.h>

#include <bfd.h>
#include <link.h>
#include <algorithm>

namespace BacktraceStatic
{
	bool initialized_ = false;

	struct Symbol
	{
		bfd_vma pc;
		char const* filename;
		char const* functionName;
		bfd_vma offset;
		unsigned int line;
	};

	static bool findSymbol_( bfd* bfdPtr, asymbol** syms, Symbol& symbol )
	{
		for(asection* section = bfdPtr->sections; section != nullptr; section = section->next)
		{
			auto& flags = bfd_get_section_flags(bfdPtr, section);

			if( ( flags & SEC_ALLOC ) == 0 )
				continue;

			bfd_vma vma = bfd_get_section_vma( bfdPtr, section );
			if(symbol.pc < vma)
				continue;

			bfd_size_type size = bfd_get_section_size(section);
			if(symbol.pc >= vma + size)
				continue;

			bfd_vma offset = symbol.pc - vma;
			if(bfd_find_nearest_line( bfdPtr, section, syms, offset, &symbol.filename, &symbol.functionName, &symbol.line ))
			{
				symbol.offset = offset;
				return true;
			}
		}
		return false;
	}

	struct Module
	{
		uintptr_t baseAddress;
		char const* path;
		char const* baseFilename;
		bfd* bfdPtr;
		asymbol** symbols;

		Module()
			: baseAddress(0)
			, path(nullptr)
			, baseFilename(nullptr)
			, bfdPtr(nullptr)
			, symbols(nullptr)
		{

		}

		Module(Module&& other)
			: baseAddress(other.baseAddress)
			, path(other.path)
			, baseFilename(other.baseFilename)
			, bfdPtr(other.bfdPtr)
			, symbols(other.symbols)
		{
			other.baseAddress = 0;
			other.path = nullptr;
			other.baseFilename = nullptr;
			other.bfdPtr = nullptr;
			other.symbols = nullptr;
		}

		Module& operator=(Module&& other)
		{
			baseAddress = other.baseAddress;
			path = other.path;
			baseFilename = other.baseFilename;
			bfdPtr = other.bfdPtr;
			symbols = other.symbols;

			other.baseAddress = 0;
			other.path = nullptr;
			other.baseFilename = nullptr;
			other.bfdPtr = nullptr;
			other.symbols = nullptr;
			return *this;
		}

		Module(char const* path_, uintptr_t addr)
			: baseAddress(addr)
			, path(path_)
			, baseFilename(path_)
			, bfdPtr(nullptr)
			, symbols(nullptr)
		{
			bfdPtr = bfd_openr(path_, nullptr);

			if(!bfdPtr)
				return;

			if (bfd_check_format(bfdPtr, bfd_archive))
			{
				bfd_close(bfdPtr);
				bfdPtr = nullptr;
				return;
			}

			char** matching = NULL;
			if (!bfd_check_format_matches(bfdPtr, bfd_object, &matching))
			{
				bfd_close(bfdPtr);
				bfdPtr = nullptr;
				return;
			}

			if ((bfd_get_file_flags(bfdPtr) & HAS_SYMS) == 0)
			{
				bfd_close(bfdPtr);
				bfdPtr = nullptr;
				return;
			}

			unsigned int size;
			long symcount = bfd_read_minisymbols( bfdPtr, FALSE, (void**)&symbols, &size );

			if(symcount == 0)
			{
				symcount = bfd_read_minisymbols( bfdPtr, TRUE, (void**)&symbols, &size );
			}

			if(symcount < 0)
			{
				bfd_close(bfdPtr);
				bfdPtr = nullptr;
				symbols = nullptr;
			}

			char const* ptr = path;
			char const sep = sprawl::path::Separator();
			char const altSep = sprawl::path::AltSeparator();
			while(*ptr != '\0')
			{
				if(*ptr == sep || *ptr == altSep)
				{
					baseFilename = ptr + 1;
				}
				++ptr;
			}
		}

		~Module()
		{
			if(symbols)
			{
				free(symbols);
			}
			if(bfdPtr)
			{
				bfd_close(bfdPtr);
			}
		}

		bool Good()
		{
			return bfdPtr != nullptr && symbols != nullptr;
		}
	};

	char moduleBuffer_[256 * sizeof(Module)];
	Module* modules_ = reinterpret_cast<Module*>(moduleBuffer_);

	static size_t nModules_ = 0;

	int dlCallback_(struct dl_phdr_info* info, size_t /*size*/, void* /*data*/)
	{
		size_t idx = nModules_;
		Module& module = modules_[idx];

		new(&module) Module(info->dlpi_name, info->dlpi_addr);

		if(module.Good())
		{
			++nModules_;
		}
		else
		{
			module.~Module();
		}

		return 0;
	}

	static Module* findModule_(uintptr_t address, int start = 0, int end = nModules_- 1)
	{
		if(end < start)
		{
			return nullptr;
		}

		size_t mid = (start + end) / 2;

		Module& module = modules_[mid];
		if( module.baseAddress < address )
		{
			if(mid < nModules_ - 1 && reinterpret_cast<Module&>(modules_[mid+1]).baseAddress > address)
				return &module;

			return findModule_(address, mid+1, end);
		}
		return findModule_(address, start, mid-1);
	}

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
	if(BacktraceStatic::initialized_)
		return;

	BacktraceStatic::initialized_ = true;

	bfd_init();

	static char path[1024];
	readlink("/proc/self/exe", path, 1024);

	size_t idx = BacktraceStatic::nModules_;
	BacktraceStatic::Module& module = BacktraceStatic::modules_[idx];

	new(&module) BacktraceStatic::Module(path, 0);

	if(module.Good())
	{
		++BacktraceStatic::nModules_;
	}
	else
	{
		module.~Module();
	}

	dl_iterate_phdr(&BacktraceStatic::dlCallback_, nullptr);

	std::sort(BacktraceStatic::modules_, BacktraceStatic::modules_ + BacktraceStatic::nModules_, [](BacktraceStatic::Module const& module, BacktraceStatic::Module const& other)
	{
		return module.baseAddress < other.baseAddress;
	});
}

void sprawl::logging::Backtrace::ShutDown()
{
	for(size_t i = 0; i < BacktraceStatic::nModules_; ++i)
	{
		BacktraceStatic::modules_[i].~Module();
	}

	BacktraceStatic::nModules_ = 0;

	BacktraceStatic::initialized_ = false;
}

sprawl::logging::Backtrace sprawl::logging::Backtrace::Get(int skipFrames)
{
	// We could use libUnwind here instead of backtrace(), and in fact, we originally did.
	// libUnwind is very very very very very slightly faster than backtrace (to the tune of 1 us per call on average)
	// But since we don't use libUnwind for symbolification and libUnwind's primary performance gain is when compared to
	// backtrace_symbols, the 1us performance improvement does not seem worth the dependency.
	Backtrace stack;
	void* bt[SPRAWL_BACKTRACE_MAX_STACK];
	stack.m_size = backtrace(bt, SPRAWL_BACKTRACE_MAX_STACK) - skipFrames;
	memcpy(stack.m_stack, bt + skipFrames, SPRAWL_BACKTRACE_MAX_STACK - skipFrames);

	return stack;
}


sprawl::logging::Backtrace::Frame sprawl::logging::Backtrace::GetFrame(size_t index) const
{
	Frame frame;

	uintptr_t addr = m_stack[index];
	frame.address = addr;

	BacktraceStatic::Module* module = BacktraceStatic::findModule_(addr);

	if(module != nullptr)
	{
		strncpy(frame.module, module->baseFilename, SPRAWL_BACKTRACE_STRING_MAX);

		BacktraceStatic::Symbol sym;
		sym.pc = addr - module->baseAddress;
		if(BacktraceStatic::findSymbol_(module->bfdPtr, module->symbols, sym))
		{
			frame.offset = sym.offset;

			if(!BacktraceStatic::demangle_( sym.functionName, frame.func, SPRAWL_BACKTRACE_STRING_MAX ))
			{
				strncpy(frame.func, sym.functionName, SPRAWL_BACKTRACE_STRING_MAX);
			}

			if(sym.filename != nullptr)
			{
				strncpy(frame.file, sym.filename, SPRAWL_BACKTRACE_STRING_MAX);

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

				frame.line = sym.line;
			}
			else
			{
				strncpy(frame.file, "Unknown", SPRAWL_BACKTRACE_STRING_MAX);
				frame.baseFile = frame.file;

				frame.line = -1;
			}
		}
	}

	return frame;
}
