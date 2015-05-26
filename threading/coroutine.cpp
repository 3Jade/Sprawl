#include "coroutine.hpp"


/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::Coroutine*> sprawl::threading::Coroutine::ms_coroutineInitHelper;
/*static*/ sprawl::threading::ThreadLocal<sprawl::threading::Coroutine> sprawl::threading::Coroutine::ms_thisThreadCoroutine;

/*static*/ void sprawl::threading::Coroutine::Yield()
{
	Coroutine routine = *Coroutine::ms_thisThreadCoroutine;
	routine.Pause();
}

sprawl::threading::Coroutine::Holder* sprawl::threading::Coroutine::Holder::Create()
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	Holder* ret = (Holder*)holderAlloc::alloc();
	new(ret) Holder();
	return ret;
}

sprawl::threading::Coroutine::Holder* sprawl::threading::Coroutine::Holder::Create(std::function<void()> function, size_t stackSize)
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	Holder* ret = (Holder*)holderAlloc::alloc();
	new(ret) Holder(function, stackSize);
	return ret;
}

void sprawl::threading::Coroutine::Holder::Release()
{
	typedef memory::DynamicPoolAllocator<sizeof(Holder)> holderAlloc;

	this->~Holder();
	holderAlloc::free(this);
}

void sprawl::threading::Coroutine::Holder::IncRef()
{
	++m_refCount;
}

bool sprawl::threading::Coroutine::Holder::DecRef()
{
	return (--m_refCount == 0);
}

sprawl::threading::Coroutine::Coroutine(std::function<void()> function, size_t stackSize)
	: m_holder(Holder::Create(function, stackSize))
{
	if(!ms_thisThreadCoroutine)
	{
		ms_thisThreadCoroutine = Coroutine();
	}
}

sprawl::threading::Coroutine::Coroutine()
	: m_holder(Holder::Create())
{
	// NOP
}

sprawl::threading::Coroutine::Coroutine(sprawl::threading::Coroutine::Holder* holder)
	: m_holder(holder)
{
	if(m_holder && !ms_thisThreadCoroutine)
	{
		ms_thisThreadCoroutine = Coroutine();
	}
}

sprawl::threading::Coroutine::Coroutine(Coroutine const& other)
	: m_holder(other.m_holder)
{
	m_holder->IncRef();
}

sprawl::threading::Coroutine& sprawl::threading::Coroutine::operator =(Coroutine const& other)
{
	if(m_holder && m_holder->DecRef())
	{
		m_holder->Release();
	}
	m_holder = other.m_holder;
	m_holder->IncRef();
	return *this;
}

sprawl::threading::Coroutine::~Coroutine()
{
	if(m_holder && m_holder->DecRef())
	{
		m_holder->Release();
	}
}

sprawl::threading::Coroutine::CoroutineState sprawl::threading::Coroutine::State()
{
	return m_holder->m_state;
}

void sprawl::threading::Coroutine::run_()
{
	m_holder->m_function();
	m_holder->m_state = CoroutineState::Completed;
	m_holder->m_priorCoroutine.reactivate_();
}

/*static*/ void sprawl::threading::Coroutine::entryPoint_()
{
	ms_coroutineInitHelper->run_();
}
