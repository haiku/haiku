/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/*
 *	POSIX Standard: 4.5.2 Process Times	<sys/times.h>
 */

/*
 * If we don't have a standard system clock_t type, this must be included
 * after config.h
 */

#ifndef	_BASH_SYSTIMES_H
#define _BASH_SYSTIMES_H	1

#if defined (HAVE_SYS_TIMES_H)
#  include <sys/times.h>
#else /* !HAVE_SYS_TIMES_H */

#include <stdc.h>

/* Structure describing CPU time used by a process and its children.  */
struct tms
  {
    clock_t tms_utime;		/* User CPU time.  */
    clock_t tms_stime;		/* System CPU time.  */

    clock_t tms_cutime;		/* User CPU time of dead children.  */
    clock_t tms_cstime;		/* System CPU time of dead children.  */
  };

/* Store the CPU time used by this process and all its
   dead descendents in BUFFER.
   Return the elapsed real time from an arbitrary point in the
   past (the bash emulation uses the epoch), or (clock_t) -1 for
   errors.  All times are in CLK_TCKths of a second.  */
extern clock_t times __P((struct tms *buffer));

#endif /* !HAVE_SYS_TIMES_H */
#endif /* _BASH_SYSTIMES_H */
