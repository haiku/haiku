
/*  GNU SED, a batch stream editor.
    Copyright (C) 1989,90,91,92,93,94,95,98,99,2002,2003,2004
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

#undef EXPERIMENTAL_DASH_N_OPTIMIZATION	/*don't use -- is very buggy*/
#define INITIAL_BUFFER_SIZE	50
#define FREAD_BUFFER_SIZE	8192

#include "sed.h"

#include <stdio.h>
#include <ctype.h>

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
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

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else
# include <string.h>
#endif /*HAVE_STRINGS_H*/
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif

#ifndef HAVE_STRCHR
# define strchr index
# define strrchr rindex
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include <sys/stat.h>


/* Sed operates a line at a time. */
struct line {
  char *text;		/* Pointer to line allocated by malloc. */
  char *active;		/* Pointer to non-consumed part of text. */
  size_t length;	/* Length of text (or active, if used). */
  size_t alloc;		/* Allocated space for active. */
  flagT chomped;	/* Was a trailing newline dropped? */
};

/* A queue of text to write out at the end of a cycle
   (filled by the "a", "r" and "R" commands.) */
struct append_queue {
  const char *fname;
  char *text;
  size_t textlen;
  struct append_queue *next;
  flagT free;
};

/* State information for the input stream. */
struct input {
  char **file_list;	/* The list of yet-to-be-opened files.
			   It is invalid for file_list to be NULL.
			   When *file_list is NULL we are
			   currently processing the last file. */
  countT bad_count;	/* count of files we failed to open */
  countT line_number;	/* current input line number (over all files) */

  flagT (*read_fn) P_((struct input *));	/* read one line */
  /* If fp is NULL, read_fn better not be one which uses fp;
     in particular, read_always_fail() is recommended. */

  char *out_file_name;
  const char *in_file_name;

  FILE *fp;		/* if NULL, none of the following are valid */
  flagT no_buffering;
};


/* Have we done any replacements lately?  This is used by the `t' command. */
static flagT replaced = FALSE;

/* The current output file (stdout if -i is not being used. */
static FILE *output_file;

/* The `current' input line. */
static struct line line;

/* An input line used to accumulate the result of the s and e commands. */
static struct line s_accum;

/* An input line that's been stored by later use by the program */
static struct line hold;

/* The buffered input look-ahead.  The only field that should be
   used outside of read_mem_line() or line_init() is buffer.length. */
static struct line buffer;

static struct append_queue *append_head = NULL;
static struct append_queue *append_tail = NULL;


#ifdef BOOTSTRAP
/* We can't be sure that the system we're boostrapping on has
   memchr(), and ../lib/memchr.c requires configuration knowledge
   about how many bits are in a `long'.  This implementation
   is far from ideal, but it should get us up-and-limping well
   enough to run the configure script, which is all that matters.
*/
# ifdef memchr
#  undef memchr
# endif
# define memchr bootstrap_memchr

static VOID *bootstrap_memchr P_((const VOID *s, int c, size_t n));
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

/* increase a struct line's length, making some attempt at
   keeping realloc() calls under control by padding for future growth.  */
static void resize_line P_((struct line *, size_t));
static void
resize_line(lb, len)
  struct line *lb;
  size_t len;
{
  int inactive;
  inactive = lb->active - lb->text;

  /* If the inactive part has got to more than two thirds of the buffer,
   * remove it. */
  if (inactive > lb->alloc * 2)
    {
      MEMMOVE(lb->text, lb->active, lb->length);
      lb->alloc += lb->active - lb->text;
      lb->active = lb->text;
      inactive = 0;

      if (lb->alloc > len)
	return;
    }

  lb->alloc *= 2;
  if (lb->alloc < len)
    lb->alloc = len;
  if (lb->alloc < INITIAL_BUFFER_SIZE)
    lb->alloc = INITIAL_BUFFER_SIZE;
    
  lb->text = REALLOC(lb->text, inactive + lb->alloc, char);
  lb->active = lb->text + inactive;
}

/* Append `length' bytes from `string' to the line `to'. */
static void str_append P_((struct line *, const char *, size_t));
static void
str_append(to, string, length)
  struct line *to;
  const char *string;
  size_t length;
{
  size_t new_length = to->length + length;

  if (to->alloc < new_length)
    resize_line(to, new_length);
  MEMCPY(to->active + to->length, string, length);
  to->length = new_length;
}

static void str_append_modified P_((struct line *, const char *, size_t,
				    enum replacement_types));
static void
str_append_modified(to, string, length, type)
  struct line *to;
  const char *string;
  size_t length;
  enum replacement_types type;
{
  size_t old_length = to->length;
  char *start, *end;

  if (length == 0)
    return;

  str_append(to, string, length);
  start = to->active + old_length;
  end = start + length;

  /* Now do the required modifications.  First \[lu]... */
  if (type & repl_uppercase_first)
    {
      *start = toupper(*start);
      start++;
      type &= ~repl_uppercase_first;
    }
  else if (type & repl_lowercase_first)
    {
      *start = tolower(*start);
      start++;
      type &= ~repl_lowercase_first;
    }

