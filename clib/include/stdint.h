#ifndef STDINT_H
#define STDINT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef char 				int8_t;	
typedef unsigned char		uint8_t;
typedef short 				int16_t;	
typedef unsigned short 		uint16_t;
typedef int					int32_t;	
typedef unsigned int 		uint32_t;
typedef long long			int64_t;
typedef unsigned long long 	uint64_t;

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

#ifdef __cplusplus
}
#endif
#endif 