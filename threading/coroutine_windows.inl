#include <Windows.h>

namespace sprawl
{
	namespace threading
	{
		namespace detail
		{
			inline VOID CALLBACK EntryPointWin32(PVOID lpParameter)
			{
				typedef void(*entryPoint)();
				entryPoint ep = (entryPoint)(lpParameter);
				ep();
			}
		}
	}
}

template<typename ReturnType>
sprawl::threading::CoroutineBase::Holder<ReturnType>::Holder()
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

template<typename ReturnType>
/*virtual*/ sprawl::threading::CoroutineBase::Holder<ReturnType>::~Holder()
{
	// NOP
}

template<typename ReturnType>
sprawl::threading::CoroutineBase::Holder<ReturnType>::Holder(std::function<ReturnType()> function, size_t stackSize)
	: m_function(function)
	, m_stackSize(stackSize)
	, m_stack(nullptr)
	, m_stackPointer(nullptr)
	, m_state(CoroutineState::Created)
	, m_refCount(1)
	, m_priorCoroutine(nullptr)
{
	m_stackPointer = CreateFiberEx(0, m_stackSize, 0, &detail::EntryPointWin32, &CoroutineBase::entryPoint_);
}