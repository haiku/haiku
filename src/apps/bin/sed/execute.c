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

#undef EXPERIMENTAL_DASH_N_OPTIMIZATION	/*don't use -- is buggy*/
#define INITIAL_BUFFER_SIZE	50
#define LCMD_OUT_LINE_LEN	70
#define FREAD_BUFFER_SIZE	8192

#include "config.h"
#include <stdio.h>
#include <ctype.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef HAVE_ISATTY
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#endif

#ifdef __GNUC__
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__-0 >= 7)
  /* silence warning about unused parameter even for "gcc -W -Wunused" */
#  define UNUSED	__attribute__((unused))
# endif
#endif
#ifndef UNUSED
# define UNUSED
#endif

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

#ifdef BOOTSTRAP
# ifdef memchr
#  undef memchr
# endif
# define memchr bootstrap_memchr
static VOID *bootstrap_memchr P_((const VOID *s, int c, size_t n));
#endif /*BOOTSTRAP*/

#ifndef HAVE_REGNEXEC
# define regnexec(x,t,l,r,n,f)	regexec(x,t,r,n,f)
#endif

/* If set, don't write out the line unless explicitly told to */
extern flagT no_default_output;

/* Do we need to be pedantically POSIX compliant? */
extern flagT POSIXLY_CORRECT;


/* Sed operates a line at a time. */
struct line {
  char *text;		/* Pointer to line allocated by malloc. */
  char *active;		/* Pointer to non-consumed part of text. */
  size_t length;	/* Length of text (or active, if used). */
  size_t alloc;		/* Allocated space for text. */
  flagT chomped;	/* Was a trailing newline dropped? */
};

/* A queue of text to write out at the end of a cycle
   (filled by the "a" and "r" commands.) */
struct append_queue {
  const char *rfile;
  const char *text;
  size_t textlen;
  struct append_queue *next;
};

/* State information for the input stream. */
struct input {
  char **file_list;	/* The list of yet-to-be-opened files.
			   It is invalid for file_list to be NULL.
			   When *file_list is NULL we are
			   currently processing the last file. */
  countT bad_count;	/* count of files we failed to open */
  countT line_number;	/* Current input line number (over all files) */

  flagT (*read_fn) P_((struct input *));	/* read one line */
  /* If fp is NULL, read_fn better not be one which uses fp;
     in particular, read_always_fail() is recommended. */

  FILE *fp;		/* if NULL, none of the following are valid */
  VOID *base;		/* if non-NULL, we are using mmap()ed input */
  const char *cur;	/* only valid if base is non-NULL */
  size_t length;	/* only valid if base is non-NULL */
  size_t left;		/* only valid if base is non-NULL */
#ifdef HAVE_ISATTY
  flagT is_tty;		/* only valid if base is NULL */
#endif
};

static void resize_line P_((struct line *lb, size_t len));
static void open_next_file P_((const char *name, struct input *input));
static void closedown P_((struct input *input));
static flagT read_always_fail P_((struct input *input));
static flagT read_mem_line P_((struct input *input));
static size_t slow_getline P_((char *buf, size_t buflen, FILE *in));
static flagT read_file_line P_((struct input *input));
static flagT read_pattern_space P_((struct input *input, flagT append));
static flagT last_file_with_data_p P_((struct input *input));
static flagT test_dollar_EOF P_((struct input *input));
static flagT match_an_address_p P_((struct addr *, struct input *, flagT));
static flagT match_address_p P_((struct sed_cmd *cmd, struct input *input));
static void do_list P_((void));
static void do_subst P_((struct subst *subst));
static flagT execute_program P_((struct vector *, struct input *input));
static void output_line
		P_((const char *text, size_t length, flagT nl, FILE *fp));
static void line_init P_((struct line *buf, size_t initial_size));
static void line_copy P_((struct line *from, struct line *to));
static void str_append P_((struct line *to, const char *string, size_t length));
static void nul_append P_((struct line *dest));
static void line_append P_((struct line *from, struct line *to));
static void dump_append_queue P_((void));
static struct append_queue *next_append_slot P_((void));

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
static countT count_branches P_((struct vector *));
static struct sed_cmd *shrink_program
		P_((struct vector *vec, struct sed_cmd *cur_cmd, countT*n));

