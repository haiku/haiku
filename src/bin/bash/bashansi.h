/* bashansi.h -- Typically included information required by picky compilers. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2, or (at your option) any later
   version.

   Bash is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License along
   with Bash; see the file COPYING.  If not, write to the Free Software
   Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if !defined (_BASHANSI_H_)
#define _BASHANSI_H_

#if defined (HAVE_STRING_H)
#  if ! defined (STDC_HEADERS) && defined (HAVE_MEMORY_H)
#    include <memory.h>
#  endif
#  include <string.h>
#endif /* !HAVE_STRING_H */

#if defined (HAVE_STRINGS_H)
#  include <strings.h>
#endif /* !HAVE_STRINGS_H */

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#endif /* !_BASHANSI_H_ */
