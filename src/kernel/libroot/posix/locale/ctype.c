/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
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


// standard ASCII tables
// ToDo: These should probably be revised for ISO-8859-1
// ToDo: change the tables depending on the current locale

const unsigned short int *__ctype_b = (const unsigned short int[]) {
	/*   0 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*   8 */	_IScntrl, _ISblank|_IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl, _IScntrl,
	/*  16 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*  24 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/*  32 */	_ISblank|_ISspace|_ISprint, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  40 */	_ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  48 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph,
	/*  56 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  64 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  72 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  80 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
	/*  88 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
	/*  96 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 104 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 112 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
	/* 120 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _IScntrl,
	/* 128 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 136 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 144 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 152 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 160 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 168 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 176 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 184 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 192 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 200 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 208 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 216 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 224 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 232 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 240 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
	/* 248 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
};

const int *__ctype_tolower = (const int[]) {
	/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
	/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
	/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
	/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
	/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
	/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
	/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
	/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
	/*  64 */	 64, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	/*  72 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	/*  80 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	/*  88 */	'x', 'y', 'z',  91,  92,  93,  94,  95,
	/*  96 */	 96, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	/* 104 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	/* 112 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	/* 120 */	'x', 'y', 'z', 123, 124, 125, 126, 127,
	/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
	/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
	/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
	/* 248 */	248, 249, 250, 251, 252, 253, 254, 255,
};

const int *__ctype_toupper = (const int[]) {
	/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
	/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
	/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
	/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
	/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
	/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
	/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
	/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
	/*  64 */	 64, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	/*  72 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	/*  80 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	/*  88 */	'X', 'Y', 'Z',  91,  92,  93,  94,  95,
	/*  96 */	 96, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	/* 104 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	/* 112 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	/* 120 */	'X', 'Y', 'Z', 123, 124, 125, 126, 127,
	/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
	/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
	/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
	/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
	/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
	/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
	/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
	/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
	/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
	/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
	/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
	/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
	/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
	/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
	/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
	/* 248 */	248, 249, 250, 251, 252, 253, 254, 255,
};


int
isalnum(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & (_ISupper | _ISlower | _ISdigit);

	return 0;
}


int
isalpha(int c)
{
	if (c >= 0 && c < 256)
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
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISblank;

	return 0;
}


int
iscntrl(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _IScntrl;

	return 0;
}


int
isdigit(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISdigit;

	return 0;
}


int
isgraph(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISgraph;

	return 0;
}


int
islower(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISlower;

	return 0;
}


int
isprint(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISprint;

	return 0;
}


int
ispunct(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISpunct;

	return 0;
}


int
isspace(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISspace;

	return 0;
}


int
isupper(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_b[c] & _ISupper;

	return 0;
}


int
isxdigit(int c)
{
	if (c >= 0 && c < 256)
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
	if (c >= 0 && c < 256)
		return __ctype_tolower[c];

	return c;
}


int
toupper(int c)
{
	if (c >= 0 && c < 256)
		return __ctype_toupper[c];

	return c;
}


