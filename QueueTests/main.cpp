#include "../collections/ConcurrentQueue.hpp"
#include "../time/time.hpp"
#include <tbb/concurrent_queue.h>
#include <boost/lockfree/queue.hpp>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <string>
#include <math.h>
#include <limits>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

constexpr size_t NUM_ELEMENTS = 1000000;

namespace ext_1024cores {
	
	template<typename T>
	class mpmc_bounded_queue
	{
	public:
	  mpmc_bounded_queue(size_t buffer_size)
		: buffer_(new cell_t [buffer_size])
		, buffer_mask_(buffer_size - 1)
	  {
		assert((buffer_size >= 2) &&
		  ((buffer_size & (buffer_size - 1)) == 0));
		for (size_t i = 0; i != buffer_size; i += 1)
		  buffer_[i].sequence_.store(i, std::memory_order_relaxed);
		enqueue_pos_.store(0, std::memory_order_relaxed);
		dequeue_pos_.store(0, std::memory_order_relaxed);
	  }
	
	  ~mpmc_bounded_queue()
	  {
		delete [] buffer_;
	  }
	
	  bool enqueue(T const& data)
	  {
		cell_t* cell;
		size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
		  cell = &buffer_[pos & buffer_mask_];
		  size_t seq =
			cell->sequence_.load(std::memory_order_acquire);
		  intptr_t dif = (intptr_t)seq - (intptr_t)pos;
		  if (dif == 0)
		  {
			if (enqueue_pos_.compare_exchange_weak
				(pos, pos + 1, std::memory_order_relaxed))
			  break;
		  }
		  else if (dif < 0)
			return false;
		  else
			pos = enqueue_pos_.load(std::memory_order_relaxed);
		}
		cell->data_ = data;
		cell->sequence_.store(pos + 1, std::memory_order_release);
		return true;
	  }
	
	  bool dequeue(T& data)
	  {
		cell_t* cell;
		size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
		  cell = &buffer_[pos & buffer_mask_];
		  size_t seq =
			cell->sequence_.load(std::memory_order_acquire);
		  intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
		  if (dif == 0)
		  {
			if (dequeue_pos_.compare_exchange_weak
				(pos, pos + 1, std::memory_order_relaxed))
			  break;
		  }
		  else if (dif < 0)
			return false;
		  else
			pos = dequeue_pos_.load(std::memory_order_relaxed);
		}
		data = cell->data_;
		cell->sequence_.store
		  (pos + buffer_mask_ + 1, std::memory_order_release);
		return true;
	  }
	
	private:
	  struct cell_t
	  {
		std::atomic<size_t>   sequence_;
		T					 data_;
	  };
	
	  static size_t const	 cacheline_size = 64;
	  typedef char			cacheline_pad_t [cacheline_size];
	
	  cacheline_pad_t		 pad0_;
	  cell_t* const		   buffer_;
	  size_t const			buffer_mask_;
	  cacheline_pad_t		 pad1_;
	  std::atomic<size_t>	 enqueue_pos_;
	  cacheline_pad_t		 pad2_;
	  std::atomic<size_t>	 dequeue_pos_;
	  cacheline_pad_t		 pad3_;
	
	  mpmc_bounded_queue(mpmc_bounded_queue const&);
	  void operator = (mpmc_bounded_queue const&);
	};
}

enum class TicketType
{
	PERSISTENT,
	EPHEMERAL,
	NONE
};

template<typename t_QueueType, TicketType t_TicketType = TicketType::PERSISTENT>
class QueueWrapper;

template<typename t_ElementType>
class QueueWrapper<ext_1024cores::mpmc_bounded_queue<t_ElementType>, TicketType::PERSISTENT>
{
public:
	QueueWrapper()
		: m_queue(pow(2, ceil(log(NUM_ELEMENTS)/log(2))))
	{
		
	}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.dequeue(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.dequeue(data);
		}
	}

