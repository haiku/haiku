/* execute_cmd.h - functions from execute_cmd.c. */

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

#if !defined (_EXECUTE_CMD_H_)
#define _EXECUTE_CMD_H_

#include "stdc.h"

extern struct fd_bitmap *new_fd_bitmap __P((int));
extern void dispose_fd_bitmap __P((struct fd_bitmap *));
extern void close_fd_bitmap __P((struct fd_bitmap *));
extern int executing_line_number __P((void));
extern int execute_command __P((COMMAND *));
extern int execute_command_internal __P((COMMAND *, int, int, int, struct fd_bitmap *));
extern int shell_execve __P((char *, char **, char **));
extern void setup_async_signals __P((void));
extern void dispose_exec_redirects __P ((void));

extern int execute_shell_function __P((SHELL_VAR *, WORD_LIST *));

#if defined (PROCESS_SUBSTITUTION)
extern void close_all_files __P((void));
#endif

#endif /* _EXECUTE_CMD_H_ */
