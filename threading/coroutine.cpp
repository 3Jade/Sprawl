#include "coroutine.hpp"


/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::CoroutineBase*> sprawl::threading::CoroutineBase::ms_coroutineInitHelper;
/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::CoroutineBase> sprawl::threading::CoroutineBase::ms_thisThreadCoroutine;

/*static*/ void sprawl::threading::CoroutineBase::Yield()
{
	CoroutineBase routine = *CoroutineBase::ms_thisThreadCoroutine;
	routine.releaseRef_();
	routine.Pause();
}

sprawl::threading::CoroutineBase::Holder* sprawl::threading::CoroutineBase::Holder::Create()
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	Holder* ret = (Holder*)holderAlloc::alloc();
	new(ret) Holder();
	return ret;
}

sprawl::threading::CoroutineBase::Holder* sprawl::threading::CoroutineBase::Holder::Create(std::function<void()> function, size_t stackSize)
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	Holder* ret = (Holder*)holderAlloc::alloc();
	new(ret) Holder(function, stackSize);
	return ret;
}

void sprawl::threading::CoroutineBase::Holder::Release()
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	this->~Holder();
	holderAlloc::free(this);
}

void sprawl::threading::CoroutineBase::Holder::IncRef()
{
	++m_refCount;
}

bool sprawl::threading::CoroutineBase::Holder::DecRef()
{
	return (--m_refCount == 0);
}

sprawl::threading::CoroutineBase::CoroutineBase()
	: m_holder(Holder::Create())
	, m_ownsHolder(true)
{
	// NOP
}

sprawl::threading::CoroutineBase::CoroutineBase(sprawl::threading::CoroutineBase::Holder* holder)
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
	if(m_ownsHolder)
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

sprawl::threading::CoroutineBase::CoroutineState sprawl::threading::CoroutineBase::State()
{
	return m_holder->m_state;
}

void sprawl::threading::CoroutineBase::run_()
{
	m_holder->m_function();
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