private:
	ext_1024cores::mpmc_bounded_queue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<tbb::concurrent_bounded_queue<t_ElementType>, TicketType::PERSISTENT>
{
public:
	QueueWrapper()
		: m_queue()
	{
		m_queue.set_capacity(NUM_ELEMENTS);
	}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.try_pop(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.try_pop(data);
		}
	}
private:
	tbb::concurrent_bounded_queue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<tbb::concurrent_queue<t_ElementType>, TicketType::PERSISTENT>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.try_pop(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.try_pop(data);
		}
	}
private:
	tbb::concurrent_queue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<boost::lockfree::queue<t_ElementType>, TicketType::PERSISTENT>
{
public:
	QueueWrapper()
		: m_queue(NUM_ELEMENTS)
	{
		
	}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.pop(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.pop(data);
		}
	}
private:
	boost::lockfree::queue<t_ElementType> m_queue;
};

template <typename t_ElementType>
using BoostBoundedQueue = boost::lockfree::queue<t_ElementType, boost::lockfree::fixed_sized<true>, boost::lockfree::capacity<NUM_ELEMENTS>>;

template<typename t_ElementType>
class QueueWrapper<BoostBoundedQueue<t_ElementType>, TicketType::PERSISTENT>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.push(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.pop(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.pop(data);
		}
	}
private:
	BoostBoundedQueue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<std::deque<t_ElementType>, TicketType::PERSISTENT>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			std::lock_guard<std::mutex> lock(m_mtx);
			m_queue.push_back(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			std::lock_guard<std::mutex> lock(m_mtx);
			m_queue.push_back(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			for (;;)
			{
				std::lock_guard<std::mutex> lock(m_mtx);
				if (m_queue.empty())
				{
					continue;
				}
				data = m_queue.front();
				m_queue.pop_front();
				break;
			}
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			if (m_queue.empty())
			{
				continue;
			}
			data = m_queue.front();
			m_queue.pop_front();
		}
	}
private:
	std::mutex m_mtx;
	std::deque<t_ElementType> m_queue;
};

template<typename t_ElementType, TicketType t_TicketType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType>, t_TicketType>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		sprawl::collections::ReadReservationTicket<t_ElementType> ticket;
		m_queue.InitializeReservationTicket(ticket);

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.Dequeue(data, ticket)) {}
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		sprawl::collections::ReadReservationTicket<t_ElementType> ticket;
		m_queue.InitializeReservationTicket(ticket);

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data, ticket);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::EPHEMERAL>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			sprawl::collections::ReadReservationTicket<t_ElementType> ticket;
			m_queue.InitializeReservationTicket(ticket);
			while (!m_queue.Dequeue(data, ticket)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		sprawl::collections::ReadReservationTicket<t_ElementType> ticket;
		m_queue.InitializeReservationTicket(ticket);

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data, ticket);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::NONE>
{
public:
	QueueWrapper()
		: m_queue(1024)
	{}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.Dequeue(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType> m_queue;
};

template<typename t_ElementType, TicketType t_TicketType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, t_TicketType>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		sprawl::collections::ReadReservationTicket<t_ElementType, NUM_ELEMENTS> ticket;
		m_queue.InitializeReservationTicket(ticket);

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.Dequeue(data, ticket)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		sprawl::collections::ReadReservationTicket<t_ElementType, NUM_ELEMENTS> ticket;
		m_queue.InitializeReservationTicket(ticket);

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data, ticket);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::EPHEMERAL>
{
public:
	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			sprawl::collections::ReadReservationTicket<t_ElementType, NUM_ELEMENTS> ticket;
			m_queue.InitializeReservationTicket(ticket);
			while (!m_queue.Dequeue(data, ticket)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		sprawl::collections::ReadReservationTicket<t_ElementType, NUM_ELEMENTS> ticket;

		m_queue.InitializeReservationTicket(ticket);
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data, ticket);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS> m_queue;
};

