/* externs.h -- extern function declarations which do not appear in their
   own header file. */

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

/* Make sure that this is included *after* config.h! */

#if !defined (_EXTERNS_H_)
#  define _EXTERNS_H_

#include "stdc.h"

/* Functions from expr.c. */
extern intmax_t evalexp __P((char *, int *));

/* Functions from print_cmd.c. */
extern char *make_command_string __P((COMMAND *));
extern void print_command __P((COMMAND *));
extern void print_simple_command __P((SIMPLE_COM *));
extern char *named_function_string __P((char *, COMMAND *, int));
extern void print_word_list __P((WORD_LIST *, char *));
extern char *indirection_level_string __P((void));
extern void xtrace_print_word_list __P((WORD_LIST *));
#if defined (DPAREN_ARITHMETIC)
extern void xtrace_print_arith_cmd __P((WORD_LIST *));
#endif
#if defined (COND_COMMAND)
extern void xtrace_print_cond_term __P((int, int, WORD_DESC *, char *, char *));
#endif

/* Functions from shell.c. */
extern void exit_shell __P((int)) __attribute__((__noreturn__));
extern void sh_exit __P((int)) __attribute__((__noreturn__));
extern void disable_priv_mode __P((void));
extern void unbind_args __P((void));

#if defined (RESTRICTED_SHELL)
extern int shell_is_restricted __P((char *));
extern int maybe_make_restricted __P((char *));
#endif

extern void unset_bash_input __P((int));
extern void get_current_user_info __P((void));

/* Functions from eval.c. */
extern int reader_loop __P((void));
extern int parse_command __P((void));
extern int read_command __P((void));

/* Functions from braces.c. */
#if defined (BRACE_EXPANSION)
extern char **brace_expand __P((char *));
#endif

/* Miscellaneous functions from parse.y */
extern int yyparse __P((void));
extern int return_EOF __P((void));
extern void reset_parser __P((void));
extern WORD_LIST *parse_string_to_word_list __P((char *, const char *));

extern char *decode_prompt_string __P((char *));

extern int get_current_prompt_level __P((void));
extern void set_current_prompt_level __P((int));

#if defined (HISTORY)
extern char *history_delimiting_chars __P((void));
#endif

/* Declarations for functions defined in locale.c */
extern void set_default_locale __P((void));
extern void set_default_locale_vars __P((void));
extern int set_locale_var __P((char *, char *));
extern int set_lang __P((char *, char *));
extern char *get_locale_var __P((char *));
extern char *localetrans __P((char *, int, int *));
extern char *mk_msgstr __P((char *, int *));
extern char *localeexpand __P((char *, int, int, int, int *));

/* Declarations for functions defined in list.c. */
extern void list_walk __P((GENERIC_LIST *, sh_glist_func_t *));
extern void wlist_walk __P((WORD_LIST *, sh_icpfunc_t *));
extern GENERIC_LIST *list_reverse ();
extern int list_length ();
extern GENERIC_LIST *list_append ();
extern GENERIC_LIST *list_remove ();

/* Declarations for functions defined in stringlib.c */
extern int find_string_in_alist __P((char *, STRING_INT_ALIST *, int));
extern char *find_token_in_alist __P((int, STRING_INT_ALIST *, int));
extern int find_index_in_alist __P((char *, STRING_INT_ALIST *, int));

extern char *substring __P((char *, int, int));
extern char *strsub __P((char *, char *, char *, int));
extern char *strcreplace __P((char *, int, char *, int));
extern void strip_leading __P((char *));
extern void strip_trailing __P((char *, int, int));
extern void xbcopy __P((char *, char *, int));

/* Functions from the bash library, lib/sh/libsh.a.  These should really
   go into a separate include file. */

/* declarations for functions defined in lib/sh/clktck.c */
extern long get_clk_tck __P((void));

/* declarations for functions defined in lib/sh/clock.c */
extern void clock_t_to_secs ();
extern void print_clock_t ();

