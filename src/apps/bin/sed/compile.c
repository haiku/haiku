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

/* compile.c: translate sed source into internal form */

#include "config.h"
#include <stdio.h>

#ifndef __STRICT_ANSI__
# define _GNU_SOURCE
#endif
#include <ctype.h>

#ifndef HAVE_STRING_H
# include <strings.h>
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#else
# include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "regex-sed.h"
#include "basicdefs.h"
#include "utils.h"
#include "sed.h"


extern flagT rx_testing;
extern flagT no_default_output;
extern flagT use_extended_syntax_p;


#define YMAP_LENGTH		256 /*XXX shouldn't this be (UCHAR_MAX+1)?*/
#define VECTOR_ALLOC_INCREMENT	40

struct prog_info {
  /* When we're reading a script command from a string, 'prog.base'
     points to the first character in the string, 'prog.cur' points
     to the current character in the string, and 'prog.end' points
     to the end of the string.  This allows us to compile script
     strings that contain nulls, which might happen if we're mmap'ing
     a file. */
  VOID *base;
  const unsigned char *cur;
  const unsigned char *end;

  /* This is the current script file.  If it is NULL, we are reading
     from a string stored at 'prog.cur' instead.  If both 'prog.file'
     and 'prog.cur' are NULL, we're in trouble! */
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


static struct vector *compile_program P_((struct vector *));
static struct vector *new_vector
	P_((struct error_info *errinfo, struct vector *old_vector));
static void read_text P_((struct text_buf *buf, int leadin_ch));
static struct sed_cmd *next_cmd_entry P_((struct vector **vectorp));
static int add_then_next P_((struct buffer *b, int ch));
static int snarf_char_class P_((struct buffer *b));
static struct buffer *match_slash P_((int slash, flagT regex, flagT keep_back));
static regex_t *compile_regex P_((struct buffer *b, flagT icase, flagT nosub));
static flagT compile_address P_((struct addr *addr, int ch));
static void compile_filename P_((flagT readit, const char **name, FILE **fp));
static struct sed_label *setup_jump
	P_((struct sed_label *list, struct sed_cmd *cmd, struct vector *vec));
static flagT mark_subst_opts P_((struct subst *cmd));
static int inchar P_((void));
static int in_nonblank P_((void));
static countT in_integer P_((int ch));
static void savchar P_((int ch));
static void bad_prog P_((const char *why));


/* Where we are in the processing of the input. */
static struct prog_info prog;
static struct error_info cur_input;

/* Information about labels and jumps-to-labels.  This is used to do
   the required backpatching after we have compiled all the scripts. */
static struct sed_label *jumps = NULL;
static struct sed_label *labels = NULL;

/* We wish to detect #n magic only in the first input argument;
   this flag tracks when we have consumed the first file of input. */
static flagT first_script = 1;

/* Allow for scripts like "sed -e 'i\' -e foo": */
static struct buffer *pending_text = NULL;
static struct text_buf *old_text_buf = NULL;

/* Various error messages we may want to print */
static const char ONE_ADDR[] = "Command only uses one address";
static const char NO_ADDR[] = "Command doesn't take any addresses";
static const char LINE_JUNK[] = "Extra characters after command";
static const char BAD_EOF[] = "Unexpected End-of-file";
static const char BAD_REGEX_FMT[] = "bad regexp: %s\n";
static const char EXCESS_OPEN_BRACKET[] = "Unmatched `{'";
static const char EXCESS_CLOSE_BRACKET[] = "Unexpected `}'";
static const char NO_REGEX[] = "No previous regular expression";
static const char NO_COMMAND[] = "Missing command";
static const char UNTERM_S_CMD[] = "Unterminated `s' command";
static const char UNTERM_Y_CMD[] = "Unterminated `y' command";


/* This structure tracks files opened by the 'r', 'w', and 's///w' commands
   so that they may all be closed cleanly at normal program termination.  */
struct fp_list {
    char *name;
    FILE *fp;
    flagT readit_p;
    struct fp_list *link;
  };
static struct fp_list *file_ptrs = NULL;

void
close_all_files()
{
  struct fp_list *p, *q;

  for (p=file_ptrs; p; p=q)
    {
      if (p->fp)
	ck_fclose(p->fp);
      FREE(p->name);
      q = p->link;
      FREE(p);
    }
  file_ptrs = NULL;
}

/* 'str' is a string (from the command line) that contains a sed command.
   Compile the command, and add it to the end of 'cur_program'. */
struct vector *
compile_string(cur_program, str)
  struct vector *cur_program;
  char *str;
{
  static countT string_expr_count = 0;
  struct vector *ret;

  prog.file = NULL;
  prog.base = VCAST(VOID *)str;
  prog.cur = CAST(unsigned char *)str;
  prog.end = prog.cur + strlen(str);

  cur_input.line = 0;
  cur_input.name = NULL;
  cur_input.string_expr_count = ++string_expr_count;

  ret = compile_program(cur_program);
  prog.base = NULL;
  prog.cur = NULL;
  prog.end = NULL;

  first_script = 0;
  return ret;
}

/* 'cmdfile' is the name of a file containing sed commands.
   Read them in and add them to the end of 'cur_program'.
 */
struct vector *
compile_file(cur_program, cmdfile)
  struct vector *cur_program;
  const char *cmdfile;
{
  size_t len;
  struct vector *ret;

  prog.base = NULL;
  prog.cur = NULL;
  prog.end = NULL;
  prog.file = stdin;
  if (cmdfile[0] != '-' || cmdfile[1] != '\0')
    prog.file = ck_fopen(cmdfile, "r");
  if (map_file(prog.file, &prog.base, &len))
    {
      prog.cur = VCAST(const unsigned char *)prog.base;
      prog.end = prog.cur + len;
    }

  cur_input.line = 1;
  cur_input.name = cmdfile;
  cur_input.string_expr_count = 0;

  ret = compile_program(cur_program);
  if (prog.base)
    {
      unmap_file(prog.base, len);
      prog.base = NULL;
      prog.cur = NULL;
      prog.end = NULL;
    }
  if (prog.file != stdin)
    ck_fclose(prog.file);
  prog.file = NULL;

  first_script = 0;
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
  struct sed_label *nxt;

  /* do all "{"s have a corresponding "}"? */
  if (program->return_v)
    {
      cur_input = *program->return_v->err_info; /* for error reporting */
      bad_prog(EXCESS_OPEN_BRACKET);
    }
  FREE(program->err_info);	/* no longer need this */
  program->err_info = NULL;

  /* was the final command an unterminated a/c/i command? */
  if (pending_text)
    {
      old_text_buf->text_len = size_buffer(pending_text);
      old_text_buf->text = MEMDUP(get_buffer(pending_text),
				  old_text_buf->text_len, char);
      free_buffer(pending_text);
      pending_text = NULL;
    }

  for (go = jumps; go; go = nxt)
    {
      for (lbl = labels; lbl; lbl = lbl->next)
	if (strcmp(lbl->name, go->name) == 0)
	  break;
      if (*go->name && !lbl)
	panic("Can't find label for jump to '%s'", go->name);
      go->v->v[go->v_index].x.jump = lbl;
      nxt = go->next;
      FREE(go->name);
      FREE(go);
    }
  jumps = NULL;

  for (lbl = labels; lbl; lbl = lbl->next)
    {
      FREE(lbl->name);
      lbl->name = NULL;
    }
}

static struct vector *
new_vector(errinfo, old_vector)
  struct error_info *errinfo;
  struct vector *old_vector;
{
  struct vector *vector = MALLOC(1, struct vector);
  vector->v = NULL;
  vector->v_allocated = 0;
  vector->v_length = 0;
  vector->err_info = MEMDUP(errinfo, 1, struct error_info);
  vector->return_v = old_vector;
  vector->return_i = old_vector ? old_vector->v_length : 0;
  return vector;
}

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
      buf->text_len = 0;
      old_text_buf = buf;
    }
  /* assert(old_text_buf != NULL); */

  if (leadin_ch == EOF)
    return;
  while ((ch = inchar()) != EOF && ch != '\n')
    {
      if (ch == '\\')
	{
	  if ((ch = inchar()) == EOF)
	    {
	      add1_buffer(pending_text, '\n');
	      return;
	    }
	}
      add1_buffer(pending_text, ch);
    }
  add1_buffer(pending_text, '\n');

  if (!buf)
    buf = old_text_buf;
  buf->text_len = size_buffer(pending_text);
  buf->text = MEMDUP(get_buffer(pending_text), buf->text_len, char);
  free_buffer(pending_text);
  pending_text = NULL;
}


