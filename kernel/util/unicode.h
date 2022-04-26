#ifndef UNICODE_H
#define UNICODE_H

#include <string>
#include <string_view>
#include <bit>

inline static std::string utf8_encode(std::basic_string_view<char32_t> input)
{
	using namespace std::literals;

	std::string out;
	for(auto c : input)
	{
		if(c <= 0x7Fu)
		{
			out.push_back(c);
		}
		else if(c <= 0x7FFu)
		{
			out.push_back(0xC0u | ((c >> 6) & 0x1Fu));
			out.push_back(0x80u | ((c >> 0) & 0x3Fu));
		}
		else if(c <= 0xFFFF)
		{
			out.push_back(0xE0u | ((c >> 12) & 0x0Fu));
			out.push_back(0x80u | ((c >> 6) & 0x3Fu));
			out.push_back(0x80u | ((c >> 0) & 0x3Fu));
		}
		else if(c <= 0x10FFFF)
		{
			out.push_back(0xF0u | ((c >> 18) & 0x07u));
			out.push_back(0x80u | ((c >> 12) & 0x3Fu));
			out.push_back(0x80u | ((c >> 6) & 0x3Fu));
			out.push_back(0x80u | ((c >> 0) & 0x3Fu));
		}
		else
		{
			out.append("\xef\xbf\xbd"sv);
		}
	}
	return out;
}

inline std::basic_string<char32_t> utf8_decode(std::string_view input)
{
	std::basic_string<char32_t> out;

	size_t num_chars	 = 0;
	char32_t next_output = 0;

	for(uint8_t c : input)
	{
		if((c & 0x80) == 0)
		{
			out.push_back(c);
			num_chars = 0;
		}
		else if((c & 0xC0u) == 0x80u && num_chars)
		{
			next_output <<= 6;
			next_output |= (c & 0x3Fu);

			if(--num_chars == 0)
			{
				out.push_back(next_output);
			}
		}
		else
		{
			num_chars	= std::countl_one(c);
			next_output = c & (0xFFu >> (num_chars + 1));
			--num_chars;
		}
	}
	return out;
}

template<typename OutChar>
inline std::basic_string<OutChar> utf16_encode(std::basic_string_view<char32_t> input) 
	requires(sizeof(OutChar) >= sizeof(uint16_t))
{
	std::basic_string<OutChar> out;
	for(auto c : input)
	{
		if(c <= 0xFFFFu)
		{
			out.push_back(c);
		}
		else
		{
			c -= 0x10000u;
			out.push_back(0xD800u | ((c >> 10) & 0x3FFu));
			out.push_back(0xDC00u | (c & 0x3FFu));
		}
	}
	return out;
}

template<typename Char>
std::basic_string<char32_t> utf16_decode(std::basic_string_view<Char> input)
	requires(sizeof(Char) >= sizeof(uint16_t))
{
	std::basic_string<char32_t> out;

	char32_t next_output = 0;

	for(char16_t c : input)
	{
		if(c < 0xD800u || c > 0xDFFFu)
		{
			out.push_back(c);
			next_output = 0;
		}
		else if((c & 0xFC00u) == 0xDC00u) //low
		{
			out.push_back((((char32_t)next_output << 10) | (c & 0x3ffu)) + 0x10000u);
			next_output = 0;
		}
		else if((c & 0xFC00u) == 0xD800u) //high
		{
			next_output = c & 0x3ffu;
		}
	}
	return out;
}

#endif