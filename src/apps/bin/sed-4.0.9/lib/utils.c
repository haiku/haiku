/*  Functions from hack's utils library.
    Copyright (C) 1989, 1990, 1991, 1998, 1999, 2003
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

#include "config.h"

#include <stdio.h>

#include <errno.h>
#ifndef errno
  extern int errno;
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "utils.h"

const char *myname;

void do_ck_fclose P_((FILE *stream));

/* Print an error message and exit */
#if !defined __STDC__ || !(__STDC__-0)
# include <varargs.h>
# define VSTART(l,a)	va_start(l)
void
panic(str, va_alist)
  char *str;
  va_dcl
#else /*__STDC__*/
# include <stdarg.h>
# define VSTART(l,a)	va_start(l, a)
void
panic(const char *str, ...)
#endif /* __STDC__ */
{
  va_list iggy;

  fprintf(stderr, "%s: ", myname);
  VSTART(iggy, str);
#ifndef HAVE_VPRINTF
# ifndef HAVE_DOPRNT
  fputs(str, stderr);	/* not great, but perhaps better than nothing... */
# else /* HAVE_DOPRNT */
  _doprnt(str, &iggy, stderr);
# endif /* HAVE_DOPRNT */
#else /* HAVE_VFPRINTF */
  vfprintf(stderr, str, iggy);
#endif /* HAVE_VFPRINTF */
  va_end(iggy);
  putc('\n', stderr);
  exit(4);
}


/* Store information about files opened with ck_fopen
   so that error messages from ck_fread, ck_fwrite, etc. can print the
   name of the file that had the error */

struct id
  {
    FILE *fp;
    char *name;
    struct id *link;
  };

static struct id *utils_id_s = NULL;

/* Internal routine to get a filename from utils_id_s */
static const char *utils_fp_name P_((FILE *fp));
static const char *
utils_fp_name(fp)
  FILE *fp;
{
  struct id *p;

  for (p=utils_id_s; p; p=p->link)
    if (p->fp == fp)
      return p->name;
  if (fp == stdin)
    return "stdin";
  else if (fp == stdout)
    return "stdout";
  else if (fp == stderr)
    return "stderr";

  return "<unknown>";
}

/* Panic on failing fopen */
FILE *
ck_fopen(name, mode, fail)
  const char *name;
  const char *mode;
  flagT fail;
{
  FILE *fp;
  struct id *p;

  fp = fopen (name, mode);
  if (!fp)
    {
      if (fail)
        panic(_("Couldn't open file %s: %s"), name, strerror(errno));

      return NULL;
    }

  for (p=utils_id_s; p; p=p->link)
    {
      if (fp == p->fp)
	{
	  FREE(p->name);
	  break;
	}
    }
  if (!p)
    {
      p = MALLOC(1, struct id);
      p->link = utils_id_s;
      utils_id_s = p;
    }
  p->name = ck_strdup(name);
  p->fp = fp;
  return fp;
}

/* Panic on failing fwrite */
void
ck_fwrite(ptr, size, nmemb, stream)
  const VOID *ptr;
  size_t size;
  size_t nmemb;
  FILE *stream;
{
  clearerr(stream);
  if (size && fwrite(ptr, size, nmemb, stream) != nmemb)
    panic(ngettext("couldn't write %d item to %s: %s",
		   "couldn't write %d items to %s: %s", nmemb), 
		nmemb, utils_fp_name(stream), strerror(errno));
}

/* Panic on failing fread */
size_t
ck_fread(ptr, size, nmemb, stream)
  VOID *ptr;
  size_t size;
  size_t nmemb;
  FILE *stream;
{
  clearerr(stream);
  if (size && (nmemb=fread(ptr, size, nmemb, stream)) <= 0 && ferror(stream))
    panic(_("read error on %s: %s"), utils_fp_name(stream), strerror(errno));

  return nmemb;
}

/* Panic on failing fflush */
void
ck_fflush(stream)
  FILE *stream;
{
  clearerr(stream);
  if (fflush(stream) == EOF && errno != EBADF)
    panic("Couldn't flush %s: %s", utils_fp_name(stream), strerror(errno));
}

/* Panic on failing fclose */
void
ck_fclose(stream)
  FILE *stream;
{
  struct id r;
  struct id *prev;
  struct id *cur;

  /* a NULL stream means to close all files */
  r.link = utils_id_s;
  prev = &r;
  while ( (cur = prev->link) )
    {
      if (!stream || stream == cur->fp)
	{
	  do_ck_fclose (cur->fp);
	  prev->link = cur->link;
	  FREE(cur->name);
	  FREE(cur);
	}
      else
	prev = cur;
    }

  utils_id_s = r.link;

  /* Also care about stdout, because if it is redirected the
     last output operations might fail and it is important
     to signal this as an error (perhaps to make). */
  if (!stream)
    {
      do_ck_fclose (stdout);
      do_ck_fclose (stderr);
    }
}

