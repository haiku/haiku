/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <ctype.h>

#define _U	0x01	/* upper */
#define _L	0x02	/* lower */
#define _D	0x04	/* digit */
#define _C	0x08	/* cntrl */
#define _P	0x10	/* punct */
#define _S	0x20	/* white space (space/lf/tab) */
#define _X	0x40	/* hex digit */
#define _SP	0x80	/* hard space (0x20) */

#define ismask(x) (_ctype[(int)(unsigned char)(x)])

static unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,			/* 0-7 */
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,		/* 8-15 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 16-23 */
_C,_C,_C,_C,_C,_C,_C,_C,			/* 24-31 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,			/* 32-39 */
_P,_P,_P,_P,_P,_P,_P,_P,			/* 40-47 */
_D,_D,_D,_D,_D,_D,_D,_D,			/* 48-55 */
_D,_D,_P,_P,_P,_P,_P,_P,			/* 56-63 */
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,	/* 64-71 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 72-79 */
_U,_U,_U,_U,_U,_U,_U,_U,			/* 80-87 */
_U,_U,_U,_P,_P,_P,_P,_P,			/* 88-95 */
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,	/* 96-103 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 104-111 */
_L,_L,_L,_L,_L,_L,_L,_L,			/* 112-119 */
_L,_L,_L,_P,_P,_P,_P,_C,			/* 120-127 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 128-143 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,		/* 144-159 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,   /* 160-175 */
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,       /* 176-191 */
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,       /* 192-207 */
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,       /* 208-223 */
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,       /* 224-239 */
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};      /* 240-255 */

unsigned char tolower(unsigned char c)
{
	if(isupper(c))
		c -= 'A'-'a';
	return c;
}

unsigned char toupper(unsigned char c)
{
	if(islower(c))
		c -= 'a'-'A';
	return c;
}

int isalnum(int c)
{
	return ((ismask(c)&(_U|_L|_D)) != 0);
}

int isalpha(int c)
{
	return ((ismask(c)&(_U|_L)) != 0);
}

int iscntrl(int c)
{
	return ((ismask(c)&(_C)) != 0);
}

int isdigit(int c)
{
	return ((ismask(c)&(_D)) != 0);
}

int isgraph(int c)
{
	return ((ismask(c)&(_P|_U|_L|_D)) != 0);
}

int islower(int c)
{
	return ((ismask(c)&(_L)) != 0);
}

int isprint(int c)
{
	return ((ismask(c)&(_P|_U|_L|_D|_SP)) != 0);
}

int ispunct(int c)
{
	return ((ismask(c)&(_P)) != 0);
}

int isspace(int c)
{
	return ((ismask(c)&(_S)) != 0);
}

int isupper(int c)
{
	return ((ismask(c)&(_U)) != 0);
}

int isxdigit(int c)
{
	return ((ismask(c)&(_D|_X)) != 0);
}

int isascii(int c)
{
	return ((unsigned char)c <= 0x7f);
}

int toascii(int c)
{
	return ((unsigned char)c & 0x7f);
}

