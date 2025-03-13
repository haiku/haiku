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
#	define EOF (-1)
#endif

#define _IO_pos_t		off_t /* obsolete */
#define _IO_fpos64_t	off_t
#define _IO_size_t		size_t
#define _IO_ssize_t		ssize_t
#define _IO_off_t		off_t
#define _IO_off64_t 	off_t
#define _IO_pid_t		pid_t
#define _IO_BUFSIZ		BUFSIZ
#define _IO_va_list		va_list

#define _G_IO_IO_FILE_VERSION 0x20001

#define _IOS_INPUT		1
#define _IOS_OUTPUT		2
#define _IOS_APPEND		8
#define _IOS_BIN		128

#define _IO_UNBUFFERED 2
#define _IO_IN_BACKUP 0x100
#define _IO_LINE_BUF 0x200

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


struct _IO_marker;


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

	void	*_private0;

	void	*_private1;

	int		_fileno;
	int		_private2;
	off_t	_unused0;

	/* 1+column number of pbase(); 0 is unknown. */
	unsigned short _cur_column;
	signed char _private3;
	char	_shortbuf[1];

	void *_private4;

	off_t _private5;
	void *_private6;
	void *_private7;
	int _private8;
	/* Make sure we don't get into trouble again.  */
	char _unused2[15 * sizeof (int) - 2 * sizeof (void *)];
} _IO_FILE;


#ifdef __cplusplus
extern "C" {
#endif

extern int _IO_putc(int c, _IO_FILE *stream);
extern int _IO_getc(_IO_FILE *stream);

extern int __underflow(_IO_FILE *stream);

extern int _IO_peekc_locked(_IO_FILE *stream);
#define _IO_peekc(_fp) _IO_peekc_locked (_fp)

extern void _IO_flockfile(_IO_FILE *);
extern void _IO_funlockfile(_IO_FILE *);
extern int _IO_ftrylockfile(_IO_FILE *);

extern int _IO_vfscanf(_IO_FILE *, const char *, va_list, int *);
extern int _IO_vfprintf(_IO_FILE *, const char *, va_list);
extern _IO_ssize_t _IO_padn(_IO_FILE *, int, _IO_ssize_t);
extern _IO_size_t _IO_sgetn(_IO_FILE *, void *, _IO_size_t);

extern _IO_fpos64_t _IO_seekoff(_IO_FILE *, _IO_off64_t, int, int);
extern _IO_fpos64_t _IO_seekpos(_IO_FILE *, _IO_fpos64_t, int);

extern void _IO_free_backup_area(_IO_FILE *);

#ifdef __cplusplus
}
#endif

#endif	/* _IO_STDIO_H_ */
