/* Uncancelable versions of cancelable interfaces.  Generic version.
   Copyright (C) 2003-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* By default we have none.  Map the name to the normal functions.  */
#define open_not_cancel(name, flags, mode) \
  open (name, flags, mode)
#define open_not_cancel_2(name, flags) \
  open (name, flags)
#define openat_not_cancel(fd, name, flags, mode) \
  openat (fd, name, flags, mode)
#define openat_not_cancel_3(fd, name, flags) \
  openat (fd, name, flags, 0)
#define openat64_not_cancel(fd, name, flags, mode) \
  openat64 (fd, name, flags, mode)
#define openat64_not_cancel_3(fd, name, flags) \
  openat64 (fd, name, flags, 0)
#define close_not_cancel(fd) \
  close (fd)
#define close_not_cancel_no_status(fd) \
  (void) close (fd)
#define read_not_cancel(fd, buf, n) \
  read (fd, buf, n)
#define write_not_cancel(fd, buf, n) \
  write (fd, buf, n)
#define writev_not_cancel_no_status(fd, iov, n) \
  (void) writev (fd, iov, n)
#define fcntl_not_cancel(fd, cmd, val) \
  fcntl (fd, cmd, val)
# define waitpid_not_cancel(pid, stat_loc, options) \
  waitpid (pid, stat_loc, options)
#define pause_not_cancel() \
  pause ()
#define nanosleep_not_cancel(requested_time, remaining) \
  nanosleep (requested_time, remaining)
#define sigsuspend_not_cancel(set) \
  sigsuspend (set)

#define NO_CANCELLATION 1
