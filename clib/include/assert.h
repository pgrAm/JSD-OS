#ifndef ASSERT_H
#define ASSERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NDEBUG

#	ifdef __cplusplus
	[[noreturn]]
#	endif
	__attribute__((noinline)) void
	__assert_fail(const char* expression, const char* file, int line);

#	ifdef __cplusplus
#		define assert(expression)                              \
			if(!(expression)) [[unlikely]]                      \
			{                                                   \
				__assert_fail(#expression, __FILE__, __LINE__); \
			}
#	else
#		define assert(expression)                              \
			if(!(expression))                                   \
			{                                                   \
				__assert_fail(#expression, __FILE__, __LINE__); \
			}
#	endif

#else

#	define assert(expression)

#endif

#ifdef __cplusplus
}
#endif

#endif