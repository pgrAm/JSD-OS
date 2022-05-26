#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <new>

void* operator new(size_t size, std::align_val_t align)
{
	return aligned_alloc(static_cast<size_t>(align), size);
}

void* operator new[](size_t size, std::align_val_t align)
{
	return aligned_alloc(static_cast<size_t>(align), size);
}

void* operator new (size_t size)
{
    return malloc(size);
}

void* operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void* p)
{
    free(p);
}

void operator delete[](void* p)
{
    free(p);
}

template<typename T, size_t Radix>
static constexpr size_t num_digits(T number)
{
	size_t count = 0;
	do 
	{
		number /= Radix;
		count++;
	} while(number != 0);
	return count;
}

template<typename T>
static std::string m_to_string(T value) requires(std::is_integral_v<T>)
{
	std::string str;

	str.resize(num_digits<T, 10>(value));
	auto it = str.end();
	do
	{
		auto digit = static_cast<char>((T)value % 10);
		*(--it)	   = digit + '0';

		value = (T)value / 10;
	} while(value != 0);
	return str;
}

std::string std::to_string(int value)
{
	return m_to_string(value);
}

std::string std::to_string(long value)
{
	return m_to_string(value);
}

std::string std::to_string(long long value)
{
	return m_to_string(value);
}

std::string std::to_string(unsigned value)
{
	return m_to_string(value);
}

std::string std::to_string(unsigned long value)
{
	return m_to_string(value);
}

std::string std::to_string(unsigned long long value)
{
	return m_to_string(value);
}