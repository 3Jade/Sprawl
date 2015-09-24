#include "coroutine.hpp"
#include "threadlocal.hpp"
#include <Windows.h>

namespace CoroutineStatic
{
	static VOID CALLBACK EntryPoint(PVOID lpParameter)
	{
		typedef void(*entryPoint)();
		entryPoint ep = (entryPoint)(lpParameter);
		ep();
	}
}

sprawl::threading::CoroutineBase::Holder::Holder()
	: m_function(nullptr)
	, m_stackSize(0)
	, m_stack(nullptr)
	, m_stackPointer(nullptr)
	, m_state(CoroutineState::Created)
	, m_refCount(1)
	, m_priorCoroutine(nullptr)
{
	if(!IsThreadAFiber())
	{
		ConvertThreadToFiber(nullptr);
	}
	m_stackPointer = GetCurrentFiber();
}

sprawl::threading::CoroutineBase::Holder::Holder(std::function<void()> function, size_t /*stackSize*/)
	: m_function(function)
	, m_stackSize(0)
	, m_stack(nullptr)
	, m_stackPointer(nullptr)
	, m_state(CoroutineState::Created)
	, m_refCount(1)
	, m_priorCoroutine(nullptr)
{
	m_stackPointer = CreateFiberEx(0, m_stackSize, 0, &CoroutineStatic::EntryPoint, &CoroutineBase::entryPoint_);
}

void sprawl::threading::CoroutineBase::Resume()
{
	m_holder->m_state = CoroutineState::Executing;

	m_holder->m_priorCoroutine = *ms_thisThreadCoroutine;

	ms_thisThreadCoroutine = *this;

	m_holder->m_priorCoroutine.m_holder->m_state = CoroutineState::Paused;

	ms_coroutineInitHelper = this;
	SwitchToFiber(m_holder->m_stackPointer);
}

void sprawl::threading::CoroutineBase::reactivate_()
{
	m_holder->m_state = CoroutineState::Executing;

	CoroutineBase currentlyActiveCoroutine = *ms_thisThreadCoroutine;
	ms_thisThreadCoroutine = *this;

	currentlyActiveCoroutine.releaseRef_();
	SwitchToFiber(m_holder->m_stackPointer);
}

void sprawl::threading::CoroutineBase::Pause()
{
	m_holder->m_state = CoroutineState::Paused;

	ms_thisThreadCoroutine = m_holder->m_priorCoroutine;

	m_holder->m_priorCoroutine.m_holder->m_state = CoroutineState::Executing;

	SwitchToFiber(m_holder->m_priorCoroutine.m_holder->m_stackPointer);
}