/* Used to attempt a simple-minded optimization. */
static countT branches_in_top_level;
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/


/* Have we done any replacements lately?  This is used by the 't' command. */
static flagT replaced = 0;

/* The 'current' input line. */
static struct line line;

/* An input line that's been stored by later use by the program */
static struct line hold;

/* The buffered input look-ahead. */
static struct line buffer;

static struct append_queue *append_head = NULL;
static struct append_queue *append_tail = NULL;


/* Apply the compiled script to all the named files. */
countT
process_files(the_program, argv)
  struct vector *the_program;
  char **argv;
{
  static char dash[] = "-";
  static char *stdin_argv[2] = { dash, NULL };
  struct input input;

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
  branches_in_top_level = count_branches(the_program);
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/
  input.file_list = stdin_argv;
  if (argv && *argv)
    input.file_list = argv;
  input.bad_count = 0;
  input.line_number = 0;
  input.read_fn = read_always_fail;
  input.fp = NULL;

  line_init(&line, INITIAL_BUFFER_SIZE);
  line_init(&hold, INITIAL_BUFFER_SIZE);
  line_init(&buffer, FREAD_BUFFER_SIZE);

  while (read_pattern_space(&input, 0))
    {
      flagT quit = execute_program(the_program, &input);
      if (!no_default_output)
	output_line(line.text, line.length, line.chomped, stdout);
      if (quit)
	break;
    }
  closedown(&input);
  return input.bad_count;
}

/* increase a struct line's length, making some attempt at
   keeping realloc() calls under control by padding for future growth.  */
static void
resize_line(lb, len)
  struct line *lb;
  size_t len;
{
  lb->alloc *= 2;
  if (lb->alloc - lb->length < len)
    lb->alloc = lb->length + len;
  lb->text = REALLOC(lb->text, lb->alloc, char);
}

/* Initialize a struct input for the named file. */
static void
open_next_file(name, input)
  const char *name;
  struct input *input;
{
  input->base = NULL;
  buffer.length = 0;

  if (name[0] == '-' && name[1] == '\0')
    {
      clearerr(stdin);	/* clear any stale EOF indication */
      input->fp = stdin;
    }
  else if ( ! (input->fp = fopen(name, "r")) )
    {
      const char *ptr = strerror(errno);
      fprintf(stderr, "%s: can't read %s: %s\n", myname, name, ptr);
      input->read_fn = read_always_fail; /* a redundancy */
      ++input->bad_count;
      return;
    }

  input->read_fn = read_file_line;
  if (map_file(input->fp, &input->base, &input->length))
    {
      input->cur = VCAST(char *)input->base;
      input->left = input->length;
      input->read_fn = read_mem_line;
    }
#ifdef HAVE_ISATTY
  else
    input->is_tty = isatty(fileno(input->fp));
#endif
}

/* Clean up an input stream that we are done with. */
static void
closedown(input)
  struct input *input;
{
  input->read_fn = read_always_fail;
  if (!input->fp)
    return;
  if (input->base)
    unmap_file(input->base, input->length);
  if (input->fp != stdin) /* stdin can be reused on tty and tape devices */
    ck_fclose(input->fp);
  input->fp = NULL;
}

/* dummy function to simplify read_pattern_space() */
static flagT
read_always_fail(input)
  struct input *input UNUSED;
{
  return 0;
}

/* The quick-and-easy mmap()'d case... */
static flagT
read_mem_line(input)
  struct input *input;
{
  const char *e;
  size_t l;

  if ( (e = memchr(input->cur, '\n', input->left)) )
    {
      /* common case */
      l = e++ - input->cur;
      line.chomped = 1;
    }
  else
    {
      /* This test is placed _after_ the memchr() fails for performance
	 reasons (because this branch is the uncommon case and the
	 memchr() will safely fail if input->left == 0).  Okay, so the
	 savings is trivial.  I'm doing it anyway. */
      if (input->left == 0)
	return 0;
      e = input->cur + input->left;
      l = input->left;
      if (*input->file_list || POSIXLY_CORRECT)
	line.chomped = 1;
    }

