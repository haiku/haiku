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

/* compile.c: translate sed source into internal form */

#include "sed.h"
#include "strverscmp.h"
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#else
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <obstack.h>

extern flagT no_default_output;
extern flagT POSIXLY_CORRECT;


#define YMAP_LENGTH		256 /*XXX shouldn't this be (UCHAR_MAX+1)?*/
#define VECTOR_ALLOC_INCREMENT	40

/* let's not confuse text editors that have only dumb bracket-matching... */
#define OPEN_BRACKET	'['
#define CLOSE_BRACKET	']'
#define OPEN_BRACE	'{'
#define CLOSE_BRACE	'}'

struct prog_info {
  /* When we're reading a script command from a string, `prog.base'
     points to the first character in the string, 'prog.cur' points
     to the current character in the string, and 'prog.end' points
     to the end of the string.  This allows us to compile script
     strings that contain nulls. */
  const unsigned char *base;
  const unsigned char *cur;
  const unsigned char *end;

  /* This is the current script file.  If it is NULL, we are reading
     from a string stored at `prog.cur' instead.  If both `prog.file'
     and `prog.cur' are NULL, we're in trouble! */
  FILE *file;
};

/* Information used to give out useful and informative error messages. */
struct error_info {
  /* This is the name of the current script file. */
  const char *name;

  /* This is the number of the current script line that we're compiling. */
  countT line;

  /* This is the index of the "-e" expressions on the command line. */
  countT string_expr_count;
};


/* Label structure used to resolve GOTO's, labels, and block beginnings. */
struct sed_label {
  countT v_index;		/* index of vector element being referenced */
  char *name;			/* NUL-terminated name of the label */
  struct error_info err_info;	/* track where `{}' blocks start */
  struct sed_label *next;	/* linked list (stack) */
};

struct special_files {
  char *name;
  FILE **pfp;
};

FILE *my_stdin, *my_stdout, *my_stderr;
struct special_files special_files[] = {
  { "/dev/stdin", &my_stdin },
  { "/dev/stdout", &my_stdout },
  { "/dev/stderr", &my_stderr },
  { NULL, NULL }
};

/* This structure tracks files opened by the `w' and `s///w' commands
   so that they may all be closed cleanly at normal program termination.
   Those marked as `special' are not closed. */
struct fp_list {
    char *name;
    int special;
    FILE *fp;
    struct fp_list *link;
  };


/* Where we are in the processing of the input. */
static struct prog_info prog;
static struct error_info cur_input;

/* Information about labels and jumps-to-labels.  This is used to do
   the required backpatching after we have compiled all the scripts. */
static struct sed_label *jumps = NULL;
static struct sed_label *labels = NULL;

/* We wish to detect #n magic only in the first input argument;
   this flag tracks when we have consumed the first file of input. */
static flagT first_script = TRUE;

/* Allow for scripts like "sed -e 'i\' -e foo": */
static struct buffer *pending_text = NULL;
static struct text_buf *old_text_buf = NULL;

/* Information about block start positions.  This is used to backpatch
   block end positions. */
static struct sed_label *blocks = NULL;

/* Use an obstack for compilation. */
static struct obstack obs;

/* Various error messages we may want to print */
static const char errors[] =
  "Multiple `!'s\0"
  "Unexpected `,'\0"
  "Cannot use +N or ~N as first address\0"
  "Unmatched `{'\0"
  "Unexpected `}'\0"
  "Extra characters after command\0"
  "Expected \\ after `a', `c' or `i'\0"
  "`}' doesn't want any addresses\0"
  ": doesn't want any addresses\0"
  "Comments don't accept any addresses\0"
  "Missing command\0"
  "Command only uses one address\0"
  "Unterminated address regex\0"
  "Unterminated `s' command\0"
  "Unterminated `y' command\0"
  "Unknown option to `s'\0"
  "multiple `p' options to `s' command\0"
  "multiple `g' options to `s' command\0"
  "multiple number options to `s' command\0"
  "number option to `s' command may not be zero\0"
  "strings for y command are different lengths\0"
  "expected newer version of sed";

#define BAD_BANG (errors)
#define BAD_COMMA (BAD_BANG + sizeof(N_("Multiple `!'s")))
#define BAD_PLUS (BAD_COMMA + sizeof(N_("Unexpected `,'")))
#define EXCESS_OPEN_BRACE (BAD_PLUS + sizeof(N_("Cannot use +N or ~N as first address")))
#define EXCESS_CLOSE_BRACE (EXCESS_OPEN_BRACE + sizeof(N_("Unmatched `{'")))
#define EXCESS_JUNK (EXCESS_CLOSE_BRACE + sizeof(N_("Unexpected `}'")))
#define EXPECTED_SLASH (EXCESS_JUNK + sizeof(N_("Extra characters after command")))
#define NO_CLOSE_BRACE_ADDR (EXPECTED_SLASH + sizeof(N_("Expected \\ after `a', `c' or `i'")))
#define NO_COLON_ADDR (NO_CLOSE_BRACE_ADDR + sizeof(N_("`}' doesn't want any addresses")))
#define NO_SHARP_ADDR (NO_COLON_ADDR + sizeof(N_(": doesn't want any addresses")))
#define NO_COMMAND (NO_SHARP_ADDR + sizeof(N_("Comments don't accept any addresses")))
#define ONE_ADDR (NO_COMMAND + sizeof(N_("Missing command")))
#define UNTERM_ADDR_RE (ONE_ADDR + sizeof(N_("Command only uses one address")))
#define UNTERM_S_CMD (UNTERM_ADDR_RE + sizeof(N_("Unterminated address regex")))
#define UNTERM_Y_CMD (UNTERM_S_CMD + sizeof(N_("Unterminated `s' command")))
#define UNKNOWN_S_OPT (UNTERM_Y_CMD + sizeof(N_("Unterminated `y' command")))
#define EXCESS_P_OPT (UNKNOWN_S_OPT + sizeof(N_("Unknown option to `s'")))
#define EXCESS_G_OPT (EXCESS_P_OPT + sizeof(N_("multiple `p' options to `s' command")))
#define EXCESS_N_OPT (EXCESS_G_OPT + sizeof(N_("multiple `g' options to `s' command")))
#define ZERO_N_OPT (EXCESS_N_OPT + sizeof(N_("multiple number options to `s' command")))
#define Y_CMD_LEN (ZERO_N_OPT + sizeof(N_("number option to `s' command may not be zero")))
#define ANCIENT_VERSION (Y_CMD_LEN + sizeof(N_("strings for y command are different lengths")))
#define END_ERRORS (ANCIENT_VERSION + sizeof(N_("expected newer version of sed")))

