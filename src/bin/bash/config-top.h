/* config-top.h - various user-settable options not under the control of autoconf. */

/* Copyright (C) 2002-2009 Free Software Foundation, Inc.

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

/* Define CONTINUE_AFTER_KILL_ERROR if you want the kill command to
   continue processing arguments after one of them fails.  This is
   what POSIX.2 specifies. */
#define CONTINUE_AFTER_KILL_ERROR

/* Define BREAK_COMPLAINS if you want the non-standard, but useful
   error messages about `break' and `continue' out of context. */
#define BREAK_COMPLAINS

/* Define BUFFERED_INPUT if you want the shell to do its own input
   buffering, rather than using stdio.  Do not undefine this; it's
   required to preserve semantics required by POSIX. */
#define BUFFERED_INPUT

/* Define ONESHOT if you want sh -c 'command' to avoid forking to execute
   `command' whenever possible.  This is a big efficiency improvement. */
#define ONESHOT

/* Define V9_ECHO if you want to give the echo builtin backslash-escape
   interpretation using the -e option, in the style of the Bell Labs 9th
   Edition version of echo.  You cannot emulate the System V echo behavior
   without this option. */
#define V9_ECHO

/* Define DONT_REPORT_SIGPIPE if you don't want to see `Broken pipe' messages
   when a job like `cat jobs.c | exit 1' terminates due to a SIGPIPE. */
#define DONT_REPORT_SIGPIPE

/* Define DONT_REPORT_BROKEN_PIPE_WRITE_ERRORS if you don't want builtins
   like `echo' and `printf' to report errors when output does not succeed
   due to EPIPE. */
/* #define DONT_REPORT_BROKEN_PIPE_WRITE_ERRORS */

/* The default value of the PATH variable. */
#ifndef DEFAULT_PATH_VALUE
#define DEFAULT_PATH_VALUE \
  ".:/boot/home/config/bin:/bin:/boot/apps:/boot/preferences" \
  ":/boot/system/apps" \
  ":/boot/system/preferences" \
  ":/boot/common/settings/develop/tools/current/bin"
#endif

/* The value for PATH when invoking `command -p'.  This is only used when
   the Posix.2 confstr () function, or CS_PATH define are not present. */
#ifndef STANDARD_UTILS_PATH
#define STANDARD_UTILS_PATH \
  "/bin:/usr/bin:/sbin:/usr/sbin:/etc:/usr/etc"
#endif

/* Default primary and secondary prompt strings. */
#define PPROMPT "\\s-\\v\\$ "
#define SPROMPT "> "

/* Undefine this if you don't want the ksh-compatible behavior of reprinting
   the select menu after a valid choice is made only if REPLY is set to NULL
   in the body of the select command.  The menu is always reprinted if the
   reply to the select query is an empty line. */
#define KSH_COMPATIBLE_SELECT

/* System-wide .bashrc file for interactive shells. */
/* #define SYS_BASHRC "/etc/bash.bashrc" */

/* System-wide .bash_logout for login shells. */
/* #define SYS_BASH_LOGOUT "/etc/bash.bash_logout" */

/* Define this to make non-interactive shells begun with argv[0][0] == '-'
   run the startup files when not in posix mode. */
/* #define NON_INTERACTIVE_LOGIN_SHELLS */

/* Define this if you want bash to try to check whether it's being run by
   sshd and source the .bashrc if so (like the rshd behavior).  This checks
   for the presence of SSH_CLIENT or SSH2_CLIENT in the initial environment,
   which can be fooled under certain not-uncommon circumstances. */
/* #define SSH_SOURCE_BASHRC */

/* Define if you want the case-capitalizing operators (~[~]) and the
   `capcase' variable attribute (declare -c). */
#define  CASEMOD_CAPCASE

/* This is used as the name of a shell function to call when a command
   name is not found.  If you want to name it something other than the
   default ("command_not_found_handle"), change it here. */
/* #define NOTFOUND_HOOK "command_not_found_handle" */
