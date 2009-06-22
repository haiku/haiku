/* Copyright (C) 1999 Free Software Foundation, Inc.
 This file is part of the GNU C Library.
 Contributed by Ulrich Drepper <drepper@cygnus.com>, 1999.

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

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define TEST(Str, Max, Exp)													\
	n = wcsnlen (L##Str, Max);												\
  	if (n != Exp) {															\
    	result = 1;															\
    	printf ("wcsnlen (L\"%s\", %d) = %d, not %d\n", Str, Max, n, Exp);	\
    }


int
main(void)
{
	int result = 0;
	int n;

	TEST ("0123456789", 0, 0);
	TEST ("0123456789", 1, 1);
	TEST ("0123456789", 2, 2);
	TEST ("0123456789", 3, 3);
	TEST ("0123456789", 4, 4);
	TEST ("0123456789", 5, 5);
	TEST ("0123456789", 6, 6);
	TEST ("0123456789", 7, 7);
	TEST ("0123456789", 8, 8);
	TEST ("0123456789", 9, 9);

	TEST ("01234", 9, 5);

	if (result == 0)
		printf("all ok.\n");

	return result;
}
