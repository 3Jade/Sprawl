inline void sprawl::threading::Mutex::Lock()
{
	pthread_mutex_lock(&m_mutexImpl);
}

inline bool sprawl::threading::Mutex::TryLock()
{
	return (pthread_mutex_trylock(&m_mutexImpl) == 0);
}

inline void sprawl::threading::Mutex::Unlock()
{
	pthread_mutex_unlock(&m_mutexImpl);
}

inline sprawl::threading::Mutex::Mutex()
	: m_mutexImpl(PTHREAD_MUTEX_INITIALIZER)
{
}

inline sprawl::threading::Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutexImpl);
}