static struct fp_list *file_read = NULL;
static struct fp_list *file_write = NULL;


/* Read the next character from the program.  Return EOF if there isn't
   anything to read.  Keep cur_input.line up to date, so error messages
   can be meaningful. */
static int inchar P_((void));
static int
inchar()
{
  int ch = EOF;

  if (prog.cur)
    {
      if (prog.cur < prog.end)
	ch = *prog.cur++;
    }
  else if (prog.file)
    {
      if (!feof(prog.file))
	ch = getc(prog.file);
    }
  if (ch == '\n')
    ++cur_input.line;
  return ch;
}

/* unget `ch' so the next call to inchar will return it.   */
static void savchar P_((int ch));
static void
savchar(ch)
  int ch;
{
  if (ch == EOF)
    return;
  if (ch == '\n' && cur_input.line > 0)
    --cur_input.line;
  if (prog.cur)
    {
      if (prog.cur <= prog.base || *--prog.cur != ch)
	panic(_("Called savchar() with unexpected pushback (%x)"),
	      CAST(unsigned char)ch);
    }
  else
    ungetc(ch, prog.file);
}

/* Read the next non-blank character from the program.  */
static int in_nonblank P_((void));
static int
in_nonblank()
{
  int ch;
  do
    ch = inchar();
    while (ISBLANK(ch));
  return ch;
}

/* Read an integer value from the program.  */
static countT in_integer P_((int ch));
static countT
in_integer(ch)
  int ch;
{
  countT num = 0;

  while (ISDIGIT(ch))
    {
      num = num * 10 + ch - '0';
      ch = inchar();
    }
  savchar(ch);
  return num;
}

static int add_then_next P_((struct buffer *b, int ch));
static int
add_then_next(b, ch)
  struct buffer *b;
  int ch;
{
  add1_buffer(b, ch);
  return inchar();
}

static char * convert_number P_((char *, char *, const char *, int, int, int));
static char *
convert_number(result, buf, bufend, base, maxdigits, default_char)
  char *result;
  char *buf;
  const char *bufend;
  int base;
  int maxdigits;
  int default_char;
{
  int n = 0;
  char *p;

  for (p=buf; p < bufend && maxdigits-- > 0; ++p)
    {
      int d = -1;
      switch (*p)
	{
	case '0': d = 0x0; break;
	case '1': d = 0x1; break;
	case '2': d = 0x2; break;
	case '3': d = 0x3; break;
	case '4': d = 0x4; break;
	case '5': d = 0x5; break;
	case '6': d = 0x6; break;
	case '7': d = 0x7; break;
	case '8': d = 0x8; break;
	case '9': d = 0x9; break;
	case 'A': case 'a': d = 0xa; break;
	case 'B': case 'b': d = 0xb; break;
	case 'C': case 'c': d = 0xc; break;
	case 'D': case 'd': d = 0xd; break;
	case 'E': case 'e': d = 0xe; break;
	case 'F': case 'f': d = 0xf; break;
	}
      if (d < 0 || base <= d)
	break;
      n = n * base + d;
    }
  if (p == buf)
    *result = default_char;
  else
    *result = n;
  return p;
}


/* Read in a filename for a `r', `w', or `s///w' command. */
static struct buffer *read_filename P_((void));
static struct buffer *
read_filename()
{
  struct buffer *b;
  int ch;

  b = init_buffer();
  ch = in_nonblank();
  while (ch != EOF && ch != '\n')
    {
#if 0 /*XXX ZZZ 1998-09-12 kpp: added, then had second thoughts*/
      if (!POSIXLY_CORRECT)
	if (ch == ';' || ch == CLOSE_BRACE || ch == '#')
	  {
	    savchar(ch);
	    break;
	  }
#endif
      add1_buffer(b, ch);
      ch = inchar();
    }
  add1_buffer(b, '\0');
  return b;
}

static FILE *get_openfile P_((struct fp_list **file_ptrs, char *mode, flagT fail));
static FILE *
get_openfile(file_ptrs, mode, fail)
     struct fp_list **file_ptrs;
     char *mode;
     flagT fail;
{
  struct buffer *b;
  char *file_name;
  struct fp_list *p;
  int is_stderr;

  b = read_filename();
  file_name = get_buffer(b);
  for (p=*file_ptrs; p; p=p->link)
    if (strcmp(p->name, file_name) == 0)
      break;

  if (!p)
    {
      FILE *fp = NULL;
      if (!POSIXLY_CORRECT)
	{
	  /* Check whether it is a special file (stdin, stdout or stderr) */
	  struct special_files *special = special_files;
		  
	  /* std* sometimes are not constants, so they
	     cannot be used in the initializer for special_files */
	  my_stdin = stdin; my_stdout = stdout; my_stderr = stderr;
	  for (special = special_files; special->name; special++)
	    if (strcmp(special->name, file_name) == 0)
	      {
		fp = *special->pfp;
		break;
	      }
	}

      p = OB_MALLOC(&obs, 1, struct fp_list);
      p->name = ck_strdup(file_name);
      p->special = fp != NULL;
      if (!fp)
	fp = ck_fopen(p->name, mode, fail);
      p->fp = fp;
      p->link = *file_ptrs;
      *file_ptrs = p;
    }
  free_buffer(b);
  return p->fp;
}


