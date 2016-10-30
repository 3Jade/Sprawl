#pragma once

#include <functional>
#include "threadlocal.hpp"
#include <atomic>
#include "../memory/PoolAllocator.hpp"
#include "../common/type_traits.hpp"
#include "../common/errors.hpp"

#ifdef _WIN32
#	undef Yield
#else
	#ifdef __APPLE__
		#include <sys/ucontext.h>
	#else
		#include <ucontext.h>
	#endif
#endif

#ifndef SPRAWL_COROUTINE_SAFETY_CHECKS
#	define SPRAWL_COROUTINE_SAFETY_CHECKS SPRAWL_DEBUG
#endif

namespace sprawl
{
	namespace threading
	{
		class CoroutineBase;
		class Coroutine;

		template<typename SendType, typename YieldType>
		class CoroutineWithChannel;

		template<typename ReturnType>
		using Generator = CoroutineWithChannel<void, ReturnType>;

		template<typename ReturnType, typename CoroutineType>
		class CoroutineIterator;

		enum class CoroutineState
		{
			Invalid,
			Created,
			Executing,
			Paused,
			Completed,
		};

		enum class CoroutineType
		{
			Basic,
			SendOnly,
			ReceiveOnly,
			BiDirectional,
		};
	}
}

////////////////////////////////////////////////////////////////////////////////
/// CoroutineBase
////////////////////////////////////////////////////////////////////////////////

class sprawl::threading::CoroutineBase
{
public:

	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> Yield();

	template<typename SendType, typename YieldType>
	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> Receive(YieldType const& value);
	template<typename SendType, typename YieldType>
	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> Receive(YieldType&& value, typename std::enable_if<!std::is_reference<YieldType>::value>::type* = 0);

	template<typename SendType>
	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> Receive();

	template<typename YieldType>
	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> Yield(YieldType const& value);
	template<typename YieldType>
	static SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> Yield(YieldType&& value, typename std::enable_if<!std::is_reference<YieldType>::value>::type* = 0);

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
	size_t StackSize();

	static CoroutineBase GetCurrentCoroutine();
	CoroutineBase GetCallingCoroutine();

	bool operator==(CoroutineBase const& other) const
	{
		return m_holder == other.m_holder;
	}

	bool operator!=(CoroutineBase const& other) const
	{
		return !operator==(other);
	}

	CoroutineType Type();

protected:
	template<typename YieldType, typename CoroutineType>
	friend class CoroutineIterator;

	static ThreadLocal<CoroutineBase*> ms_coroutineInitHelper;
	static ThreadLocal<CoroutineBase> ms_thisThreadCoroutine;

	static void entryPoint_();
	void run_();
	void reactivate_();
	void releaseRef_();

	template<typename ReturnType>
	struct Holder;

	explicit CoroutineBase(Holder<void>* holder);

	Holder<void>* m_holder;
	bool m_ownsHolder;
};

template<typename ReturnType>
struct sprawl::threading::CoroutineBase::Holder
{
	inline void IncRef()
	{
		++m_refCount;
	}

	inline bool DecRef()
	{
		return (--m_refCount == 0);
	}

	std::function<ReturnType ()> m_function;
	size_t m_stackSize;
	void* m_stack;
	void* m_stackPointer;
	CoroutineState m_state;

	#ifndef _WIN32
		ucontext_t m_context;
	#endif

	std::atomic<int> m_refCount;

	CoroutineBase m_priorCoroutine;

#if SPRAWL_EXCEPTIONS_ENABLED
	std::exception_ptr m_exception;
#endif

	static inline Holder<ReturnType>* Create(std::function<ReturnType()> function, size_t stackSize)
	{
		typedef memory::PoolAllocator<sizeof(Holder)> holderAlloc;

		Holder* ret = (Holder*)holderAlloc::alloc();
		new(ret)Holder(function, stackSize);
		return ret;
	}

	static inline Holder<ReturnType>* Create()
	{
		typedef memory::PoolAllocator<sizeof(Holder)> holderAlloc;

		Holder* ret = (Holder*)holderAlloc::alloc();
		new(ret)Holder();
		return ret;
	}

