template<typename T>
sprawl::threading::ThreadLocal<T>::ThreadLocal()
{
	m_key = TlsAlloc();
}

template<typename T>
sprawl::threading::ThreadLocal<T>::ThreadLocal(T const& value)
{
	m_key = TlsAlloc();
	set(value);
}

template<typename T>
sprawl::threading::ThreadLocal<T>::~ThreadLocal()
{
	TlsFree(m_key);
}

template<typename T>
T* sprawl::threading::ThreadLocal<T>::get()
{
	return reinterpret_cast<T*>(TlsGetValue(m_key));
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T>::get() const
{
	return reinterpret_cast<T*>(TlsGetValue(m_key));
}

template<typename T>
void sprawl::threading::ThreadLocal<T>::set(T const& value)
{
	T* oldValue = reinterpret_cast<T*>(TlsGetValue(m_key));
	if(oldValue)
	{
		*oldValue = value;
	}
	else
	{
		TlsSetValue(m_key, (void*)(new T(value)));
	}
}

template<typename T>
void sprawl::threading::ThreadLocal<T>::Clear()
{
	T* oldValue = reinterpret_cast<T*>(TlsGetValue(m_key));
	if(oldValue)
	{
		delete oldValue;
		TlsSetValue(m_key, nullptr);
	}
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::ThreadLocal()
{
	m_key = TlsAlloc();
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::ThreadLocal(T const* value)
{
	m_key = TlsAlloc();
	set(value);
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::~ThreadLocal()
{
	TlsFree(m_key);
}

template<typename T>
T* sprawl::threading::ThreadLocal<T*>::get()
{
	return reinterpret_cast<T*>(TlsGetValue(m_key));
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T*>::get() const
{
	return reinterpret_cast<T*>(TlsGetValue(m_key));
}

template<typename T>
void sprawl::threading::ThreadLocal<T*>::set(T const* value)
{
	TlsSetValue(m_key, (void*)(value));
}