  if (line.alloc - line.length < l)
    resize_line(&line, l);
  memcpy(line.text + line.length, input->cur, l);
  line.length += l;
  input->left -= e - input->cur;
  input->cur = e;
  return 1;
}

#ifdef HAVE_ISATTY
/* fgets() doesn't let us handle NULs and fread() buffers too much
 * for interactive use.
 */
static size_t
slow_getline(buf, buflen, in)
  char *buf;
  size_t buflen;
  FILE *in;
{
  size_t resultlen = 0;
  int c;

  while (resultlen<buflen && (c=getc(in))!=EOF)
    {
      ++resultlen;
      *buf++ = c;
      if (c == '\n')
	break;
    }
  if (ferror(in))
    panic("input read error: %s", strerror(errno));
  return resultlen;
}
#endif /*HAVE_ISATTY*/

static flagT
read_file_line(input)
  struct input *input;
{
  char *b;
  size_t blen;
  size_t initial_length = line.length;

  if (!buffer.active)
    buffer.active = buffer.text;
  while (!(b = memchr(buffer.active, '\n', buffer.length)))
    {
      if (line.alloc-line.length < buffer.length)
	resize_line(&line, buffer.length);
      memcpy(line.text+line.length, buffer.active, buffer.length);
      line.length += buffer.length;
      buffer.length = 0;
      if (!feof(input->fp))
	{
#ifdef HAVE_ISATTY
	  if (input->is_tty)
	    buffer.length = slow_getline(buffer.text, buffer.alloc, input->fp);
	  else
#endif
	    buffer.length = ck_fread(buffer.text, 1, buffer.alloc, input->fp);
	}

      if (buffer.length == 0)
	{
	  if (*input->file_list || POSIXLY_CORRECT)
	    line.chomped = 1;
	  /* Did we hit EOF without reading anything?  If so, try
	     the next file; otherwise just go with what we did get. */
	  return (line.length > initial_length);
	}
      buffer.active = buffer.text;
    }

  blen = b - buffer.active;
  if (line.alloc-line.length < blen)
    resize_line(&line, blen);
  memcpy(line.text+line.length, buffer.active, blen);
  line.length += blen;
  ++blen;
  line.chomped = 1;
  buffer.active += blen;
  buffer.length -= blen;
  return 1;
}

/* Read in the next line of input, and store it in the pattern space.
   Return zero if there is nothing left to input. */
static flagT
read_pattern_space(input, append)
  struct input *input;
  flagT append;
{
  dump_append_queue();
  replaced = 0;
  line.chomped = 0;
  if (!append)
    line.length = 0;

  while ( ! (*input->read_fn)(input) )
    {
      closedown(input);
      if (!*input->file_list)
	return 0;
      open_next_file(*input->file_list++, input);
    }

  ++input->line_number;
  return 1;
}

static flagT
last_file_with_data_p(input)
  struct input *input;
{
  /*XXX reference implementation is equivalent to:
      return !*input->file_list;
   */
  for (;;)
    {
      int ch;

      closedown(input);
      if (!*input->file_list)
	return 1;
      open_next_file(*input->file_list++, input);
      if (input->fp)
	{
	  if (input->base)
	    {
	      if (0 < input->left)
		return 0;
	    }
	  else if ((ch = getc(input->fp)) != EOF)
	    {
	      ungetc(ch, input->fp);
	      return 0;
	    }
	}
    }
}

/* Determine if we match the '$' address.  */
static flagT
test_dollar_EOF(input)
  struct input *input;
{
  int ch;

  if (buffer.length)
    return 0;
  if (!input->fp)
    return last_file_with_data_p(input);
  if (input->base)
    return (input->left==0 && last_file_with_data_p(input));
  if (feof(input->fp))
    return last_file_with_data_p(input);
  if ((ch = getc(input->fp)) == EOF)
    return last_file_with_data_p(input);
  ungetc(ch, input->fp);
  return 0;
}

/* Append a NUL char after the end of DEST, without resizing DEST.
   This is to permit the use of regexp routines which expect C strings
   instead of counted strings.
 */
static void
nul_append(dest)
  struct line *dest;
{
#ifndef HAVE_REGNEXEC
  if (dest->alloc - dest->length < 1)
    resize_line(dest, 1);
  dest->text[dest->length] = '\0';
#endif
}