/* Close a single file and update a count of closed files. */
void
do_ck_fclose(stream)
  FILE *stream;
{
  ck_fflush(stream);
  clearerr(stream);
  if (fclose(stream) == EOF)
    panic("Couldn't close %s: %s", utils_fp_name(stream), strerror(errno));
}



char *
temp_file_template(tmpdir, program)
  const char *tmpdir;    
  char *program;
{
  char *template;
  if (tmpdir == NULL)
    tmpdir = getenv("TMPDIR");
  if (tmpdir == NULL)
    {
      tmpdir = getenv("TMP");
      if (tmpdir == NULL)
#ifdef P_tmpdir
	tmpdir = P_tmpdir;
#else
	tmpdir = "/tmp";
#endif
    }

  template = xmalloc (strlen (tmpdir) + strlen (program) + 8);
  sprintf (template, "%s/%sXXXXXX", tmpdir, program);
  return (template);
}


/* Panic on failing malloc */
VOID *
ck_malloc(size)
  size_t size;
{
  VOID *ret = calloc(1, size ? size : 1);
  if (!ret)
    panic("Couldn't allocate memory");
  return ret;
}

/* Panic on failing malloc */
VOID *
xmalloc(size)
  size_t size;
{
  return ck_malloc(size);
}

/* Panic on failing realloc */
VOID *
ck_realloc(ptr, size)
  VOID *ptr;
  size_t size;
{
  VOID *ret;

  if (size == 0)
    {
      FREE(ptr);
      return NULL;
    }
  if (!ptr)
    return ck_malloc(size);
  ret = realloc(ptr, size);
  if (!ret)
    panic("Couldn't re-allocate memory");
  return ret;
}

/* Return a malloc()'d copy of a string */
char *
ck_strdup(str)
  const char *str;
{
  char *ret = MALLOC(strlen(str)+1, char);
  return strcpy(ret, str);
}

/* Return a malloc()'d copy of a block of memory */
VOID *
ck_memdup(buf, len)
  const VOID *buf;
  size_t len;
{
  VOID *ret = ck_malloc(len);
  return memcpy(ret, buf, len);
}

/* Release a malloc'd block of memory */
void
ck_free(ptr)
  VOID *ptr;
{
  if (ptr)
    free(ptr);
}


/* Implement a variable sized buffer of `stuff'.  We don't know what it is,
nor do we care, as long as it doesn't mind being aligned by malloc. */

struct buffer
  {
    size_t allocated;
    size_t length;
    char *b;
  };

#define MIN_ALLOCATE 50

struct buffer *
init_buffer()
{
  struct buffer *b = MALLOC(1, struct buffer);
  b->b = MALLOC(MIN_ALLOCATE, char);
  b->allocated = MIN_ALLOCATE;
  b->length = 0;
  return b;
}

char *
get_buffer(b)
  struct buffer *b;
{
  return b->b;
}

size_t
size_buffer(b)
  struct buffer *b;
{
  return b->length;
}

static void resize_buffer P_((struct buffer *b, size_t newlen));
static void
resize_buffer(b, newlen)
  struct buffer *b;
  size_t newlen;
{
  char *try = NULL;
  size_t alen = b->allocated;

  if (newlen <= alen)
    return;
  alen *= 2;
  if (newlen < alen)
    try = realloc(b->b, alen);	/* Note: *not* the REALLOC() macro! */
  if (!try)
    {
      alen = newlen;
      try = REALLOC(b->b, alen, char);
    }
  b->allocated = alen;
  b->b = try;
}

void
add_buffer(b, p, n)
  struct buffer *b;
  const char *p;
  size_t n;
{
  if (b->allocated - b->length < n)
    resize_buffer(b, b->length+n);
  memcpy(b->b + b->length, p, n);
  b->length += n;
}

void
add1_buffer(b, c)
  struct buffer *b;
  int c;
{
  /* This special case should be kept cheap;
   *  don't make it just a mere convenience
   *  wrapper for add_buffer() -- even "builtin"
   *  versions of memcpy(a, b, 1) can become
   *  expensive when called too often.
   */
  if (c != EOF)
    {
      if (b->allocated - b->length < 1)
	resize_buffer(b, b->length+1);
      b->b[b->length++] = c;
    }
}

void
free_buffer(b)
  struct buffer *b;
{
  if (b)
    FREE(b->b);
  FREE(b);
}
