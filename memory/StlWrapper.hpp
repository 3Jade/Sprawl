#pragma once

#include "PoolAllocator.hpp"

namespace sprawl
{
	namespace memory
	{
		template<typename T, typename allocator=DynamicPoolAllocator<sizeof(T), 32> >
		class StlWrapper
		{
		public:
			typedef StlWrapper<T, allocator> other;

			typedef T value_type;

			typedef T* pointer;
			typedef const T* const_pointer;
			typedef T& reference;
			typedef const T& const_reference;

			typedef size_t size_type;
			typedef ptrdiff_t difference_type;

#ifdef _WIN32
#	if _HAS_CPP0X
			typedef std::false_type propagate_on_container_copy_assignment;
			typedef std::false_type propagate_on_container_move_assignment;
			typedef std::false_type propagate_on_container_swap;

			StlWrapper<T, allocator> select_on_container_copy_construction() const
			{	// return this allocator
				return (*this);
			}
#	endif
#endif

			template<class T2>
			struct rebind
			{
				typedef StlWrapper<T2, typename allocator::template rebind<T2>::otherAllocator> other;
			};

			value_type* address(value_type& val) const
			{
				return reinterpret_cast<value_type*>(&reinterpret_cast<char&>(val));
			}

			const value_type* address(const value_type& val) const
			{
				return reinterpret_cast<const value_type*>(&reinterpret_cast<const char&>(val));
			}

			StlWrapper()
			{
				//
			}

			StlWrapper(const StlWrapper<value_type>& /*other*/)
			{
				//
			}

			template<class T2, class alloc>
			StlWrapper(const StlWrapper<T2, alloc>& /*other*/)
			{
				//
			}

			template<class T2>
			StlWrapper<value_type>& operator=(const StlWrapper<T2>& /*other*/)
			{
				return (*this);
			}

			bool operator==(const StlWrapper<value_type, allocator>& /*other*/) const
			{
				return true;
			}

			bool operator!=(const StlWrapper<value_type, allocator>& /*other*/) const
			{
				return false;
			}

			template<typename otherAllocator>
			bool operator==(const StlWrapper<value_type, otherAllocator>& /*other*/) const
			{
				return false;
			}

			template<typename otherAllocator>
			bool operator!=(const StlWrapper<value_type, otherAllocator>& /*other*/) const
			{
				return true;
			}

			void deallocate(value_type* ptr, size_type /*unused*/)
			{
				allocator::free(ptr);
			}

			value_type* allocate(size_type count)
			{
				if(count == 1)
				{
					return (value_type*)(allocator::alloc());
				}
				return (value_type*)(allocator::alloc(count));
			}

			value_type* allocate(size_type count, const void* /*unused*/)
			{
				return (value_type*)(allocate(count));
			}

			void construct(value_type* ptr)
			{
				::new ((void *)ptr) T();
			}

			void construct(value_type* ptr, const value_type& other)
			{
				::new ((void *)ptr) T(other);
			}

#ifdef _WIN32
#	define BF_ALLOC_MEMBER_CONSTRUCT( \
	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, X2, X3, X4) \
	template<class _Objty COMMA LIST(_CLASS_TYPE)> \
	void construct(_Objty *_Ptr COMMA LIST(_TYPE_REFREF_ARG)) \
			{ \
	::new ((void *)_Ptr) _Objty(LIST(_FORWARD_ARG)); \
		}

			_VARIADIC_EXPAND_0X(BF_ALLOC_MEMBER_CONSTRUCT, , , , )
#	undef BF_ALLOC_MEMBER_CONSTRUCT
#else
			template< typename T2, typename... Args >
			void construct( T2* ptr, Args&&... args )
			{
				::new ((void*)ptr) T2(std::forward<Args>(args)...);
			}

#endif

			template<class T2>
			void destroy(T2* ptr)
			{
				(void)(ptr);
				ptr->~T2();
			}

			size_t max_size() const
			{
				return ((size_t)(-1) / sizeof (T));
			}
		};
	}
}