/* Return non-zero if the current line matches the address
   pointed to by 'addr'.  */
static flagT
match_an_address_p(addr, input, is_addr2_p)
  struct addr *addr;
  struct input *input;
  flagT is_addr2_p;
{
  switch (addr->addr_type)
    {
    case addr_is_null:
      return 1;
    case addr_is_num:
      if (is_addr2_p)
	return (input->line_number >= addr->a.addr_number);
      return (input->line_number == addr->a.addr_number);
    case addr_is_mod:
      if (addr->a.m.offset < addr->a.m.modulo)
	return (input->line_number%addr->a.m.modulo == addr->a.m.offset);
      /* offset >= modulo implies we have an extra initial skip */
      if (input->line_number < addr->a.m.offset)
	return 0;
      /* normalize */
      addr->a.m.offset %= addr->a.m.modulo;
      return 1;
    case addr_is_last:
      return test_dollar_EOF(input);
    case addr_is_regex:
      nul_append(&line);
      return !regnexec(addr->a.addr_regex,
			line.text, line.length, 0, NULL, 0);
    default:
      panic("INTERNAL ERROR: bad address type");
    }
  /*NOTREACHED*/
  return 0;
}

/* return non-zero if current address is valid for cmd */
static flagT
match_address_p(cmd, input)
  struct sed_cmd *cmd;
  struct input *input;
{
  flagT addr_matched = cmd->a1_matched;

  if (addr_matched)
    {
      if (match_an_address_p(&cmd->a2, input, 1))
	cmd->a1_matched = 0;
    }
  else if (match_an_address_p(&cmd->a1, input, 0))
    {
      addr_matched = 1;
      if (cmd->a2.addr_type != addr_is_null)
	if (   (cmd->a2.addr_type == addr_is_regex)
	    || !match_an_address_p(&cmd->a2, input, 1))
	  cmd->a1_matched = 1;
    }
  if (cmd->addr_bang)
    addr_matched = !addr_matched;
  return addr_matched;
}

static void
do_list()
{
  unsigned char *p = CAST(unsigned char *)line.text;
  countT len = line.length;
  countT width = 0;
  char obuf[180];	/* just in case we encounter a 512-bit char ;-) */
  char *o;
  size_t olen;

  for (; len--; ++p) {
      o = obuf;
      if (ISPRINT(*p)) {
	  *o++ = *p;
	  if (*p == '\\')
	    *o++ = '\\';
      } else {
	  *o++ = '\\';
	  switch (*p) {
#if defined __STDC__ && __STDC__
	    case '\a': *o++ = 'a'; break;
#else
	    /* If not STDC we'll just assume ASCII */
	    case 007:  *o++ = 'a'; break;
#endif
	    case '\b': *o++ = 'b'; break;
	    case '\f': *o++ = 'f'; break;
	    case '\n': *o++ = 'n'; break;
	    case '\r': *o++ = 'r'; break;
	    case '\t': *o++ = 't'; break;
	    case '\v': *o++ = 'v'; break;
	    default:
	      sprintf(o, "%03o", *p);
	      o += strlen(o);
	      break;
	    }
      }
      olen = o - obuf;
      if (width+olen >= LCMD_OUT_LINE_LEN) {
	  ck_fwrite("\\\n", 1, 2, stdout);
	  width = 0;
      }
      ck_fwrite(obuf, 1, olen, stdout);
      width += olen;
  }
  ck_fwrite("$\n", 1, 2, stdout);
}

static void
do_subst(sub)
  struct subst *sub;
{
  /* s_accum will accumulate the result of the s command. */
  /* static so as to keep malloc() calls down */
  static struct line s_accum;

  size_t start;		/* where to start scan for match in LINE */
  size_t offset;	/* where in LINE a match was found */
  size_t remain;	/* length after START, sans trailing newline */
  countT count;		/* number of matches found */
  char *rep;		/* the replacement string */
  char *rep_end;	/* end of the replacement string */
  flagT not_bol_p;
  flagT did_subst;
  regmatch_t regs[10];

  if (s_accum.alloc == 0)
    line_init(&s_accum, INITIAL_BUFFER_SIZE);
  s_accum.length = 0;