static struct sed_cmd *next_cmd_entry P_((struct vector **vectorp));
static struct sed_cmd *
next_cmd_entry(vectorp)
  struct vector **vectorp;
{
  struct sed_cmd *cmd;
  struct vector *v;

  v = *vectorp;
  if (v->v_length == v->v_allocated)
    {
      v->v_allocated += VECTOR_ALLOC_INCREMENT;
      v->v = REALLOC(v->v, v->v_allocated, struct sed_cmd);
    }

  cmd = v->v + v->v_length;
  cmd->a1 = NULL;
  cmd->a2 = NULL;
  cmd->a1_matched = FALSE;
  cmd->addr_bang = FALSE;
  cmd->cmd = '\0';	/* something invalid, to catch bugs early */

  *vectorp  = v;
  return cmd;
}

static int snarf_char_class P_((struct buffer *b));
static int
snarf_char_class(b)
  struct buffer *b;
{
  int ch;

  ch = inchar();
  if (ch == '^')
    ch = add_then_next(b, ch);
  if (ch == CLOSE_BRACKET)
    ch = add_then_next(b, ch);
  while (ch != EOF && ch != '\n' && ch != CLOSE_BRACKET)
    {
      if (ch == OPEN_BRACKET)
	{
	  int prev;
	  int delim = ch = add_then_next(b, ch);

	  if (delim != '.'  &&  delim != ':'  &&  delim != '=')
	    continue; /* bypass the add_then_next() call at bottom of loop */
	  for (prev=ch=add_then_next(b, ch);
		  !(ch==CLOSE_BRACKET && prev==delim); ch=add_then_next(b, ch))
	    {
	      if (ch == EOF || ch == '\n')
		return ch;
	      prev = ch;
	    }
	}
#ifndef REG_PERL
      else if (ch == '\\')
	{
	  ch = inchar();
	  if (ch == EOF)
	    break;
	  if (ch != 'n' && ch != '\n')
	    {
	      add1_buffer(b, '\\');
	      continue; /* bypass the add_then_next() call at bottom of loop */
	    }
	  ch = '\n';
	}
#endif
      ch = add_then_next(b, ch);
    }

  return ch;
}

static struct buffer *match_slash P_((int slash, flagT regex, flagT keep_back));
static struct buffer *
match_slash(slash, regex, keep_backwhack)
  int slash;
  flagT regex;
  flagT keep_backwhack;
{
  struct buffer *b;
  int ch;

  b = init_buffer();
  while ((ch = inchar()) != EOF && ch != '\n' && ch != slash)
    {
      if (ch == '\\')
	{
	  ch = inchar();
	  if (ch == EOF)
	    break;
#ifndef REG_PERL
	  else if (ch == 'n' && regex)
	    ch = '\n';
#endif
	  else if (ch != '\n' && (ch != slash || keep_backwhack))
	    add1_buffer(b, '\\');
	}
      else if (ch == OPEN_BRACKET && regex)
	{
	  add1_buffer(b, ch);
	  ch = snarf_char_class(b);
	  if (ch != CLOSE_BRACKET)
	    break;
	}
      add1_buffer(b, ch);
    }
  if (ch == slash)
    return b;

  if (ch == '\n')
    savchar(ch);	/* for proper line number in error report */
  free_buffer(b);
  return NULL;
}

static flagT mark_subst_opts P_((struct subst *cmd));
static flagT
mark_subst_opts(cmd)
  struct subst *cmd;
{
  int flags = 0;
  int ch;

  cmd->global = FALSE;
  cmd->print = FALSE;
  cmd->eval = FALSE;
  cmd->numb = 0;
  cmd->fp = NULL;

  for (;;)
    switch ( (ch = in_nonblank()) )
      {
      case 'i':	/* GNU extension */
      case 'I':	/* GNU extension */
	flags |= REG_ICASE;
	break;

#ifdef REG_PERL
      case 's':	/* GNU extension */
      case 'S':	/* GNU extension */
	if (extended_regexp_flags & REG_PERL)
	  flags |= REG_DOTALL;
	break;

      case 'x':	/* GNU extension */
      case 'X':	/* GNU extension */
	if (extended_regexp_flags & REG_PERL)
	  flags |= REG_EXTENDED;
	break;
#endif

      case 'm':	/* GNU extension */
      case 'M':	/* GNU extension */
	flags |= REG_NEWLINE;
	break;

      case 'e':
	cmd->eval = TRUE;
	break;

      case 'p':
	if (cmd->print)
	  bad_prog(_(EXCESS_P_OPT));
	cmd->print |= (TRUE << cmd->eval); /* 1=before eval, 2=after */
	break;

      case 'g':
	if (cmd->global)
	  bad_prog(_(EXCESS_G_OPT));
	cmd->global = TRUE;
	break;

      case 'w':
	cmd->fp = get_openfile(&file_write, "w", TRUE);
	return flags;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	if (cmd->numb)
	  bad_prog(_(EXCESS_N_OPT));
	cmd->numb = in_integer(ch);
	if (!cmd->numb)
	  bad_prog(_(ZERO_N_OPT));
	break;

      case CLOSE_BRACE:
      case '#':
	savchar(ch);
	/* Fall Through */
      case EOF:
      case '\n':
      case ';':
	return flags;

      case '\r':
	if (inchar() == '\n')
	  return flags;
	/* FALLTHROUGH */

      default:
	bad_prog(_(UNKNOWN_S_OPT));
	/*NOTREACHED*/
      }
}


