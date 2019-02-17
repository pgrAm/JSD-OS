#include <string.h>
#include <ctype.h>

char* strtok(char* str, const char* delimiters)
{
    static char* buffer;
	
    if(str != NULL) 
	{
		buffer = str;
	}
    if(buffer[0] == '\0') 
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

void* memcpy(void* dest, const void* src, size_t num) 
{
	char* dst8 = (char*)dest;
    char* src8 = (char*)src;

    while(num--) 
	{
		*dst8++ = *src8++;
    }
    return dest;
}

int memcmp (const void * ptr1, const void * ptr2, size_t num)
{
	const unsigned char* ptra = (const unsigned char*)ptr1;
	const unsigned char* ptrb = (const unsigned char*)ptr2;

    while(num--) 
	{
		unsigned char result = *ptra++ - *ptrb++;

		if(result != 0)
		{
			return result;
		}
    }
	
    return 0;
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

void* memset(void* ptr, int value, size_t num)
{
	unsigned char* ptr8 = (unsigned char*)ptr;

    while(num--) 
	{
		*ptr8++ = (unsigned char)value;
    }
    return ptr;
}

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