  count = 0;
  did_subst = 0;
  not_bol_p = 0;
  start = 0;
  remain = line.length - start;
  rep = sub->replacement;
  rep_end = rep + sub->replace_length;

  nul_append(&line);
  while ( ! regnexec(sub->regx,
		     line.text + start, remain,
		     sizeof(regs) / sizeof(regs[0]), regs, not_bol_p) )
    {
      ++count;
      offset = regs[0].rm_so + start;

      /*
       *  +- line.text   +-[offset]
       *  V              V
       * "blah blah blah xyzzy blah blah blah blah"
       *       ^             ^                    ^
       *       +-[start]     +-[end]              +-[start + remain]
       *
       *
       * regs[0].rm_so == offset - start
       * regs[0].rm_eo == end - start
       */

      /* Copy stuff to the left of the next match into the output
       * string.
       */
      if (start < offset)
	str_append(&s_accum, line.text + start, offset - start);

      /* If we're counting up to the Nth match, are we there yet? */
      if (count < sub->numb)
	{
	  /* Not there yet...so skip this match. */
	  size_t matched = regs[0].rm_eo - regs[0].rm_so;

	  /* If the match was vacuous, skip ahead one character
	   * anyway.   It isn't at all obvious to me that this is
	   * the right behavior for this case.    -t	XXX
	   */
	  if (matched == 0 && offset < line.length)
	    matched = 1;
	  str_append(&s_accum, line.text + offset, matched);
	  start = offset + matched;
	  remain = line.length - start;
	  not_bol_p = 1;
	  continue;
	}

      /* Expand the replacement string into the output string. */
      {
	char *rep_cur;	/* next burst being processed in replacement */
	char *rep_next;	/* end of burst in replacement */

	/* Replacement strings can be viewed as a sequence of variable
	 * length commands.  A command can be a literal string or a
	 * backreference (either numeric or "&").
	 *
	 * This loop measures off the next command between
	 * REP_CUR and REP_NEXT, handles that command, and loops.
	 */
	for (rep_next = rep_cur = rep;
	     rep_next < rep_end;
	     rep_next++)
	  {
	    if (*rep_next == '\\')
	      {
		/* Preceding the backslash may be some literal
		 * text:
		 */
		if (rep_cur < rep_next)
		  str_append(&s_accum, rep_cur,
			     CAST(size_t)(rep_next - rep_cur));

		/* Skip the backslash itself and look for
		 * a numeric back-reference:
		 */
		rep_next++;
		if (rep_next < rep_end
		    && '0' <= *rep_next && *rep_next <= '9')
		  {
		    int i = *rep_next - '0';
		    str_append(&s_accum, line.text + start + regs[i].rm_so,
			       CAST(size_t)(regs[i].rm_eo-regs[i].rm_so));
		  }
		else
		  str_append(&s_accum, rep_next, 1);
		rep_cur = rep_next + 1;
	      }
	    else if (*rep_next == '&')
	      {
		/* Preceding the ampersand may be some literal
		 * text:
		 */
		if (rep_cur < rep_next)
		  str_append(&s_accum, rep_cur,
			     CAST(size_t)(rep_next - rep_cur));
		str_append(&s_accum, line.text + start + regs[0].rm_so,
			   CAST(size_t)(regs[0].rm_eo - regs[0].rm_so));
		rep_cur = rep_next + 1;
	      }
	  }

	/* There may be a trailing literal in the replacement string. */
	if (rep_cur < rep_next)
	  str_append(&s_accum, rep_cur, CAST(size_t)(rep_next - rep_cur));
      }

      did_subst = 1;
      not_bol_p = 1;
      start += regs[0].rm_eo;
      remain = line.length - start;
      if (!sub->global || remain == 0)
	break;

      /* If the match was vacuous, skip over one character
       * and add that character to the output.
       */
      if (regs[0].rm_so == regs[0].rm_eo)
	{
	  str_append(&s_accum, line.text + offset, 1);
	  ++start;
	  --remain;
	}
    }

