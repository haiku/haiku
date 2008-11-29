/* Ensures that the FINDLIB_REPLACE_FUNCS macro in configure.in works 
   Copyright (C) 2004, 2005 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/* Written by James Youngman. */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


extern void forcefindlib(void);	/* prevent GCC warning... */



/* forcefindlib
 *
 * This function exists only to be pulled into libfind.a by the 
 * FINDLIB_REPLACE_FUNCS macro in configure.in.   We already have 
 * AC_REPLACE_FUNCS, but that adds to LIBOBJS, and that's a gnulib thing
 * in the case of findutils.  Hence we have out own library of replacement 
 * functions which aren't in gnulib (or aren't in it any more).  An example
 * of this is waitpid().   I develop on a system that doesn't 
 * lack waitpid, for example.   Therefore FINDLIB_REPLACE_FUNCS(waitpid)
 * never puts waitpid.o into FINDLIBOBJS.  Hence, to ensure that these 
 * macros are tested every time, we use FINDLIB_REPLACE_FUNCS on a function 
 * that never exists anywhere, so always needs to be pulled in.  That function 
 * is forcefindlib().
 */
void
forcefindlib(void)
{
  /* does nothing, exists only to ensure that FINDLIB_REPLACE_FUNCS works. */
}