/* Declarations for functions defined in lib/sh/fmtulong.c */
#define FL_PREFIX     0x01    /* add 0x, 0X, or 0 prefix as appropriate */
#define FL_ADDBASE    0x02    /* add base# prefix to converted value */
#define FL_HEXUPPER   0x04    /* use uppercase when converting to hex */
#define FL_UNSIGNED   0x08    /* don't add any sign */

extern char *fmtulong __P((unsigned long int, int, char *, size_t, int));

/* Declarations for functions defined in lib/sh/fmtulong.c */
#if defined (HAVE_LONG_LONG)
extern char *fmtullong __P((unsigned long long int, int, char *, size_t, int));
#endif

/* Declarations for functions defined in lib/sh/fmtumax.c */
extern char *fmtumax __P((uintmax_t, int, char *, size_t, int));

/* Declarations for functions defined in lib/sh/getcwd.c */
#if !defined (HAVE_GETCWD)
extern char *getcwd __P((char *, size_t));
#endif

/* Declarations for functions defined in lib/sh/itos.c */
extern char *inttostr __P((intmax_t, char *, size_t));
extern char *itos __P((intmax_t));
extern char *uinttostr __P((uintmax_t, char *, size_t));
extern char *uitos __P((uintmax_t));

/* declarations for functions defined in lib/sh/makepath.c */
#define MP_DOTILDE	0x01
#define MP_DOCWD	0x02
#define MP_RMDOT	0x04

extern char *sh_makepath __P((const char *, const char *, int));

/* declarations for functions defined in lib/sh/netconn.c */
extern int isnetconn __P((int));

/* declarations for functions defined in lib/sh/netopen.c */
extern int netopen __P((char *));

/* Declarations for  functions defined in lib/sh/oslib.c */

#if !defined (HAVE_DUP2) || defined (DUP2_BROKEN)
extern int dup2 __P((int, int));
#endif

#if !defined (HAVE_GETDTABLESIZE)
extern int getdtablesize __P((void));
#endif /* !HAVE_GETDTABLESIZE */

#if !defined (HAVE_GETHOSTNAME)
extern int gethostname __P((char *, int));
#endif /* !HAVE_GETHOSTNAME */

extern int getmaxgroups __P((void));
extern long getmaxchild __P((void));

/* declarations for functions defined in lib/sh/pathcanon.c */
#define PATH_CHECKDOTDOT	0x0001
#define PATH_CHECKEXISTS	0x0002
#define PATH_HARDPATH		0x0004
#define PATH_NOALLOC		0x0008

extern char *sh_canonpath __P((char *, int));

/* declarations for functions defined in lib/sh/pathphys.c */
extern char *sh_physpath __P((char *, int));
extern char *sh_realpath __P((const char *, char *));

/* declarations for functions defined in lib/sh/setlinebuf.c */
#ifdef NEED_SH_SETLINEBUF_DECL
extern int sh_setlinebuf __P((FILE *));
#endif

/* declarations for functions defined in lib/sh/shquote.c */
extern char *sh_single_quote __P((char *));
extern char *sh_double_quote __P((char *));
extern char *sh_un_double_quote __P((char *));
extern char *sh_backslash_quote __P((char *));
extern char *sh_backslash_quote_for_double_quotes __P((char *));
extern int sh_contains_shell_metas __P((char *));

/* declarations for functions defined in lib/sh/spell.c */
extern int spname __P((char *, char *));

/* declarations for functions defined in lib/sh/strcasecmp.c */
#if !defined (HAVE_STRCASECMP)
extern int strncasecmp __P((const char *, const char *, int));
extern int strcasecmp __P((const char *, const char *));
#endif /* HAVE_STRCASECMP */

/* declarations for functions defined in lib/sh/strerror.c */
#if !defined (strerror)
extern char *strerror __P((int));
#endif

/* declarations for functions defined in lib/sh/strftime.c */
#if !defined (HAVE_STRFTIME) && defined (NEED_STRFTIME_DECL)
extern size_t strftime __P((char *, size_t, const char *, const struct tm *));
#endif

/* declarations for functions defined in lib/sh/strindex.c */
extern char *strindex __P((const char *, const char *));

/* declarations for functions and structures defined in lib/sh/stringlist.c */

