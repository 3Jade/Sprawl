#include <pthread.h>

class sprawl::threading::Handle
{
public:
	Handle()
		: m_thread()
	{
		//
	}

	Handle(pthread_t const& thread)
		: m_thread(thread)
	{
		//
	}

	Handle(Handle const& other)
		: m_thread(other.m_thread)
	{
		//
	}

	bool operator==(Handle const& other) { return m_thread == other.m_thread; }
	bool operator!=(Handle const& other) { return m_thread != other.m_thread; }

	int64_t GetUniqueId() const;
	pthread_t& GetNativeHandle() { return m_thread; }
	pthread_t const& GetNativeHandle() const { return m_thread; }
private:
	pthread_t m_thread;
};