/* Bounded-pointer definitions for PowerPC assembler.
   Copyright (C) 2000 Free Software Foundation, Inc.
   Contributed by Greg McGary <greg@mcgary.org>
   This file is part of the GNU C Library.  Its master source is NOT part of
   the C library, however.  The master source lives in the GNU MP Library.

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

#if __BOUNDED_POINTERS__

/* Byte offsets of BP components.  */
# define oVALUE	0
# define oLOW	4
# define oHIGH	8

/* Don't check bounds, just convert the BP register to its simple
   pointer value.  */

# define DISCARD_BOUNDS(rBP)			\
	lwz	rBP, oVALUE(rBP)

/* Check low bound, with the side effect that the BP register is converted
   its simple pointer value.  Move the high bound into a register for
   later use.  */

# define CHECK_BOUNDS_LOW(rBP, rLOW, rHIGH)	\
	lwz	rHIGH, oHIGH(rBP);		\
	lwz	rLOW, oLOW(rBP);		\
	lwz	rBP, oVALUE(rBP);		\
	twllt	rBP, rLOW

/* Check the high bound, which is in a register, using the given
   conditional trap instruction.  */

# define CHECK_BOUNDS_HIGH(rVALUE, rHIGH, TWLcc) \
	TWLcc	rVALUE, rHIGH

/* Check the high bound, which is stored at the return-value's high
   bound slot, using the given conditional trap instruction.  */

# define CHECK_BOUNDS_HIGH_RTN(rVALUE, rHIGH, TWLcc)	\
	lwz	rHIGH, oHIGH(rRTN);			\
	TWLcc	rVALUE, rHIGH

/* Check both bounds, with the side effect that the BP register is
   converted to its simple pointer value.  */

# define CHECK_BOUNDS_BOTH(rBP, rLOW, rHIGH)	\
	CHECK_BOUNDS_LOW(rBP, rLOW, rHIGH);	\
	twlge	rBP, rHIGH

/* Check bounds on a memory region of given length, with the side
   effect that the BP register is converted to its simple pointer
   value.  */

# define CHECK_BOUNDS_BOTH_WIDE(rBP, rLOW, rHIGH, rLENGTH)	\
	CHECK_BOUNDS_LOW (rBP, rLOW, rHIGH);			\
	sub	rHIGH, rHIGH, rLENGTH;				\
	twlgt	rBP, rHIGH

# define CHECK_BOUNDS_BOTH_WIDE_LIT(rBP, rLOW, rHIGH, LENGTH)	\
	CHECK_BOUNDS_LOW (rBP, rLOW, rHIGH);			\
	subi	rHIGH, rHIGH, LENGTH;				\
	twlgt	rBP, rHIGH

/* Store a pointer value register into the return-value's pointer
   value slot.  */

# define STORE_RETURN_VALUE(rVALUE)		\
	stw	rVALUE, oVALUE(rRTN)

/* Store a low and high bounds into the return-value's pointer bounds
   slots.  */

# define STORE_RETURN_BOUNDS(rLOW, rHIGH)	\
	stw	rLOW, oLOW(rRTN);		\
	stw	rHIGH, oHIGH(rRTN)

/* Stuff zero value/low/high into the BP addressed by rRTN.  */

# define RETURN_NULL_BOUNDED_POINTER		\
	li	r4, 0;				\
	STORE_RETURN_VALUE (r4);		\
	STORE_RETURN_BOUNDS (r4, r4)

#else

# define DISCARD_BOUNDS(rBP)
# define CHECK_BOUNDS_LOW(rBP, rLOW, rHIGH)
# define CHECK_BOUNDS_HIGH(rVALUE, rHIGH, TWLcc)
# define CHECK_BOUNDS_HIGH_RTN(rVALUE, rHIGH, TWLcc)
# define CHECK_BOUNDS_BOTH(rBP, rLOW, rHIGH)
# define CHECK_BOUNDS_BOTH_WIDE(rBP, rLOW, rHIGH, rLENGTH)
# define CHECK_BOUNDS_BOTH_WIDE_LIT(rBP, rLOW, rHIGH, LENGTH)
# define STORE_RETURN_VALUE(rVALUE)
# define STORE_RETURN_BOUNDS(rLOW, rHIGH)

# define RETURN_NULL_BOUNDED_POINTER li rRTN, 0

#endif