  if (type == repl_asis)
    return;

  /* ...and then \[LU] */
  if (type == repl_uppercase)
    for (; start != end; start++)
      *start = toupper(*start);
  else
    for (; start != end; start++)
      *start = tolower(*start);
}

/* initialize a "struct line" buffer */
static void line_init P_((struct line *, size_t initial_size));
static void
line_init(buf, initial_size)
  struct line *buf;
  size_t initial_size;
{
  buf->text = MALLOC(initial_size, char);
  buf->active = buf->text;
  buf->alloc = initial_size;
  buf->length = 0;
  buf->chomped = TRUE;
}

/* Copy the contents of the line `from' into the line `to'.
   This destroys the old contents of `to'. */
static void line_copy P_((struct line *from, struct line *to));
static void
line_copy(from, to)
  struct line *from;
  struct line *to;
{
  /* Remove the inactive portion in the destination buffer. */
  to->alloc += to->active - to->text;

  if (to->alloc < from->length)
    {
      to->alloc *= 2;
      if (to->alloc < from->length)
	to->alloc = from->length;
      if (to->alloc < INITIAL_BUFFER_SIZE)
	to->alloc = INITIAL_BUFFER_SIZE;
      /* Use FREE()+MALLOC() instead of REALLOC() to
	 avoid unnecessary copying of old text. */
      FREE(to->text);
      to->text = MALLOC(to->alloc, char);
    }

  to->active = to->text;
  to->length = from->length;
  to->chomped = from->chomped;
  MEMCPY(to->active, from->active, from->length);
}

/* Append the contents of the line `from' to the line `to'. */
static void line_append P_((struct line *from, struct line *to));
static void
line_append(from, to)
  struct line *from;
  struct line *to;
{
  str_append(to, "\n", 1);
  str_append(to, from->active, from->length);
  to->chomped = from->chomped;
}

/* Exchange the contents of two "struct line" buffers. */
static void line_exchange P_((struct line *, struct line *));
static void
line_exchange(a, b)
  struct line *a;
  struct line *b;
{
  struct line t;

  MEMCPY(&t,  a, sizeof(struct line));
  MEMCPY( a,  b, sizeof(struct line));
  MEMCPY( b, &t, sizeof(struct line));
}


/* dummy function to simplify read_pattern_space() */
static flagT read_always_fail P_((struct input *));
static flagT
read_always_fail(input)
  struct input *input UNUSED;
{
  return FALSE;
}

static flagT read_file_line P_((struct input *));
static flagT
read_file_line(input)
  struct input *input;
{
  static char *b;
  static size_t blen;

  long result = getline (&b, &blen, input->fp);

  /* Remove the trailing new-line that is left by getline. */
  if (result > 0 && b[result - 1] == '\n')
    --result;
  else
    {
      /* No trailing new line found. */
      if (!*input->file_list && !POSIXLY_CORRECT)
	line.chomped = FALSE;

      if (result <= 0)
	return FALSE;
    }

  str_append(&line, b, result);
  return TRUE;
}


static void output_line P_((const char *, size_t, flagT, FILE *));
static void
output_line(text, length, nl, fp)
  const char *text;
  size_t length;
  flagT nl;
  FILE *fp;
{
  if (length)
    ck_fwrite(text, 1, length, fp);
  if (nl)
    ck_fwrite("\n", 1, 1, fp);
  if (fp != stdout || unbuffered_output)
    ck_fflush(fp);
}

static struct append_queue *next_append_slot P_((void));
static struct append_queue *
next_append_slot()
{
  struct append_queue *n = MALLOC(1, struct append_queue);

  n->fname = NULL;
  n->text = NULL;
  n->textlen = 0;
  n->next = NULL;
  n->free = FALSE;

  if (append_tail)
      append_tail->next = n;
  else
      append_head = n;
  return append_tail = n;
}

static void release_append_queue P_((void));
static void
release_append_queue()
{
  struct append_queue *p, *q;

  for (p=append_head; p; p=q)
    {
      if (p->free)
	FREE(p->text);

      q = p->next;
      FREE(p);
    }
  append_head = append_tail = NULL;
}

static void dump_append_queue P_((void));
static void
dump_append_queue()
{
  struct append_queue *p;

  for (p=append_head; p; p=p->next)
    {
      if (p->text)
	  output_line(p->text, p->textlen, FALSE, output_file);
      if (p->fname)
	{
	  char buf[FREAD_BUFFER_SIZE];
	  size_t cnt;
	  FILE *fp;

	  /* "If _fname_ does not exist or cannot be read, it shall
	     be treated as if it were an empty file, causing no error
	     condition."  IEEE Std 1003.2-1992
	     So, don't fail. */
	  fp = ck_fopen(p->fname, "r", FALSE);
	  if (fp)
	    {
	      while ((cnt = ck_fread(buf, 1, sizeof buf, fp)) > 0)
		ck_fwrite(buf, 1, cnt, output_file);
	      ck_fclose(fp);
	    }
	}
    }
  release_append_queue();
}


