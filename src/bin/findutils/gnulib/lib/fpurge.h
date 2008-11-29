/* Flushing buffers of a FILE stream.
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _GL_FPURGE_H
#define _GL_FPURGE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Discard all pending buffered I/O on the stream STREAM.
   STREAM must not be wide-character oriented.
   Return 0 if successful.  Upon error, return -1 and set errno.  */
#if HAVE_FPURGE
# define fpurge rpl_fpurge
#endif
extern int fpurge (FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* _GL_FPURGE_H */
