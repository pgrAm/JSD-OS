#ifndef JSD_OS_UTIL_H
#define JSD_OS_UTIL_H

#include <type_traits>
#include <limits>

template<unsigned long MaxVal>
using smallest_uint = std::conditional_t<
	MaxVal <= std::numeric_limits<unsigned char>::max(), unsigned char,
	std::conditional_t<
		MaxVal <= std::numeric_limits<unsigned short>::max(), unsigned short,
		std::conditional_t<
			MaxVal <= std::numeric_limits<unsigned int>::max(), unsigned int,
			std::conditional_t<
				MaxVal <= std::numeric_limits<unsigned long>::max(), unsigned long,
				std::conditional_t<
					MaxVal <= std::numeric_limits<unsigned long long>::max(),
					unsigned long long, void>>>>>;

#endif