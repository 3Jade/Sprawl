template<typename T>
sprawl::threading::ThreadLocal<T>::ThreadLocal()
{
	pthread_key_create(&m_key, NULL);
}

template<typename T>
sprawl::threading::ThreadLocal<T>::ThreadLocal(T const& value)
{
	pthread_key_create(&m_key, NULL);
	set(value);
}

template<typename T>
sprawl::threading::ThreadLocal<T>::~ThreadLocal()
{
	pthread_key_delete(m_key);
}

template<typename T>
T* sprawl::threading::ThreadLocal<T>::get()
{
	return reinterpret_cast<T*>(pthread_getspecific(m_key));
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T>::get() const
{
	return reinterpret_cast<T*>(pthread_getspecific(m_key));
}

template<typename T>
void sprawl::threading::ThreadLocal<T>::set(T const& value)
{
	T* oldValue = reinterpret_cast<T*>(pthread_getspecific(m_key));
	if(oldValue)
	{
		*oldValue = value;
	}
	else
	{
		pthread_setspecific(m_key, (void*)(new T(value)));
	}
}

template<typename T>
void sprawl::threading::ThreadLocal<T>::Clear()
{
	T* oldValue = reinterpret_cast<T*>(pthread_getspecific(m_key));
	if(oldValue)
	{
		delete oldValue;
		pthread_setspecific(m_key, nullptr);
	}
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::ThreadLocal()
{
	pthread_key_create(&m_key, NULL);
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::ThreadLocal(T const* value)
{
	pthread_key_create(&m_key, NULL);
	set(value);
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::~ThreadLocal()
{
	pthread_key_delete(m_key);
}

template<typename T>
T* sprawl::threading::ThreadLocal<T*>::get()
{
	return reinterpret_cast<T*>(pthread_getspecific(m_key));
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T*>::get() const
{
	return reinterpret_cast<T*>(pthread_getspecific(m_key));
}

template<typename T>
void sprawl::threading::ThreadLocal<T*>::set(T const* value)
{
	pthread_setspecific(m_key, (void*)(value));
}
