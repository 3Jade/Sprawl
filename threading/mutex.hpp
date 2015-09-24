#pragma once

#ifdef _WIN32
#	include <Windows.h>
typedef	CRITICAL_SECTION MutexType;
#else
#	include <pthread.h>
typedef pthread_mutex_t MutexType;
#endif

#include "../common/noncopyable.hpp"

namespace sprawl
{
	namespace threading
	{
		class Mutex;
		class ScopedLock;
		class SharedLock;
	}
}

class sprawl::threading::Mutex : public sprawl::noncopyable
{
public:
	void Lock();
	bool TryLock();
	void Unlock();

	Mutex();
	~Mutex();
	Mutex(Mutex&& other)
		: m_mutexImpl(other.m_mutexImpl)
	{
		other.m_mutexImpl = MutexType();
	}

	MutexType& GetNativeMutex() { return m_mutexImpl; }
private:
	MutexType m_mutexImpl;
};



class sprawl::threading::ScopedLock
{
public:
	ScopedLock(Mutex& mutex);
	~ScopedLock();
private:
	Mutex& m_mutex;
};

inline sprawl::threading::ScopedLock::ScopedLock(Mutex& mutex)
	: m_mutex(mutex)
{
	m_mutex.Lock();
}

inline sprawl::threading::ScopedLock::~ScopedLock()
{
	m_mutex.Unlock();
}



enum class LockType
{
	Immediate,
	TryLock,
	Deferred
};

class sprawl::threading::SharedLock
{
public:
	SharedLock(Mutex& mutex, LockType type = LockType::Immediate);
	SharedLock();
	SharedLock(SharedLock&& other);
	~SharedLock();

	void Lock() { m_mutex->Lock(); }
	bool TryLock() { return m_mutex->TryLock(); }
	void Unlock() { m_mutex->Unlock(); }

	bool IsOwned() { return m_owned; }

	void Release() { m_mutex = nullptr; m_owned = false; }

	SharedLock& operator=(SharedLock&& other);

	Mutex* GetMutex() { return m_mutex; }
private:
	Mutex* m_mutex;
	bool m_owned;
};

inline sprawl::threading::SharedLock::SharedLock(Mutex& mutex, LockType type)
	: m_mutex(&mutex)
	, m_owned(false)
{
	switch(type)
	{
		case LockType::Immediate:
			m_mutex->Lock();
			m_owned = true;
			break;
		case LockType::TryLock:
			m_owned = m_mutex->TryLock();
			break;
		case LockType::Deferred:
		default:
			break;
	}
}

inline sprawl::threading::SharedLock::SharedLock()
	: m_mutex(nullptr)
	, m_owned(false)
{
	//
}

inline sprawl::threading::SharedLock::SharedLock(sprawl::threading::SharedLock&& other)
	: m_mutex(other.m_mutex)
	, m_owned(other.m_owned)
{
	other.m_mutex = nullptr;
	other.m_owned = false;
}

inline sprawl::threading::SharedLock::~SharedLock()
{
	if(m_owned)
	{
		m_mutex->Unlock();
	}
}

inline sprawl::threading::SharedLock& sprawl::threading::SharedLock::operator=(sprawl::threading::SharedLock&& other)
{
	m_mutex = other.m_mutex;
	m_owned = other.m_owned;
	other.m_mutex = nullptr;
	other.m_owned = false;
	return *this;
}

#ifdef _WIN32
#	include "mutex_windows.inl"
#else
#	include "mutex_linux.inl"
#endif