/* Read a program (or a subprogram within '{' '}' pairs) in and store
   the compiled form in *'vector'.  Return a pointer to the new vector.  */
static struct vector *
compile_program(vector)
  struct vector *vector;
{
  struct sed_cmd *cur_cmd;
  struct buffer *b;
  struct buffer *b2;
  unsigned char *ustring;
  size_t len;
  int slash;
  int ch;

  if (!vector)
    vector = new_vector(&cur_input, NULL);
  if (pending_text)
    read_text(NULL, '\n');

  for (;;)
    {
      while ((ch=inchar()) == ';' || ISSPACE(ch))
	;
      if (ch == EOF)
	break;

      cur_cmd = next_cmd_entry(&vector);
      if (compile_address(&cur_cmd->a1, ch))
	{
	  ch = in_nonblank();
	  if (ch == ',')
	    {
	      if (!compile_address(&cur_cmd->a2, in_nonblank()))
		bad_prog("Unexpected ','");
	      ch = in_nonblank();
	    }
	}
      if (cur_cmd->a2.addr_type == addr_is_num
	  && cur_cmd->a1.addr_type == addr_is_num
	  && cur_cmd->a2.a.addr_number < cur_cmd->a1.a.addr_number)
	cur_cmd->a2.a.addr_number = cur_cmd->a1.a.addr_number;
      if (ch == '!')
	{
	  cur_cmd->addr_bang = 1;
	  ch = in_nonblank();
	  if (ch == '!')
	    bad_prog("Multiple '!'s");
	}

      cur_cmd->cmd = ch;
      switch (ch)
	{
	case '#':
	  if (cur_cmd->a1.addr_type != addr_is_null)
	    bad_prog(NO_ADDR);
	  ch = inchar();
	  if (ch=='n' && first_script && cur_input.line < 2)
	    if (   (prog.base && prog.cur==2+CAST(unsigned char *)prog.base)
	        || (prog.file && !prog.base && 2==ftell(prog.file)))
	      no_default_output = 1;
	  while (ch != EOF && ch != '\n')
	    ch = inchar();
	  continue;	/* restart the for (;;) loop */

	case '{':
	  ++vector->v_length;	/* be sure to count this insn */
	  vector = cur_cmd->x.sub = new_vector(&cur_input, vector);
	  continue;	/* restart the for (;;) loop */

	case '}':
	  /* a return insn for subprograms -t */
	  if (!vector->return_v)
	    bad_prog(EXCESS_CLOSE_BRACKET);
	  if (cur_cmd->a1.addr_type != addr_is_null)
	    bad_prog(/*{*/ "} doesn't want any addresses");
	  ch = in_nonblank();
	  if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(LINE_JUNK);
	  ++vector->v_length;		/* we haven't counted this insn yet */
	  FREE(vector->err_info);	/* no longer need this */
	  vector->err_info = NULL;
	  vector = vector->return_v;
	  continue;	/* restart the for (;;) loop */

	case 'a':
	case 'i':
	  if (cur_cmd->a2.addr_type != addr_is_null)
	    bad_prog(ONE_ADDR);
	  /* Fall Through */
	case 'c':
	  ch = in_nonblank();
	  if (ch != '\\')
	    bad_prog(LINE_JUNK);
	  ch = inchar();
	  if (ch != '\n' && ch != EOF)
	    bad_prog(LINE_JUNK);
	  read_text(&cur_cmd->x.cmd_txt, ch);
	  break;

	case ':':
	  if (cur_cmd->a1.addr_type != addr_is_null)
	    bad_prog(": doesn't want any addresses");
	  labels = setup_jump(labels, cur_cmd, vector);
	  break;
	case 'b':
	case 't':
	  jumps = setup_jump(jumps, cur_cmd, vector);
	  break;

	case 'q':
	case '=':
	  if (cur_cmd->a2.addr_type != addr_is_null)
	    bad_prog(ONE_ADDR);
	  /* Fall Through */
	case 'd':
	case 'D':
	case 'g':
	case 'G':
	case 'h':
	case 'H':
	case 'l':
	case 'n':
	case 'N':
	case 'p':
	case 'P':
	case 'x':
	  ch = in_nonblank();
	  if (ch != EOF && ch != '\n' && ch != ';')
	    bad_prog(LINE_JUNK);
	  break;

	case 'r':
	  if (cur_cmd->a2.addr_type != addr_is_null)
	    bad_prog(ONE_ADDR);
	  compile_filename(1, &cur_cmd->x.rfile, NULL);
	  break;
	case 'w':
	  compile_filename(0, NULL, &cur_cmd->x.wfile);
	  break;

	case 's':
	  slash = inchar();
	  if ( !(b  = match_slash(slash, 1, 1)) )
	    bad_prog(UNTERM_S_CMD);
	  if ( !(b2 = match_slash(slash, 0, 1)) )
	    bad_prog(UNTERM_S_CMD);

	  cur_cmd->x.cmd_regex.replace_length = size_buffer(b2);
	  cur_cmd->x.cmd_regex.replacement = MEMDUP(get_buffer(b2),
				    cur_cmd->x.cmd_regex.replace_length, char);
	  free_buffer(b2);

	  {
	    flagT caseless = mark_subst_opts(&cur_cmd->x.cmd_regex);
	    cur_cmd->x.cmd_regex.regx = compile_regex(b, caseless, 0);
	  }
	  free_buffer(b);
	  break;

	case 'y':
	  ustring = MALLOC(YMAP_LENGTH, unsigned char);
	  for (len = 0; len < YMAP_LENGTH; len++)
	    ustring[len] = len;
	  cur_cmd->x.translate = ustring;

	  slash = inchar();
	  if ( !(b = match_slash(slash, 0, 0)) )
	    bad_prog(UNTERM_Y_CMD);
	  ustring = CAST(unsigned char *)get_buffer(b);
	  for (len = size_buffer(b); len; --len)
	    {
	      ch = inchar();
	      if (ch == slash)
		bad_prog("strings for y command are different lengths");
	      if (ch == '\n')
		bad_prog(UNTERM_Y_CMD);
	      if (ch == '\\')
		ch = inchar();
	      if (ch == EOF)
		bad_prog(BAD_EOF);
	      cur_cmd->x.translate[*ustring++] = ch;
	    }
	  free_buffer(b);

	  if (inchar() != slash ||
	      ((ch = in_nonblank()) != EOF && ch != '\n' && ch != ';'))
	    bad_prog(LINE_JUNK);
	  break;

	case EOF:
	  bad_prog(NO_COMMAND);
	  /*NOTREACHED*/
	default:
	  {
	    static char unknown_cmd[] = "Unknown command: ``_''";
	    unknown_cmd[sizeof(unknown_cmd)-4] = ch;
	    bad_prog(unknown_cmd);
	  }
	  /*NOTREACHED*/
	}

      /* this is buried down here so that "continue" statements will miss it */
      ++vector->v_length;
    }
  return vector;
}

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
  cmd->a1.addr_type = addr_is_null;
  cmd->a2.addr_type = addr_is_null;
  cmd->a1_matched = 0;
  cmd->addr_bang = 0;

  *vectorp  = v;
  return cmd;
}


