/* arrayfunc.h -- declarations for miscellaneous array functions in arrayfunc.c */

/* Copyright (C) 2001 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Bash is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111 USA. */

#if !defined (_ARRAYFUNC_H_)
#define _ARRAYFUNC_H_

/* Must include variables.h before including this file. */

#if defined (ARRAY_VARS)

extern SHELL_VAR *convert_var_to_array __P((SHELL_VAR *));

extern SHELL_VAR *bind_array_variable __P((char *, arrayind_t, char *));
extern SHELL_VAR *assign_array_element __P((char *, char *));

extern SHELL_VAR *find_or_make_array_variable __P((char *, int));

extern SHELL_VAR *assign_array_from_string  __P((char *, char *));
extern SHELL_VAR *assign_array_var_from_word_list __P((SHELL_VAR *, WORD_LIST *));
extern SHELL_VAR *assign_array_var_from_string __P((SHELL_VAR *, char *));

extern int unbind_array_element __P((SHELL_VAR *, char *));
extern int skipsubscript __P((const char *, int));
extern void print_array_assignment __P((SHELL_VAR *, int));

extern arrayind_t array_expand_index __P((char *, int));
extern int valid_array_reference __P((char *));
extern char *array_value __P((char *, int, int *));
extern char *get_array_value __P((char *, int, int *));

extern char *array_variable_name __P((char *, char **, int *));
extern SHELL_VAR *array_variable_part __P((char *, char **, int *));

#endif

#endif /* !_ARRAYFUNC_H_ */