/* Compute the name of the backup file for in-place editing */
static char *get_backup_file_name P_((const char *));
static char *
get_backup_file_name(name)
  const char *name;
{
  char *old_asterisk, *asterisk, *backup, *p;
  int name_length = strlen(name), backup_length = strlen(in_place_extension);

  /* Compute the length of the backup file */
  for (asterisk = in_place_extension - 1, old_asterisk = asterisk + 1;
       asterisk = strchr(old_asterisk, '*');
       old_asterisk = asterisk + 1)
    backup_length += name_length - 1;

  p = backup = xmalloc(backup_length + 1);

  /* Each iteration gobbles up to an asterisk */
  for (asterisk = in_place_extension - 1, old_asterisk = asterisk + 1;
       asterisk = strchr(old_asterisk, '*');
       old_asterisk = asterisk + 1)
    {
      memcpy (p, old_asterisk, asterisk - old_asterisk);
      p += asterisk - old_asterisk;
      strcpy (p, name);
      p += name_length;
    }

  /* Tack on what's after the last asterisk */
  strcpy (p, old_asterisk);
  return backup;
}

/* Initialize a struct input for the named file. */
static void open_next_file P_((const char *name, struct input *));
static void
open_next_file(name, input)
  const char *name;
  struct input *input;
{
  buffer.length = 0;

  if (name[0] == '-' && name[1] == '\0')
    {
      clearerr(stdin);	/* clear any stale EOF indication */
      input->fp = stdin;
    }
  else if ( ! (input->fp = ck_fopen(name, "r", FALSE)) )
    {
      const char *ptr = strerror(errno);
      fprintf(stderr, _("%s: can't read %s: %s\n"), myname, name, ptr);
      input->read_fn = read_always_fail; /* a redundancy */
      ++input->bad_count;
      return;
    }

  input->read_fn = read_file_line;

  if (in_place_extension)
    {
      int output_fd;
      char *tmpdir = ck_strdup(name), *p;

      /* get the base name */
      if (p = strrchr(tmpdir, '/'))
	*(p + 1) = 0;
      else
	strcpy(tmpdir, ".");
      
      input->in_file_name = name;
      input->out_file_name = temp_file_template (tmpdir, "sed");
      output_fd = mkstemp (input->out_file_name);
      free (tmpdir);

      if (output_fd == -1)
        panic(_("Couldn't open temporary file %s: %s"), input->out_file_name, strerror(errno));

#ifdef HAVE_FCHMOD
      {
	struct stat st;
	fstat (fileno (input->fp), &st);
	fchmod (output_fd, st.st_mode);
      }
#endif

      output_file = fdopen (output_fd, "w");
      if (!output_file)
        panic(_("Couldn't open temporary file %s: %s"), input->out_file_name, strerror(errno));
    }
  else
    output_file = stdout;
}


/* Clean up an input stream that we are done with. */
static void closedown P_((struct input *));
static void
closedown(input)
  struct input *input;
{
  input->read_fn = read_always_fail;
  if (!input->fp)
    return;
  if (input->fp != stdin) /* stdin can be reused on tty and tape devices */
    ck_fclose(input->fp);

  if (in_place_extension && output_file != NULL)
    {
      int fd = fileno (output_file);
      if (strcmp(in_place_extension, "*") != 0)
        {
          char *backup_file_name = get_backup_file_name(input->in_file_name);
          rename (input->in_file_name, backup_file_name);
          free (backup_file_name);
        }

      ck_fclose (output_file);
      close (fd);
      rename (input->out_file_name, input->in_file_name);
      free (input->out_file_name);
    }

  input->fp = NULL;

  if (separate_files)
    rewind_read_files ();
}

/* Reset range commands so that they are marked as non-matching */
static void reset_addresses P_((struct vector *));
static void
reset_addresses(vec)
     struct vector *vec;
{
  struct sed_cmd *cur_cmd;
  int n;

  for (cur_cmd = vec->v, n = vec->v_length; n--; cur_cmd++)
    if (cur_cmd->a1)
      {
	enum addr_types type = cur_cmd->a1->addr_type;
        cur_cmd->a1_matched = (type == addr_is_num || type == addr_is_num_mod)
                              && cur_cmd->a1->addr_number == 0;
      }
    else
      cur_cmd->a1_matched = FALSE;
}

/* Read in the next line of input, and store it in the pattern space.
   Return zero if there is nothing left to input. */
static flagT read_pattern_space P_((struct input *, struct vector *, flagT));
static flagT
read_pattern_space(input, the_program, append)
  struct input *input;
  struct vector *the_program;
  flagT append;
{
  if (append_head) /* redundant test to optimize for common case */
    dump_append_queue();
  replaced = FALSE;
  if (!append)
    line.length = 0;
  line.chomped = TRUE;  /* default, until proved otherwise */

  while ( ! (*input->read_fn)(input) )
    {
      closedown(input);

      if (!*input->file_list)
	{
	  line.chomped = FALSE;
	  return FALSE;
	}

      if (separate_files)
	{
	  input->line_number = 0;
	  reset_addresses (the_program);
	}

      open_next_file (*input->file_list++, input);
    }

  ++input->line_number;
  return TRUE;
}