/* let's not confuse text editors that have only dumb bracket-matching... */
#define OPEN_BRACKET	'['
#define CLOSE_BRACKET	']'

static int
add_then_next(b, ch)
  struct buffer *b;
  int ch;
{
  add1_buffer(b, ch);
  return inchar();
}

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
		return EOF;
	      prev = ch;
	    }
	}
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
      ch = add_then_next(b, ch);
    }

  /* ensure that a newline here is an error, not a "slash" match: */
  return ch == '\n' ? EOF : ch;
}

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
	  else if (ch == 'n' && regex)
	    ch = '\n';
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
  if (regex && rx_testing)
    {
      /* This is slightly bogus.  Sed is noticing an ill-formed regexp,
       * but rx is getting the credit.  Fortunately, this only affects
       * a few spencer tests.
       */
      size_t sz = size_buffer(b);
      char *re_txt;
      add1_buffer(b, '\0');
      re_txt = get_buffer(b);
      if (sz > 0 && re_txt[sz] == '\n')
	re_txt[sz] = '\0';
      printf(BAD_REGEX_FMT, re_txt);
      exit(1);
    }
  free_buffer(b);
  return NULL;
}

static regex_t *
compile_regex(b, ignore_case, nosub)
  struct buffer *b;
  flagT ignore_case;
  flagT nosub;
{
  /* ``An empty regular expression is equivalent to the last regular
     expression read'' so we have to keep track of the last regexp read.
     We keep track the _source_ form of the regular expression because
     it is possible for different modifiers to be specified. */
  static char *last_re = NULL;
  regex_t *new_regex = MALLOC(1, regex_t);
  size_t sz;
  int cflags;
  int error;

  sz = size_buffer(b);
  if (sz > 0)
    {
      add1_buffer(b, '\0');
      FREE(last_re);
      last_re = ck_strdup(get_buffer(b));
    }
  else if (!last_re)
    bad_prog(NO_REGEX);

  cflags = 0;
  if (ignore_case)
    cflags |= REG_ICASE;
  if (nosub)
    cflags |= REG_NOSUB;
  if (use_extended_syntax_p)
    cflags |= REG_EXTENDED;
  if ( (error = regcomp(new_regex, last_re, cflags)) )
    {
      char msg[1000];
      if (rx_testing)
	{
	  printf(BAD_REGEX_FMT, last_re);
	  exit(1);
	}
      regerror(error, NULL, msg, sizeof msg);
      bad_prog(msg);
  }

  return new_regex;
}

