#pragma once

#include <functional>
#include "threadlocal.hpp"
#include <atomic>
#include "../memory/PoolAllocator.hpp"

#ifdef _WIN32
#	undef Yield
#else
	#ifdef __APPLE__
		#include <sys/ucontext.h>
	#else
		#include <ucontext.h>
	#endif
#endif

namespace sprawl
{
	namespace threading
	{
		class CoroutineBase;
		class Coroutine;

		template<typename SendType, typename ReceiveType>
		class CoroutineWithChannel;

		template<typename ReturnType>
		using Generator = CoroutineWithChannel<void, ReturnType>;
	}
}

class sprawl::threading::CoroutineBase
{
public:

	enum class CoroutineState
	{
		Created,
		Executing,
		Paused,
		Completed,
	};

	static void Yield();

	template<typename SendType, typename ReceiveType>
	static SendType& Receive(ReceiveType const& value);

	template<typename SendType>
	static SendType& Receive();

	template<typename ReceiveType>
	static void Yield(ReceiveType const& value);

	CoroutineBase();

	CoroutineBase(CoroutineBase const& other);

	CoroutineBase& operator=(CoroutineBase const& other);

	virtual ~CoroutineBase();

	void Start() { Resume(); }
	void Pause();
	void Resume();
	void Reset();

	void operator()() { Resume(); }

	CoroutineState State();
protected:
	static ThreadLocal<CoroutineBase*> ms_coroutineInitHelper;
	static ThreadLocal<CoroutineBase> ms_thisThreadCoroutine;

	static void entryPoint_();
	void run_();
	void reactivate_();
	void releaseRef_();

	struct Holder;

	explicit CoroutineBase(Holder* holder);

	Holder* m_holder;
	bool m_ownsHolder;
};

struct sprawl::threading::CoroutineBase::Holder
{
	void IncRef();
	bool DecRef();

	std::function<void ()> m_function;
	size_t m_stackSize;
	void* m_stack;
	void* m_stackPointer;
	CoroutineState m_state;

	#ifndef _WIN32
		ucontext_t m_context;
	#endif

	std::atomic<int> m_refCount;

	CoroutineBase m_priorCoroutine;

	static Holder* Create(std::function<void()> function, size_t stackSize);
	static Holder* Create();
	virtual void Release();

	virtual ~Holder() {}
protected:
	Holder();
	Holder(std::function<void()> function, size_t stackSize);
};

class sprawl::threading::Coroutine : public sprawl::threading::CoroutineBase
{
public:
	Coroutine()
		: CoroutineBase()
	{
		//
	}

	Coroutine(std::function<void ()> function, size_t stackSize = 0)
		: CoroutineBase(CoroutineBase::Holder::Create(function, stackSize))
	{
		//
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!std::is_same<std::function<void ()>, typename std::remove_reference<Callable>::type>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
		>::type
	>
	Coroutine(Callable && callable, Params &&... params)
		: CoroutineBase(CoroutineBase::Holder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}
};

template<typename SendType, typename ReceiveType>
class sprawl::threading::CoroutineWithChannel : public sprawl::threading::CoroutineBase
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		// NOP
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!std::is_same<std::function<void ()>, typename std::remove_reference<Callable>::type>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
		>::type
	>
	CoroutineWithChannel(Callable && callable, Params &&... params)
		: CoroutineBase(ChannelHolder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}

	SendType& Receive(ReceiveType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = value;
		Pause();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue;
	}

