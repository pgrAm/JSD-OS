#ifndef __CHAR_TRAITS_H
#define __CHAR_TRAITS_H

namespace std {
	template<typename _CharT> class char_traits;
}

template<typename _CharT>
class std::char_traits
{
public:
	using char_type = _CharT;
	using int_type = unsigned long;
	using pos_type = size_t;
	using off_type = size_t;

	static void assign(char_type& c1, const char_type& c2)
	{
		c1 = c2;
	}

	static bool eq(const char_type& c1, const char_type& c2)
	{
		return c1 == c2;
	}

	static bool lt(const char_type& c1, const char_type& c2)
	{
		return c1 < c2;
	}

	static int compare(const char_type* s1, const char_type* s2, size_t n)
	{
		for(size_t i = 0; i < n; ++i)
		{
			if(lt(s1[i], s2[i]))
			{
				return -1;
			}
			else if(lt(s2[i], s1[i]))
			{
				return 1;
			}
		}
		return 0;
	}

	static size_t length(const char_type* s)
	{
		size_t i = 0;
		while(!eq(s[i], char_type()))
		{
			++i;
		}
		return i;
	}

	static const char_type* find(const char_type* s, size_t n, const char_type& a)
	{
		for(size_t i = 0; i < n; ++i)
		{
			if(eq(s[i], a))
				return s + i;
		}
		return 0;
	}

	static char_type to_char_type(const int_type& __c)
	{
		return static_cast<char_type>(__c);
	}

	static int_type to_int_type(const char_type& __c)
	{
		return static_cast<int_type>(__c);
	}

	static bool eq_int_type(const int_type& __c1, const int_type& __c2)
	{
		return __c1 == __c2;
	}
};

#endif