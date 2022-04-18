#ifndef K_ASSERT_H
#define K_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG

#ifdef __cplusplus
	[[noreturn]]
#endif
	__attribute__((noinline)) void __kassert_fail(const char* expression, const char* file, int line);

#ifdef __cplusplus
#define k_assert(expression)     \
		if(!(expression)) [[unlikely]] { \
		__kassert_fail(#expression, __FILE__, __LINE__);}
#else 
#define k_assert(expression)     \
		if(!(expression)) { \
		__kassert_fail(#expression, __FILE__, __LINE__);}
#endif

#else

#define k_assert(expression)

#endif

#ifdef __cplusplus
}
#endif

#endif