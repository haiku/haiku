/* shell.h -- The data structures used by the shell */

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bashjmp.h"

#include "command.h"
#include "syntax.h"
#include "general.h"
#include "error.h"
#include "variables.h"
#include "arrayfunc.h"
#include "quit.h"
#include "maxpath.h"
#include "unwind_prot.h"
#include "dispose_cmd.h"
#include "make_cmd.h"
#include "ocache.h"
#include "subst.h"
#include "sig.h"
#include "pathnames.h"
#include "externs.h"
#include "version.h"

extern int EOF_Reached;

#define NO_PIPE -1
#define REDIRECT_BOTH -2

#define NO_VARIABLE -1

/* Values that can be returned by execute_command (). */
#define EXECUTION_FAILURE 1
#define EXECUTION_SUCCESS 0

/* Usage messages by builtins result in a return status of 2. */
#define EX_BADUSAGE	2

/* Special exit statuses used by the shell, internally and externally. */
#define EX_BINARY_FILE	126
#define EX_NOEXEC	126
#define EX_NOINPUT	126
#define EX_NOTFOUND	127

#define EX_SHERRBASE	256	/* all special error values are > this. */

#define EX_BADSYNTAX	257	/* shell syntax error */
#define EX_USAGE	258	/* syntax error in usage */
#define EX_REDIRFAIL	259	/* redirection failed */
#define EX_BADASSIGN	260	/* variable assignment error */
#define EX_EXPFAIL	261	/* word expansion failed */

/* Flag values that control parameter pattern substitution. */
#define MATCH_ANY	0x0
#define MATCH_BEG	0x1
#define MATCH_END	0x2

#define MATCH_TYPEMASK	0x3

#define MATCH_GLOBREP	0x10
#define MATCH_QUOTED	0x20

/* Some needed external declarations. */
extern char **shell_environment;
extern WORD_LIST *rest_of_args;

/* Generalized global variables. */
extern int executing, login_shell;
extern int interactive, interactive_shell;
extern int startup_state;

/* Structure to pass around that holds a bitmap of file descriptors
   to close, and the size of that structure.  Used in execute_cmd.c. */
struct fd_bitmap {
  int size;
  char *bitmap;
};

#define FD_BITMAP_SIZE 32

#define CTLESC '\001'
#define CTLNUL '\177'

/* Information about the current user. */
struct user_info {
  uid_t uid, euid;
  gid_t gid, egid;
  char *user_name;
  char *shell;		/* shell from the password file */
  char *home_dir;
};

extern struct user_info current_user;

/* Force gcc to not clobber X on a longjmp().  Old versions of gcc mangle
   this badly. */
#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 8)
#  define USE_VAR(x)	((void) &(x))
#else
#  define USE_VAR(x)
#endif
