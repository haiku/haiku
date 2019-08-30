/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <ctype.h>


// disable macros defined in ctype.h
#undef isalnum
#undef isalpha
#undef isascii
#undef isblank
#undef iscntrl
#undef isdigit
#undef islower
#undef isgraph
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#undef toascii
#undef tolower
#undef toupper


extern "C"
{


unsigned short int __ctype_mb_cur_max = 1;


unsigned short
__ctype_get_mb_cur_max()
{
	return __ctype_mb_cur_max;
}


int
isalnum(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & (_ISupper | _ISlower | _ISdigit);

	return 0;
}


int
isalpha(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & (_ISupper | _ISlower);

	return 0;
}


int
isascii(int c)
{
	/* ASCII characters have bit 8 cleared */
	return (c & ~0x7f) == 0;
}


int
isblank(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISblank;

	return 0;
}


int
iscntrl(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _IScntrl;

	return 0;
}


int
isdigit(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISdigit;

	return 0;
}


int
isgraph(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISgraph;

	return 0;
}


int
islower(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISlower;

	return 0;
}


int
isprint(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISprint;

	return 0;
}


int
ispunct(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISpunct;

	return 0;
}


int
isspace(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISspace;

	return 0;
}


int
isupper(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISupper;

	return 0;
}


int
isxdigit(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_b[c] & _ISxdigit;

	return 0;
}


int
toascii(int c)
{
	/* Clear higher bits */
	return c & 0x7f;
}


int
tolower(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_tolower[c];

	return c;
}


int
toupper(int c)
{
	if (c >= -128 && c < 256)
		return __ctype_toupper[c];

	return c;
}


}