template<typename t_ElementType>
class QueueWrapper<sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>
{
public:
	QueueWrapper()
		: m_queue(1024)
	{}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue.Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue.Dequeue(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue.Dequeue(data);
		}
	}
private:
	sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS> m_queue;
};

template<typename t_ElementType, TicketType t_TicketType>
class QueueWrapper<sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>, t_TicketType>
{
public:
	QueueWrapper()
		: m_queue(new sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>())
	{}
	
	~QueueWrapper()
	{
		delete m_queue;
	}

	void enqueue(size_t nElements)
	{
		sprawl::collections::BoundedWriteReservationTicket<t_ElementType> ticket;

		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue->Enqueue(data, ticket);
		}
	}
	void enqueueMove(size_t nElements)
	{
		sprawl::collections::BoundedWriteReservationTicket<t_ElementType> ticket;

		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue->Enqueue(std::move(data), ticket);
		}
	}
	void dequeue(size_t nElements)
	{
		sprawl::collections::BoundedReadReservationTicket<t_ElementType> ticket;

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue->Dequeue(data, ticket)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		sprawl::collections::BoundedReadReservationTicket<t_ElementType> ticket;

		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue->Dequeue(data, ticket);
		}
	}
private:
	sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>* m_queue;
};


template<typename t_ElementType>
class QueueWrapper<sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>
{
public:
	QueueWrapper()
		: m_queue(new sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>(1024, 1024))
	{}

	~QueueWrapper()
	{
		delete m_queue;
	}

	void enqueue(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue->Enqueue(data);
		}
	}
	void enqueueMove(size_t nElements)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			t_ElementType data = t_ElementType();
			m_queue->Enqueue(std::move(data));
		}
	}
	void dequeue(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			while (!m_queue->Dequeue(data)) {};
		}
	}
	void dequeueEmpty(size_t nElements)
	{
		t_ElementType data = t_ElementType();
		for (size_t i = 0; i < nElements; ++i)
		{
			m_queue->Dequeue(data);
		}
	}
private:
	sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>* m_queue;
};
template<typename t_Type>
struct TypeName
{
private:
	static constexpr size_t prefix_size = sizeof("static std::string TypeName<") - 1;
public:
	static std::string GetName(bool useMove, TicketType ticketType)
	{
#ifdef _WIN32
		std::string ret = __FUNCTION__;
		ret = ret.substr(ret.find('<')+7);
#else
		std::string ret = __PRETTY_FUNCTION__;
		ret = ret.substr(ret.find('<')+1);
#endif
		ret = ret.substr(0, ret.find("GetName") - 4);
		if (useMove)
		{
			ret += " [moves]";
		}
		switch(ticketType)
		{
		case TicketType::EPHEMERAL:
			ret += " [Ephemeral Tickets]";
			break;
		case TicketType::NONE:
			ret += " [No Tickets]";
			break;
		default:
			break;
		}
		return ret;
	}
};

std::atomic<int64_t> timer(-1);
std::atomic<int64_t> started(0);

void timeFn(std::function<void()> fn)
{
	++started;
	while(timer.load() == -1) {}
	fn();
	timer.store(sprawl::time::SteadyNow());
}

int64_t mean(std::vector<int64_t> const& data)
{
	int64_t total = 0;
	for(auto& item : data)
	{
		total += item;
	}
	return total / data.size();
}

int64_t Max(std::vector<int64_t> const& data)
{
	int64_t val = 0;
	for(auto& item : data)
	{
		val = val > item ? val : item;
	}
	return val;
}

int64_t Min(std::vector<int64_t> const& data)
{
	int64_t val = (std::numeric_limits<int64_t>::max)();
	for(auto& item : data)
	{
		val = val < item ? val : item;
	}
	return val;
}

