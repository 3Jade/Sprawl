#include <functional>
#include <type_traits>
#include "../common/compat.hpp"

namespace sprawl
{
	namespace memory
	{
		// 8 is too large for some types, but no type should be larger than 8
		// and 8 is a multiple of other valid aligments (1,2,4) so it's a safe default
		template<size_t size, size_t align = 8>
		class OpaqueType;

		template<typename T>
		struct CreateAs{};

		namespace detail
		{
			template<size_t accumulatedSoFar, size_t align, typename... Types>
			struct ComputeSize
			{
				static size_t const size = accumulatedSoFar + ((accumulatedSoFar % align != 0) ? (align - (accumulatedSoFar % align)) : 0);
			};

//alignof(double) is broken in 32 bit gcc and clang, and returns 8 when it should return 4.
#if defined(__GNUC__) && SPRAWL_32_BIT
			template<size_t accumulatedSoFar, size_t align, typename... MoreTypes>
			struct ComputeSize<accumulatedSoFar, align, double, MoreTypes...>
			{
				static size_t const size = ComputeSize<
					accumulatedSoFar + sizeof(double) + (accumulatedSoFar % 4 != 0 ? (4 - (accumulatedSoFar % 4)) : 0),
					align,
					MoreTypes...
				>::size;
			};
#endif

			template<size_t accumulatedSoFar, size_t align, typename FirstType, typename... MoreTypes>
			struct ComputeSize<accumulatedSoFar, align, FirstType, MoreTypes...>
			{
				static size_t const size = ComputeSize<
					accumulatedSoFar + sizeof(FirstType) + (accumulatedSoFar % alignof(FirstType) != 0 ? (alignof(FirstType) - (accumulatedSoFar % alignof(FirstType))) : 0),
					align,
					MoreTypes...
				>::size;
			};

			template<size_t highestSoFar, typename... Types>
			struct ComputeAlign { static size_t const align = highestSoFar; };

#if defined(__GNUC__) && SPRAWL_32_BIT
			template<size_t highestSoFar, typename... MoreTypes>
			struct ComputeAlign<highestSoFar, double, MoreTypes...>
			{
				static size_t const align = ComputeAlign<
					(highestSoFar < 4 ? 4 : highestSoFar),
					MoreTypes...
				>::align;
			};
#endif

			template<size_t highestSoFar, typename FirstType, typename... MoreTypes>
			struct ComputeAlign<highestSoFar, FirstType, MoreTypes...>
			{
				static size_t const align = ComputeAlign<
					(highestSoFar < alignof(FirstType) ? alignof(FirstType) : highestSoFar),
					MoreTypes...
				>::align;
			};
		}

		template<typename... Types>
		using OpaqueTypeList = OpaqueType<
			detail::ComputeSize<0, detail::ComputeAlign<0, Types...>::align, Types...>::size,
			detail::ComputeAlign<0, Types...>::align
		>;
	}
}

template<size_t size, size_t align>
class sprawl::memory::OpaqueType
{
public:
	class Deleter
	{
	public:
		virtual void Delete(void*) = 0;
	};

	template<typename T>
	class TypeDeleter : public Deleter
	{
	public:
		virtual void Delete(void* ptr)
		{
			reinterpret_cast<T*>(ptr)->~T();
		}
	};

	template<typename T, typename... Params>
	OpaqueType(CreateAs<T> const&, Params&&... params)
		: m_ptr()
		, m_deleter()
	{
		static_assert(sizeof(T) == size, "Opaque type delcared with size that does not match the type used to construct it.");
		static_assert(align >= alignof(T) && (align % alignof(T) == 0), "Opaque type definition does not match alignment requirements of the type used to construct it.");
		new(&m_ptr) T(std::forward<Params>(params)...);
		new(&m_deleter) TypeDeleter<T>();
	}

	~OpaqueType()
	{
		reinterpret_cast<Deleter*>(&m_deleter)->Delete(&m_ptr);
	}

	template<typename T>
	T& As()
	{
		static_assert(sizeof(T) == size, "Opaque type delcared with size that does not match the type used to access it.");
		static_assert(align >= alignof(T) && (align % alignof(T) == 0), "Opaque type definition does not match alignment requirements of the type used to access it.");
		return *reinterpret_cast<T*>(&m_ptr);
	}

	template<typename T>
	T const& As() const
	{
		static_assert(sizeof(T) == size, "Opaque type delcared with size that does not match the type used to access it.");
		static_assert(align >= alignof(T) && (align % alignof(T) == 0), "Opaque type definition does not match alignment requirements of the type used to access it.");
		return *reinterpret_cast<T*>(&m_ptr);
	}

	template<typename T>
	operator T&()
	{
		static_assert(sizeof(T) == size, "Opaque type delcared with size that does not match the type used to access it.");
		static_assert(align >= alignof(T) && (align % alignof(T) == 0), "Opaque type definition does not match alignment requirements of the type used to access it.");
		return *reinterpret_cast<T*>(&m_ptr);
	}

	template<typename T>
	operator T const&() const
	{
		static_assert(sizeof(T) == size, "Opaque type delcared with size that does not match the type used to access it.");
		static_assert(align >= alignof(T) && (align % alignof(T) == 0), "Opaque type definition does not match alignment requirements of the type used to access it.");
		return *reinterpret_cast<T*>(&m_ptr);
	}

private:
	typename std::aligned_storage<size, align>::type m_ptr;
	typename std::aligned_storage<sizeof(Deleter), alignof(Deleter)>::type m_deleter;
};