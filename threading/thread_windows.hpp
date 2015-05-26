#include <Windows.h>

class sprawl::threading::Handle
{
public:
	Handle()
		: m_thread()
		, duplicate(false)
	{
		//
	}

	Handle(HANDLE const& thread)
		: m_thread(nullptr)
		, duplicate(true)
	{
		DuplicateHandle(GetCurrentProcess(), thread, GetCurrentProcess(), &m_thread, 0, false, DUPLICATE_SAME_ACCESS);
	}
	
	Handle(Handle const& other)
		: m_thread(other.m_thread)
		, duplicate(false)
	{

	}

	~Handle()
	{
		if(duplicate)
		{
			CloseHandle(m_thread);
		}
	}

	bool operator==(Handle const& other) { return m_thread == other.m_thread; }
	bool operator!=(Handle const& other) { return m_thread != other.m_thread; }

	int64_t GetUniqueId() const;
	HANDLE& GetNativeHandle() { return m_thread; }
	HANDLE const& GetNativeHandle() const { return m_thread; }
private:
	HANDLE m_thread;
	bool duplicate;
};


