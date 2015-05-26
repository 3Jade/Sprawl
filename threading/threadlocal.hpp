#pragma once

namespace sprawl
{
	namespace threading
	{
		template<typename T>
		class ThreadLocal;
	}
}

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <pthread.h>
#endif

template<typename T>
class sprawl::threading::ThreadLocal
{
public:
	ThreadLocal();
	ThreadLocal(T const& value);
	~ThreadLocal();

	void operator=(T const& value);

	T& operator*();
	T const& operator*() const;
	T* operator->();
	T const* operator->() const;

	operator bool() const;
private:
	T* get();
	T const* get() const;
	void set(T const& value);

	#ifdef _WIN32
		typedef	DWORD KeyType;
	#else
		typedef pthread_key_t KeyType;
	#endif

	KeyType m_key;
};

template<typename T>
class sprawl::threading::ThreadLocal<T*>
{
public:
	ThreadLocal();
	ThreadLocal(T const* value);
	~ThreadLocal();

	void operator=(T const* value);

	T* operator*();
	T const* operator*() const;
	T* operator->();
	T const* operator->() const;

	operator bool() const;
private:
	T* get();
	T const* get() const;
	void set(T const* value);

	#ifdef _WIN32
		typedef	DWORD KeyType;
	#else
		typedef pthread_key_t KeyType;
	#endif

	KeyType m_key;
};

template<typename T>
void sprawl::threading::ThreadLocal<T>::operator=(T const& value)
{
	set(value);
}

template<typename T>
T& sprawl::threading::ThreadLocal<T>::operator*()
{
	return *get();
}

template<typename T>
T const& sprawl::threading::ThreadLocal<T>::operator*() const
{
	return *get();
}

template<typename T>
T* sprawl::threading::ThreadLocal<T>::operator->()
{
	return get();
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T>::operator->() const
{
	return get();
}

template<typename T>
sprawl::threading::ThreadLocal<T>::operator bool() const
{
	return get() != nullptr;
}

template<typename T>
void sprawl::threading::ThreadLocal<T*>::operator=(T const* value)
{
	set(value);
}

template<typename T>
T* sprawl::threading::ThreadLocal<T*>::operator*()
{
	return get();
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T*>::operator*() const
{
	return get();
}

template<typename T>
T* sprawl::threading::ThreadLocal<T*>::operator->()
{
	return get();
}

template<typename T>
T const* sprawl::threading::ThreadLocal<T*>::operator->() const
{
	return get();
}

template<typename T>
sprawl::threading::ThreadLocal<T*>::operator bool() const
{
	return get() != nullptr;
}



#ifdef _WIN32
#	include "threadlocal_windows.inl"
#else
#	include "threadlocal_linux.inl"
#endif
