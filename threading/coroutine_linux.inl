#include <sys/mman.h>
#ifdef __APPLE__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#	include <ucontext.h>
#endif

template<typename ReturnType>
sprawl::threading::CoroutineBase::Holder<ReturnType>::Holder()
	: m_function(nullptr)
	, m_stackSize(0)
	, m_stack(nullptr)
	, m_stackPointer(nullptr)
	, m_state(CoroutineState::Created)
	, m_context()
	, m_refCount(1)
	, m_priorCoroutine(nullptr)
{
	m_stackPointer = &m_context;

	getcontext(&m_context);
}

template<typename ReturnType>
/*virtual*/ sprawl::threading::CoroutineBase::Holder<ReturnType>::~Holder()
{
	if(m_stack)
	{
		munmap(m_stack, m_stackSize);
	}
}

template<typename ReturnType>
sprawl::threading::CoroutineBase::Holder<ReturnType>::Holder(std::function<ReturnType()> function, size_t stackSize)
	: m_function(function)
	, m_stackSize(stackSize == 0 ? 1024 * 1024 : stackSize)
	, m_stack(nullptr)
	, m_stackPointer(nullptr)
	, m_state(CoroutineState::Created)
	, m_context()
	, m_refCount(1)
	, m_priorCoroutine(nullptr)
{
	m_stack = mmap(NULL, m_stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

	m_stackPointer = &m_context;

	getcontext(&m_context);

	m_context.uc_link = nullptr;
	m_context.uc_stack.ss_sp = m_stack;
	m_context.uc_stack.ss_size = m_stackSize;

	makecontext(&m_context, &CoroutineBase::entryPoint_, 0);
}

#ifdef __APPLE__
#	pragma GCC diagnostic pop
#endif