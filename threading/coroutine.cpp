#include "coroutine.hpp"

/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::CoroutineBase*> sprawl::threading::CoroutineBase::ms_coroutineInitHelper;
/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::CoroutineBase> sprawl::threading::CoroutineBase::ms_thisThreadCoroutine;

sprawl::threading::CoroutineBase::CoroutineBase()
	: m_holder(Holder<void>::Create())
	, m_ownsHolder(true)
{
	// NOP
}

sprawl::threading::CoroutineBase::CoroutineBase(sprawl::threading::CoroutineBase::Holder<void>* holder)
	: m_holder(holder)
	, m_ownsHolder(true)
{
	if(m_holder && !ms_thisThreadCoroutine)
	{
		ms_thisThreadCoroutine = CoroutineBase();
	}
}

sprawl::threading::CoroutineBase::CoroutineBase(CoroutineBase const& other)
	: m_holder(other.m_holder)
	, m_ownsHolder(true)
{
	m_holder->IncRef();
}

sprawl::threading::CoroutineBase& sprawl::threading::CoroutineBase::operator =(CoroutineBase const& other)
{
	if(m_ownsHolder && m_holder && m_holder->DecRef())
	{
		m_holder->Release();
	}
	m_holder = other.m_holder;
	if(m_ownsHolder && m_holder)
	{
		m_holder->IncRef();
	}
	return *this;
}

sprawl::threading::CoroutineBase::~CoroutineBase()
{
	if(m_ownsHolder && m_holder && m_holder->DecRef())
	{
		m_holder->Release();
	}
}

sprawl::threading::CoroutineState sprawl::threading::CoroutineBase::State()
{
	return m_holder ? m_holder->m_state : CoroutineState::Invalid;
}

void sprawl::threading::CoroutineBase::run_()
{
#if SPRAWL_EXCEPTIONS_ENABLED
	try
	{
		m_holder->RunFunction();
	}
	catch(...)
	{
		m_holder->m_exception = std::current_exception();
	}
#else
	m_holder->RunFunction();
#endif
	m_holder->m_state = CoroutineState::Completed;
	m_holder->m_priorCoroutine.reactivate_();
}

/*static*/ void sprawl::threading::CoroutineBase::entryPoint_()
{
	ms_coroutineInitHelper->run_();
}

void sprawl::threading::CoroutineBase::releaseRef_()
{
	if(m_holder && m_ownsHolder)
	{
		m_holder->DecRef();
		m_ownsHolder = false;
	}
}

size_t sprawl::threading::CoroutineBase::StackSize()
{
	return m_holder->m_stackSize;
}

/*static*/ sprawl::threading::CoroutineBase sprawl::threading::CoroutineBase::GetCurrentCoroutine()
{
	return *ms_thisThreadCoroutine;
}

sprawl::threading::CoroutineBase sprawl::threading::CoroutineBase::GetCallingCoroutine()
{
	return m_holder->m_priorCoroutine;
}

sprawl::threading::CoroutineType sprawl::threading::CoroutineBase::Type()
{
	return m_holder->Type();
}
