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
#ifndef FAT_ICONV_H
#define FAT_ICONV_H


// Modified to support the Haiku FAT driver.

#ifndef FS_SHELL
#include <ctype.h>
#include <string.h>

#include <fs_info.h>
#include <libs/iconv/iconv.h>
#else
#include <fssh_api_wrapper.h>
#endif


// In Haiku, the function names iconv_open and iconv_close are already taken.
int fat_iconv_open(const char* to, const char* from, void** handle);
int fat_iconv_close(void* handle);
int iconv_conv(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft);
int iconv_convchr(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft);
int iconv_convchr_case(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
	size_t* outbytesleft, int casetype);

#define KICONV_LOWER 1 /* tolower converted character */
#define KICONV_UPPER 2 /* toupper converted character */
#define KICONV_FROM_LOWER 4 /* tolower source character, then convert */
#define KICONV_FROM_UPPER 8 /* toupper source character, then convert */
#define KICONV_WCTYPE 16 /* towlower/towupper characters */

/*
 * Bridge struct of iconv functions
 */
struct iconv_functions {
	int (*open)(const char* to, const char* from, void** handle);
	int (*close)(void* handle);
	int (*conv)(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
		size_t* outbytesleft);
	int (*convchr)(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
		size_t* outbytesleft);
	int (*convchr_case)(void* handle, const char** inbuf, size_t* inbytesleft, char** outbuf,
		size_t* outbytesleft, int casetype);
};

#define VFS_DECLARE_ICONV(fsname) extern struct iconv_functions* fsname##_iconv;


#endif // FAT_ICONV_H