/* read in a label for a `:', `b', or `t' command */
static char *read_label P_((void));
static char *
read_label()
{
  struct buffer *b;
  int ch;
  char *ret;

  b = init_buffer();
  ch = in_nonblank();

  while (ch != EOF && ch != '\n'
	 && !ISBLANK(ch) && ch != ';' && ch != CLOSE_BRACE && ch != '#')
    {
      add1_buffer(b, ch);
      ch = inchar();
    }
  savchar(ch);
  add1_buffer(b, '\0');
  ret = ck_strdup(get_buffer(b));
  free_buffer(b);
  return ret;
}

/* Store a label (or label reference) created by a `:', `b', or `t'
   command so that the jump to/from the label can be backpatched after
   compilation is complete, or a reference created by a `{' to be
   backpatched when the corresponding `}' is found.  */
static struct sed_label *setup_label
  P_((struct sed_label *, countT, char *, const struct error_info *));
static struct sed_label *
setup_label(list, idx, name, err_info)
  struct sed_label *list;
  countT idx;
  char *name;
  const struct error_info *err_info;
{
  struct sed_label *ret = OB_MALLOC(&obs, 1, struct sed_label);
  ret->v_index = idx;
  ret->name = name;
  if (err_info)
    MEMCPY(&ret->err_info, err_info, sizeof ret->err_info);
  ret->next = list;
  return ret;
}

static struct sed_label *release_label P_((struct sed_label *list_head));
static struct sed_label *
release_label(list_head)
  struct sed_label *list_head;
{
  struct sed_label *ret;

  if (!list_head)
    return NULL;
  ret = list_head->next;

  FREE(list_head->name);

#if 0
  /* We use obstacks */
  FREE(list_head);
#endif
  return ret;
}

static struct replacement *new_replacement P_((char *, size_t,
					       enum replacement_types));
static struct replacement *
new_replacement(text, length, type)
  char *text;
  size_t length;
  enum replacement_types type;
{
  struct replacement *r = OB_MALLOC(&obs, 1, struct replacement);

  r->prefix = text;
  r->prefix_length = length;
  r->subst_id = -1;
  r->repl_type = type;

  /* r-> next = NULL; */
  return r;
}

static void setup_replacement P_((struct subst *, const char *, size_t));
static void
setup_replacement(sub, text, length)
     struct subst *sub;
     const char *text;
     size_t length;
{
  char *base;
  char *p;
  char *text_end;
  enum replacement_types repl_type = repl_asis, save_type = repl_asis;
  struct replacement root;
  struct replacement *tail;

  sub->max_id = 0;
  base = MEMDUP(text, length, char);
  length = normalize_text(base, length);

  text_end = base + length;
  tail = &root;

  for (p=base; p<text_end; ++p)
    {
      if (*p == '\\')
	{
	  /* Preceding the backslash may be some literal text: */
	  tail = tail->next =
	    new_replacement(base, CAST(size_t)(p - base), repl_type);

	  repl_type = save_type;

	  /* Skip the backslash and look for a numeric back-reference: */
	  ++p;
	  if (p<text_end)
	    switch (*p)
	      {
	      case '0': case '1': case '2': case '3': case '4': 
	      case '5': case '6': case '7': case '8': case '9': 
		tail->subst_id = *p - '0';
		if (sub->max_id < tail->subst_id)
		  sub->max_id = tail->subst_id;
		break;

	      case 'L':
		repl_type = repl_lowercase;
		save_type = repl_lowercase;
		break;

	      case 'U':
		repl_type = repl_uppercase;
		save_type = repl_uppercase;
		break;
		
	      case 'E':
		repl_type = repl_asis;
		save_type = repl_asis;
		break;

	      case 'l':
		save_type = repl_type;
		repl_type |= repl_lowercase_first;
		break;

	      case 'u':
		save_type = repl_type;
		repl_type |= repl_uppercase_first;
		break;
		
	      default:
		p[-1] = *p;
		++tail->prefix_length;
	      }

	  base = p + 1;
	}
      else if (*p == '&')
	{
	  /* Preceding the ampersand may be some literal text: */
	  tail = tail->next =
	    new_replacement(base, CAST(size_t)(p - base), repl_type);

	  repl_type = save_type;
	  tail->subst_id = 0;
	  base = p + 1;
	}
  }
  /* There may be some trailing literal text: */
  if (base < text_end)
    tail = tail->next =
      new_replacement(base, CAST(size_t)(text_end - base), repl_type);

  tail->next = NULL;
  sub->replacement = root.next;
}

static void read_text P_((struct text_buf *buf, int leadin_ch));
static void
read_text(buf, leadin_ch)
  struct text_buf *buf;
  int leadin_ch;
{
  int ch;

  /* Should we start afresh (as opposed to continue a partial text)? */
  if (buf)
    {
      if (pending_text)
	free_buffer(pending_text);
      pending_text = init_buffer();
      buf->text = NULL;
      buf->text_length = 0;
      old_text_buf = buf;
    }
  /* assert(old_text_buf != NULL); */

  if (leadin_ch == EOF)
    return;
#ifndef NO_INPUT_INDENT
  if (leadin_ch != '\n')
    add1_buffer(pending_text, leadin_ch);
  ch = inchar();
#else /*NO_INPUT_INDENT*/
  if (leadin_ch == '\n')
    ch = in_nonblank();
  else
    {
      add1_buffer(pending_text, leadin_ch);
      ch = inchar();
    }
#endif /*NO_INPUT_INDENT*/

  while (ch!=EOF && ch!='\n')
    {
      if (ch == '\\')
	ch = inchar();
      if (ch == EOF)
	{
	  add1_buffer(pending_text, '\n');
	  return;
	}
      add1_buffer(pending_text, ch);
#ifdef NO_INPUT_INDENT
      if (ch == '\n')
	ch = in_nonblank();
      else
#endif /*NO_INPUT_INDENT*/
	ch = inchar();
    }
  add1_buffer(pending_text, '\n');

  if (!buf)
    buf = old_text_buf;
  buf->text_length = size_buffer(pending_text);
  buf->text = MEMDUP(get_buffer(pending_text), buf->text_length, char);
  free_buffer(pending_text);
  pending_text = NULL;
}


