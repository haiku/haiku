/*  GNU SED, a batch stream editor.
    Copyright (C) 1989,90,91,92,93,94,95,98,99,2002,2003
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_AIX)
#pragma alloca
#else
# if !defined(alloca)           /* predefined by HP cc +Olibcalls */
#  ifdef __GNUC__
#   define alloca(size) __builtin_alloca(size)
#  else
#   if HAVE_ALLOCA_H
#    include <alloca.h>
#   else
#    if defined(__hpux)
        void *alloca ();
#    else
#     if !defined(__OS2__) && !defined(WIN32)
	char *alloca ();
#     else
#      include <malloc.h>       /* OS/2 defines alloca in here */
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#ifndef BOOTSTRAP
#include <stdio.h>
#endif
#include "basicdefs.h"
#include "regex.h"
#include "utils.h"

#ifdef HAVE_WCHAR_H
# include <wchar.h>
#endif
#ifdef HAVE_WCTYPE_H
# include <wctype.h>
#endif


/* Struct vector is used to describe a compiled sed program. */
struct vector {
  struct sed_cmd *v;	/* a dynamically allocated array */
  size_t v_allocated;	/* ... number slots allocated */
  size_t v_length;	/* ... number of slots in use */
};


struct text_buf {
  char *text;
  size_t text_length;
};

enum replacement_types {
  repl_asis = 0,
  repl_uppercase = 1,
  repl_lowercase = 2,
  repl_uppercase_first = 4,
  repl_lowercase_first = 8,
  repl_modifiers = repl_uppercase_first | repl_lowercase_first,

  /* These are given to aid in debugging */
  repl_uppercase_uppercase = repl_uppercase_first | repl_uppercase,
  repl_uppercase_lowercase = repl_uppercase_first | repl_lowercase,
  repl_lowercase_uppercase = repl_lowercase_first | repl_uppercase,
  repl_lowercase_lowercase = repl_lowercase_first | repl_lowercase
};

enum addr_types {
  addr_is_null,		/* null address */
  addr_is_regex,	/* a.addr_regex is valid */
  addr_is_num,		/* a.addr_number is valid */
  addr_is_num_mod,	/* a.addr_number is valid, addr_step is modulo */
  addr_is_num2,		/* a.addr_number is valid (only valid for addr2) */
  addr_is_step,		/* address is +N (only valid for addr2) */
  addr_is_step_mod,	/* address is ~N (only valid for addr2) */
  addr_is_last		/* address is $ */
};

struct addr {
  enum addr_types addr_type;
  countT addr_number;
  countT addr_step;
  regex_t *addr_regex;
};


struct replacement {
  char *prefix;
  size_t prefix_length;
  int subst_id;
  enum replacement_types repl_type;
  struct replacement *next;
};

struct subst {
  regex_t *regx;
  struct replacement *replacement;
  countT numb;		/* if >0, only substitute for match number "numb" */
  FILE *fp;		/* 'w' option given */
  unsigned global : 1;	/* 'g' option given */
  unsigned print : 2;	/* 'p' option given (before/after eval) */
  unsigned eval : 1;	/* 'e' option given */
  unsigned max_id : 4;  /* maximum backreference on the RHS */
};

#ifdef REG_PERL
/* This is the structure we store register match data in.  See
   regex.texinfo for a full description of what registers match.  */
struct re_registers
{
  unsigned num_regs;
  regoff_t *start;
  regoff_t *end;
};
#endif



struct sed_cmd {
  struct addr *a1;	/* save space: usually is NULL */
  struct addr *a2;

  /* non-zero if a1 has been matched; apply command until a2 matches */
  char a1_matched;

  /* non-zero: only apply command to non-matches */
  char addr_bang;

  /* the actual command */
  char cmd;

  /* auxiliary data for various commands */
  union {
    /* This structure is used for a, i, and c commands */
    struct text_buf cmd_txt;

    /* This is used for the l, q and Q commands */
    int int_arg;

    /* This is used for {}, b, and t commands */
    countT jump_index;

    /* This for r command */
    char *fname;

    /* This for the hairy s command */
    struct subst *cmd_subst;

    /* This for R and w command */
    FILE *fp;

    /* This for the y command */
    unsigned char *translate;
    char **translatemb;
  } x;
};



void bad_prog P_((const char *why));
size_t normalize_text P_((char *, size_t));
struct vector *compile_string P_((struct vector *, char *str, size_t len));
struct vector *compile_file P_((struct vector *, const char *cmdfile));
void check_final_program P_((struct vector *));
void rewind_read_files P_((void));
void finish_program P_((struct vector *));

regex_t *compile_regex P_((struct buffer *b, int flags, int needed_sub));
int match_regex P_((regex_t *regex,
		    char *buf, size_t buflen, size_t buf_start_offset,
		    struct re_registers *regarray, int regsize));
#ifdef DEBUG_LEAKS
void release_regex P_((regex_t *));
#endif

int process_files P_((struct vector *, char **argv));

int main P_((int, char **));

extern void fmt P_ ((char *line, char *line_end, int max_length, FILE *output_file));

extern int extended_regexp_flags;

/* If set, fflush(stdout) on every line output. */
extern flagT unbuffered_output;

/* If set, don't write out the line unless explicitly told to. */
extern flagT no_default_output;

/* If set, reset line counts on every new file. */
extern flagT separate_files;

/* Do we need to be pedantically POSIX compliant? */
extern flagT POSIXLY_CORRECT;

/* How long should the `l' command's output line be? */
extern countT lcmd_out_line_len;

/* How do we edit files in-place? (we don't if NULL) */
extern char *in_place_extension;
