#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef _GNU_SOURCE

#include <sys/types.h>
#include <stdio.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#else
# include <string.h>
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <limits.h>
#include <errno.h>

/* Read up to (and including) a '\n' from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */

size_t
getline (lineptr, n, stream)
     char **lineptr;
     size_t *n;
     FILE *stream;
{
  char *line, *p;
  long size, copy;

  if (lineptr == NULL || n == NULL)
    {
      errno = EINVAL;
      return (size_t) -1;
    }

  if (ferror (stream))
    return (size_t) -1;

  /* Make sure we have a line buffer to start with.  */
  if (*lineptr == NULL || *n < 2) /* !seen and no buf yet need 2 chars.  */
    {
#ifndef	MAX_CANON
#define	MAX_CANON	256
#endif
      if (!*lineptr)
        line = (char *) malloc (MAX_CANON);
      else
        line = (char *) realloc (*lineptr, MAX_CANON);
      if (line == NULL)
	return (size_t) -1;
      *lineptr = line;
      *n = MAX_CANON;
    }

  line = *lineptr;
  size = *n;

  copy = size;
  p = line;

  while (1)
    {
      long len;
      
      while (--copy > 0)
	{
	  register int c = getc (stream);
	  if (c == EOF)
	    goto lose;
	  else if ((*p++ = c) == '\n')
	    goto win;
	}
      
      /* Need to enlarge the line buffer.  */
      len = p - line;
      size *= 2;
      line = (char *) realloc (line, size);
      if (line == NULL)
	goto lose;
      *lineptr = line;
      *n = size;
      p = line + len;
      copy = size - len;
    }

 lose:
  if (p == *lineptr)
    return (size_t) -1;

  /* Return a partial line since we got an error in the middle.  */
 win:
#if defined(WIN32) || defined(_WIN32) || defined(__CYGWIN__) || defined(MSDOS) || defined(__EMX__)
  if (p - 2 >= *lineptr && p[-2] == '\r')
    p[-2] = p[-1], --p;
#endif
  *p = '\0';
  return p - *lineptr;
}