/* Try to read an address for a sed command.  If it succeeds,
   return non-zero and store the resulting address in `*addr'.
   If the input doesn't look like an address read nothing
   and return zero.  */
static flagT compile_address P_((struct addr *addr, int ch));
static flagT
compile_address(addr, ch)
  struct addr *addr;
  int ch;
{
  addr->addr_type = addr_is_null;
  addr->addr_step = 0;
  addr->addr_number = ~(countT)0;  /* extremely unlikely to ever match */
  addr->addr_regex = NULL;

  if (ch == '/' || ch == '\\')
    {
      int flags = 0;
      struct buffer *b;
      addr->addr_type = addr_is_regex;
      if (ch == '\\')
	ch = inchar();
      if ( !(b = match_slash(ch, TRUE, TRUE)) )
	bad_prog(_(UNTERM_ADDR_RE));

      for(;;)
	{
	  ch = in_nonblank();
          switch(ch)
	    {
	    case 'I':	/* GNU extension */
	      flags |= REG_ICASE;
	      break;

#ifdef REG_PERL
	    case 'S':	/* GNU extension */
	      if (extended_regexp_flags & REG_PERL)
		flags |= REG_DOTALL;
	      break;

	    case 'X':	/* GNU extension */
	      if (extended_regexp_flags & REG_PERL)
		flags |= REG_EXTENDED;
	      break;
#endif

	    case 'M':	/* GNU extension */
	      flags |= REG_NEWLINE;
	      break;

	    default:
	      savchar (ch);
	      addr->addr_regex = compile_regex (b, flags, 0);
	      free_buffer(b);
	      return TRUE;
	    }
	}
    }
  else if (ISDIGIT(ch))
    {
      addr->addr_number = in_integer(ch);
      addr->addr_type = addr_is_num;
      ch = in_nonblank();
      if (ch != '~')
	{
	  savchar(ch);
	}
      else
	{
	  countT step = in_integer(in_nonblank());
	  if (step > 0)
	    {
	      addr->addr_step = step;
	      addr->addr_type = addr_is_num_mod;
	    }
	}
    }
  else if (ch == '+' || ch == '~')
    {
      addr->addr_step = in_integer(in_nonblank());
      if (addr->addr_step==0)
	; /* default to addr_is_null; forces matching to stop on next line */
      else if (ch == '+')
	addr->addr_type = addr_is_step;
      else
	addr->addr_type = addr_is_step_mod;
    }
  else if (ch == '$')
    {
      addr->addr_type = addr_is_last;
    }
  else
    return FALSE;

  return TRUE;
}

/* Read a program (or a subprogram within `{' `}' pairs) in and store
   the compiled form in `*vector'.  Return a pointer to the new vector.  */
static struct vector *compile_program P_((struct vector *));
static struct vector *
compile_program(vector)
  struct vector *vector;
{
  struct sed_cmd *cur_cmd;
  struct buffer *b;
  int ch;

  if (!vector)
    {
      vector = MALLOC(1, struct vector);
      vector->v = NULL;
      vector->v_allocated = 0;
      vector->v_length = 0;

      obstack_init (&obs);
    }
  if (pending_text)
    read_text(NULL, '\n');

  for (;;)
    {
      struct addr a;

      while ((ch=inchar()) == ';' || ISSPACE(ch))
	;
      if (ch == EOF)
	break;

      cur_cmd = next_cmd_entry(&vector);
      if (compile_address(&a, ch))
	{
	  if (a.addr_type == addr_is_step
	      || a.addr_type == addr_is_step_mod)
	    bad_prog(_(BAD_PLUS));

	  cur_cmd->a1 = MEMDUP(&a, 1, struct addr);
	  ch = in_nonblank();
	  if (ch == ',')
	    {
	      if (!compile_address(&a, in_nonblank()))
		bad_prog(_(BAD_COMMA));
	      if (a.addr_type == addr_is_num)
		a.addr_type = addr_is_num2;
	      cur_cmd->a2 = MEMDUP(&a, 1, struct addr);
	      ch = in_nonblank();
	      if ((cur_cmd->a1->addr_type == addr_is_num
		   || cur_cmd->a1->addr_type == addr_is_num_mod)
	      	  && cur_cmd->a1->addr_number == 0)
		cur_cmd->a1_matched = TRUE;
	    }
	}
      if (ch == '!')
	{
	  cur_cmd->addr_bang = TRUE;
	  ch = in_nonblank();
	  if (ch == '!')
	    bad_prog(_(BAD_BANG));
	}

      cur_cmd->cmd = ch;
      switch (ch)
	{
	case '#':
	  if (cur_cmd->a1)
	    bad_prog(_(NO_SHARP_ADDR));
	  ch = inchar();
	  if (ch=='n' && first_script && cur_input.line < 2)
	    if (   (prog.base && prog.cur==2+prog.base)
		|| (prog.file && !prog.base && 2==ftell(prog.file)))
	      no_default_output = TRUE;
	  while (ch != EOF && ch != '\n')
	    ch = inchar();
	  continue;	/* restart the for (;;) loop */

	case 'v':
          /* This is an extension.  Programs needing GNU sed might start
           * with a `v' command so that other seds will stop.
           * We compare the version ignore it.
           */
          {
            char *version = read_label ();
            char *compared_version;
            compared_version = (*version == '\0') ? "4.0" : version;
            if (strverscmp (compared_version, SED_FEATURE_VERSION) > 0)
              bad_prog(_(ANCIENT_VERSION));

            free (version);
	    POSIXLY_CORRECT = 0;
          }
	  continue;

	case '{':
	  blocks = setup_label(blocks, vector->v_length, NULL, &cur_input);
	  cur_cmd->addr_bang = !cur_cmd->addr_bang;
	  break;

	case '}':
	  if (!blocks)
	    bad_prog(_(EXCESS_CLOSE_BRACE));
	  if (cur_cmd->a1)
	    bad_prog(_(NO_CLOSE_BRACE_ADDR));
	  ch = in_nonblank();
	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));

