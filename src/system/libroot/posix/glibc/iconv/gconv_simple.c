/* Simple transformations functions.
   Copyright (C) 1997-2003, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <gconv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gconv_int.h>

#include <libroot/wchar_private.h>

#define BUILTIN_TRANSFORMATION(From, To, Cost, Name, Fct, BtowcFct, \
			       MinF, MaxF, MinT, MaxT) \
  extern int Fct (struct __gconv_step *, struct __gconv_step_data *,	      \
		  __const unsigned char **, __const unsigned char *,	      \
		  unsigned char **, size_t *, int, int);
#include "gconv_builtin.h"


/* Convert from ISO 646-IRV to the internal (UCS4-like) format.  */
#define DEFINE_INIT			0
#define DEFINE_FINI			0
#define MIN_NEEDED_FROM		1
#define MIN_NEEDED_TO		4
#define FROM_DIRECTION		1
#define FROM_LOOP			ascii_internal_loop
#define TO_LOOP				ascii_internal_loop /* This is not used.  */
#define FUNCTION_NAME		__gconv_transform_ascii_internal
#define ONE_DIRECTION		1

#define MIN_NEEDED_INPUT	MIN_NEEDED_FROM
#define MIN_NEEDED_OUTPUT	MIN_NEEDED_TO
#define LOOPFCT				FROM_LOOP
#define BODY																\
{																			\
    if (__builtin_expect(*inptr > '\x7f', 0)) {								\
		/* The value is too large.  We don't try transliteration here since	\
		   this is not an error because of the lack of possibilities to		\
		   represent the result.  This is a genuine bug in the input since	\
		   ASCII does not allow such values.  */							\
		STANDARD_FROM_LOOP_ERR_HANDLER(1);									\
    } else {																\
    	/* It's an one byte sequence.  */									\
		*((uint32_t*)outptr) = *inptr++;									\
		outptr += sizeof(uint32_t);											\
    }																		\
}
#define LOOP_NEED_FLAGS
#include <iconv/loop.c>
#include <iconv/skeleton.c>


/* Convert from the internal (UCS4-like) format to ISO 646-IRV.  */
#define DEFINE_INIT			0
#define DEFINE_FINI			0
#define MIN_NEEDED_FROM		4
#define MIN_NEEDED_TO		1
#define FROM_DIRECTION		1
#define FROM_LOOP			internal_ascii_loop
#define TO_LOOP				internal_ascii_loop /* This is not used.  */
#define FUNCTION_NAME		__gconv_transform_internal_ascii
#define ONE_DIRECTION		1

#define MIN_NEEDED_INPUT	MIN_NEEDED_FROM
#define MIN_NEEDED_OUTPUT	MIN_NEEDED_TO
#define LOOPFCT				FROM_LOOP
#define BODY																\
{																			\
    if (__builtin_expect(*((const uint32_t*)inptr) > 0x7f, 0)) {			\
		UNICODE_TAG_HANDLER(*((const uint32_t*)inptr), 4);					\
		STANDARD_TO_LOOP_ERR_HANDLER(4);									\
    } else {																\
		/* It's an one byte sequence.  */									\
		*outptr++ = *((const uint32_t*)inptr);								\
		inptr += sizeof(uint32_t);											\
    }																		\
}
#define LOOP_NEED_FLAGS
#include <iconv/loop.c>
#include <iconv/skeleton.c>


/* Convert from multibyte to wchar_t format.  */
#define DEFINE_INIT			0
#define DEFINE_FINI			0
#define MIN_NEEDED_FROM		1
#define MAX_NEEDED_FROM		MB_LEN_MAX
#define MIN_NEEDED_TO		4
#define MAX_NEEDED_TO		4
#define FROM_DIRECTION		1
#define FROM_LOOP			multibyte_wchar_loop
#define TO_LOOP				multibyte_wchar_loop /* This is not used.  */
#define FUNCTION_NAME		__gconv_transform_multibyte_wchar
#define ONE_DIRECTION		1

#define MIN_NEEDED_INPUT	MIN_NEEDED_FROM
#define MAX_NEEDED_INPUT	MAX_NEEDED_FROM
#define MIN_NEEDED_OUTPUT	MIN_NEEDED_TO
#define MAX_NEEDED_OUTPUT	MAX_NEEDED_TO
#define LOOPFCT				FROM_LOOP
#define BODY \
{									      									\
	size_t inLen = inend - inptr;											\
	mbstate_t *state = step_data->__statep;									\
	size_t result 															\
    	= __mbrtowc((wchar_t*)outptr, (const char*)inptr, inLen, state);	\
	if (result == (size_t)-1) {									      		\
		/* illegal character, skip it */									\
		STANDARD_FROM_LOOP_ERR_HANDLER(1);									\
	} else if (result == (size_t)-2) {										\
		/* input too short, do nothing */									\
	} else if (result == 0) {												\
		/* termination character found */									\
		outptr += sizeof(wchar_t);											\
		++inptr;															\
	} else {																\
		/* a character has been converted */								\
		inptr += result;													\
      	outptr += sizeof(wchar_t);											\
	}																		\
}
#define LOOP_NEED_FLAGS
#include <iconv/loop.c>
#include <iconv/skeleton.c>


/* Convert from wchar_t to multibyte format.  */
#define DEFINE_INIT			0
#define DEFINE_FINI			0
#define MIN_NEEDED_FROM		4
#define MAX_NEEDED_FROM		4
#define MIN_NEEDED_TO		MB_LEN_MAX
#define MAX_NEEDED_TO		MB_LEN_MAX
#define FROM_DIRECTION		1
#define FROM_LOOP			wchar_multibyte_loop
#define TO_LOOP				wchar_multibyte_loop /* This is not used.  */
#define FUNCTION_NAME		__gconv_transform_wchar_multibyte
#define ONE_DIRECTION		1

#define MIN_NEEDED_INPUT	MIN_NEEDED_FROM
#define MAX_NEEDED_INPUT	MAX_NEEDED_FROM
#define MIN_NEEDED_OUTPUT	MIN_NEEDED_TO
#define MAX_NEEDED_OUTPUT	MAX_NEEDED_TO
#define LOOPFCT				FROM_LOOP
#define BODY \
{									      									\
	mbstate_t *state = step_data->__statep;									\
	size_t result = __wcrtomb((char*)outptr, *(wchar_t*)inptr, state);		\
	if (result == (size_t)-1) {									      		\
		/* illegal character, skip it */									\
		STANDARD_TO_LOOP_ERR_HANDLER(sizeof(wchar_t));						\
	} else if (result == 0) {												\
		/* termination character found */									\
		inptr += sizeof(wchar_t);											\
		++outptr;															\
	} else {																\
		/* a character has been converted */								\
		outptr += result;													\
      	inptr += sizeof(wchar_t);											\
	}																		\
}
#define LOOP_NEED_FLAGS
#include <iconv/loop.c>
#include <iconv/skeleton.c>
