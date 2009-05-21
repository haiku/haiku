/* Implementation details of FILE streams.
   Copyright (C) 2007-2008 Free Software Foundation, Inc.

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

/* Many stdio implementations have the same logic and therefore can share
   the same implementation of stdio extension API, except that some fields
   have different naming conventions, or their access requires some casts.  */


/* BSD stdio derived implementations.  */

#if defined __sferror || defined __DragonFly__ /* FreeBSD, NetBSD, OpenBSD, DragonFly, MacOS X, Cygwin */

# if defined __DragonFly__          /* DragonFly */
  /* See <http://www.dragonflybsd.org/cvsweb/src/lib/libc/stdio/priv_stdio.h?rev=HEAD&content-type=text/x-cvsweb-markup>.  */
#  define fp_ ((struct { struct __FILE_public pub; \
			 struct { unsigned char *_base; int _size; } _bf; \
			 void *cookie; \
			 void *_close; \
			 void *_read; \
			 void *_seek; \
			 void *_write; \
			 struct { unsigned char *_base; int _size; } _ub; \
			 int _ur; \
			 unsigned char _ubuf[3]; \
			 unsigned char _nbuf[1]; \
			 struct { unsigned char *_base; int _size; } _lb; \
			 int _blksize; \
			 fpos_t _offset; \
			 /* More fields, not relevant here.  */ \
		       } *) fp)
  /* See <http://www.dragonflybsd.org/cvsweb/src/include/stdio.h?rev=HEAD&content-type=text/x-cvsweb-markup>.  */
#  define _p pub._p
#  define _flags pub._flags
#  define _r pub._r
#  define _w pub._w
# else
#  define fp_ fp
# endif

# if defined __NetBSD__ || defined __OpenBSD__ /* NetBSD, OpenBSD */
  /* See <http://cvsweb.netbsd.org/bsdweb.cgi/src/lib/libc/stdio/fileext.h?rev=HEAD&content-type=text/x-cvsweb-markup>
     and <http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libc/stdio/fileext.h?rev=HEAD&content-type=text/x-cvsweb-markup> */
  struct __sfileext
    {
      struct  __sbuf _ub; /* ungetc buffer */
      /* More fields, not relevant here.  */
    };
#  define fp_ub ((struct __sfileext *) fp->_ext._base)->_ub
# else                                         /* FreeBSD, DragonFly, MacOS X, Cygwin */
#  define fp_ub fp_->_ub
# endif

# define HASUB(fp) (fp_ub._base != NULL)

#endif


/* SystemV derived implementations.  */

#if defined _IOERR

# if defined __sun && defined _LP64 /* Solaris/{SPARC,AMD64} 64-bit */
#  define fp_ ((struct { unsigned char *_ptr; \
			 unsigned char *_base; \
			 unsigned char *_end; \
			 long _cnt; \
			 int _file; \
			 unsigned int _flag; \
		       } *) fp)
# else
#  define fp_ fp
# endif

# if defined _SCO_DS                /* OpenServer */
#  define _cnt __cnt
#  define _ptr __ptr
#  define _base __base
#  define _flag __flag
# endif

#endif
