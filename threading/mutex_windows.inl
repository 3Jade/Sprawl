inline void sprawl::threading::Mutex::Lock()
{
	EnterCriticalSection(&m_mutexImpl);
}

inline bool sprawl::threading::Mutex::TryLock()
{
	return TryEnterCriticalSection(&m_mutexImpl);
}

inline void sprawl::threading::Mutex::Unlock()
{
	LeaveCriticalSection(&m_mutexImpl);
}

inline sprawl::threading::Mutex::Mutex()
	: m_mutexImpl()
{
	InitializeCriticalSection(&m_mutexImpl);
}

inline sprawl::threading::Mutex::~Mutex()
{
	DeleteCriticalSection(&m_mutexImpl);
}
