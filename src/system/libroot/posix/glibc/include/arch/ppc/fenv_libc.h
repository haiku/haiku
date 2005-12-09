/* Internal libc stuff for floating point environment routines.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

#ifndef _FENV_LIBC_H
#define _FENV_LIBC_H	1

#include <fenv.h>

/* The sticky bits in the FPSCR indicating exceptions have occurred.  */
#define FPSCR_STICKY_BITS ((FE_ALL_EXCEPT | FE_ALL_INVALID) & ~FE_INVALID)

/* Equivalent to fegetenv, but returns a fenv_t instead of taking a
   pointer.  */
#define fegetenv_register() \
        ({ fenv_t env; asm volatile ("mffs %0" : "=f" (env)); env; })

/* Equivalent to fesetenv, but takes a fenv_t instead of a pointer.  */
#define fesetenv_register(env) \
        ({ double d = (env); asm volatile ("mtfsf 0xff,%0" : : "f" (d)); })

/* This very handy macro:
   - Sets the rounding mode to 'round to nearest';
   - Sets the processor into IEEE mode; and
   - Prevents exceptions from being raised for inexact results.
   These things happen to be exactly what you need for typical elementary
   functions.  */
#define relax_fenv_state() asm ("mtfsfi 7,0")

/* Set/clear a particular FPSCR bit (for instance,
   reset_fpscr_bit(FPSCR_VE);
   prevents INVALID exceptions from being raised).  */
#define set_fpscr_bit(x) asm volatile ("mtfsb1 %0" : : "i"(x))
#define reset_fpscr_bit(x) asm volatile ("mtfsb0 %0" : : "i"(x))

typedef union
{
  fenv_t fenv;
  unsigned int l[2];
} fenv_union_t;

/* Definitions of all the FPSCR bit numbers */
enum {
  FPSCR_FX = 0,    /* exception summary */
  FPSCR_FEX,       /* enabled exception summary */
  FPSCR_VX,        /* invalid operation summary */
  FPSCR_OX,        /* overflow */
  FPSCR_UX,        /* underflow */
  FPSCR_ZX,        /* zero divide */
  FPSCR_XX,        /* inexact */
  FPSCR_VXSNAN,    /* invalid operation for SNaN */
  FPSCR_VXISI,     /* invalid operation for Inf-Inf */
  FPSCR_VXIDI,     /* invalid operation for Inf/Inf */
  FPSCR_VXZDZ,     /* invalid operation for 0/0 */
  FPSCR_VXIMZ,     /* invalid operation for Inf*0 */
  FPSCR_VXVC,      /* invalid operation for invalid compare */
  FPSCR_FR,        /* fraction rounded [fraction was incremented by round] */
  FPSCR_FI,        /* fraction inexact */
  FPSCR_FPRF_C,    /* result class descriptor */
  FPSCR_FPRF_FL,   /* result less than (usually, less than 0) */
  FPSCR_FPRF_FG,   /* result greater than */
  FPSCR_FPRF_FE,   /* result equal to */
  FPSCR_FPRF_FU,   /* result unordered */
  FPSCR_20,        /* reserved */
  FPSCR_VXSOFT,    /* invalid operation set by software */
  FPSCR_VXSQRT,    /* invalid operation for square root */
  FPSCR_VXCVI,     /* invalid operation for invalid integer convert */
  FPSCR_VE,        /* invalid operation exception enable */
  FPSCR_OE,        /* overflow exception enable */
  FPSCR_UE,        /* underflow exception enable */
  FPSCR_ZE,        /* zero divide exception enable */
  FPSCR_XE,        /* inexact exception enable */
  FPSCR_NI         /* non-IEEE mode (typically, no denormalised numbers) */
  /* the remaining two least-significant bits keep the rounding mode */
};

/* This operation (i) sets the appropriate FPSCR bits for its
   parameter, (ii) converts SNaN to the corresponding NaN, and (iii)
   otherwise passes its parameter through unchanged (in particular, -0
   and +0 stay as they were).  The `obvious' way to do this is optimised
   out by gcc.  */
#define f_wash(x) \
   ({ double d; asm volatile ("fmul %0,%1,%2" \
			      : "=f"(d) \
			      : "f" (x), "f"((float)1.0)); d; })
#define f_washf(x) \
   ({ float f; asm volatile ("fmuls %0,%1,%2" \
			     : "=f"(f) \
			     : "f" (x), "f"((float)1.0)); f; })
 
#endif /* fenv_libc.h */