static flagT last_file_with_data_p P_((struct input *));
static flagT
last_file_with_data_p(input)
  struct input *input;
{
  for (;;)
    {
      int ch;

      closedown(input);
      if (!*input->file_list)
	return TRUE;
      open_next_file(*input->file_list++, input);
      if (input->fp)
	{
	  if ((ch = getc(input->fp)) != EOF)
	    {
	      ungetc(ch, input->fp);
	      return FALSE;
	    }
	}
    }
}

/* Determine if we match the `$' address. */
static flagT test_eof P_((struct input *));
static flagT
test_eof(input)
  struct input *input;
{
  int ch;

  if (buffer.length)
    return FALSE;
  if (!input->fp)
    return separate_files || last_file_with_data_p(input);
  if (feof(input->fp))
    return separate_files || last_file_with_data_p(input);
  if ((ch = getc(input->fp)) == EOF)
    return separate_files || last_file_with_data_p(input);
  ungetc(ch, input->fp);
  return FALSE;
}

/* Return non-zero if the current line matches the address
   pointed to by `addr'. */
static flagT match_an_address_p P_((struct addr *, struct input *));
static flagT
match_an_address_p(addr, input)
  struct addr *addr;
  struct input *input;
{
  switch (addr->addr_type)
    {
    case addr_is_null:
      return TRUE;

    case addr_is_regex:
      return match_regex(addr->addr_regex, line.active, line.length, 0, NULL, 0);

    case addr_is_num:
      return (addr->addr_number == input->line_number);

    case addr_is_num_mod:
      if (addr->addr_number < addr->addr_step)
	return (addr->addr_number == input->line_number%addr->addr_step);
      /* addr_number >= step implies we have an extra initial skip */
      if (input->line_number < addr->addr_number)
	return FALSE;
      /* normalize */
      addr->addr_number %= addr->addr_step;
      return (addr->addr_number == 0);

    case addr_is_num2:
    case addr_is_step:
    case addr_is_step_mod:
      /* reminder: these are only meaningful for a2 addresses */
      /* a2->addr_number needs to be recomputed each time a1 address
	 matches for the step and step_mod types */
      return (addr->addr_number <= input->line_number);

    case addr_is_last:
      return test_eof(input);

    default:
      panic(_("INTERNAL ERROR: bad address type"));
    }
  /*NOTREACHED*/
  return FALSE;
}

/* return non-zero if current address is valid for cmd */
static flagT match_address_p P_((struct sed_cmd *, struct input *));
static flagT
match_address_p(cmd, input)
  struct sed_cmd *cmd;
  struct input *input;
{
  flagT addr_matched = cmd->a1_matched;

  if (addr_matched)
    {
      if (match_an_address_p(cmd->a2, input))
	cmd->a1_matched = FALSE;
    }
  else if (!cmd->a1 || match_an_address_p(cmd->a1, input))
    {
      addr_matched = TRUE;
      if (cmd->a2)
	{
	  cmd->a1_matched = TRUE;
	  switch (cmd->a2->addr_type)
	    {
	    case addr_is_regex:
	      break;
	    case addr_is_step:
	      cmd->a2->addr_number = input->line_number + cmd->a2->addr_step;
	      break;
	    case addr_is_step_mod:
	      cmd->a2->addr_number = input->line_number + cmd->a2->addr_step
				     - (input->line_number%cmd->a2->addr_step);
	      break;
	    default:
	      if (match_an_address_p(cmd->a2, input))
		cmd->a1_matched = FALSE;
	      break;
	    }
	}
    }
  if (cmd->addr_bang)
    return !addr_matched;
  return addr_matched;
}


