/*
** Distributed under the terms of the OpenBeOS License.
*/

/* GCC's stdarg.h defines _STDARG_H - and we intend to include
 * this header with the line below.
 * It contains the actual platform dependent varargs definitions.
 */

#ifndef	_STDARG_H
#	include_next <stdarg.h>
#endif