double OpsPerSecond(int64_t duration, size_t numOps)
{
	double avgNanosPerOp = double(duration) / numOps;
	double opsPerSecond = sprawl::time::Convert(1, sprawl::time::Resolution::Seconds, sprawl::time::Resolution::Nanoseconds) / avgNanosPerOp;
	return opsPerSecond;
}

constexpr int nIters = 5;

template<typename t_ElementType, typename t_QueueType, TicketType t_TicketType = TicketType::PERSISTENT>
void RunTestsOnQueueTypeWithThreadCounts(size_t enqueueThreads, size_t dequeueThreads, bool useMoves = false)
{
	size_t adjustedNumElements = NUM_ELEMENTS;
	while(adjustedNumElements % enqueueThreads != 0 || adjustedNumElements % dequeueThreads != 0)
	{
		--adjustedNumElements;
	}
	size_t nEnqueueElements = adjustedNumElements / enqueueThreads;
	size_t nDequeueElements = adjustedNumElements / dequeueThreads;

	std::vector<int64_t> dequeues;
	dequeues.resize(dequeueThreads);
	std::vector<int64_t> enqueues;
	enqueues.resize(enqueueThreads);
	std::vector<int64_t> times[4];
	for (auto& timeVect : times)
	{
		timeVect.resize(nIters);
	}

	for (int iter = 0; iter < nIters; ++iter)
	{
		QueueWrapper<t_QueueType, t_TicketType> separateEnqueueDequeueWrapper;
		{
			// Time the enqueues only.
			std::vector<std::thread> threads;
			threads.reserve(enqueueThreads);

			for (size_t i = 0; i < enqueueThreads; ++i)
			{
				std::function<void()> enqueueFunc = std::bind(&QueueWrapper<t_QueueType, t_TicketType>::enqueue, &separateEnqueueDequeueWrapper, nEnqueueElements);
				threads.emplace_back(
					std::bind(
						timeFn,
						enqueueFunc
					)
				);
#ifdef _WIN32
				if(!SetThreadAffinityMask(threads.back().native_handle(), 1 << (i % std::thread::hardware_concurrency())))
				{
					abort();
				}
#else
				cpu_set_t cpuset;
				pthread_t thread = threads.back().native_handle();

				CPU_ZERO(&cpuset);
				CPU_SET((i % std::thread::hardware_concurrency()), &cpuset);
				if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
				{
					abort();
				}
#endif
			}
			while(started.load() < enqueueThreads) {}
			started.store(0);
			int64_t start = sprawl::time::SteadyNow();
			timer.store(start);
			for (auto& thread : threads)
			{
				thread.join();
			}
			times[0][iter] = timer.exchange(-1) - start;
		}

		{
			// Time the dequeues only.
			std::vector<std::thread> threads;
			threads.reserve(dequeueThreads);

			for (size_t i = 0; i < dequeueThreads; ++i)
			{
				std::function<void()> dequeueFunc = std::bind(&QueueWrapper<t_QueueType, t_TicketType>::dequeue, &separateEnqueueDequeueWrapper, nDequeueElements);
				threads.emplace_back(
					std::bind(
						timeFn,
						dequeueFunc
					)
				);
#ifdef _WIN32
				if(!SetThreadAffinityMask(threads.back().native_handle(), 1 << (i % std::thread::hardware_concurrency())))
				{
					abort();
				}
#else
				cpu_set_t cpuset;
				pthread_t thread = threads.back().native_handle();

				CPU_ZERO(&cpuset);
				CPU_SET((i % std::thread::hardware_concurrency()), &cpuset);
				if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
				{
					abort();
				}
#endif
			}
			while(started.load() < dequeueThreads) {}
			started.store(0);
			int64_t start = sprawl::time::SteadyNow();
			timer.store(start);
			for (auto& thread : threads)
			{
				thread.join();
			}
			times[1][iter] = timer.exchange(-1) - start;
		}

		{
			// Time both happening concurrently.
			QueueWrapper<t_QueueType, t_TicketType> dualWrapper;
			std::vector<std::thread> threads;
			threads.reserve(enqueueThreads + dequeueThreads);

			size_t enq = 0;
			size_t deq = 0;
			for (;;)
			{
				if (++deq <= dequeueThreads)
				{
					std::function<void()> fn = std::bind(&QueueWrapper<t_QueueType, t_TicketType>::dequeue, &dualWrapper, nDequeueElements);
					threads.emplace_back(
						std::bind(
							timeFn,
							fn
						)
					);
#ifdef _WIN32
					if(!SetThreadAffinityMask(threads.back().native_handle(), 1 << ((enq + deq) % std::thread::hardware_concurrency())))
					{
						abort();
					}
#else
					cpu_set_t cpuset;
					pthread_t thread = threads.back().native_handle();

					CPU_ZERO(&cpuset);
					CPU_SET(((enq + deq) % std::thread::hardware_concurrency()), &cpuset);
					if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
					{
						abort();
					}
#endif
				}
				if (++enq <= enqueueThreads)
				{
					std::function<void()> fn = std::bind(&QueueWrapper<t_QueueType, t_TicketType>::enqueue, &dualWrapper, nEnqueueElements);
					threads.emplace_back(
						std::bind(
							timeFn,
							fn
						)
					);
#ifdef _WIN32
					if(!SetThreadAffinityMask(threads.back().native_handle(), 1 << ((enq + deq) % std::thread::hardware_concurrency())))
					{
						abort();
					}
#else
					cpu_set_t cpuset;
					pthread_t thread = threads.back().native_handle();

					CPU_ZERO(&cpuset);
					CPU_SET(((enq + deq) % std::thread::hardware_concurrency()), &cpuset);
					if(pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset))
					{
						abort();
					}
#endif
				}
				if (enq >= enqueueThreads && deq >= dequeueThreads)
				{
					break;
				}
			}
			while(started.load() < dequeueThreads + enqueueThreads) {}
			started.store(0);
			int64_t start = sprawl::time::SteadyNow();
			timer.store(start);
			for (auto& thread : threads)
			{
				thread.join();
			}
			times[2][iter] = timer.exchange(-1) - start;
		}

		{
			// Time dequeues from an empty queue
			QueueWrapper<t_QueueType, t_TicketType> emptyWrapper;
			std::vector<std::thread> threads;
			threads.reserve(dequeueThreads);

			for (size_t i = 0; i < dequeueThreads; ++i)
			{
				std::function<void()> fn = std::bind(&QueueWrapper<t_QueueType, t_TicketType>::dequeueEmpty, &emptyWrapper, nDequeueElements * 10);
				threads.emplace_back(
					std::bind(
						timeFn,
						fn
					)
				);
#ifdef _WIN32
				if (!SetThreadAffinityMask(threads.back().native_handle(), 1 << (i % std::thread::hardware_concurrency())))
				{
					abort();
				}
#else
				cpu_set_t cpuset;
				pthread_t thread = threads.back().native_handle();

				CPU_ZERO(&cpuset);
				CPU_SET((i % std::thread::hardware_concurrency()), &cpuset);
				if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
				{
					abort();
				}
#endif
			}
			while(started.load() < dequeueThreads) {}
			started.store(0);
			int64_t start = sprawl::time::SteadyNow();
			timer.store(start);
			for (auto& thread : threads)
			{
				thread.join();
			}
			times[3][iter] = timer.exchange(-1) - start;
		}
	}

	std::cout << TypeName<t_QueueType>::GetName(useMoves, t_TicketType) << "\t" << 1 << "\t" << enqueueThreads << "\t" << dequeueThreads << "\t" <<
		OpsPerSecond(mean(times[0]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Max(times[0]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Min(times[0]), adjustedNumElements) << std::endl;
		
	std::cout << TypeName<t_QueueType>::GetName(useMoves, t_TicketType) << "\t" << 2 << "\t" << enqueueThreads << "\t" << dequeueThreads << "\t" <<
		OpsPerSecond(mean(times[1]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Max(times[1]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Min(times[1]), adjustedNumElements) << std::endl;
		
	std::cout << TypeName<t_QueueType>::GetName(useMoves, t_TicketType) << "\t" << 3 << "\t" << enqueueThreads << "\t" << dequeueThreads << "\t" <<
		OpsPerSecond(mean(times[2]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Max(times[2]), adjustedNumElements) << "\t" <<
		OpsPerSecond(Min(times[2]), adjustedNumElements) << std::endl;
		
	std::cout << TypeName<t_QueueType>::GetName(useMoves, t_TicketType) << "\t" << 4 << "\t" << enqueueThreads << "\t" << dequeueThreads << "\t" <<
		OpsPerSecond(mean(times[3]), adjustedNumElements * 10) << "\t" <<
		OpsPerSecond(Max(times[3]), adjustedNumElements * 10) << "\t" <<
		OpsPerSecond(Min(times[3]), adjustedNumElements * 10) << std::endl;
}

template<typename t_ElementType, typename t_QueueType, TicketType t_TicketType = TicketType::PERSISTENT>
void RunTestsOnQueueType(bool useMoves = false)
{
	size_t totalThreads = std::thread::hardware_concurrency();
	for(size_t i = 1; i <= totalThreads; ++i) {
		for(size_t j = 1; j <= totalThreads; ++j) {
			RunTestsOnQueueTypeWithThreadCounts<t_ElementType, t_QueueType, t_TicketType>(i, j, useMoves);
		}
	}
}


template<typename t_ElementType, typename t_QueueType>
void PrintEmpty()
{
	size_t totalThreads = std::thread::hardware_concurrency();
	for (size_t i = 1; i <= totalThreads; ++i) {
		for (size_t j = 1; j <= totalThreads; ++j) {
			std::cout << TypeName<t_QueueType>::GetName(false, TicketType::PERSISTENT) << "\t" << 0 << "\t" << 0 << "\t" << 0 << "\t" << 0 << std::endl;
		}
	}
}

class NoBoost {};

template<typename t_ElementType>
void RunTestsOnElementType()
{
	RunTestsOnQueueType<t_ElementType, ext_1024cores::mpmc_bounded_queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, tbb::concurrent_bounded_queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, tbb::concurrent_queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, boost::lockfree::queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, std::deque<t_ElementType>>();
	
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::EPHEMERAL>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::EPHEMERAL>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::NONE>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>();
}

template<typename t_ElementType>
void RunTestsOnElementType(NoBoost const&)
{
	RunTestsOnQueueType<t_ElementType, ext_1024cores::mpmc_bounded_queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, tbb::concurrent_bounded_queue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, tbb::concurrent_queue<t_ElementType>>();

	// Boost fails to compile with this element type
	PrintEmpty<t_ElementType, boost::lockfree::queue<t_ElementType>>();

	RunTestsOnQueueType<t_ElementType, std::deque<t_ElementType>>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::EPHEMERAL>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::EPHEMERAL>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType>, TicketType::NONE>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>();

	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>>();
	RunTestsOnQueueType<t_ElementType, sprawl::collections::ConcurrentBoundedQueue<t_ElementType, NUM_ELEMENTS>, TicketType::NONE>();
}

template<size_t t_Size>
class FixedStaticString
{
public:
	FixedStaticString(){}

	FixedStaticString(FixedStaticString const& other)
	{
		memcpy(m_str, other.m_str, t_Size);
	}

	FixedStaticString& operator=(FixedStaticString const& other)
	{
		memcpy(m_str, other.m_str, t_Size);
		return *this;
	}
private:
	char m_str[t_Size];
};

int main()
{
	std::cout << std::fixed;
	RunTestsOnElementType<char>();
	RunTestsOnElementType<int64_t>();
	RunTestsOnElementType<FixedStaticString<64>>(NoBoost());
}
