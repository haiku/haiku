/* Copyright (C) 1991,92,93,94,95,97,98,99,2000,2001 Free Software Foundation, Inc.
   This file is part of the GNU IO Library.
   Written by Per Bothner <bothner@cygnus.com>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

   As a special exception, if you link this library with files
   compiled with a GNU compiler to produce an executable, this does
   not cause the resulting executable to be covered by the GNU General
   Public License.  This exception does not however invalidate any
   other reasons why the executable file might be covered by the GNU
   General Public License.
*/
#ifndef _IO_STDIO_H_
#define _IO_STDIO_H_

#include <sys/types.h>
#include <stdarg.h>

#ifndef EOF
#	define EOF -1
#endif

#define _IO_pos_t		off_t /* obsolete */
#define _IO_fpos_t		off_t
#define _IO_fpos64_t	off_t
#define _IO_size_t		size_t
#define _IO_ssize_t		ssize_t
#define _IO_off_t		off_t
#define _IO_off64_t 	off_t
#define _IO_pid_t		pid_t
#define _IO_uid_t		uid_t
#define _IO_HAVE_SYS_WAIT _G_HAVE_SYS_WAIT
#define _IO_HAVE_ST_BLKSIZE _G_HAVE_ST_BLKSIZE
#define _IO_BUFSIZ		BUFSIZ
#define _IO_va_list		va_list

#define _IOS_INPUT		1
#define _IOS_OUTPUT		2
#define _IOS_ATEND		4
#define _IOS_APPEND		8
#define _IOS_TRUNC		16
#define _IOS_NOCREATE	32
#define _IOS_NOREPLACE	64
#define _IOS_BIN		128

/* Magic numbers and bits for the _flags field.
   The magic numbers use the high-order bits of _flags;
   the remaining bits are available for variable flags.
   Note: The magic numbers must all be negative if stdio
   emulation is desired. */

#define _IO_MAGIC 0xFBAD0000 /* Magic number */
#define _OLD_STDIO_MAGIC 0xFABC0000 /* Emulate old stdio. */
#define _IO_MAGIC_MASK 0xFFFF0000
#define _IO_USER_BUF 1 /* User owns buffer; don't delete it on close. */
#define _IO_UNBUFFERED 2
#define _IO_NO_READS 4 /* Reading not allowed */
#define _IO_NO_WRITES 8 /* Writing not allowd */
#define _IO_EOF_SEEN 0x10
#define _IO_ERR_SEEN 0x20
#define _IO_DELETE_DONT_CLOSE 0x40 /* Don't call close(_fileno) on cleanup. */
#define _IO_LINKED 0x80 /* Set if linked (using _chain) to streambuf::_list_all.*/
#define _IO_IN_BACKUP 0x100
#define _IO_LINE_BUF 0x200
#define _IO_TIED_PUT_GET 0x400 /* Set if put and get pointer logicly tied. */
#define _IO_CURRENTLY_PUTTING 0x800
#define _IO_IS_APPENDING 0x1000
#define _IO_IS_FILEBUF 0x2000
#define _IO_BAD_SEEN 0x4000

/* These are "formatting flags" matching the iostream fmtflags enum values. */
#define _IO_SKIPWS 01
#define _IO_LEFT 02
#define _IO_RIGHT 04
#define _IO_INTERNAL 010
#define _IO_DEC 020
#define _IO_OCT 040
#define _IO_HEX 0100
#define _IO_SHOWBASE 0200
#define _IO_SHOWPOINT 0400
#define _IO_UPPERCASE 01000
#define _IO_SHOWPOS 02000
#define _IO_SCIENTIFIC 04000
#define _IO_FIXED 010000
#define _IO_UNITBUF 020000
#define _IO_STDIO 040000
#define _IO_DONT_CLOSE 0100000
#define _IO_BOOLALPHA 0200000


struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;
typedef void _IO_lock_t;


struct _IO_marker {
	struct _IO_marker	*_next;
	struct _IO_FILE		*_sbuf;
	int					_pos;
};

