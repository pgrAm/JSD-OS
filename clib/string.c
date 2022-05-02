#include <string.h>
#include <ctype.h>
#include <stdbool.h>

char* strchr(const char* str, int character)
{
    while (*str != (char)character)
    {
        if (*str == '\0')
        {
            return NULL;
        }
        str++;
    }
    return str;
}

char* strtok(char* str, const char* delimiters)
{
    static char* buffer = NULL;
	
    if(str != NULL) 
	{
		buffer = str;
	}
    if(buffer == NULL || buffer[0] == '\0')
	{
		return NULL;
	}
 
    char *token = buffer;
	char *c = buffer;
 
    for(; *c != '\0'; c++)
	{
		const char *dchar = delimiters;
        for(; *dchar != '\0'; dchar++)
		{
            if(*c == *dchar)
			{
                *c = '\0';
                buffer = c + 1;
 
                if(c == token)
				{ 
                    token++; 
                    continue; 
                }
                return token;
            }
        }
    }
 
    return token;
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
	{
        s1++;
		s2++;
	}
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strcasecmp(const char* s1, const char* s2)
{
    while(*s1 && (toupper(*s1) == toupper(*s2)))
	{
        s1++;
		s2++;
	}
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

//void* memcpy(void* dest, const void* src, size_t num) 
//{
//	char* dst8 = (char*)dest;
//    char* src8 = (char*)src;
//
//    while(num--) 
//	{
//		*dst8++ = *src8++;
//    }               
//    return dest;
//}

int memcmp(const void* ptr1, const void* ptr2, size_t num)
{
	if(!num) return 0;

	const unsigned char* ptra = (const unsigned char*)ptr1;
	const unsigned char* ptrb = (const unsigned char*)ptr2;

	while(--num && *ptra == *ptrb)
	{
		ptra++;
		ptrb++;
	}

	return (*((unsigned char*)ptra) - *((unsigned char*)ptrb));
}

/*
void* memset(void* ptr, int value, size_t num)
{
    uint8_t u8  = (uint8_t)value;
	uint8_t* sp  = (uint8_t*)ptr;
    
    while(((uint32_t)sp & (sizeof(uint32_t) - 1)) && num--)
    {
		*sp++ = u8;
    }
	
	uint32_t u32 = (uint32_t)value & 0xff;
	u32 = (u32 << 24) | (u32 << 16) | (u32 << 8) | u32;
    uint32_t* lp = (uint32_t*)sp;
    
    while(num / sizeof(uint32_t))
    {
        *lp++ = u32;
        num -= sizeof(uint32_t);
    }
    
    sp = (uint8_t*)lp;
    
    while(num--)
    {
        *sp++ = u8;
    }
    
    return ptr;
}*/

//void* memset(void* ptr, int value, size_t num)
//{
//	unsigned char* ptr8 = (unsigned char*)ptr;
//
//    while(num--) 
//	{
//		*ptr8++ = (unsigned char)value;
//    }
//    return ptr;
//}

size_t strlen(const char* str)
{
	size_t len = 0;
	
	while(*(str++) != '\0') { len++; }
	
	return len;
}

char* strcat(char* destination, const char* source)
{
	memcpy(destination + strlen(destination), source, strlen(source) + 1);
	return destination;
}

char* strcpy(char* destination, const char* source)
{
	memcpy(destination, source, strlen(source) + 1);
	return destination;
}

void* memmove(void* dest, const void* src, size_t num)
{
	uint8_t* dst_ptr = dest;
	const uint8_t* src_ptr = src;

	if(src_ptr < dst_ptr && dst_ptr < src_ptr + num)
	{
		src_ptr += num;
		dst_ptr += num;
		while(num--)
		{
			*--dst_ptr = *--src_ptr;
		}
	}
	else
	{
		while(num--)
		{
			*dst_ptr++ = *src_ptr++;
		}
	}

	return dest;
}