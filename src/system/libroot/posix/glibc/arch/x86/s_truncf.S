/* Truncate float value.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

#include <machine/asm.h>

ENTRY(__truncf)
	flds	4(%esp)
	subl	$8, %esp
	fstcw	4(%esp)
	movl	$0xc00, %edx
	orl	4(%esp), %edx
	movl	%edx, (%esp)
	fldcw	(%esp)
	frndint
	fldcw	4(%esp)
	addl	$8, %esp
	ret
END(__truncf)
weak_alias (__truncf, truncf)
