/* common.h -- extern declarations for functions defined in common.c. */

/* Copyright (C) 1993-2002 Free Software Foundation, Inc.

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

#if  !defined (__COMMON_H)
#  define __COMMON_H

#include "stdc.h"

#define ISOPTION(s, c)	(s[0] == '-' && !s[2] && s[1] == c)

/* Flag values for parse_and_execute () */
#define SEVAL_NONINT	0x001
#define SEVAL_INTERACT	0x002
#define SEVAL_NOHIST	0x004
#define SEVAL_NOFREE	0x008

/* Flags for describe_command, shared between type.def and command.def */
#define CDESC_ALL		0x001	/* type -a */
#define CDESC_SHORTDESC		0x002	/* command -V */
#define CDESC_REUSABLE		0x004	/* command -v */
#define CDESC_TYPE		0x008	/* type -t */
#define CDESC_PATH_ONLY		0x010	/* type -p */
#define CDESC_FORCE_PATH	0x020	/* type -ap or type -P */
#define CDESC_NOFUNCS		0x040	/* type -f */

/* Flags for get_job_by_name */
#define JM_PREFIX		0x01	/* prefix of job name */
#define JM_SUBSTRING		0x02	/* substring of job name */
#define JM_EXACT		0x04	/* match job name exactly */
#define JM_STOPPED		0x08	/* match stopped jobs only */
#define JM_FIRSTMATCH		0x10	/* return first matching job */

/* Flags for remember_args and value of changed_dollar_vars */
#define ARGS_NONE		0x0
#define ARGS_INVOC		0x01
#define ARGS_FUNC		0x02
#define ARGS_SETBLTIN		0x04

/* Functions from common.c */
extern void builtin_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));
extern void builtin_usage __P((void));
extern void no_args __P((WORD_LIST *));
extern int no_options __P((WORD_LIST *));

/* common error message functions */
extern void sh_needarg __P((char *));
extern void sh_neednumarg __P((char *));
extern void sh_notfound __P((char *));
extern void sh_invalidopt __P((char *));
extern void sh_invalidoptname __P((char *));
extern void sh_invalidid __P((char *));
extern void sh_invalidnum __P((char *));
extern void sh_invalidsig __P((char *));
extern void sh_erange __P((char *, char *));
extern void sh_badpid __P((char *));
extern void sh_badjob __P((char *));
extern void sh_readonly __P((const char *));
extern void sh_nojobs __P((char *));
extern void sh_restricted __P((char *));

extern char **make_builtin_argv __P((WORD_LIST *, int *));
extern void remember_args __P((WORD_LIST *, int));

extern int dollar_vars_changed __P((void));
extern void set_dollar_vars_unchanged __P((void));
extern void set_dollar_vars_changed __P((void));

extern intmax_t get_numeric_arg __P((WORD_LIST *, int));
extern int get_exitstat __P((WORD_LIST *));
extern int read_octal __P((char *));

/* Keeps track of the current working directory. */
extern char *the_current_working_directory;
extern char *get_working_directory __P((char *));
extern void set_working_directory __P((char *));

#if defined (JOB_CONTROL)
extern int get_job_by_name __P((const char *, int));
extern int get_job_spec __P((WORD_LIST *));
#endif
extern int display_signal_list __P((WORD_LIST *, int));

/* It's OK to declare a function as returning a Function * without
   providing a definition of what a `Function' is. */
extern struct builtin *builtin_address_internal __P((char *, int));
extern sh_builtin_func_t *find_shell_builtin __P((char *));
extern sh_builtin_func_t *builtin_address __P((char *));
extern sh_builtin_func_t *find_special_builtin __P((char *));
extern void initialize_shell_builtins __P((void));

/* Functions from getopts.def */
extern void getopts_reset __P((int));

/* Functions from set.def */
extern int minus_o_option_value __P((char *));
extern void list_minus_o_opts __P((int, int));
extern char **get_minus_o_opts __P((void));
extern int set_minus_o_option __P((int, char *));

extern void set_shellopts __P((void));
extern void parse_shellopts __P((char *));
extern void initialize_shell_options __P((int));

extern void reset_shell_options __P((void));

/* Functions from shopt.def */
extern void reset_shopt_options __P((void));
extern char **get_shopt_options __P((void));

extern int shopt_setopt __P((char *, int));
extern int shopt_listopt __P((char *, int));

extern int set_login_shell __P((int));

/* Functions from type.def */
extern int describe_command __P((char *, int));

/* Functions from setattr.def */
extern int set_or_show_attributes __P((WORD_LIST *, int, int));
extern int show_var_attributes __P((SHELL_VAR *, int, int));
extern int show_name_attributes __P((char *, int));
extern void set_var_attribute __P((char *, int, int));

/* Functions from pushd.def */
extern char *get_dirstack_from_string __P((char *));
extern char *get_dirstack_element __P((intmax_t, int));
extern void set_dirstack_element __P((intmax_t, int, char *));
extern WORD_LIST *get_directory_stack __P((void));

/* Functions from evalstring.c */
extern int parse_and_execute __P((char *, const char *, int));
extern void parse_and_execute_cleanup __P((void));

/* Functions from evalfile.c */
extern int maybe_execute_file __P((const char *, int));
extern int source_file __P((const char *));
extern int fc_execute_file __P((const char *));

#endif /* !__COMMON_H */