	  vector->v[blocks->v_index].x.jump_index = vector->v_length;
	  blocks = release_label(blocks);	/* done with this entry */
	  break;

	case 'e':
	  ch = in_nonblank();
	  if (ch == EOF || ch == '\n')
	    {
	      cur_cmd->x.cmd_txt.text_length = 0;
	      break;
	    }
	  else
	    goto read_text_to_slash;

	case 'a':
	case 'i':
	  if (POSIXLY_CORRECT && cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));
	  /* Fall Through */

	case 'c':
	  ch = in_nonblank();

	read_text_to_slash:
	  if (ch == EOF)
	    bad_prog(_(EXPECTED_SLASH));
	      
	  if (ch == '\\')
	    ch = inchar();
	  else
	    {
	      savchar(ch);
	      ch = '\n';
	    }

	  read_text(&cur_cmd->x.cmd_txt, ch);
	  break;

	case ':':
	  if (cur_cmd->a1)
	    bad_prog(_(NO_COLON_ADDR));
	  labels = setup_label(labels, vector->v_length, read_label(), NULL);
	  break;
	
	case 'b':
	case 't':
	case 'T':
	  jumps = setup_label(jumps, vector->v_length, read_label(), NULL);
	  break;

	case 'q':
	case 'Q':
	  if (cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));
	  /* Fall through */

	case 'l':
	case 'L':
	  if (POSIXLY_CORRECT && cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));

	  ch = in_nonblank();
	  if (ISDIGIT(ch)) 
	    {
	      cur_cmd->x.int_arg = in_integer(ch);
	      ch = in_nonblank();
	    }
	  else
	    cur_cmd->x.int_arg = -1;

	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));

	  break;

	case '=':
	  if (POSIXLY_CORRECT && cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));
	  /* Fall Through */
	case 'd':
	case 'D':
	case 'g':
	case 'G':
	case 'h':
	case 'H':
	case 'n':
	case 'N':
	case 'p':
	case 'P':
	case 'x':
	  ch = in_nonblank();
	  if (ch == CLOSE_BRACE || ch == '#')
	    savchar(ch);
	  else if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(_(EXCESS_JUNK));
	  break;

	case 'r':
	  if (POSIXLY_CORRECT && cur_cmd->a2)
	    bad_prog(_(ONE_ADDR));
	  b = read_filename();
	  cur_cmd->x.fname = ck_strdup(get_buffer(b));
	  free_buffer(b);
	  break;

        case 'R':
	  cur_cmd->x.fp = get_openfile(&file_read, "r", FALSE);
	  break;

	case 'w':
        case 'W':
	  cur_cmd->x.fp = get_openfile(&file_write, "w", TRUE);
	  break;

	case 's':
	  {
	    struct buffer *b2;
	    int flags;
	    int slash;

	    slash = inchar();
	    if ( !(b  = match_slash(slash, TRUE, TRUE)) )
	      bad_prog(_(UNTERM_S_CMD));
	    if ( !(b2 = match_slash(slash, FALSE, TRUE)) )
	      bad_prog(_(UNTERM_S_CMD));

	    cur_cmd->x.cmd_subst = OB_MALLOC(&obs, 1, struct subst);
	    setup_replacement(cur_cmd->x.cmd_subst,
			      get_buffer(b2), size_buffer(b2));
	    free_buffer(b2);

	    flags = mark_subst_opts(cur_cmd->x.cmd_subst);
	    cur_cmd->x.cmd_subst->regx =
	      compile_regex(b, flags, cur_cmd->x.cmd_subst->max_id);
	    free_buffer(b);
	  }
	  break;

	case 'y':
	  {
	    unsigned char *ustring;
	    size_t len;
	    int slash;

#if defined HAVE_MBRTOWC && !defined (REG_PERL)
	    if (MB_CUR_MAX == 1)
#endif
	      {
	        ustring = OB_MALLOC(&obs, YMAP_LENGTH, unsigned char);
	        for (len = 0; len < YMAP_LENGTH; len++)
	          ustring[len] = len;
	        cur_cmd->x.translate = ustring;
	      }

	    slash = inchar();
	    if ( !(b = match_slash(slash, FALSE, FALSE)) )
	      bad_prog(_(UNTERM_Y_CMD));

#if defined HAVE_MBRTOWC && !defined (REG_PERL)
            if (MB_CUR_MAX > 1)
	      {
                int i, j, idx, src_char_num, len = size_buffer(b);
                size_t *src_lens = MALLOC(len, size_t);
                char *src_buf, *dest_buf, **trans_pairs;
                size_t mbclen;
                mbstate_t cur_stat;
	        struct buffer *b2;

                /* Enumerate how many character the source buffer has.  */
                memset(&cur_stat, 0, sizeof(mbstate_t));
                src_buf = get_buffer(b);
                for (i = 0, j = 0; i < len;)
                  {
                    mbclen = mbrlen(src_buf + i, len - i, &cur_stat);
                    /* An invalid sequence, or a truncated multibyte character.
                       We treat it as a singlebyte character.  */
                    if (mbclen == (size_t) -1 || mbclen == (size_t) -2
                        || mbclen == 0)
                      mbclen = 1;
                    src_lens[j++] = mbclen;
                    i += mbclen;
                  }
                src_char_num = j;

                memset(&cur_stat, 0, sizeof(mbstate_t));
                if ( !(b2 = match_slash(slash, FALSE, FALSE)) )
 		  bad_prog(_(UNTERM_Y_CMD));
                dest_buf = get_buffer(b2);
                idx = 0;
                len = size_buffer(b2);

                /* trans_pairs = {src(0), dest(0), src(1), dest(1), ..., NULL}
                     src(i) : pointer to i-th source character.
                     dest(i) : pointer to i-th destination character.
                     NULL : terminator */
                trans_pairs = MALLOC(2 * src_char_num + 1, char*);
                cur_cmd->x.translatemb = trans_pairs;
                for (i = 0; i < src_char_num; i++)
                  {
                    if (idx >= len)
                      bad_prog(_(Y_CMD_LEN));

                    /* Set the i-th source character.  */
                    trans_pairs[2 * i] = MALLOC(src_lens[i] + 1, char);
                    strncpy(trans_pairs[2 * i], src_buf, src_lens[i]);
                    trans_pairs[2 * i][src_lens[i]] = '\0';
                    src_buf += src_lens[i]; /* Forward to next character.  */

                    /* Fetch the i-th destination character.  */
                    mbclen = mbrlen(dest_buf + idx, len - idx, &cur_stat);
                    /* An invalid sequence, or a truncated multibyte character.
                       We treat it as a singlebyte character.  */
                    if (mbclen == (size_t) -1 || mbclen == (size_t) -2
                        || mbclen == 0)
                      mbclen = 1;

                    /* Set the i-th destination character.  */
                    trans_pairs[2 * i + 1] = MALLOC(mbclen + 1, char);
                    strncpy(trans_pairs[2 * i + 1], dest_buf + idx, mbclen);
                    trans_pairs[2 * i + 1][mbclen] = '\0';
                    idx += mbclen; /* Forward to next character.  */
                  }
                trans_pairs[2 * i] = NULL;
                if (idx != len)
                  bad_prog(_(Y_CMD_LEN));
                free_buffer(b);
                free_buffer(b2);
              }
            else
#endif
              {
                ustring = CAST(unsigned char *)get_buffer(b);
                for (len = size_buffer(b); len; --len)
                  {
                    ch = inchar();
                    if (ch == slash)
                      bad_prog(_(Y_CMD_LEN));
                    if (ch == '\n')
                      bad_prog(UNTERM_Y_CMD);
                    if (ch == '\\')
                      ch = inchar();
                    if (ch == EOF)
                      bad_prog(UNTERM_Y_CMD);
                    cur_cmd->x.translate[*ustring++] = ch;
                  }
                free_buffer(b);

                if (inchar() != slash)
		  bad_prog(_(Y_CMD_LEN));
                else if ((ch = in_nonblank()) != EOF && ch != '\n' && ch != ';')
                  bad_prog(_(EXCESS_JUNK));
	      }
	  }
	break;

	case EOF:
	  bad_prog(_(NO_COMMAND));
	  /*NOTREACHED*/
	default:
	  {
	    const char *msg = _("Unknown command:");
	    char *unknown_cmd = xmalloc(strlen(msg) + 5);
	    sprintf(unknown_cmd, "%s `%c'", msg, ch);
	    bad_prog(unknown_cmd);
	    /*NOTREACHED*/
	  }
	}

      /* this is buried down here so that "continue" statements will miss it */
      ++vector->v_length;
    }
  return vector;
}


