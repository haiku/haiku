/* Subprocesses with pipes.

   Copyright (C) 2002, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Paul Eggert <eggert@twinsun.com>
   and Florian Krohm <florian@edamail.fishkill.ibm.com>.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "subpipe.h"

#include <errno.h>

#include <signal.h>
#if ! defined SIGCHLD && defined SIGCLD
# define SIGCHLD SIGCLD
#endif

#include <stdlib.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif
#if ! HAVE_DUP2 && ! defined dup2
# if HAVE_FCNTL_H
#  include <fcntl.h>
# endif
# define dup2(f, t) (close (t), fcntl (f, F_DUPFD, t))
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned int) (stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#if HAVE_VFORK_H
# include <vfork.h>
#endif
#if ! HAVE_WORKING_VFORK
# define vfork fork
#endif

#include "error.h"

#include "gettext.h"
#define _(Msgid)  gettext (Msgid)


/* Initialize this module.  */

void
init_subpipe (void)
{
#ifdef SIGCHLD
  /* System V fork+wait does not work if SIGCHLD is ignored.  */
  signal (SIGCHLD, SIG_DFL);
#endif
}


/* Create a subprocess that is run as a filter.  ARGV is the
   NULL-terminated argument vector for the subprocess.  Store read and
   write file descriptors for communication with the subprocess into
   FD[0] and FD[1]: input meant for the process can be written into
   FD[0], and output from the process can be read from FD[1].  Return
   the subprocess id.

   To avoid deadlock, the invoker must not let incoming data pile up
   in FD[1] while writing data to FD[0].  */

pid_t
create_subpipe (char const * const *argv, int fd[2])
{
  int pipe_fd[2];
  int from_in_fd;
  int from_out_fd;
  int to_in_fd;
  int to_out_fd;
  pid_t pid;

  if (pipe (pipe_fd) != 0)
    error (EXIT_FAILURE, errno, "pipe");
  to_in_fd = pipe_fd[0];
  to_out_fd = pipe_fd[1];

  if (pipe (pipe_fd) != 0)
    error (EXIT_FAILURE, errno, "pipe");
  from_in_fd = pipe_fd[0];
  from_out_fd = pipe_fd[1];

  pid = vfork ();
  if (pid < 0)
    error (EXIT_FAILURE, errno, "fork");

  if (! pid)
    {
      /* Child.  */
      close (to_out_fd);
      close (from_in_fd);

      if (to_in_fd != STDIN_FILENO)
	{
	  dup2 (to_in_fd, STDIN_FILENO);
	  close (to_in_fd);
	}
      if (from_out_fd != STDOUT_FILENO)
	{
	  dup2 (from_out_fd, STDOUT_FILENO);
	  close (from_out_fd);
	}

      /* The cast to (char **) rather than (char * const *) is needed
	 for portability to older hosts with a nonstandard prototype
	 for execvp.  */
      execvp (argv[0], (char **) argv);
    
      _exit (errno == ENOENT ? 127 : 126);
    }

  /* Parent.  */
  close (to_in_fd);
  close (from_out_fd);
  fd[0] = to_out_fd;
  fd[1] = from_in_fd;
  return pid;
}


/* Wait for the subprocess to exit.  */

void
reap_subpipe (pid_t pid, char const *program)
{
#if HAVE_WAITPID || defined waitpid
  int wstatus;
  if (waitpid (pid, &wstatus, 0) < 0)
    error (EXIT_FAILURE, errno, "waitpid");
  else
    {
      int status = WIFEXITED (wstatus) ? WEXITSTATUS (wstatus) : -1;
      if (status)
	error (EXIT_FAILURE, 0,
	       _(status == 126
		 ? "subsidiary program `%s' could not be invoked"
		 : status == 127
		 ? "subsidiary program `%s' not found"
		 : status < 0
		 ? "subsidiary program `%s' failed"
		 : "subsidiary program `%s' failed (exit status %d)"),
	       program, status);
    }
#endif
}
