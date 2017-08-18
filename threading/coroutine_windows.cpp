#include "coroutine.hpp"
#include "threadlocal.hpp"

void sprawl::threading::CoroutineBase::Resume()
{
	m_holder->m_state = CoroutineState::Executing;

	m_holder->m_priorCoroutine = *ms_thisThreadCoroutine;

	ms_thisThreadCoroutine = *this;

	m_holder->m_priorCoroutine.m_holder->m_state = CoroutineState::Paused;

	ms_coroutineInitHelper = this;
	SwitchToFiber(m_holder->m_stackPointer);
#if SPRAWL_EXCEPTIONS_ENABLED
	if(m_holder->m_exception)
	{
		std::rethrow_exception(m_holder->m_exception);
	}
#endif
}

void sprawl::threading::CoroutineBase::reactivate_()
{
	m_holder->m_state = CoroutineState::Executing;

	CoroutineBase currentlyActiveCoroutine = *ms_thisThreadCoroutine;
	ms_thisThreadCoroutine = *this;

	currentlyActiveCoroutine.releaseRef_();
	SwitchToFiber(m_holder->m_stackPointer);
#if SPRAWL_EXCEPTIONS_ENABLED
	if(m_holder->m_exception)
	{
		std::rethrow_exception(m_holder->m_exception);
	}
#endif
}

void sprawl::threading::CoroutineBase::Pause()
{
	m_holder->m_state = CoroutineState::Paused;

	ms_thisThreadCoroutine = m_holder->m_priorCoroutine;

	m_holder->m_priorCoroutine.m_holder->m_state = CoroutineState::Executing;

	SwitchToFiber(m_holder->m_priorCoroutine.m_holder->m_stackPointer);
}
