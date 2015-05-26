#pragma once

#include <functional>
#include "threadlocal.hpp"
#include <atomic>
#include "../memory/PoolAllocator.hpp"

#ifdef _WIN32
#	undef Yield
#else
#	include <ucontext.h>
#endif

///TODO:
/// - Get rid of std::shared_ptr, make coroutines have shared data instead
/// - Get rid of std::function, allow Start() to accept function parameters
/// - Windows implementation

namespace sprawl
{
	namespace threading
	{
		class Coroutine;

		template<typename SendType, typename ReceiveType>
		class CoroutineWithChannel;

		template<typename ReturnType>
		using Generator = CoroutineWithChannel<void, ReturnType>;
	}
}

class sprawl::threading::Coroutine
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

	Coroutine();
	Coroutine(std::function<void ()> function, size_t stackSize = 0);

	Coroutine(Coroutine const& other);

	Coroutine& operator=(Coroutine const& other);

	virtual ~Coroutine();

	void Start() { Resume(); }
	void Pause();
	void Resume();
	void Reset();

	void operator()() { Resume(); }

	CoroutineState State();
protected:
	static ThreadLocal<Coroutine*> ms_coroutineInitHelper;
	static ThreadLocal<Coroutine> ms_thisThreadCoroutine;

	static void entryPoint_();
	void run_();
	void reactivate_();

	struct Holder;

	explicit Coroutine(Holder* holder);

	Holder* m_holder;
};

struct sprawl::threading::Coroutine::Holder
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

	Coroutine m_priorCoroutine;

	static Holder* Create(std::function<void()> function, size_t stackSize);
	static Holder* Create();
	virtual void Release();

	virtual ~Holder() {}
protected:
	Holder();
	Holder(std::function<void()> function, size_t stackSize);
};

template<typename SendType, typename ReceiveType>
class sprawl::threading::CoroutineWithChannel : public sprawl::threading::Coroutine
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: Coroutine(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(Coroutine const& other)
		: Coroutine(other)
	{
		// NOP
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

	struct ChannelHolder : public Coroutine::Holder
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
class sprawl::threading::CoroutineWithChannel<SendType, void> : public sprawl::threading::Coroutine
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: Coroutine(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(Coroutine const& other)
		: Coroutine(other)
	{
		// NOP
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

	using Coroutine::operator();

	void operator()(SendType const& value)
	{
		Send(value);
	}

	void operator()(SendType&& value)
	{
		Send(std::move(value));
	}

private:

	struct ChannelHolder : public Coroutine::Holder
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
class sprawl::threading::CoroutineWithChannel<void, ReceiveType> : public sprawl::threading::Coroutine
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: Coroutine(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(Coroutine const& other)
		: Coroutine(other)
	{
		// NOP
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
		Coroutine::Resume();
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
	struct ChannelHolder : public Coroutine::Holder
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
class sprawl::threading::CoroutineWithChannel<void, void> : public sprawl::threading::Coroutine
{

};

template<typename SendType, typename ReceiveType>
/*static*/ SendType& sprawl::threading::Coroutine::Receive(ReceiveType const& value)
{
	Coroutine crt = *Coroutine::ms_thisThreadCoroutine;
	CoroutineWithChannel<SendType, ReceiveType> withChannel = crt;
	return withChannel.Receive(value);
}

template<typename SendType>
/*static*/ SendType& sprawl::threading::Coroutine::Receive()
{
	Coroutine crt = *Coroutine::ms_thisThreadCoroutine;
	CoroutineWithChannel<SendType, void> withChannel = crt;
	return withChannel.Receive();
}

template<typename ReceiveType>
/*static*/ void sprawl::threading::Coroutine::Yield(ReceiveType const& value)
{
	Coroutine crt = *Coroutine::ms_thisThreadCoroutine;
	CoroutineWithChannel<void, ReceiveType> withChannel = crt;
	withChannel.Yield(value);
}
