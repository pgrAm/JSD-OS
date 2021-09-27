#ifndef LIMITS_H
#define LIMITS_H

#ifdef __cplusplus
extern "C" {
#endif

#define CHAR_BIT	8 		
#define SCHAR_MIN	-127 	
#define SCHAR_MAX	127		
#define UCHAR_MAX	255		
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX
#define MB_LEN_MAX	1		
#define SHRT_MIN	-32767	
#define SHRT_MAX	32767	
#define USHRT_MAX	65535	
#define INT_MIN		-2147483647
#define INT_MAX		2147483647
#define UINT_MAX	4294967295
#define LONG_MIN	-2147483647
#define LONG_MAX	2147483647
#define ULONG_MAX	4294967295
#define LLONG_MIN	-9223372036854775807	
#define LLONG_MAX	9223372036854775807		
#define ULLONG_MAX	18446744073709551615	

#ifdef __cplusplus
}
#endif

#endif