/* Complain about a programming error and exit. */
void
bad_prog(why)
  const char *why;
{
  if (cur_input.name)
    fprintf(stderr, _("%s: file %s line %lu: %s\n"),
	    myname, cur_input.name, CAST(unsigned long)cur_input.line, why);
  else
    fprintf(stderr, _("%s: -e expression #%lu, char %lu: %s\n"),
	    myname,
	    CAST(unsigned long)cur_input.string_expr_count,
	    CAST(unsigned long)(prog.cur-prog.base),
	    why);
  exit(EXIT_FAILURE);
}


/* deal with \X escapes */
size_t
normalize_text(buf, len)
  char *buf;
  size_t len;
{
  const char *bufend = buf + len;
  char *p = buf;
  char *q = buf;

  /* I'm not certain whether POSIX.2 allows these escapes.
     Play it safe for now... */
  if (POSIXLY_CORRECT && !(extended_regexp_flags))
    return len;

  while (p < bufend)
    {
      int c;

      *q = *p++;
      if (*q == '\\' && p < bufend)
	switch ( (c = *p++) )
	  {
#if defined __STDC__ && __STDC__-0
	  case 'a': *q = '\a'; break;
#else /* Not STDC; we'll just assume ASCII */
	  case 'a': *q = '\007'; break;
#endif
	  /* case 'b': *q = '\b'; break; --- conflicts with \b RE */
	  case 'f': *q = '\f'; break;
	  case '\n': /*fall through */
	  case 'n': *q = '\n'; break;
	  case 'r': *q = '\r'; break;
	  case 't': *q = '\t'; break;
	  case 'v': *q = '\v'; break;

	  case 'd': /* decimal byte */
	    p = convert_number(q, p, bufend, 10, 3, 'd');
	    break;

	  case 'x': /* hexadecimal byte */
	    p = convert_number(q, p, bufend, 16, 2, 'x');
	    break;

#ifdef REG_PERL
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	    if ((extended_regexp_flags & REG_PERL) &&
		p < bufend && isdigit(*p))
	      {
		p--;
		p = convert_number(q, p, bufend, 8, 3, *p);
	      }
	    else
	      /* we just pass the \ up one level for interpretation */
	      *++q = p[-1];

	    break;

	  case 'o': /* octal byte */
	    if (!(extended_regexp_flags & REG_PERL))
	      p = convert_number(q, p, bufend,  8, 3, 'o');
	    else
	      /* we just pass the \ up one level for interpretation */
	      *++q = p[-1];
	    
	    break;

#else
	  case 'o': /* octal byte */
	    p = convert_number(q, p, bufend,  8, 3, 'o');
	    break;
#endif

	  case 'c':
	    if (p < bufend)
	      {
		*q = toupper(*p) ^ 0x40;
		p++;
		break;
	      }
	    /* else FALLTHROUGH */

	  default:
	    /* we just pass the \ up one level for interpretation */
	    *++q = p[-1];
	    break;
	  }
      ++q;
    }
    return (size_t)(q - buf);
}