	virtual void Release()
	{
		typedef memory::PoolAllocator<sizeof(Holder)> holderAlloc;

		this->~Holder();
		holderAlloc::free(this);
	}

	virtual CoroutineType Type() { return CoroutineType::Basic; }

	virtual size_t SizeOfSendType() { return 0; }
	virtual size_t SizeOfYieldType() { return 0; }

	virtual void RunFunction() { m_function(); }

	virtual ~Holder();
protected:
	Holder();
	Holder(std::function<ReturnType ()> function, size_t stackSize);
};

////////////////////////////////////////////////////////////////////////////////
/// CoroutineIterator class
////////////////////////////////////////////////////////////////////////////////

template<typename YieldType, typename CoroutineType>
class sprawl::threading::CoroutineIterator : public std::iterator<std::forward_iterator_tag, YieldType, std::ptrdiff_t, YieldType*, YieldType&>
{
public:
	CoroutineIterator(CoroutineBase& routine);
	CoroutineIterator(CoroutineBase& routine, CoroutineState state);
	CoroutineIterator& operator++();
	bool operator==(CoroutineIterator const& other);
	bool operator!=(CoroutineIterator const& other);

	YieldType* operator->();
	YieldType& operator*();
private:
	CoroutineBase& m_routine;
	YieldType m_value;
	CoroutineState m_state;
};

////////////////////////////////////////////////////////////////////////////////
/// Coroutine - Basic start/pause Configuration
////////////////////////////////////////////////////////////////////////////////

class sprawl::threading::Coroutine : public sprawl::threading::CoroutineBase
{
public:
	Coroutine()
		: CoroutineBase()
	{
		//
	}

	Coroutine(std::function<void()> function, size_t stackSize = 0)
		: CoroutineBase(CoroutineBase::Holder<void>::Create(function, stackSize))
	{
		//
	}

	Coroutine(std::nullptr_t npt, size_t stackSize = 0)
		: CoroutineBase(CoroutineBase::Holder<void>::Create(npt, stackSize))
	{
		//
	}