static void do_list P_((int line_len));
static void
do_list(line_len)
     int line_len;
{
  unsigned char *p = CAST(unsigned char *)line.active;
  countT len = line.length;
  countT width = 0;
  char obuf[180];	/* just in case we encounter a 512-bit char (;-) */
  char *o;
  size_t olen;

  for (; len--; ++p) {
      o = obuf;
      
      /* Some locales define 8-bit characters as printable.  This makes the
	 testsuite fail at 8to7.sed because the `l' command in fact will not
	 convert the 8-bit characters. */
#if defined isascii || defined HAVE_ISASCII
      if (isascii(*p) && ISPRINT(*p)) {
#else
      if (ISPRINT(*p)) {
#endif
	  *o++ = *p;
	  if (*p == '\\')
	    *o++ = '\\';
      } else {
	  *o++ = '\\';
	  switch (*p) {
#if defined __STDC__ && __STDC__-0
	    case '\a': *o++ = 'a'; break;
#else /* Not STDC; we'll just assume ASCII */
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
      if (width+olen >= line_len && line_len > 0) {
	  ck_fwrite("\\\n", 1, 2, output_file);
	  width = 0;
      }
      ck_fwrite(obuf, 1, olen, output_file);
      width += olen;
  }
  ck_fwrite("$\n", 1, 2, output_file);
}

static enum replacement_types append_replacement P_((struct line *, struct replacement *,
				                     struct re_registers *,
				                     enum replacement_types));
static enum replacement_types
append_replacement (buf, p, regs, repl_mod)
  struct line *buf;
  struct replacement *p;
  struct re_registers *regs;
  enum replacement_types repl_mod;
{
  for (; p; p=p->next)
    {
      int i = p->subst_id;
      enum replacement_types curr_type;

      /* Apply a \[lu] modifier that was given earlier, but which we
	 have not had yet the occasion to apply.  But don't do it
	 if this replacement has a modifier of its own. */
      curr_type = (p->repl_type & repl_modifiers)
	? p->repl_type
	: p->repl_type | repl_mod;

      repl_mod = 0;
      if (p->prefix_length)
	{
	  str_append_modified(buf, p->prefix, p->prefix_length,
			      curr_type);
	  curr_type &= ~repl_modifiers;
	}

      if (0 <= i)
	if (regs->end[i] == regs->start[i] && p->repl_type & repl_modifiers)
	  /* Save this modifier, we shall apply it later.
	     e.g. in s/()([a-z])/\u\1\2/
	     the \u modifier is applied to \2, not \1 */
	  repl_mod = curr_type & repl_modifiers;

	else
	  str_append_modified(buf, line.active + regs->start[i],
			      CAST(size_t)(regs->end[i] - regs->start[i]),
			      curr_type);
    }

  return repl_mod;
}

static void do_subst P_((struct subst *));
static void
do_subst(sub)
  struct subst *sub;
{
  size_t start = 0;     /* where to start scan for (next) match in LINE */
  size_t last_end = 0;  /* where did the last successful match end in LINE */
  countT count = 0;     /* number of matches found */
  flagT again = TRUE;

#define MAX_BACKREFERENCES 10
  static struct re_registers regs;

  if (s_accum.alloc == 0)
    line_init(&s_accum, INITIAL_BUFFER_SIZE);
  s_accum.length = 0;

  /* The first part of the loop optimizes s/xxx// when xxx is at the
     start, and s/xxx$// */
  if (!match_regex(sub->regx, line.active, line.length, start,
		   &regs, MAX_BACKREFERENCES))
    return;

  if (!sub->replacement && sub->numb <= 1)
    if (regs.start[0] == 0 && !sub->global)
      {
	/* We found a match, set the `replaced' flag. */
	replaced = TRUE;

	line.active += regs.end[0];
	line.length -= regs.end[0];
	line.alloc -= regs.end[0];
	goto post_subst;
      }
    else if (regs.end[0] == line.length)
      {
	/* We found a match, set the `replaced' flag. */
	replaced = TRUE;

	line.length = regs.start[0];
	goto post_subst;
      }

  do
    {
      enum replacement_types repl_mod = 0;

      size_t offset = regs.start[0];
      size_t matched = regs.end[0] - regs.start[0];

      /* Copy stuff to the left of this match into the output string. */
      if (start < offset)
	str_append(&s_accum, line.active + start, offset - start);

      /* If we're counting up to the Nth match, are we there yet?
	 And even if we are there, there is another case we have to
	 skip: are we matching an empty string immediately following
	 another match?

	 This latter case avoids that baaaac, when passed through
	 s,a*,x,g, gives `xbxxcx' instead of xbxcx.  This behavior is
	 unacceptable because it is not consistently applied (for
	 example, `baaaa' gives `xbx', not `xbxx'). */
      if ((matched > 0 || count == 0 || offset > last_end)
	  && (last_end = regs.end[0], ++count >= sub->numb))
	{
	  /* We found a match, set the `replaced' flag. */
	  replaced = TRUE;

	  /* Now expand the replacement string into the output string. */
	  repl_mod = append_replacement (&s_accum, sub->replacement, &regs, repl_mod);
	  again = sub->global;
	}
      else
	{
	  /* The match was not replaced.  Copy the text until its
	     end; if it was vacuous, skip over one character and
	     add that character to the output.  */
	  if (matched == 0)
	    {
	      if (start < line.length)
		matched = 1;
	      else
		break;
	    }

	  str_append(&s_accum, line.active + offset, matched);
	}

      /* Start after the match.  */
      start = offset + matched;
    }
  while (again
	 && start <= line.length
	 && match_regex(sub->regx, line.active, line.length, start,
			&regs, MAX_BACKREFERENCES));

  /* Copy stuff to the right of the last match into the output string. */
  if (start < line.length)
    str_append(&s_accum, line.active + start, line.length-start);
  s_accum.chomped = line.chomped;

  /* Exchange line and s_accum.  This can be much cheaper
     than copying s_accum.active into line.text (for huge lines). */
  line_exchange(&line, &s_accum);

  /* Finish up. */
  if (count < sub->numb)
    return;

 post_subst:
  if (sub->print & 1)
    output_line(line.active, line.length, line.chomped, output_file);

  if (sub->eval)
    {
#ifdef HAVE_POPEN
      FILE *pipe;
      s_accum.length = 0;

      str_append (&line, "", 1);
      pipe = popen(line.active, "r");
      
      if (pipe != NULL) 
	{
	  while (!feof (pipe)) 
	    {
	      char buf[4096];
	      int n = fread (buf, sizeof(char), 4096, pipe);
	      if (n > 0)
		str_append(&s_accum, buf, n);
	    }
	  
	  pclose (pipe);

	  line_exchange(&line, &s_accum);
	  if (line.length &&
	      line.active[line.length - 1] == '\n')
	    line.length--;
	}
      else
	panic(_("error in subprocess"));
#else
      panic(_("option `e' not supported"));
#endif
    } 
  
  if (sub->print & 2)
    output_line(line.active, line.length, line.chomped, output_file);
  if (sub->fp)
    output_line(line.active, line.length, line.chomped, sub->fp);
}

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
/* Used to attempt a simple-minded optimization. */

static countT branches;

static countT count_branches P_((struct vector *));
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
	case 'b':
	case 't':
	case 'T':
	case '{':
	  ++cnt;
	}
    }
  return cnt;
}

