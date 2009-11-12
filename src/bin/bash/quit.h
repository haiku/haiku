/* quit.h -- How to handle SIGINT gracefully. */

/* Copyright (C) 1993-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined (_QUIT_H_)
#define _QUIT_H_

/* Non-zero means SIGINT has already ocurred. */
extern volatile int interrupt_state;
extern volatile int terminating_signal;

/* Macro to call a great deal.  SIGINT just sets the interrupt_state variable.
   When it is safe, put QUIT in the code, and the "interrupt" will take
   place.  The same scheme is used for terminating signals (e.g., SIGHUP)
   and the terminating_signal variable.  That calls a function which will
   end up exiting the shell. */
#define QUIT \
  do { \
    if (terminating_signal) termsig_handler (terminating_signal); \
    if (interrupt_state) throw_to_top_level (); \
  } while (0)

#define SETINTERRUPT interrupt_state = 1
#define CLRINTERRUPT interrupt_state = 0

#define ADDINTERRUPT interrupt_state++
#define DELINTERRUPT interrupt_state--

/* The same sort of thing, this time just for signals that would ordinarily
   cause the shell to terminate. */

#define CHECK_TERMSIG \
  do { \
    if (terminating_signal) termsig_handler (terminating_signal); \
  } while (0)

#endif /* _QUIT_H_ */