/* Try to read an address for a sed command.  If it succeeds,
   return non-zero and store the resulting address in *'addr'.
   If the input doesn't look like an address read nothing
   and return zero.  */
static flagT
compile_address(addr, ch)
  struct addr *addr;
  int ch;
{
  if (ISDIGIT(ch))
    {
      countT num = in_integer(ch);
      ch = in_nonblank();
#if 1
      if (ch != '~')
	{
	  savchar(ch);
	}
      else
	{
	  countT num2 = in_integer(in_nonblank());
	  if (num2 > 0)
	    {
	      addr->addr_type = addr_is_mod;
	      addr->a.m.offset = num;
	      addr->a.m.modulo = num2;
	      return 1;
	    }
	}
      addr->addr_type = addr_is_num;
      addr->a.addr_number = num;
#else
      if (ch == '~')
	{
	  countT num2 = in_integer(in_nonblank());
	  if (num2 == 0)
	    bad_prog("Zero step size for ~ addressing is not valid");
	  addr->addr_type = addr_is_mod;
	  addr->a.m.offset = num;
	  addr->a.m.modulo = num2;
	}
      else
	{
	  savchar(ch);
	  addr->addr_type = addr_is_num;
	  addr->a.addr_number = num;
	}
#endif
      return 1;
    }
  else if (ch == '/' || ch == '\\')
    {
      flagT caseless = 0;
      struct buffer *b;
      addr->addr_type = addr_is_regex;
      if (ch == '\\')
	ch = inchar();
      if ( !(b = match_slash(ch, 1, 1)) )
	bad_prog("Unterminated address regex");
      ch = in_nonblank();
      if (ch == 'I')	/* lower case i would break existing programs */
	caseless = 1;
      else
	savchar(ch);
      addr->a.addr_regex = compile_regex(b, caseless, 1);
      free_buffer(b);
      return 1;
    }
  else if (ch == '$')
    {
      addr->addr_type = addr_is_last;
      return 1;
    }
  return 0;
}

