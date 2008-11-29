/* buildcmd.h -- build command lines from a stream of arguments
   Copyright (C) 2005, 2006 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * Written by James Youngman.
 */
#ifndef INC_BUILDCMD_H
#define INC_BUILDCMD_H 1

struct buildcmd_state
{
  /* Number of valid elements in `cmd_argv'.  */
  int cmd_argc;			/* 0 */

  /* The list of args being built.  */
  char **cmd_argv; /* NULL */
  
  /* Number of elements allocated for `cmd_argv'.  */
  int cmd_argv_alloc;
  
  /* Storage for elements of `cmd_argv'.  */
  char *argbuf;
  
  /* Number of chars being used in `cmd_argv'.  */
  size_t cmd_argv_chars;

  /* Number of chars being used in `cmd_argv' for the initial args..  */
  size_t cmd_initial_argv_chars;

  /* User context information. */
  void *usercontext;

  /* to-do flag. */
  int todo;
};

struct buildcmd_control
{
  /* If true, exit if lines_per_exec or args_per_exec is exceeded.  */
  int exit_if_size_exceeded; /* false */

  /* POSIX limits on the argument length. */
  size_t posix_arg_size_max;
  size_t posix_arg_size_min;
  
  /* The maximum number of characters that can be used per command line.  */
  size_t arg_max;

  /* max_arg_count: the maximum number of arguments that can be used.
   *
   * Many systems include the size of the pointers in ARG_MAX.
   * Linux on PPC fails if we just subtract 1 here.
   *
   * However, not all systems define ARG_MAX.  Our bc_get_arg_max()
   * function returns a useful value even if ARG_MAX is not defined.
   * However, sometimes, max_arg_count is LONG_MAX!
   */
  long max_arg_count;

  
  /* The length of `replace_pat'.  */
  size_t rplen;
  
  /* If nonzero, then instead of putting the args from stdin at
   the end of the command argument list, they are each stuck into the
   initial args, replacing each occurrence of the `replace_pat' in the
   initial args.  */
  char *replace_pat;
  
  /* Number of initial arguments given on the command line.  */
  int initial_argc;		/* 0 */
  
  /* exec callback. */
  int (*exec_callback)(const struct buildcmd_control *, struct buildcmd_state *);
  
  /* If nonzero, the maximum number of nonblank lines from stdin to use
     per command line.  */
  long lines_per_exec;		/* 0 */
  
  /* The maximum number of arguments to use per command line.  */
  long args_per_exec;
};

enum BC_INIT_STATUS 
  {
    BC_INIT_OK = 0,
    BC_INIT_ENV_TOO_BIG
  };

extern size_t bc_size_of_environment (void);


extern void bc_do_insert (const struct buildcmd_control *ctl,
			  struct buildcmd_state *state,
			  char *arg, size_t arglen,
			  const char *prefix, size_t pfxlen,
			  const char *linebuf, size_t lblen,
			  int initial_args);

extern void bc_push_arg (const struct buildcmd_control *ctl,
			 struct buildcmd_state *state,
			 const char *arg,    size_t len,
			 const char *prefix, size_t pfxlen,
			 int initial_args);

extern void  bc_init_state(const struct buildcmd_control *ctl,
					 struct buildcmd_state *state,
					 void *usercontext);
extern enum BC_INIT_STATUS bc_init_controlinfo(struct buildcmd_control *ctl);
extern size_t bc_get_arg_max(void);
extern void bc_use_sensible_arg_max(struct buildcmd_control *ctl);
extern void bc_clear_args(const struct buildcmd_control *ctl,
			  struct buildcmd_state *state);


#endif
