/*  GNU SED, a batch stream editor.
    Copyright (C) 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1998
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

#include <stdio.h>

/* type countT is used to keep track of line numbers, etc. */
typedef unsigned long countT;

/* type flagT is used for boolean values */
typedef int flagT;

/* Oftentimes casts are used as an ugly hack to silence warnings
 * from the compiler.  However, sometimes those warnings really
 * do point to something worth avoiding.  I define this
 * dummy marker to make searching for them with a text editor
 * much easier, in case I want to verify that they are all
 * legitimate.  It is defined in the way it is so that it is
 * easy to disable all casts so that the compiler (or lint)
 * can tell me potentially interesting things about what would
 * happen to the code without the explicit casts.
 */
#ifdef LOUD_LINT
# define CAST(x)
#else
# define CAST(x) (x)
#endif


/* Struct vector is used to describe a chunk of a compiled sed program.
 * There is one vector for the main program, and one for each { } pair.
 * For {} blocks, RETURN_[VI] tells where to continue execution after
 * this VECTOR.
 */

struct vector {
  struct sed_cmd *v;		/* a dynamically allocated array */
  size_t v_allocated;		/* ... number slots allocated */
  size_t v_length;		/* ... number of slots in use */
  struct error_info *err_info;	/* info for unclosed '{'s */
  struct vector *return_v;	/* where the matching '}' will return to */
  countT return_i;		/* ... further info */
};


/* Goto structure is used to hold both GOTO's and labels.  There are two
 * separate lists, one of goto's, called 'jumps', and one of labels, called
 * 'labels'.
 * The V element points to the descriptor for the program-chunk in which the
 * goto was encountered.
 * The v_index element counts which element of the vector actually IS the
 * goto/label.
 * The NAME element is the null-terminated name of the label.
 * "next" is the next goto/label in the linked list.
 */

struct sed_label {
  struct vector *v;
  countT v_index;
  char *name;
  struct sed_label *next;
};

struct text_buf {
  char *text;
  size_t text_len;
};


struct addr {
  enum {
    addr_is_null,  /* null address */
    addr_is_num,   /* a.addr_number is valid */
    addr_is_mod,   /* doing every n'th line; a.m.* are valid */
    addr_is_last,  /* address is $ */
    addr_is_regex  /* a.addr_regex is valid */
  } addr_type;

  union {
    regex_t *addr_regex;
    countT addr_number;
    struct {countT modulo; countT offset;} m;
  } a;
};

struct subst {
    regex_t *regx;
    char *replacement;
    size_t replace_length;
    countT numb;	/* if >0, only substitute for match number "numb" */
    FILE *wfile;	/* 'w' option given */
    char global;	/* 'g' option given */
    char print;		/* 'p' option given */
};


struct sed_cmd {
  struct addr a1;
  struct addr a2;

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

      /* This is used for b and t commands */
      struct sed_label *jump;

      /* This for r command */
      const char *rfile;

      /* This for w command */
      FILE *wfile;

      /* This for the y command */
      unsigned char *translate;

      /* For {} */
      struct vector *sub;

      /* This for the hairy s command */
      struct subst cmd_regex;
    } x;
};



struct vector *compile_string P_((struct vector *, char *str));
struct vector *compile_file P_((struct vector *, const char *cmdfile));
void check_final_program P_((struct vector *));
void close_all_files P_((void));

countT process_files P_((struct vector *, char **argv));

int main P_((int, char **));
flagT map_file P_((FILE *fp, VOID **base, size_t *len));
void unmap_file P_((VOID *base, size_t len));

extern flagT use_extended_syntax_p;
extern flagT rx_testing;
extern flagT no_default_output;
extern flagT POSIXLY_CORRECT;