typedef struct _IO_FILE {
	int		_flags;		/* High-order word is _IO_MAGIC; rest is flags. */
#	define _IO_file_flags _flags

	/* The following pointers correspond to the C++ streambuf protocol. */
	/* Note:  Tk uses the _IO_read_ptr and _IO_read_end fields directly. */
	char	*_IO_read_ptr;	/* Current read pointer */
	char	*_IO_read_end;	/* End of get area. */
	char	*_IO_read_base;	/* Start of putback+get area. */
	char	*_IO_write_base;	/* Start of put area. */
	char	*_IO_write_ptr;	/* Current put pointer. */
	char	*_IO_write_end;	/* End of put area. */
	char	*_IO_buf_base;	/* Start of reserve area. */
	char	*_IO_buf_end;	/* End of reserve area. */
	/* The following fields are used to support backing up and undo. */
	char	*_IO_save_base; /* Pointer to start of non-current get area. */
	char	*_IO_backup_base;  /* Pointer to first valid character of backup area */
	char	*_IO_save_end; /* Pointer to end of non-current get area. */

	struct _IO_marker *_markers;

	struct _IO_FILE *_chain;

	int		_fileno;
/*	int		_blksize; */
	int		_flags2;
	off_t	_old_offset; /* This used to be _offset but it's too small. */
		/* -> not true on BeOS, but who cares */

	/* 1+column number of pbase(); 0 is unknown. */
	unsigned short _cur_column;
	signed char _vtable_offset;
	char	_shortbuf[1];

	_IO_lock_t *_lock;

	off_t	_offset;
/* #if defined _LIBC || defined _GLIBCPP_USE_WCHAR_T */
	/* Wide character stream stuff.  */
	struct _IO_codecvt *_codecvt;
	struct _IO_wide_data *_wide_data;
/* #else
 *	void	*__pad1;
 *	void	*__pad2;
 * #endif */
	int _mode;
	/* Make sure we don't get into trouble again.  */
	char _unused2[15 * sizeof (int) - 2 * sizeof (void *)];
} _IO_FILE;


#ifdef __cplusplus
extern "C" {
#endif

extern int _IO_feof(_IO_FILE *stream);
#define    _IO_feof_unlocked(__fp) (((__fp)->_flags & _IO_EOF_SEEN) != 0)
extern int _IO_ferror(_IO_FILE *stream);
#define    _IO_ferror_unlocked(__fp) (((__fp)->_flags & _IO_ERR_SEEN) != 0)
extern int _IO_putc(int c, _IO_FILE *stream);
extern int _IO_getc(_IO_FILE *stream);

extern int __underflow(_IO_FILE *stream);
extern int __uflow(_IO_FILE *stream);
extern int __overflow(_IO_FILE *stream, int c);

extern int _IO_peekc_locked(_IO_FILE *stream);

/* This one is for Emacs. */
#define _IO_PENDING_OUTPUT_COUNT(_fp)	\
	((_fp)->_IO_write_ptr - (_fp)->_IO_write_base)

extern void _IO_flockfile(_IO_FILE *);
extern void _IO_funlockfile(_IO_FILE *);
extern int _IO_ftrylockfile(_IO_FILE *);

#ifdef _IO_MTSAFE_IO
#	define _IO_peekc(_fp) _IO_peekc_locked (_fp)
#else
#	define _IO_peekc(_fp) _IO_peekc_unlocked (_fp)
#	define _IO_flockfile(_fp) /**/
#	define _IO_funlockfile(_fp) /**/
#	define _IO_ftrylockfile(_fp) /**/
#	define _IO_cleanup_region_start(_fct, _fp) /**/
#	define _IO_cleanup_region_end(_Doit) /**/
#endif /* !_IO_MTSAFE_IO */

extern int _IO_vfscanf(_IO_FILE *, const char *, va_list, int *);
extern int _IO_vfprintf(_IO_FILE *, const char *, va_list);
extern _IO_ssize_t _IO_padn(_IO_FILE *, int, _IO_ssize_t);
extern _IO_size_t _IO_sgetn(_IO_FILE *, void *, _IO_size_t);

extern _IO_fpos64_t _IO_seekoff(_IO_FILE *, _IO_off64_t, int, int);
extern _IO_fpos64_t _IO_seekpos(_IO_FILE *, _IO_fpos64_t, int);

extern void _IO_free_backup_area(_IO_FILE *);

#if  __GNUC__ >= 3
# define _IO_BE(expr, res) __builtin_expect (expr, res)
#else
# define _IO_BE(expr, res) (expr)
#endif

#define _IO_getc_unlocked(_fp) \
       (_IO_BE ((_fp)->_IO_read_ptr >= (_fp)->_IO_read_end, 0) \
        ? __uflow (_fp) : *(unsigned char *) (_fp)->_IO_read_ptr++)
#define _IO_peekc_unlocked(_fp) \
       (_IO_BE ((_fp)->_IO_read_ptr >= (_fp)->_IO_read_end, 0) \
          && __underflow (_fp) == EOF ? EOF \
        : *(unsigned char *) (_fp)->_IO_read_ptr)
#define _IO_putc_unlocked(_ch, _fp) \
   (_IO_BE ((_fp)->_IO_write_ptr >= (_fp)->_IO_write_end, 0) \
    ? __overflow (_fp, (unsigned char) (_ch)) \
    : (unsigned char) (*(_fp)->_IO_write_ptr++ = (_ch)))



#ifdef __cplusplus
}
#endif

#endif	/* _IO_STDIO_H_ */
