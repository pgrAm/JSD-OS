#ifndef STD_FROM_CHARS_H
#define STD_FROM_CHARS_H

#include <type_traits>
#include <stdint.h>
#include <ctype.h>

namespace std 
{
	enum class errc {
		invalid_argument = 1
	};

	struct from_chars_result {
		const char* ptr;
		errc ec;
	};

	inline from_chars_result from_chars(const char* first, const char* last, unsigned int& value)
	{
		auto str = first;
		for(; isspace(*str); str++)
		{
			if(str >= last)
				return {first, errc::invalid_argument};
		}
		//
		unsigned int n = 0;
		while(str < last && isdigit(*str))
		{
			n = n * 10 + *str - '0';
			str++;
		}

		value = n;

		return {str, {}};
	}
};

#endif