  if (did_subst)
    {
      struct line tmp;
      if (start < line.length)
	str_append(&s_accum, line.text + start, remain);
      memcpy(VCAST(VOID *)&tmp, VCAST(VOID *)&line, sizeof line);
      memcpy(VCAST(VOID *)&line, VCAST(VOID *)&s_accum, sizeof line);
      line.chomped = tmp.chomped;
      memcpy(VCAST(VOID *)&s_accum, VCAST(VOID *)&tmp, sizeof line);
      if (sub->wfile)
	output_line(line.text, line.length, line.chomped, sub->wfile);
      if (sub->print)
	output_line(line.text, line.length, line.chomped, stdout);
      replaced = 1;
    }
}

/* Execute the program 'vec' on the current input line.
   Return non-zero if caller should quit, 0 otherwise.  */
static flagT
execute_program(vec, input)
  struct vector *vec;
  struct input *input;
{
  struct vector *cur_vec;
  struct sed_cmd *cur_cmd;
  size_t n;

  cur_vec = vec;
  cur_cmd = cur_vec->v;
  n = cur_vec->v_length;
  while (n)
    {
      if (match_address_p(cur_cmd, input))
	{
	  switch (cur_cmd->cmd)
	    {
	    case '{':		/* Execute sub-program */
	      if (cur_cmd->x.sub->v_length)
		{
		  cur_vec = cur_cmd->x.sub;
		  cur_cmd = cur_vec->v;
		  n = cur_vec->v_length;
		  continue;
		}
	      break;

	    case '}':
	      {
		countT i = cur_vec->return_i;
		cur_vec = cur_vec->return_v;
		cur_cmd = cur_vec->v + i;
		n = cur_vec->v_length - i;
	      }
	      continue;

	    case 'a':
	      {
		struct append_queue *aq = next_append_slot();
		aq->text = cur_cmd->x.cmd_txt.text;
		aq->textlen = cur_cmd->x.cmd_txt.text_len;
	      }
	      break;

	    case 'b':
	      if (cur_cmd->x.jump)
		{
		  struct sed_label *j = cur_cmd->x.jump;
		  countT i = j->v_index;
		  cur_vec = j->v;
		  cur_cmd = cur_vec->v + i;
		  n = cur_vec->v_length - i;
		  continue;
		}
	      return 0;

	    case 'c':
	      line.length = 0;
	      line.chomped = 0;
	      if (!cur_cmd->a1_matched)
		output_line(cur_cmd->x.cmd_txt.text,
			    cur_cmd->x.cmd_txt.text_len, 0, stdout);
	      /* POSIX.2 is silent about c starting a new cycle,
		 but it seems to be expected (and make sense). */
	      return 0;

	    case 'd':
	      line.length = 0;
	      line.chomped = 0;
	      return 0;

	    case 'D':
	      {
		char *p = memchr(line.text, '\n', line.length);
		if (!p)
		  {
		    line.length = 0;
		    line.chomped = 0;
		    return 0;
		  }
		++p;
		memmove(line.text, p, line.length);
		line.length -= p - line.text;

		/* reset to start next cycle without reading a new line: */
		cur_vec = vec;
		cur_cmd = cur_vec->v;
		n = cur_vec->v_length;
		continue;
	      }

	    case 'g':
	      line_copy(&hold, &line);
	      break;

	    case 'G':
	      line_append(&hold, &line);
	      break;

	    case 'h':
	      line_copy(&line, &hold);
	      break;

	    case 'H':
	      line_append(&line, &hold);
	      break;

	    case 'i':
	      output_line(cur_cmd->x.cmd_txt.text,
			  cur_cmd->x.cmd_txt.text_len, 0, stdout);
	      break;

	    case 'l':
	      do_list();
	      break;

	    case 'n':
	      if (!no_default_output)
		output_line(line.text, line.length, line.chomped, stdout);
	      if (!read_pattern_space(input, 0))
		return 1;
	      break;

	    case 'N':
	      str_append(&line, "\n", 1);
	      if (!read_pattern_space(input, 1))
		return 1;
	      break;

	    case 'p':
	      output_line(line.text, line.length, line.chomped, stdout);
	      break;

	    case 'P':
	      {
		char *p = memchr(line.text, '\n', line.length);
		output_line(line.text, p ? p - line.text : line.length,
			    p ? 1 : line.chomped, stdout);
	      }
	      break;

	    case 'q':
	      return 1;

	    case 'r':
	      if (cur_cmd->x.rfile)
		{
		  struct append_queue *aq = next_append_slot();
		  aq->rfile = cur_cmd->x.rfile;
		}
	      break;

	    case 's':
	      do_subst(&cur_cmd->x.cmd_regex);
	      break;

	    case 't':
	      if (replaced)
		{
		  replaced = 0;
		  if (cur_cmd->x.jump)
		    {
		      struct sed_label *j = cur_cmd->x.jump;
		      countT i = j->v_index;
		      cur_vec = j->v;
		      cur_cmd = cur_vec->v + i;
		      n = cur_vec->v_length - i;
		      continue;
		    }
		  return 0;
		}
	      break;

	    case 'w':
	      if (cur_cmd->x.wfile)
		output_line(line.text, line.length,
			    line.chomped, cur_cmd->x.wfile);
	      break;

	    case 'x':
	      {
		struct line temp;
		memcpy(VCAST(VOID *)&temp, VCAST(VOID *)&line, sizeof line);
		memcpy(VCAST(VOID *)&line, VCAST(VOID *)&hold, sizeof line);
		memcpy(VCAST(VOID *)&hold, VCAST(VOID *)&temp, sizeof line);
	      }
	      break;

	    case 'y':
	      {
		unsigned char *p, *e;
		p = CAST(unsigned char *)line.text;
		for (e=p+line.length; p<e; ++p)
		  *p = cur_cmd->x.translate[*p];
	      }
	      break;

	    case ':':
	      /* Executing labels is easy. */
	      break;

	    case '=':
	      printf("%lu\n", CAST(unsigned long)input->line_number);
	      break;

	    default:
	      panic("INTERNAL ERROR: Bad cmd %c", cur_cmd->cmd);
	    }
	}
#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
/* If our top-level program consists solely of commands with addr_is_num
 * address then once we past the last mentioned line we should be able
 * to quit if no_default_output is true, or otherwise quickly copy input
 * to output.  Now whether this optimization is a win or not depends
 * on how cheaply we can implement this for the cases where it doesn't
 * help, as compared against how much time is saved.
 * One semantic difference (which I think is an improvement) is
 * that *this* version will terminate after printing line two
 * in the script "yes | sed -n 2p".
 */
      else
	{
	  /* can we ever match again? */
	  if (cur_cmd->a1.addr_type == addr_is_num &&
	      ((input->line_number < cur_cmd->a1.a.addr_number)
		!= !cur_cmd->addr_bang))
	    {
	      /* skip all this next time */
	      cur_cmd->a1.addr_type = addr_is_null;
	      cur_cmd->addr_bang = 1;
	      /* can we make an optimization? */
	      if (cur_cmd->cmd == '{' || cur_cmd->cmd == '}'
		    || cur_cmd->cmd == 'b' || cur_cmd->cmd == 't')
		{
		  if (cur_vec == vec)
		    --branches_in_top_level;
		  cur_cmd->cmd = ':';	/* replace with no-op */
		}
	      if (cur_vec == vec && branches_in_top_level == 0)
		{
		  /* whew!  all that just so that we can get to here! */
		  countT new_n = n;
		  cur_cmd = shrink_program(cur_vec, cur_cmd, &new_n);
		  n = new_n;
		  if (!cur_cmd && no_default_output)
		    return 1;
		  continue;
		}
	    }
	}
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/

      /* these are buried down here so that a "continue" statement
	 can skip them */
      ++cur_cmd;
      --n;
    }
    return 0;
}

