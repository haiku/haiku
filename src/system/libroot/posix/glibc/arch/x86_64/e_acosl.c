/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Adapted for `long double' by Ulrich Drepper <drepper@cygnus.com>.
 */

#include "math_private.h"

long double
__ieee754_acosl (long double x)
{
  long double res;

  /* acosl = atanl (sqrtl(1 - x^2) / x) */
  asm (	"fld	%%st\n"
	"fmul	%%st(0)\n"		/* x^2 */
	"fld1\n"
	"fsubp\n"			/* 1 - x^2 */
	"fsqrt\n"			/* sqrtl (1 - x^2) */
	"fxch	%%st(1)\n"
	"fpatan"
	: "=t" (res) : "0" (x) : "st(1)");
  return res;
}