/* This is a general-purpose argv-style array struct. */
typedef struct _list_of_strings {
  char **list;
  int list_size;
  int list_len;
} STRINGLIST;

typedef int sh_strlist_map_func_t __P((char *));

extern STRINGLIST *strlist_create __P((int));
extern STRINGLIST *strlist_resize __P((STRINGLIST *, int));
extern void strlist_flush __P((STRINGLIST *));
extern void strlist_dispose __P((STRINGLIST *));
extern int strlist_remove __P((STRINGLIST *, char *));
extern STRINGLIST *strlist_copy __P((STRINGLIST *));
extern STRINGLIST *strlist_merge __P((STRINGLIST *, STRINGLIST *));
extern STRINGLIST *strlist_append __P((STRINGLIST *, STRINGLIST *));
extern STRINGLIST *strlist_prefix_suffix __P((STRINGLIST *, char *, char *));
extern void strlist_print __P((STRINGLIST *, char *));
extern void strlist_walk __P((STRINGLIST *, sh_strlist_map_func_t *));
extern void strlist_sort __P((STRINGLIST *));

/* declarations for functions defined in lib/sh/stringvec.c */

extern char **strvec_create __P((int));
extern char **strvec_resize __P((char **, int));
extern void strvec_flush __P((char **));
extern void strvec_dispose __P((char **));
extern int strvec_remove __P((char **, char *));
extern int strvec_len __P((char **));
extern int strvec_search __P((char **, char *));
extern char **strvec_copy __P((char **));
extern int strvec_strcmp __P((char **, char **));
extern void strvec_sort __P((char **));

extern char **strvec_from_word_list __P((WORD_LIST *, int, int, int *));
extern WORD_LIST *strvec_to_word_list __P((char **, int, int));

/* declarations for functions defined in lib/sh/strtod.c */
#if !defined (HAVE_STRTOD)
extern double strtod __P((const char *, char **));
#endif

/* declarations for functions defined in lib/sh/strtol.c */
#if !HAVE_DECL_STRTOL
extern long strtol __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strtoll.c */
#if defined (HAVE_LONG_LONG) && !HAVE_DECL_STRTOLL
extern long long strtoll __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strtoul.c */
#if !HAVE_DECL_STRTOUL
extern unsigned long strtoul __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strtoull.c */
#if defined (HAVE_LONG_LONG) && !HAVE_DECL_STRTOULL
extern unsigned long long strtoull __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strimax.c */
#if !HAVE_DECL_STRTOIMAX
extern intmax_t strtoimax __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strumax.c */
#if !HAVE_DECL_STRTOUMAX
extern uintmax_t strtoumax __P((const char *, char **, int));
#endif

/* declarations for functions defined in lib/sh/strtrans.c */
extern char *ansicstr __P((char *, int, int, int *, int *));
extern char *ansic_quote __P((char *, int, int *));
extern int ansic_shouldquote __P((const char *));
extern char *ansiexpand __P((char *, int, int, int *));

/* declarations for functions defined in lib/sh/timeval.c.  No prototypes
   so we don't have to count on having a definition of struct timeval in
   scope when this file is included. */
extern void timeval_to_secs ();
extern void print_timeval ();

/* declarations for functions defined in lib/sh/tmpfile.c */
#define MT_USETMPDIR		0x0001
#define MT_READWRITE		0x0002
#define MT_USERANDOM		0x0004

extern char *sh_mktmpname __P((char *, int));
extern int sh_mktmpfd __P((char *, int, char **));
/* extern FILE *sh_mktmpfp __P((char *, int, char **)); */

/* declarations for functions defined in lib/sh/xstrchr.c */
#undef xstrchr
extern char *xstrchr __P((const char *, int));

/* declarations for functions defined in lib/sh/zread.c */
extern ssize_t zread __P((int, char *, size_t));
extern ssize_t zreadintr __P((int, char *, size_t));
extern ssize_t zreadc __P((int, char *));
extern void zreset __P((void));
extern void zsyncfd __P((int));

/* declarations for functions defined in lib/sh/zwrite.c */
extern int zwrite __P((int, char *, size_t));

#endif /* _EXTERNS_H_ */