/* read in a filename for a 'r', 'w', or 's///w' command, and
   update the internal structure about files.  The file is
   opened if it isn't already open in the given mode.  */
static void
compile_filename(readit, name, fp)
  flagT readit;
  const char **name;
  FILE **fp;
{
  struct buffer *b;
  int ch;
  char *file_name;
  struct fp_list *p;

  b = init_buffer();
  ch = in_nonblank();
  while (ch != EOF && ch != '\n')
    {
      add1_buffer(b, ch);
      ch = inchar();
    }
  add1_buffer(b, '\0');
  file_name = get_buffer(b);

  for (p=file_ptrs; p; p=p->link)
    if (p->readit_p == readit && strcmp(p->name, file_name) == 0)
      break;

  if (!p)
    {
      p = MALLOC(1, struct fp_list);
      p->name = ck_strdup(file_name);
      p->readit_p = readit;
      p->fp = NULL;
      if (!readit)
	p->fp = ck_fopen(p->name, "w");
      p->link = file_ptrs;
      file_ptrs = p;
    }

  free_buffer(b);
  if (name)
    *name = p->name;
  if (fp)
    *fp = p->fp;
}

/* Store a label (or label reference) created by a ':', 'b', or 't'
   command so that the jump to/from the label can be backpatched after
   compilation is complete.  */