/* `str' is a string (from the command line) that contains a sed command.
   Compile the command, and add it to the end of `cur_program'. */
struct vector *
compile_string(cur_program, str, len)
  struct vector *cur_program;
  char *str;
  size_t len;
{
  static countT string_expr_count = 0;
  struct vector *ret;

  prog.file = NULL;
  prog.base = CAST(unsigned char *)str;
  prog.cur = prog.base;
  prog.end = prog.cur + len;

  cur_input.line = 0;
  cur_input.name = NULL;
  cur_input.string_expr_count = ++string_expr_count;

  ret = compile_program(cur_program);
  prog.base = NULL;
  prog.cur = NULL;
  prog.end = NULL;

  first_script = FALSE;
  return ret;
}

/* `cmdfile' is the name of a file containing sed commands.
   Read them in and add them to the end of `cur_program'.
 */
struct vector *
compile_file(cur_program, cmdfile)
  struct vector *cur_program;
  const char *cmdfile;
{
  size_t len;
  struct vector *ret;

  prog.file = stdin;
  if (cmdfile[0] != '-' || cmdfile[1] != '\0')
    prog.file = ck_fopen(cmdfile, "rt", TRUE);

  cur_input.line = 1;
  cur_input.name = cmdfile;
  cur_input.string_expr_count = 0;

  ret = compile_program(cur_program);
  if (prog.file != stdin)
    ck_fclose(prog.file);
  prog.file = NULL;

  first_script = FALSE;
  return ret;
}

/* Make any checks which require the whole program to have been read.
   In particular: this backpatches the jump targets.
   Any cleanup which can be done after these checks is done here also.  */
void
check_final_program(program)
  struct vector *program;
{
  struct sed_label *go;
  struct sed_label *lbl;

  /* do all "{"s have a corresponding "}"? */
  if (blocks)
    {
      /* update info for error reporting: */
      MEMCPY(&cur_input, &blocks->err_info, sizeof cur_input);
      bad_prog(_(EXCESS_OPEN_BRACE));
    }

  /* was the final command an unterminated a/c/i command? */
  if (pending_text)
    {
      old_text_buf->text_length = size_buffer(pending_text);
      old_text_buf->text = MEMDUP(get_buffer(pending_text),
				  old_text_buf->text_length, char);
      free_buffer(pending_text);
      pending_text = NULL;
    }

  for (go = jumps; go; go = release_label(go))
    {
      for (lbl = labels; lbl; lbl = lbl->next)
	if (strcmp(lbl->name, go->name) == 0)
	  break;
      if (lbl)
	{
	  program->v[go->v_index].x.jump_index = lbl->v_index;
	}
      else
	{
	  if (*go->name)
	    panic(_("Can't find label for jump to `%s'"), go->name);
	  program->v[go->v_index].x.jump_index = program->v_length;
	}
    }
  jumps = NULL;

  for (lbl = labels; lbl; lbl = release_label(lbl))
    ;
  labels = NULL;

  /* There is no longer a need to track file names: */
  {
    struct fp_list *p;

    for (p=file_read; p; p=p->link)
      if (p->name)
	{
	  FREE(p->name);
	  p->name = NULL;
	}

    for (p=file_write; p; p=p->link)
      if (p->name)
	{
	  FREE(p->name);
	  p->name = NULL;
	}
  }
}

/* Release all resources which were allocated in this module. */
void
rewind_read_files()
{
  struct fp_list *p, *q;

  for (p=file_read; p; p=p->link)
    if (p->fp && !p->special)
      rewind(p->fp);
}

/* Release all resources which were allocated in this module. */
void
finish_program(program)
  struct vector *program;
{
  /* close all files... */
  {
    struct fp_list *p, *q;

    for (p=file_read; p; p=q)
      {
	if (p->fp && !p->special)
	  ck_fclose(p->fp);
	q = p->link;
#if 0
	/* We use obstacks. */
	FREE(p);
#endif
      }

    for (p=file_write; p; p=q)
      {
	if (p->fp && !p->special)
	  ck_fclose(p->fp);
	q = p->link;
#if 0
	/* We use obstacks. */
	FREE(p);
#endif
      }
    file_read = file_write = NULL;
  }

#ifdef DEBUG_LEAKS
  obstack_free (&obs, NULL);
#endif /*DEBUG_LEAKS*/
}
