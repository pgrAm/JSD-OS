#ifndef JSD_OS_UTIL_H
#define JSD_OS_UTIL_H

#include <type_traits>
#include <limits>
#include <bit>
#include <utility>
#include <assert.h>

template<unsigned long MaxVal>
using smallest_uint = std::conditional_t<
	MaxVal <= std::numeric_limits<unsigned char>::max(), unsigned char,
	std::conditional_t<
		MaxVal <= std::numeric_limits<unsigned short>::max(), unsigned short,
		std::conditional_t<
			MaxVal <= std::numeric_limits<unsigned int>::max(), unsigned int,
			std::conditional_t<
				MaxVal <= std::numeric_limits<unsigned long>::max(),
				unsigned long,
				std::conditional_t<
					MaxVal <= std::numeric_limits<unsigned long long>::max(),
					unsigned long long, void>>>>>;

//align must be a power of 2
constexpr static inline uintptr_t align_addr(uintptr_t addr, size_t align)
{
	assert(std::has_single_bit(align));
	return (addr + (align - 1)) & ~(align - 1);
}

template<class T>
	requires(!std::is_array_v<T>)
class intrusive_ptr
{
public:
	constexpr intrusive_ptr() noexcept = default;
	constexpr intrusive_ptr(std::nullptr_t) noexcept
	{
	}

	template<typename... Args>
	explicit intrusive_ptr(Args&&... args) :
		ctrl{new control_block{{std::forward<Args>(args)...}, 1}}
	{
		assert(ctrl);
		assert(ctrl->ref_count);
	}

	intrusive_ptr(const intrusive_ptr& s) : ctrl{s.ctrl}
	{
		if(ctrl)
		{
			assert(ctrl->ref_count);
			++ctrl->ref_count;
		}
	}

	intrusive_ptr& operator=(const intrusive_ptr& s) noexcept
	{
		ctrl = s.ctrl;
		if(ctrl)
		{
			assert(ctrl->ref_count);
			++ctrl->ref_count;
		}
		return *this;
	}

	intrusive_ptr& operator=(intrusive_ptr&& s) noexcept
	{
		std::swap(ctrl, s.ctrl);
		return *this;
	}

	intrusive_ptr(intrusive_ptr&& s) noexcept
	{
		ctrl   = s.ctrl;
		s.ctrl = nullptr;

		if(ctrl)
		{
			assert(ctrl->ref_count);
		}
	}

	~intrusive_ptr()
	{
		if(ctrl)
		{
			assert(ctrl->ref_count);
			if(--ctrl->ref_count == 0)
			{
				delete ctrl;
			}
		}
	}

	T& operator*() const noexcept
	{
		assert(ctrl);
		return ctrl->value;
	}

	T* operator->() const noexcept
	{
		assert(ctrl);
		return &ctrl->value;
	}

	explicit operator bool() const noexcept
	{
		return !!ctrl;
	}

private:
	struct control_block
	{
		T value;
		size_t ref_count = 0;
	};

	control_block* ctrl = nullptr;
};

template<typename T>
T read_addr(uintptr_t addr) requires(std::is_trivially_copyable_v<T>)
{
	T value;
	memcpy(&value, (void*)addr, sizeof(T));
	return value;
}

#endif