	Coroutine(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		//
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!type_traits::HasEmptyOperatorParens<Callable>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
			&& !std::is_same<std::nullptr_t, Callable>::value
		>::type
	>
	Coroutine(Callable && callable, Params &&... params)
		: CoroutineBase(CoroutineBase::Holder<void>::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}
};

////////////////////////////////////////////////////////////////////////////////
/// CoroutineWithChannel<SendType, YieldType> - Bidirectional Configuration
////////////////////////////////////////////////////////////////////////////////

template<typename SendType, typename YieldType>
class sprawl::threading::CoroutineWithChannel : public sprawl::threading::CoroutineBase
{
public:
	CoroutineWithChannel(std::function<YieldType()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(std::nullptr_t npt, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(npt, stackSize))
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
			!type_traits::HasEmptyOperatorParens<Callable>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
			&& !std::is_same<std::nullptr_t, Callable>::value
		>::type
	>
	CoroutineWithChannel(Callable && callable, Params &&... params)
		: CoroutineBase(ChannelHolder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}

	CoroutineWithChannel()
		: CoroutineBase()
	{
		// NOP
	}

	SendType& Receive(YieldType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = value;
		Pause();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue;
	}

	YieldType& Send(SendType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = value;
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	SendType& Receive(YieldType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = std::move(value);
		Pause();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue;
	}

	YieldType& Send(SendType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_sentValue = std::move(value);
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	YieldType& Start()
	{
		Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	YieldType& operator()()
	{
		return Start();
	}

	YieldType& operator()(SendType const& value)
	{
		return Send(value);
	}

	YieldType& operator()(SendType&& value)
	{
		return Send(std::move(value));
	}

private:

	struct ChannelHolder : public CoroutineBase::Holder<YieldType>
	{
		static_assert(sizeof(CoroutineBase::Holder<void>) == sizeof(CoroutineBase::Holder<YieldType>), "Holder size must not change based on template type.");
		static CoroutineBase::Holder<void>* Create(std::function<YieldType()> function, size_t stackSize)
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret)ChannelHolder(function, stackSize);
			return reinterpret_cast<CoroutineBase::Holder<void>*>(ret);
		}

		virtual void Release() override
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		virtual CoroutineType Type() override { return CoroutineType::BiDirectional; }

		virtual size_t SizeOfSendType() override { return sizeof(SendType); }
		virtual size_t SizeOfYieldType() override { return sizeof(YieldType); }

		virtual void RunFunction() override
		{
			m_receivedValue = this->m_function();
		}

		ChannelHolder(std::function<YieldType ()> function, size_t stackSize)
			: Holder<YieldType>(function, stackSize)
			, m_sentValue()
			, m_receivedValue()
		{
			// NOP
		}

		SendType m_sentValue;
		YieldType m_receivedValue;
	};
};

////////////////////////////////////////////////////////////////////////////////
/// CoroutineWithChannel<SendType, void> - Send-Only Configuration
////////////////////////////////////////////////////////////////////////////////

template<typename SendType>
class sprawl::threading::CoroutineWithChannel<SendType, void> : public sprawl::threading::CoroutineBase
{
public:
	CoroutineWithChannel(std::function<void ()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(std::nullptr_t npt, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(npt, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		// NOP
	}

	CoroutineWithChannel()
		: CoroutineBase()
	{
		// NOP
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!type_traits::HasEmptyOperatorParens<Callable>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
			&& !std::is_same<std::nullptr_t, Callable>::value
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

	struct ChannelHolder : public CoroutineBase::Holder<void>
	{
		static CoroutineBase::Holder<void>* Create(std::function<void()> function, size_t stackSize = 0)
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret) ChannelHolder(function, stackSize);
			return ret;
		}

		virtual void Release() override
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		virtual CoroutineType Type() override { return CoroutineType::SendOnly; }

		virtual size_t SizeOfSendType() override { return sizeof(SendType); }

		ChannelHolder(std::function<void()> function, size_t stackSize)
			: Holder(function, stackSize)
			, m_sentValue()
		{
			// NOP
		}

		SendType m_sentValue;
	};
};

////////////////////////////////////////////////////////////////////////////////
/// CoroutineWithChannel<void, YieldType> - Yield-Only Configuration
////////////////////////////////////////////////////////////////////////////////

template<typename YieldType>
class sprawl::threading::CoroutineWithChannel<void, YieldType> : public sprawl::threading::CoroutineBase
{
public:
	typedef CoroutineIterator<YieldType, CoroutineWithChannel<void, YieldType>> iterator;

	CoroutineWithChannel(std::function<YieldType ()> function, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(function, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(std::nullptr_t npt, size_t stackSize = 0)
		: CoroutineBase(ChannelHolder::Create(npt, stackSize))
	{
		// NOP
	}

	CoroutineWithChannel(CoroutineBase const& other)
		: CoroutineBase(other)
	{
		// NOP
	}

	CoroutineWithChannel()
		: CoroutineBase()
	{
		// NOP
	}

	template<
		typename Callable,
		typename... Params,
		typename = typename std::enable_if<
			!type_traits::HasEmptyOperatorParens<Callable>::value
			&& !std::is_base_of<CoroutineBase, typename std::remove_reference<Callable>::type>::value
			&& !std::is_same<std::nullptr_t, Callable>::value
		>::type
	>
	CoroutineWithChannel(Callable && callable, Params &&... params)
		: CoroutineBase(ChannelHolder::Create(std::bind(std::forward<Callable>(callable), std::forward<Params>(params)...), 0))
	{
		//
	}

	void Yield(YieldType const& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = value;
		Pause();
	}

	void Yield(YieldType&& value)
	{
		reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue = std::move(value);
		Pause();
	}

	YieldType& Resume()
	{
		CoroutineBase::Resume();
		return reinterpret_cast<ChannelHolder*>(m_holder)->m_receivedValue;
	}

	YieldType& Start()
	{
		return Resume();
	}

	YieldType& operator()()
	{
		return Resume();
	}

	iterator begin()
	{
		return iterator(*this);
	}

	iterator end()
	{
		return iterator(*this, CoroutineState::Completed);
	}

private:
	struct ChannelHolder : public CoroutineBase::Holder<YieldType>
	{
		static_assert(sizeof(CoroutineBase::Holder<void>) == sizeof(CoroutineBase::Holder<YieldType>), "Holder size must not change based on template type.");
		static CoroutineBase::Holder<void>* Create(std::function<YieldType()> function, size_t stackSize)
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			ChannelHolder* ret = (ChannelHolder*)holderAlloc::alloc();
			new(ret) ChannelHolder(function, stackSize);
			return reinterpret_cast<CoroutineBase::Holder<void>*>(ret);
		}

		virtual void Release() override
		{
			typedef memory::PoolAllocator<sizeof(ChannelHolder)> holderAlloc;

			this->~ChannelHolder();
			holderAlloc::free(this);
		}

		virtual CoroutineType Type() override { return CoroutineType::ReceiveOnly; }

		virtual size_t SizeOfYieldType() override { return sizeof(YieldType); }

		virtual void RunFunction() override
		{
			m_receivedValue = this->m_function();
		}

		ChannelHolder(std::function<YieldType ()> function, size_t stackSize)
			: Holder<YieldType>(function, stackSize)
			, m_receivedValue()
		{
			// NOP
		}

		YieldType m_receivedValue;
	};
};

////////////////////////////////////////////////////////////////////////////////
/// CoroutineWithChannel<void, void> - Invalid Configuration
////////////////////////////////////////////////////////////////////////////////

template<>
class sprawl::threading::CoroutineWithChannel<void, void> : public sprawl::threading::CoroutineBase
{

};

////////////////////////////////////////////////////////////////////////////////
/// Static Functions
////////////////////////////////////////////////////////////////////////////////

template<typename SendType, typename YieldType>
/*static*/ SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> sprawl::threading::CoroutineBase::Receive(YieldType const& value)
{
	CoroutineWithChannel<SendType, YieldType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(withChannel.m_holder->Type() != CoroutineType::BiDirectional)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
	if(withChannel.m_holder->SizeOfSendType() != sizeof(SendType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidSendType());
	}
	if(withChannel.m_holder->SizeOfYieldType() != sizeof(YieldType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidYieldType());
	}
#endif

	withChannel.releaseRef_();
	return withChannel.Receive(value);
}

template<typename SendType, typename YieldType>
/*static*/ SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> sprawl::threading::CoroutineBase::Receive(YieldType&& value, typename std::enable_if<!std::is_reference<YieldType>::value>::type*)
{
	CoroutineWithChannel<SendType, YieldType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(withChannel.m_holder->Type() != CoroutineType::BiDirectional)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
	if(withChannel.m_holder->SizeOfSendType() != sizeof(SendType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidSendType());
	}
	if(withChannel.m_holder->SizeOfYieldType() != sizeof(YieldType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidYieldType());
	}
#endif

	withChannel.releaseRef_();
	return withChannel.Receive(std::move(value));
}

template<typename SendType>
/*static*/ SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<SendType&> sprawl::threading::CoroutineBase::Receive()
{
	CoroutineWithChannel<SendType, void> withChannel = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(withChannel.m_holder->Type() != CoroutineType::SendOnly)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
	if(withChannel.m_holder->SizeOfSendType() != sizeof(SendType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidSendType());
	}
#endif

	withChannel.releaseRef_();
	return withChannel.Receive();
}

template<typename YieldType>
/*static*/ SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> sprawl::threading::CoroutineBase::Yield(YieldType const& value)
{
	CoroutineWithChannel<void, YieldType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(withChannel.m_holder->Type() != CoroutineType::ReceiveOnly)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
	if(withChannel.m_holder->SizeOfYieldType() != sizeof(YieldType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidYieldType());
	}
#endif

	withChannel.releaseRef_();
	withChannel.Yield(value);

	return ErrorState<void>();
}

template<typename YieldType>
/*static*/ SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> sprawl::threading::CoroutineBase::Yield(YieldType&& value, typename std::enable_if<!std::is_reference<YieldType>::value>::type*)
{
	CoroutineWithChannel<void, YieldType> withChannel = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(withChannel.m_holder->Type() != CoroutineType::ReceiveOnly)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
	if(withChannel.m_holder->SizeOfYieldType() != sizeof(YieldType))
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidYieldType());
	}
#endif

	withChannel.releaseRef_();
	withChannel.Yield(std::move(value));

	return ErrorState<void>();
}

/*static*/ inline SPRAWL_WARN_UNUSED_RESULT sprawl::ErrorState<void> sprawl::threading::CoroutineBase::Yield()
{
	CoroutineBase routine = *CoroutineBase::ms_thisThreadCoroutine;

#if SPRAWL_COROUTINE_SAFETY_CHECKS
	if(routine.m_holder->Type() != CoroutineType::Basic)
	{
		SPRAWL_THROW_EXCEPTION(sprawl::InvalidCoroutineType());
	}
#endif

	routine.releaseRef_();
	routine.Pause();

	return ErrorState<void>();
}

////////////////////////////////////////////////////////////////////////////////
/// CoroutineIterator functions
////////////////////////////////////////////////////////////////////////////////

template<typename YieldType, typename CoroutineType>
sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::CoroutineIterator(CoroutineBase& routine)
	: m_routine(routine)
	, m_value(reinterpret_cast<CoroutineType&>(m_routine)())
	, m_state(m_routine.State())
{

}

template<typename YieldType, typename CoroutineType>
sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::CoroutineIterator(CoroutineBase& routine, CoroutineState state)
	: m_routine(routine)
	, m_value()
	, m_state(state)
{

}

template<typename YieldType, typename CoroutineType>
sprawl::threading::CoroutineIterator<YieldType, CoroutineType>& sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::operator++()
{
	m_value = reinterpret_cast<CoroutineType&>(m_routine)();
	m_state = m_routine.State();
	return *this;
}

template<typename YieldType, typename CoroutineType>
bool sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::operator==(CoroutineIterator const& other)
{
	return m_routine.m_holder == other.m_routine.m_holder && m_state == other.m_state;
}

template<typename YieldType, typename CoroutineType>
bool sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::operator!=(CoroutineIterator const& other)
{
	return !operator==(other);
}


template<typename YieldType, typename CoroutineType>
YieldType* sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::operator->()
{
	return &m_value;
}

template<typename YieldType, typename CoroutineType>
YieldType& sprawl::threading::CoroutineIterator<YieldType, CoroutineType>::operator*()
{
	return m_value;
}

////////////////////////////////////////////////////////////////////////////////
/// Macros
////////////////////////////////////////////////////////////////////////////////

#ifndef SPRAWL_NO_COROUTINE_KEYWORDS

#	ifndef SPRAWL_NO_YIELD_KEWYORD
#		if defined(SPRAWL_COROUTINE_KEYWORDS_CRT_PREFIX) || defined(SPRAWL_YIELD_CRT_PREFIX)
#			define crt_yield(...) sprawl::threading::Coroutine::Yield(__VA_ARGS__)
#		else
#			define yield(...) sprawl::threading::Coroutine::Yield(__VA_ARGS__)
#		endif
#	endif

#	ifndef SPRAWL_NO_RECEIVE_KEYWORD
#		if defined(SPRAWL_COROUTINE_KEYWORDS_CRT_PREFIX) || defined(SPRAWL_RECEIVE_CRT_PREFIX)
#			define crt_receive(received) received = sprawl::threading::Coroutine::Receive<decltype(received)>();
#		else
#			define receive(received) received = sprawl::threading::Coroutine::Receive<decltype(received)>();
#		endif
#	endif

#	ifndef SPRAWL_NO_YIELD_RECEIVE_KEYWORD
#		if defined(SPRAWL_COROUTINE_KEYWORDS_CRT_PREFIX) || defined(SPRAWL_YIELD_RECEIVE_CRT_PREFIX)
#			define crt_yield_receive(yielded, received) received = sprawl::threading::Coroutine::Receive<decltype(received)>(yielded);
#		else
#			define yield_receive(yielded, received) received = sprawl::threading::Coroutine::Receive<decltype(received), decltype(yielded)>(yielded);
#		endif
#	endif

#endif

#ifdef _WIN32
#	include "coroutine_windows.inl"
#else
#	include "coroutine_linux.inl"
#endif
