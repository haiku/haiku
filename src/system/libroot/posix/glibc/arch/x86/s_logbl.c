/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * Public domain.
 */

#include <math_private.h>

long double
__logbl (long double x)
{
  long double res;

  asm ("fxtract\n"
       "fstp	%%st" : "=t" (res) : "0" (x));
  return res;
}

weak_alias (__logbl, logbl)
