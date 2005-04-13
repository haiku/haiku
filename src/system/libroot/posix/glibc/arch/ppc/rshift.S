/* Shift a limb right, low level routine.
   Copyright (C) 1995, 1997, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <sysdep.h>
#include <bp-sym.h>
#include <bp-asm.h>

/* INPUT PARAMETERS
   res_ptr	r3
   s1_ptr	r4
   size		r5
   cnt		r6  */

ENTRY (BP_SYM (__mpn_rshift))
#if __BOUNDED_POINTERS__
	slwi r10,r5,2		/* convert limbs to bytes */
	CHECK_BOUNDS_BOTH_WIDE (r3, r8, r9, r10)
	CHECK_BOUNDS_BOTH_WIDE (r4, r8, r9, r10)
#endif
	mtctr	r5		# copy size into CTR
	addi	r7,r3,-4	# move adjusted res_ptr to free return reg
	subfic	r8,r6,32
	lwz	r11,0(r4)	# load first s1 limb
	slw	r3,r11,r8	# compute function return value
	bdz	L(1)

L(0):	lwzu	r10,4(r4)
	srw	r9,r11,r6
	slw	r12,r10,r8
	or	r9,r9,r12
	stwu	r9,4(r7)
	bdz	L(2)
	lwzu	r11,4(r4)
	srw	r9,r10,r6
	slw	r12,r11,r8
	or	r9,r9,r12
	stwu	r9,4(r7)
	bdnz	L(0)

L(1):	srw	r0,r11,r6
	stw	r0,4(r7)
	blr

L(2):	srw	r0,r10,r6
	stw	r0,4(r7)
	blr
END (BP_SYM (__mpn_rshift))