static struct sed_label *
setup_jump(list, cmd, vec)
  struct sed_label *list;
  struct sed_cmd *cmd;
  struct vector *vec;
{
  struct sed_label *ret;
  struct buffer *b;
  int ch;

  b = init_buffer();
  ch = in_nonblank();

  while (ch != EOF && ch != '\n'
	 && !ISBLANK(ch) && ch != ';' && ch != /*{*/ '}')
    {
      add1_buffer(b, ch);
      ch = inchar();
    }
  savchar(ch);
  add1_buffer(b, '\0');
  ret = MALLOC(1, struct sed_label);
  ret->name = ck_strdup(get_buffer(b));
  ret->v = vec;
  ret->v_index = cmd - vec->v;
  ret->next = list;
  free_buffer(b);
  return ret;
}

static flagT
mark_subst_opts(cmd)
  struct subst *cmd;
{
  flagT caseless = 0;
  int ch;

  cmd->global = 0;
  cmd->print = 0;
  cmd->numb = 0;
  cmd->wfile = NULL;

  for (;;)
    switch ( (ch = in_nonblank()) )
      {
      case 'i':	/* GNU extension */
      case 'I':	/* GNU extension */
	caseless = 1;
	break;

      case 'p':
	cmd->print = 1;
	break;

      case 'g':
	cmd->global = 1;
	break;

      case 'w':
	compile_filename(0, NULL, &cmd->wfile);
	return caseless;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	if (cmd->numb)
	  bad_prog("multiple number options to 's' command");
	cmd->numb = in_integer(ch);
	if (!cmd->numb)
	  bad_prog("number option to 's' command may not be zero");
	break;

      case EOF:
      case '\n':
      case ';':
	return caseless;

      default:
	bad_prog("Unknown option to 's'");
	/*NOTREACHED*/
      }
}


/* Read the next character from the program.  Return EOF if there isn't
   anything to read.  Keep cur_input.line up to date, so error messages
   can be meaningful. */
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

/* Read the next non-blank character from the program.  */
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

/* unget 'ch' so the next call to inchar will return it.   */
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
      if (prog.cur <= CAST(const unsigned char *)prog.base
	  || *--prog.cur != ch)
	panic("Called savchar() with unexpected pushback (%x)",
	      CAST(unsigned char)ch);
    }
  else
    ungetc(ch, prog.file);
}

/* Complain about a programming error and exit. */
static void
bad_prog(why)
  const char *why;
{
  if (cur_input.name)
    fprintf(stderr, "%s: file %s line %lu: %s\n",
	    myname, cur_input.name, CAST(unsigned long)cur_input.line, why);
  else
    fprintf(stderr, "%s: -e expression #%lu, char %lu: %s\n",
	    myname,
	    CAST(unsigned long)cur_input.string_expr_count,
	    CAST(unsigned long)(prog.cur-CAST(unsigned char *)prog.base),
	    why);
  exit(1);
}
