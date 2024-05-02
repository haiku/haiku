/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2000-2001 Boris Popov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


// Modified to support the Haiku FAT driver.

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/cdefs.h"
#include "sys/malloc.h"
#include "sys/iconv.h"


int printf(const char* format, ...);


/*! Create an instance of a converter.

*/
int
fat_iconv_open(const char* to, const char* from, void** handle)
{
#ifdef USER
	iconv_t descriptor = iconv_open(to, from);
	if (descriptor == (iconv_t)(-1))
		return errno;
	*handle = descriptor;

	return 0;
#else
	return ENOTSUP;
#endif // !USER
}


int
fat_iconv_close(void* handle)
{
#ifdef USER
	iconv_t* descriptor = (iconv_t*)handle;
	int error = 0;

	error = iconv_close(descriptor);

	return error;
#else
	return ENOTSUP;
#endif // !USER
}


int
iconv_conv(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft)
{
#ifdef USER
	while (iconv(handle, __DECONST(char**, inbuf), inbytesleft, outbuf, outbytesleft) != (size_t)-1
		&& *inbytesleft > 0 && strlen(*inbuf) > 0)
		;

	return 0;
#else
	return ENOTSUP;
#endif // !USER
}


int
iconv_convchr(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft)
{
#ifdef USER
	int status = iconv(handle, __DECONST(char**, inbuf), inbytesleft, outbuf, outbytesleft);

	return status;
#else
	return ENOTSUP;
#endif // !USER
}


/*! Only those casetype values used by the FAT driver are implemented.

*/
int
iconv_convchr_case(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft, int casetype)
{
#ifdef USER
	size_t status = 0;
	int inbufAdvanced = 0;

	if (casetype == KICONV_FROM_LOWER || casetype == KICONV_FROM_UPPER) {
		char finalInbuf[4];
		char* finalInbufPtr = finalInbuf;
		int (*tocase)(int ch) = (casetype == KICONV_FROM_LOWER) ? tolower : toupper;

		// The characters must be set to the indicated case before conversion.
		// Since iconv only takes one char at a time, just set 1 (possibly multibyte)
		// character, not the whole inbuf.
		int i;
		for (i = 0; i < 4; i++)
			finalInbuf[i] = tocase((*inbuf)[i]);

		status = iconv(handle, &finalInbufPtr, inbytesleft, outbuf, outbytesleft);
		if (status == (size_t)(-1))
			printf("FAT iconv_convchr_case:  %s\n", strerror(errno));

		// iconv has incremented finalInbufPtr instead of inbuf; update inbuf manually
		inbufAdvanced = finalInbufPtr - finalInbuf;
		*inbuf += inbufAdvanced;

		return status;
	}
	if (casetype == KICONV_LOWER) {
		status = iconv(handle, __DECONST(char**, inbuf), inbytesleft, outbuf, outbytesleft);
		// Outbuf will be advanced one slot by iconv each time it's called.
		// Just deal with the one char that is pointed to right now.
		**outbuf = tolower(**outbuf);
		return status;
	}

	printf("FAT iconv_convchr_case: unrecognized casetype\n");
	return -1;
#else
	return ENOTSUP;
#endif // !USER
}