static void
output_line(text, length, nl, fp)
  const char *text;
  size_t length;
  flagT nl;
  FILE *fp;
{
  ck_fwrite(text, 1, length, fp);
  if (nl)
    ck_fwrite("\n", 1, 1, fp);
  if (fp != stdout)
    ck_fflush(fp);
}


/* initialize a "struct line" buffer */
static void
line_init(buf, initial_size)
  struct line *buf;
  size_t initial_size;
{
  buf->text = MALLOC(initial_size, char);
  buf->active = NULL;
  buf->alloc = initial_size;
  buf->length = 0;
  buf->chomped = 1;
}

/* Copy the contents of the line 'from' into the line 'to'.
   This destroys the old contents of 'to'.  It will still work
   if the line 'from' contains NULs. */
static void
line_copy(from, to)
  struct line *from;
  struct line *to;
{
  if (to->alloc < from->length)
    {
      to->alloc = from->length;
      to->text = REALLOC(to->text, to->alloc, char);
    }
  memcpy(to->text, from->text, from->length);
  to->length = from->length;
  to->chomped = from->chomped;
}

/* Append 'length' bytes from 'string' to the line 'to'
   This routine *will* append NUL bytes without failing.  */
static void
str_append(to, string, length)
  struct line *to;
  const char *string;
  size_t length;
{
  if (to->alloc - to->length < length)
    resize_line(to, length);
  memcpy(to->text + to->length, string, length);
  to->length += length;
}