static struct sed_cmd *shrink_program P_((struct vector *, struct sed_cmd *));
static struct sed_cmd *
shrink_program(vec, cur_cmd)
  struct vector *vec;
  struct sed_cmd *cur_cmd;
{
  struct sed_cmd *v = vec->v;
  struct sed_cmd *last_cmd = v + vec->v_length;
  struct sed_cmd *p;
  countT cmd_cnt;

  for (p=v; p < cur_cmd; ++p)
    if (p->cmd != ':')
      MEMCPY(v++, p, sizeof *v);
  cmd_cnt = v - vec->v;

  for (; p < last_cmd; ++p)
    if (p->cmd != ':')
      MEMCPY(v++, p, sizeof *v);
  vec->v_length = v - vec->v;

  return (0 < vec->v_length) ? (vec->v + cmd_cnt) : CAST(struct sed_cmd *)0;
}
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/

/* Execute the program `vec' on the current input line.
   Return exit status if caller should quit, -1 otherwise. */
static int execute_program P_((struct vector *, struct input *));
static int
execute_program(vec, input)
  struct vector *vec;
  struct input *input;
{
  struct sed_cmd *cur_cmd;
  struct sed_cmd *end_cmd;

  cur_cmd = vec->v;
  end_cmd = vec->v + vec->v_length;
  while (cur_cmd < end_cmd)
    {
      if (match_address_p(cur_cmd, input))
	{
	  switch (cur_cmd->cmd)
	    {
	    case 'a':
	      {
		struct append_queue *aq = next_append_slot();
		aq->text = cur_cmd->x.cmd_txt.text;
		aq->textlen = cur_cmd->x.cmd_txt.text_length;
	      }
	      break;

	    case '{':
	    case 'b':
	      cur_cmd = vec->v + cur_cmd->x.jump_index;
	      continue;

	    case '}':
	    case ':':
	      /* Executing labels and block-ends are easy. */
	      break;

	    case 'c':
	      if (!cur_cmd->a1_matched)
		output_line(cur_cmd->x.cmd_txt.text,
			    cur_cmd->x.cmd_txt.text_length, FALSE, output_file);
	      /* POSIX.2 is silent about c starting a new cycle,
		 but it seems to be expected (and make sense). */
	      /* Fall Through */
	    case 'd':
	      return -1;

	    case 'D':
	      {
		char *p = memchr(line.active, '\n', line.length);
		if (!p)
		  {
		    line.length = 0;
		    line.chomped = FALSE;
		    return -1;
		  }
		++p;
		line.alloc -= p - line.active;
		line.length -= p - line.active;
		line.active += p - line.active;

		/* reset to start next cycle without reading a new line: */
		cur_cmd = vec->v;
		continue;
	      }

	    case 'e': {
#ifdef HAVE_POPEN
	      FILE *pipe;
	      int cmd_length = cur_cmd->x.cmd_txt.text_length;
	      if (s_accum.alloc == 0)
		line_init(&s_accum, INITIAL_BUFFER_SIZE);
	      s_accum.length = 0;

	      if (!cmd_length)
		{
		  str_append (&line, "", 1);
		  pipe = popen(line.active, "r");
		} 
	      else
		{
		  cur_cmd->x.cmd_txt.text[cmd_length - 1] = 0;
		  pipe = popen(cur_cmd->x.cmd_txt.text, "r");
		}

	      if (pipe != NULL) 
		{
		  while (!feof (pipe)) 
		    {
		      char buf[4096];
		      int n = fread (buf, sizeof(char), 4096, pipe);
		      if (n > 0)
			if (!cmd_length)
			  str_append(&s_accum, buf, n);
			else
			  output_line(buf, n, FALSE, output_file);
		    }
		  
		  pclose (pipe);
		  if (!cmd_length)
		    {
		      /* Store into pattern space for plain `e' commands */
		      if (s_accum.length &&
			  s_accum.active[s_accum.length - 1] == '\n')
			s_accum.length--;

		      /* Exchange line and s_accum.  This can be much
			 cheaper than copying s_accum.active into line.text
			 (for huge lines). */
		      line_exchange(&line, &s_accum);
		    }
		}
	      else
		panic(_("error in subprocess"));
#else
	      panic(_("`e' command not supported"));
#endif
	      break;
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
			  cur_cmd->x.cmd_txt.text_length, FALSE, output_file);
	      break;

	    case 'l':
	      do_list(cur_cmd->x.int_arg == -1
		      ? lcmd_out_line_len
		      : cur_cmd->x.int_arg);
	      break;

	    case 'L':
	      fmt(line.active, line.active + line.length,
		  cur_cmd->x.int_arg == -1
		  ? lcmd_out_line_len
		  : cur_cmd->x.int_arg,
		  output_file);
	      break;

	    case 'n':
	      if (!no_default_output)
		output_line(line.active, line.length, line.chomped, output_file);
	      if (test_eof(input) || !read_pattern_space(input, vec, FALSE))
		return -1;
	      break;

	    case 'N':
	      str_append(&line, "\n", 1);

	      if (test_eof(input) || !read_pattern_space(input, vec, TRUE))
		{
		  line.length--;
                  if (!POSIXLY_CORRECT && !no_default_output)
                    output_line(line.active, line.length, line.chomped,
				output_file);
		  return -1;
		}
	      break;

	    case 'p':
	      output_line(line.active, line.length, line.chomped, output_file);
	      break;

	    case 'P':
	      {
		char *p = memchr(line.active, '\n', line.length);
		output_line(line.active, p ? p - line.active : line.length,
			    p ? 1 : line.chomped, output_file);
	      }
	      break;

            case 'q':
              if (!no_default_output)
                output_line(line.active, line.length, line.chomped, output_file);

            case 'Q':
              return cur_cmd->x.int_arg == -1 ? 0 : cur_cmd->x.int_arg;

	    case 'r':
	      if (cur_cmd->x.fname)
		{
		  struct append_queue *aq = next_append_slot();
		  aq->fname = cur_cmd->x.fname;
		}
	      break;

	    case 'R':
	      if (cur_cmd->x.fp && !feof (cur_cmd->x.fp))
		{
		  struct append_queue *aq;
		  size_t buflen;
		  char *text = NULL;
		  int result;

		  result = getline (&text, &buflen, cur_cmd->x.fp);

		  if (result != EOF)
		    {
		      aq = next_append_slot();
		      aq->free = TRUE;
		      aq->text = text;
		      aq->textlen = result;
		    }
		}
	      break;

	    case 's':
	      do_subst(cur_cmd->x.cmd_subst);
	      break;

	    case 't':
	      if (replaced)
		{
		  replaced = FALSE;
		  cur_cmd = vec->v + cur_cmd->x.jump_index;
		  continue;
		}
	      break;

	    case 'T':
	      if (!replaced)
		{
		  cur_cmd = vec->v + cur_cmd->x.jump_index;
		  continue;
		}
	      else
		replaced = FALSE;
	      break;

	    case 'w':
	      if (cur_cmd->x.fp)
		output_line(line.active, line.length,
			    line.chomped, cur_cmd->x.fp);
	      break;

	    case 'W':
	      if (cur_cmd->x.fp)
		{
		  char *p = memchr(line.active, '\n', line.length);
		  output_line(line.active, p ? p - line.active : line.length,
			      p ? 1 : line.chomped, cur_cmd->x.fp);
		}
	      break;

	    case 'x':
	      line_exchange(&line, &hold);
	      break;

	    case 'y':
	      {
#if defined HAVE_MBRTOWC && !defined REG_PERL
	       if (MB_CUR_MAX > 1)
		 {
		   int idx, prev_idx; /* index in the input line.  */
		   char **trans;
		   mbstate_t cur_stat;
		   memset(&cur_stat, 0, sizeof(mbstate_t));
		   for (idx = 0; idx < line.length;)
		     {
		       int mbclen, i;
		       mbclen = mbrlen(line.active + idx, line.length - idx,
				       &cur_stat);
		       /* An invalid sequence, or a truncated multibyte
			  character.  We treat it as a singlebyte character.
		       */
		       if (mbclen == (size_t) -1 || mbclen == (size_t) -2
			   || mbclen == 0)
			 mbclen = 1;

		       trans = cur_cmd->x.translatemb;
		       /* `i' indicate i-th translate pair.  */
		       for (i = 0; trans[2*i] != NULL; i++)
			 {
			   if (strncmp(line.active + idx, trans[2*i], mbclen)
			       == 0)
			     {
			       flagT move_remain_buffer = FALSE;
			       int trans_len = strlen(trans[2*i+1]);

			       if (mbclen < trans_len)
				 {
				   int new_len;
				   new_len = line.length + 1 + trans_len - mbclen;
				   /* We must extend the line buffer.  */
				   if (line.alloc < new_len)
				     {
				       /* And we must resize the buffer.  */
				       resize_line(&line, new_len);
				     }
				   move_remain_buffer = TRUE;
				 }
			       else if (mbclen > trans_len)
				 {
				   /* We must truncate the line buffer.  */
				   move_remain_buffer = TRUE;
				 }
			       prev_idx = idx;
			       if (move_remain_buffer)
				 {
				   int move_len, move_offset;
				   char *move_from, *move_to;
				   /* Move the remaining with \0.  */
				   move_from = line.active + idx + mbclen;
				   move_to = line.active + idx + trans_len;
				   move_len = line.length + 1 - idx - mbclen;
				   move_offset = trans_len - mbclen;
				   memmove(move_to, move_from, move_len);
				   line.length += move_offset;
				   idx += move_offset;
				 }
			       strncpy(line.active + prev_idx, trans[2*i+1],
				       trans_len);
			       break;
			     }
			 }
		       idx += mbclen;
		     }
		 }
	       else
#endif /* HAVE_MBRTOWC */
		 {
		   unsigned char *p, *e;
		   p = CAST(unsigned char *)line.active;
		   for (e=p+line.length; p<e; ++p)
		     *p = cur_cmd->x.translate[*p];
		 }
	      }
	      break;

	    case '=':
	      fprintf(output_file, "%lu\n",
		      CAST(unsigned long)input->line_number);
	      break;

	    default:
	      panic(_("INTERNAL ERROR: Bad cmd %c"), cur_cmd->cmd);
	    }
	}

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
      /* If our top-level program consists solely of commands with
       * addr_is_num addresses then once we past the last mentioned
       * line we should be able to quit if no_default_output is true,
       * or otherwise quickly copy input to output.  Now whether this
       * optimization is a win or not depends on how cheaply we can
       * implement this for the cases where it doesn't help, as
       * compared against how much time is saved.  One semantic
       * difference (which I think is an improvement) is that *this*
       * version will terminate after printing line two in the script
       * "yes | sed -n 2p". 
       *
       * Don't use this when in-place editing is active, because line
       * numbers restart each time then. */
      else if (output_file == stdout)
	{
	  /* can we ever match again? */
	  if (cur_cmd->a1->addr_type == addr_is_num &&
	      ((input->line_number < cur_cmd->a1->addr_number)
	       != !cur_cmd->addr_bang))
	    {
	      /* skip all this next time */
	      cur_cmd->a1->addr_type = addr_is_null;
	      cur_cmd->addr_bang = TRUE;

	      /* can we make an optimization? */
	      if (cur_cmd->cmd == 'b' || cur_cmd->cmd == 't'
		  || cur_cmd->cmd == '{')
		--branches;
	      cur_cmd->cmd = ':';	/* replace with no-op */
	      if (branches == 0)
		{
		  /* whew!  all that just so that we can get to here! */
		  cur_cmd = shrink_program(vec, cur_cmd);
		  if (!cur_cmd && no_default_output)
		    return 0;
		  end_cmd = vec->v + vec->v_length;
		  if (!cur_cmd)
		    cur_cmd = end_cmd;
		  continue;
		}
	    }
	}
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/

      /* this is buried down here so that a "continue" statement can skip it */
      ++cur_cmd;
    }

