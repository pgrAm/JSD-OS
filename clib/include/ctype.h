#ifndef CTYPE_H
#define	CTYPE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CT_UP	0x01	// upper case 
#define CT_LOW	0x02	// lower case 
#define CT_DIG	0x04	// digit 
#define CT_CTL	0x08	// control 
#define CT_PUN	0x10	// punctuation
#define CT_WHT	0x20	// white space (space/cr/lf/tab)
#define CT_HEX	0x40	// hex digit 
#define CT_SP	0x80	// hard space (0x20) 
		
extern const char *_ctype;

static inline int isalnum(int c)
{ 
	return _ctype[(unsigned)(c)] & (CT_UP | CT_LOW | CT_DIG);
}
static inline int isalpha(int c)
{ 
	return _ctype[(unsigned)(c)] & (CT_UP | CT_LOW);
}
static inline int iscntrl(int c)
{ 
	return _ctype[(unsigned)(c)] & CT_CTL;
}
static inline int isdigit(int c)
{ 
	return _ctype[(unsigned)c] & CT_DIG;
}
static inline int isgraph(int c)	{
	return _ctype[(unsigned)(c)] & (CT_PUN | CT_UP | CT_LOW | CT_DIG);
}
static inline int islower(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_LOW));
}
static inline int isprint(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_PUN | CT_UP | CT_LOW | CT_DIG | CT_SP));
}
static inline int ispunct(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_PUN));
}
static inline int isspace(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_WHT));
}
static inline int isupper(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_UP));
}
static inline int isxdigit(int c)	{
	return (_ctype[(unsigned)(c)] & (CT_DIG | CT_HEX));
}
static inline int isascii(int c)	{
	return ((unsigned)(c) <= 0x7F);
}
static inline int toascii(int c)	{
	return ((unsigned)(c) & 0x7F);
}
static inline int tolower(int c)	{
	return (isupper(c) ? c + 'a' - 'A' : c);
}
static inline int toupper(int c) {
	return (islower(c) ? c + 'A' - 'a' : c);
}

#ifdef __cplusplus
}
#endif

#endif