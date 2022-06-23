#ifndef UNICODE_H
#define UNICODE_H

#include <string>
#include <string_view>
#include <bit>

template<typename OutputIt>
inline static OutputIt utf8_encode_char(OutputIt first, char32_t c)
{
	using namespace std::literals;

	if(c <= 0x7Fu)
	{
		*first++ = (char)c;
	}
	else if(c <= 0x7FFu)
	{
		*first++ = ((char)(0xC0u | ((c >> 6) & 0x1Fu)));
		*first++ = ((char)(0x80u | ((c >> 0) & 0x3Fu)));
	}
	else if(c <= 0xFFFF)
	{
		*first++ = ((char)(0xE0u | ((c >> 12) & 0x0Fu)));
		*first++ = ((char)(0x80u | ((c >> 6) & 0x3Fu)));
		*first++ = ((char)(0x80u | ((c >> 0) & 0x3Fu)));
	}
	else if(c <= 0x10FFFF)
	{
		*first++ = ((char)(0xF0u | ((c >> 18) & 0x07u)));
		*first++ = ((char)(0x80u | ((c >> 12) & 0x3Fu)));
		*first++ = ((char)(0x80u | ((c >> 6) & 0x3Fu)));
		*first++ = ((char)(0x80u | ((c >> 0) & 0x3Fu)));
	}
	else
	{
		*first++ = '\xef';
		*first++ = '\xbf';
		*first++ = '\xbd';
	}
	return first;
}

inline static std::string utf8_encode(std::basic_string_view<char32_t> input)
{
	using namespace std::literals;

	std::string out;
	for(auto c : input)
	{
		utf8_encode_char(std::back_inserter(out), c);
	}
	return out;
}

template<typename Iterator>
struct decode_result
{
	char32_t ch;
	Iterator pos;
};

template<typename Iterator>
inline decode_result<Iterator> utf8_decode_char(Iterator begin, Iterator end)
{
	std::basic_string<char32_t> out;

	size_t num_chars	 = 0;
	char32_t next_output = 0;

	auto inputIt = begin;
	for(; inputIt != end; ++inputIt)
	{
		auto c = static_cast<uint8_t>(*inputIt);

		if((c & 0x80) == 0)
		{
			return {c, ++inputIt};
		}
		else if((c & 0xC0u) == 0x80u && num_chars)
		{
			next_output <<= 6;
			next_output |= (c & 0x3Fu);

			if(--num_chars == 0)
			{
				return {next_output, ++inputIt};
			}
		}
		else
		{
			num_chars	= (size_t)std::countl_one(c);
			next_output = c & (0xFFu >> (num_chars + 1));
			--num_chars;
		}
	}
	return {'\0', inputIt};
}

inline std::basic_string_view<char32_t> utf8_decode(std::string_view input)
{
	std::basic_string<char32_t> out;
	auto it = input.begin();

	while(it != input.cend())
	{
		auto [ch, out_it] = utf8_decode_char(it, input.cend());
		out.push_back(ch);
		it = out_it;
	}

	return out;
}

template<typename OutputIt>
inline void utf16_encode_char(OutputIt& out,
							  char32_t c) //requires(sizeof(OutChar) >=
										  //sizeof(uint16_t))
{
	if(c <= 0xFFFFu)
	{
		*out++ = static_cast<char16_t>(c);
	}
	else
	{
		c -= 0x10000u;
		*out++ = (0xD800u | ((c >> 10) & 0x3FFu));
		*out++ = (0xDC00u | (c & 0x3FFu));
	}
}

template<typename OutChar>
inline std::basic_string<OutChar> utf16_encode(std::basic_string_view<char32_t> input) 
	requires(sizeof(OutChar) >= sizeof(uint16_t))
{
	std::basic_string<OutChar> out;
	for(auto c : input)
	{
		utf16_encode_char(std::back_inserter(out), c);
	}
	return out;
}

template<typename Iterator>
inline decode_result<Iterator>
utf16_decode_char(Iterator begin,
				  Iterator end) //requires(sizeof(Char) >= sizeof(uint16_t))
{
	char32_t next_output = 0;

	auto inputIt = begin;
	for(; inputIt != end; ++inputIt)
	{
		auto c = *inputIt;
		if(c < 0xD800u || c > 0xDFFFu)
		{
			return {c, ++inputIt};
		}
		else if((c & 0xFC00u) == 0xDC00u) //low
		{
			return {(((char32_t)next_output << 10) | (c & 0x3ffu)) + 0x10000u,
					++inputIt};
		}
		else if((c & 0xFC00u) == 0xD800u) //high
		{
			next_output = c & 0x3ffu;
		}
	}
	return {'\0', inputIt};
}


template<typename Char>
inline std::basic_string_view<char32_t>
utf16_decode(std::basic_string_view<Char> input)
{
	std::basic_string<char32_t> out;
	auto it = input.begin();
	while(it != input.cend())
	{
		auto [ch, out_it] = utf16_decode_char(it, input.cend());
		out.push_back(ch);
		it = out_it;
	}

	return out;
}

template<typename Char, typename OutputIt>
void utf16_to_utf8(OutputIt first, std::basic_string_view<Char> input) requires(
	sizeof(Char) >= sizeof(uint16_t))
{
	auto it = input.begin();
	while(it != input.cend())
	{
		auto [ch, out_it] = utf16_decode_char(it, input.cend());
		utf8_encode_char(first, ch);
		it = out_it;
	}
}

template<typename Char>
std::string
utf16_to_utf8(std::basic_string_view<Char> input) requires(sizeof(Char) >=
														  sizeof(uint16_t))
{
	std::string out;
	utf16_to_utf8(std::back_inserter(out), input);
	return out;
}

template<typename OutputIt>
inline void utf8_to_utf16(OutputIt first, std::string_view input)
{
	auto it = input.begin();
	while(it != input.cend())
	{
		auto [ch, out_it] = utf8_decode_char(it, input.cend());
		utf16_encode_char(first, ch);
		it = out_it;
	}
}

template<typename OutChar, typename inChar>
inline std::basic_string<OutChar>
utf8_to_utf16(std::basic_string_view<inChar> input) requires(sizeof(OutChar) >=
															 sizeof(uint16_t))
{
	std::basic_string<OutChar> out;
	utf8_to_utf16(std::back_inserter(out), input);
	return out;
}



#endif