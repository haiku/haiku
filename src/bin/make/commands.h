/* Definition of data structures describing shell commands for GNU Make.
Copyright (C) 1988, 1989, 1991, 1993 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Structure that gives the commands to make a file
   and information about where these commands came from.  */

struct commands
  {
    struct floc fileinfo;	/* Where commands were defined.  */
    char *commands;		/* Commands text.  */
    unsigned int ncommand_lines;/* Number of command lines.  */
    char **command_lines;	/* Commands chopped up into lines.  */
    char *lines_flags;		/* One set of flag bits for each line.  */
    int any_recurse;		/* Nonzero if any `lines_recurse' elt has */
				/* the COMMANDS_RECURSE bit set.  */
  };

/* Bits in `lines_flags'.  */
#define	COMMANDS_RECURSE	1 /* Recurses: + or $(MAKE).  */
#define	COMMANDS_SILENT		2 /* Silent: @.  */
#define	COMMANDS_NOERROR	4 /* No errors: -.  */

extern void execute_file_commands PARAMS ((struct file *file));
extern void print_commands PARAMS ((struct commands *cmds));
extern void delete_child_targets PARAMS ((struct child *child));
extern void chop_commands PARAMS ((struct commands *cmds));