    if (!no_default_output)
      output_line(line.active, line.length, line.chomped, output_file);
    return -1;
}



/* Apply the compiled script to all the named files. */
int
process_files(the_program, argv)
  struct vector *the_program;
  char **argv;
{
  static char dash[] = "-";
  static char *stdin_argv[2] = { dash, NULL };
  struct input input;
  int status;

  line_init(&line, INITIAL_BUFFER_SIZE);
  line_init(&hold, 0);
  line_init(&buffer, 0);

#ifdef EXPERIMENTAL_DASH_N_OPTIMIZATION
  branches = count_branches(the_program);
#endif /*EXPERIMENTAL_DASH_N_OPTIMIZATION*/
  input.file_list = stdin_argv;
  if (argv && *argv)
    input.file_list = argv;
  input.bad_count = 0;
  input.line_number = 0;
  input.read_fn = read_always_fail;
  input.fp = NULL;

  status = EXIT_SUCCESS;
  while (read_pattern_space(&input, the_program, FALSE))
    {
      status = execute_program(the_program, &input);
      if (status == -1)
	status = EXIT_SUCCESS;
      else
	break;
    }
  closedown(&input);

#ifdef DEBUG_LEAKS
  /* We're about to exit, so these free()s are redundant.
     But if we're running under a memory-leak detecting
     implementation of malloc(), we want to explicitly
     deallocate in order to avoid extraneous noise from
     the allocator. */
  release_append_queue();
  FREE(buffer.text);
  FREE(hold.text);
  FREE(line.text);
  FREE(s_accum.text);
#endif /*DEBUG_LEAKS*/

  if (input.bad_count)
    status = 2;

  return status;
}
