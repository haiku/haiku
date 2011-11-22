/*
 * Copyright 2003-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CTYPE_H
#define _CTYPE_H


#ifdef __cplusplus
extern "C" {
#endif

int isalnum(int);
int isalpha(int);
int isascii(int);
int isblank(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int toascii(int);
int tolower(int);
int toupper(int);

enum {
	_ISblank = 0x0001,		/* blank */
	_IScntrl = 0x0002,		/* control */
	_ISpunct = 0x0004,		/* punctuation */
	_ISalnum = 0x0008,		/* alpha-numeric */
	_ISupper = 0x0100,		/* uppercase */
	_ISlower = 0x0200,		/* lowercase */
	_ISalpha = 0x0400,		/* alphabetic */
	_ISdigit = 0x0800,		/* digit */
	_ISxdigit = 0x1000,		/* hexadecimal digit */
	_ISspace = 0x2000,		/* white space */
	_ISprint = 0x4000,		/* printing */
	_ISgraph = 0x8000		/* graphical */
};

/* Characteristics */
extern const unsigned short int *__ctype_b;
/* Case conversions */
extern const int *__ctype_tolower;
extern const int *__ctype_toupper;

#define __isctype(c, type) \
	(__ctype_b[(int)(c)] & (unsigned short int)type)

#define isascii(c) (((c) & ~0x7f) == 0)	/* ASCII characters have bit 8 cleared */
#define toascii(c) ((c) & 0x7f)			/* Clear higher bits */

#define tolower(c) ((int)__ctype_tolower[(int)(c)])
#define toupper(c) ((int)__ctype_toupper[(int)(c)])
#define _tolower(c)	tolower(c)
#define _toupper(c)	toupper(c)

#define isalnum(c)	__isctype((c), _ISalnum)
#define isalpha(c)	__isctype((c), _ISalpha)
#define isblank(c)	__isctype((c), _ISblank)
#define iscntrl(c)	__isctype((c), _IScntrl)
#define isdigit(c)	__isctype((c), _ISdigit)
#define islower(c)	__isctype((c), _ISlower)
#define isgraph(c)	__isctype((c), _ISgraph)
#define isprint(c)	__isctype((c), _ISprint)
#define ispunct(c)	__isctype((c), _ISpunct)
#define isspace(c)	__isctype((c), _ISspace)
#define isupper(c)	__isctype((c), _ISupper)
#define isxdigit(c)	__isctype((c), _ISxdigit)

extern unsigned short int __ctype_mb_cur_max;

#ifdef __cplusplus
}
#endif

#endif /* _CTYPE_H */
