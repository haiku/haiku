# Copyright (C) 1991-1999,2000,2001,2002 Free Software Foundation, Inc.
# This file is part of the GNU C Library.

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

#
#	Sub-makefile for string portion of library.
#
subdir	:= string

headers	:= string.h strings.h memory.h endian.h bits/endian.h \
	   argz.h envz.h byteswap.h bits/byteswap.h bits/string.h \
	   bits/string2.h

routines	:= strcat strchr strcmp strcoll strcpy strcspn		\
		   strverscmp strdup strndup				\
		   strerror _strerror strlen strnlen			\
		   strncat strncmp strncpy				\
		   strrchr strpbrk strsignal strspn strstr strtok	\
		   strtok_r strxfrm memchr memcmp memmove memset	\
		   mempcpy bcopy bzero ffs ffsll stpcpy stpncpy		\
		   strcasecmp strncase strcasecmp_l strncase_l		\
		   memccpy memcpy wordcopy strsep strcasestr		\
		   swab strfry memfrob memmem rawmemchr strchrnul	\
		   $(addprefix argz-,append count create ctsep next	\
			             delete extract insert stringify	\
				     addsep replace)			\
		   envz basename					\
		   strcoll_l strxfrm_l string-inlines memrchr

# Gcc internally generates calls to unbounded memcpy and memset
# for -fbounded-pointer compiles.  Glibc uses memchr for explicit checks.
o-objects.ob	:= memcpy.o memset.o memchr.o

strop-tests	:= memchr memcmp memcpy memmove mempcpy memset stpcpy	\
		   stpncpy strcat strchr strcmp strcpy strcspn strlen	\
		   strncmp strncpy strpbrk strrchr strspn
tests		:= tester inl-tester noinl-tester testcopy test-ffs	\
		   tst-strlen stratcliff tst-svc tst-inlcall		\
		   bug-strncat1 bug-strspn1 bug-strpbrk1 tst-bswap	\
		   tst-strtok tst-strxfrm bug-strcoll1			\
		   $(addprefix test-,$(strop-tests))
distribute	:= memcopy.h pagecopy.h tst-svc.expect test-string.h


include ../Rules

tester-ENV = LANGUAGE=C
inl-tester-ENV = LANGUAGE=C
noinl-tester-ENV = LANGUAGE=C
tst-strxfrm-ENV = LOCPATH=$(common-objpfx)localedata
bug-strcoll1-ENV = LOCPATH=$(common-objpfx)localedata
CFLAGS-noinl-tester.c = -fno-builtin
CFLAGS-tst-strlen.c = -fno-builtin
CFLAGS-stratcliff.c = -fno-builtin
CFLAGS-test-ffs.c = -fno-builtin

ifeq ($(cross-compiling),no)
tests: $(objpfx)tst-svc.out
$(objpfx)tst-svc.out: tst-svc.input $(objpfx)tst-svc
	GCONV_PATH=$(common-objpfx)iconvdata LC_ALL=C \
	  $(built-program-cmd) < $(word 1,$^) > $@
	-cmp tst-svc.expect $(objpfx)tst-svc.out
endif