	ReceiveType& Send(SendType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = value;
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	SendType& Receive(ReceiveType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = std::move(value);
		Pause();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue;
	}

	ReceiveType& Send(SendType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = std::move(value);
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	ReceiveType& Start()
	{
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	ReceiveType& operator()()
	{
		return Start();
	}

	ReceiveType& operator()(SendType const& value)
	{
		return Send(value);
	}

	ReceiveType& operator()(SendType&& value)
	{
		return Send(std::move(value));
	}

private:

	struct ChannelHolder : public CoroutineBase::Holder
	{
		static ChannelHolder* Create(std::function<void()> function, size_t stackSize)
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret) ChannelHolder(function, stackSize);
			return ret;
		}

		virtual void Release() override
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		ChannelHolder(std::function<void()> function, size_t stackSize)
			: Holder(function, stackSize)
			, m_sentValue()
			, m_receivedValue()
		{
			// NOP
		}

		SendType m_sentValue;
		ReceiveType m_receivedValue;
	};
};

template<typename SendType>
class sprawl::threading::CoroutineWithChannel<SendType, void> : public sprawl::threading::CoroutineBase
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		// NOP
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!std::is_same<std::function<void ()>, typename std::remove_reference<Callable>::type>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
		>::type
	>
	CoroutineWithChannel(Callable && callable, Params &&... params)
		: CoroutineBase(ChannelHolder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}

	SendType& Receive()
	{
		Pause();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue;
	}

	void Send(SendType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = value;
		Resume();
	}

	void Send(SendType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = std::move(value);
		Resume();
	}

	using CoroutineBase::operator();

	void operator()(SendType const& value)
	{
		Send(value);
	}

	void operator()(SendType&& value)
	{
		Send(std::move(value));
	}

private:

	struct ChannelHolder : public CoroutineBase::Holder
	{
		static ChannelHolder* Create(std::function<void()> function, size_t stackSize = 0)
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret) ChannelHolder(function, stackSize);
			return ret;
		}

		virtual void Release() override
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		ChannelHolder(std::function<void()> function, size_t stackSize)
			: Holder(function, stackSize)
			, m_sentValue()
		{
			// NOP
		}

		SendType m_sentValue;
	};
};

template<typename ReceiveType>
class sprawl::threading::CoroutineWithChannel<void, ReceiveType> : public sprawl::threading::CoroutineBase
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		// NOP
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!std::is_same<std::function<void ()>, typename std::remove_reference<Callable>::type>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
		>::type
	>
	CoroutineWithChannel(Callable && callable, Params &&... params)
		: CoroutineBase(ChannelHolder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}

	void Yield(ReceiveType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = value;
		Pause();
	}

	void Yield(ReceiveType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = std::move(value);
		Pause();
	}

	ReceiveType& Resume()
	{
		CoroutineBase::Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	ReceiveType& Start()
	{
		return Resume();
	}

	ReceiveType& operator()()
	{
		return Resume();
	}

private:
	struct ChannelHolder : public CoroutineBase::Holder
	{
		static ChannelHolder* Create(std::function<void()> function, size_t stackSize)
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret) ChannelHolder(function, stackSize);
			return ret;
		}

		virtual void Release() override
		{
			typedef memory::DynamicPoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		ChannelHolder(std::function<void()> function, size_t stackSize)
			: Holder(function, stackSize)
			, m_receivedValue()
		{
			// NOP
		}

		ReceiveType m_receivedValue;
	};
};

template<>
class sprawl::threading::CoroutineWithChannel<void, void> : public sprawl::threading::CoroutineBase
{

};

template<typename SendType, typename ReceiveType>
/*static*/ SendType& sprawl::threading::CoroutineBase::Receive(ReceiveType const& value)
{
	CoroutineWithChannel<SendType, ReceiveType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;
	withChannel.releaseRef_();
	return withChannel.Receive(value);
}

template<typename SendType>
/*static*/ SendType& sprawl::threading::CoroutineBase::Receive()
{
	CoroutineWithChannel<SendType, void> withChannel = *CoroutineBase::ms_thisThreadCoroutine;
	withChannel.releaseRef_();
	return withChannel.Receive();
}

template<typename ReceiveType>
/*static*/ void sprawl::threading::CoroutineBase::Yield(ReceiveType const& value)
{
	CoroutineWithChannel<void, ReceiveType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;
	withChannel.releaseRef_();
	withChannel.Yield(value);
}
