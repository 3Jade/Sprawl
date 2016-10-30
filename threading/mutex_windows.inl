inline void sprawl::threading::Mutex::Lock()
{
	AcquireSRWLockExclusive(&m_mutexImpl);
}

inline bool sprawl::threading::Mutex::TryLock()
{
	return TryAcquireSRWLockExclusive(&m_mutexImpl);
}

inline void sprawl::threading::Mutex::Unlock()
{
	ReleaseSRWLockExclusive(&m_mutexImpl);
}

inline sprawl::threading::Mutex::Mutex()
	: m_mutexImpl(SRWLOCK_INIT)
{

}

inline sprawl::threading::Mutex::~Mutex()
{

}