/* Append the contents of the line 'from' to the line 'to'.
   This routine will work even if the line 'from' contains nulls */
static void
line_append(from, to)
  struct line *from;
  struct line *to;
{
  str_append(to, "\n", 1);
  str_append(to, from->text, from->length);
  to->chomped = from->chomped;
}



static struct append_queue *
next_append_slot()
{
  struct append_queue *n = MALLOC(1, struct append_queue);

  n->rfile = NULL;
  n->text = NULL;
  n->textlen = 0;
  n->next = NULL;

  if (append_tail)
      append_tail->next = n;
  else
      append_head = n;
  return append_tail = n;
}

static void
dump_append_queue()
{
  struct append_queue *p, *q;

  for (p=append_head; p; p=q)
    {
      if (p->text)
	  output_line(p->text, p->textlen, 0, stdout);
      if (p->rfile)
	{
	  char buf[FREAD_BUFFER_SIZE];
	  size_t cnt;
	  FILE *fp;

	  fp = fopen(p->rfile, "r");
	  /* Not ck_fopen() because: "If _rfile_ does not exist or cannot be
	     read, it shall be treated as if it were an empty file, causing
	     no error condition."  IEEE Std 1003.2-1992 */
	  if (fp)
	    {
	      while ((cnt = ck_fread(buf, 1, sizeof buf, fp)) > 0)
		ck_fwrite(buf, 1, cnt, stdout);
	      fclose(fp);
	    }
	}
      q = p->next;
      FREE(p);
    }
  append_head = append_tail = NULL;
}

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
static countT
count_branches(program)
  struct vector *program;
{
  struct sed_cmd *cur_cmd = program->v;
  countT isn_cnt = program->v_length;
  countT cnt = 0;

  while (isn_cnt-- > 0)
    {
      switch (cur_cmd->cmd)
	{
	case '{':
	case '}':
	case 'b':
	case 't':
	  ++cnt;
	}
    }
  return cnt;
}

static struct sed_cmd *
shrink_program(vec, cur_cmd, n)
  struct vector *vec;
  struct sed_cmd *cur_cmd;
  countT *n;
{
  struct sed_cmd *v = vec->v;
  struct sed_cmd *last_cmd = v + vec->v_length;
  struct sed_cmd *p;
  countT cmd_cnt;

  for (p=v; p < cur_cmd; ++p)
    if (p->cmd != ':')
      *v++ = *p;
  cmd_cnt = v - vec->v;

  for (++p; p < last_cmd; ++p)
    if (p->cmd != ':')
      *v++ = *p;
  vec->v_length = v - vec->v;

  *n = vec->v_length - cmd_cnt;
  return 0 < vec->v_length ? cur_cmd-cmd_cnt : CAST(struct sed_cmd *)0;
}
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/

#ifdef BOOTSTRAP
/* We can't be sure that the system we're boostrapping on has
   memchr(), and ../lib/memchr.c requires configuration knowledge
   about how many bits are in a `long'.  This implementation
   is far from ideal, but it should get us up-and-limping well
   enough to run the configure script, which is all that matters.
*/
static VOID *
bootstrap_memchr(s, c, n)
  const VOID *s;
  int c;
  size_t n;
{
  char *p;

  for (p=(char *)s; n-- > 0; ++p)
    if (*p == c)
      return p;
  return CAST(VOID *)0;
}
#endif /*BOOTSTRAP*/
