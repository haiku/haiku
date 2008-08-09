/* Wrappers around malloc and memory debugging support.
   Copyright (C) 2003, 2004, 2005, 2006, 2007,
   2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "wget.h"
#include "xmalloc.h"
#include "hash.h"               /* for hash_pointer */

/* This file implements several wrappers around the basic allocation
   routines.  This is done for two reasons: first, so that the callers
   of these functions need not check for errors, which is easy to
   forget.  If there is not enough virtual memory for running Wget,
   something is seriously wrong, and Wget exits with an appropriate
   error message.

   The second reason why these are useful is that, if DEBUG_MALLOC is
   defined, they also provide a handy (if crude) malloc debugging
   interface that checks for memory leaks.  */

/* Croak the fatal memory error and bail out with non-zero exit
   status.  */

static void
memfatal (const char *context, long attempted_size)
{
  /* Make sure we don't try to store part of the log line, and thus
     call malloc.  */
  log_set_save_context (false);
  logprintf (LOG_ALWAYS,
             _("%s: %s: Failed to allocate %ld bytes; memory exhausted.\n"),
             exec_name, context, attempted_size);
  exit (1);
}

/* These functions end with _real because they need to be
   distinguished from the debugging functions, and from the macros.
   Explanation follows:

   If memory debugging is not turned on, xmalloc.h defines these:

     #define xmalloc checking_malloc
     #define xmalloc0 checking_malloc0
     #define xrealloc checking_realloc
     #define xstrdup checking_strdup
     #define xfree checking_free

   In case of memory debugging, the definitions are a bit more
   complex, because we want to provide more information, *and* we want
   to call the debugging code.  (The former is the reason why xmalloc
   and friends need to be macros in the first place.)  Then it looks
   like this:

     #define xmalloc(a) debugging_malloc (a, __FILE__, __LINE__)
     #define xmalloc0(a) debugging_malloc0 (a, __FILE__, __LINE__)
     #define xrealloc(a, b) debugging_realloc (a, b, __FILE__, __LINE__)
     #define xstrdup(a) debugging_strdup (a, __FILE__, __LINE__)
     #define xfree(a) debugging_free (a, __FILE__, __LINE__)

   Each of the debugging_* functions does its magic and calls the
   corresponding checking_* one.  */

#ifdef DEBUG_MALLOC
# define STATIC_IF_DEBUG static
#else
# define STATIC_IF_DEBUG
#endif

STATIC_IF_DEBUG void *
checking_malloc (size_t size)
{
  void *ptr = malloc (size);
  if (!ptr)
    memfatal ("malloc", size);
  return ptr;
}

STATIC_IF_DEBUG void *
checking_malloc0 (size_t size)
{
  /* Using calloc can be faster than malloc+memset because some calloc
     implementations know when they're dealing with zeroed-out memory
     from the system and can avoid unnecessary memset.  */
  void *ptr = calloc (1, size);
  if (!ptr)
    memfatal ("calloc", size);
  return ptr;
}

STATIC_IF_DEBUG void *
checking_realloc (void *ptr, size_t newsize)
{
  void *newptr;

  /* Not all Un*xes have the feature of realloc() that calling it with
     a NULL-pointer is the same as malloc(), but it is easy to
     simulate.  */
  if (ptr)
    newptr = realloc (ptr, newsize);
  else
    newptr = malloc (newsize);
  if (!newptr)
    memfatal ("realloc", newsize);
  return newptr;
}

STATIC_IF_DEBUG char *
checking_strdup (const char *s)
{
  char *copy;

#ifndef HAVE_STRDUP
  int l = strlen (s);
  copy = malloc (l + 1);
  if (!copy)
    memfatal ("strdup", l + 1);
  memcpy (copy, s, l + 1);
#else  /* HAVE_STRDUP */
  copy = strdup (s);
  if (!copy)
    memfatal ("strdup", 1 + strlen (s));
#endif /* HAVE_STRDUP */

  return copy;
}

STATIC_IF_DEBUG void
checking_free (void *ptr)
{
  /* Wget's xfree() must not be passed a NULL pointer.  This is for
     historical reasons: pre-C89 systems were reported to bomb at
     free(NULL), and Wget was careful to not call xfree when there was
     a possibility of PTR being NULL.  (It might have been better to
     simply have xfree() do nothing if ptr==NULL.)

     Since the code is already written that way, this assert simply
     enforces the existing constraint.  The benefit is double-checking
     the logic: code that thinks it can't be passed a NULL pointer,
     while it in fact can, aborts here.  If you trip on this, either
     the code has a pointer handling bug or should have called
     xfree_null instead of xfree.  Correctly written code should never
     trigger this assertion.

     The downside is that the uninitiated might not expect xfree(NULL)
     to abort.  If the assertion proves to be too much of a hassle, it
     can be removed and a check that makes NULL a no-op placed in its
     stead.  If that is done, xfree_null is no longer needed and
     should be removed.  */
  assert (ptr != NULL);

  free (ptr);
}

#ifdef DEBUG_MALLOC

/* Crude home-grown routines for debugging some malloc-related
   problems.  Featured:

   * Counting the number of malloc and free invocations, and reporting
     the "balance", i.e. how many times more malloc was called than it
     was the case with free.

   * Making malloc store its entry into a simple array and free remove
     stuff from that array.  At the end, print the pointers which have
     not been freed, along with the source file and the line number.

   * Checking for "invalid frees", where free is called on a pointer
     not obtained with malloc, or where the same pointer is freed
     twice.

   Note that this kind of memory leak checking strongly depends on
   every malloc() being followed by a free(), even if the program is
   about to finish.  Wget is careful to free the data structure it
   allocated in init.c.  */

static int malloc_count, free_count;

/* Home-grown hash table of mallocs: */

#define SZ 100003               /* Prime just over 100,000.  Increase
                                   it to debug larger Wget runs.  */

static struct {
  const void *ptr;
  const char *file;
  int line;
} malloc_table[SZ];

/* Find PTR's position in malloc_table.  If PTR is not found, return
   the next available position.  */

static inline int
ptr_position (const void *ptr)
{
  int i = hash_pointer (ptr) % SZ;
  for (; malloc_table[i].ptr != NULL; i = (i + 1) % SZ)
    if (malloc_table[i].ptr == ptr)
      return i;
  return i;
}

/* Register PTR in malloc_table.  Abort if this is not possible
   (presumably due to the number of current allocations exceeding the
   size of malloc_table.)  */

static void
register_ptr (const void *ptr, const char *file, int line)
{
  int i;
  if (malloc_count - free_count > SZ)
    {
      fprintf (stderr, "Increase SZ to a larger value and recompile.\n");
      fflush (stderr);
      abort ();
    }

  i = ptr_position (ptr);
  malloc_table[i].ptr = ptr;
  malloc_table[i].file = file;
  malloc_table[i].line = line;
}

/* Unregister PTR from malloc_table.  Return false if PTR is not
   present in malloc_table.  */

static bool
unregister_ptr (void *ptr)
{
  int i = ptr_position (ptr);
  if (malloc_table[i].ptr == NULL)
    return false;
  malloc_table[i].ptr = NULL;

  /* Relocate malloc_table entries immediately following PTR. */
  for (i = (i + 1) % SZ; malloc_table[i].ptr != NULL; i = (i + 1) % SZ)
    {
      const void *ptr2 = malloc_table[i].ptr;
      /* Find the new location for the key. */
      int j = hash_pointer (ptr2) % SZ;
      for (; malloc_table[j].ptr != NULL; j = (j + 1) % SZ)
        if (ptr2 == malloc_table[j].ptr)
          /* No need to relocate entry at [i]; it's already at or near
             its hash position. */
          goto cont_outer;
      malloc_table[j] = malloc_table[i];
      malloc_table[i].ptr = NULL;
    cont_outer:
      ;
    }
  return true;
}

/* Print the malloc debug stats gathered from the above information.
   Currently this is the count of mallocs, frees, the difference
   between the two, and the dump of the contents of malloc_table.  The
   last part are the memory leaks.  */

void
print_malloc_debug_stats (void)
{
  int i;
  printf ("\nMalloc:  %d\nFree:    %d\nBalance: %d\n\n",
          malloc_count, free_count, malloc_count - free_count);
  for (i = 0; i < SZ; i++)
    if (malloc_table[i].ptr != NULL)
      printf ("0x%0*lx: %s:%d\n", PTR_FORMAT (malloc_table[i].ptr),
              malloc_table[i].file, malloc_table[i].line);
}

void *
debugging_malloc (size_t size, const char *source_file, int source_line)
{
  void *ptr = checking_malloc (size);
  ++malloc_count;
  register_ptr (ptr, source_file, source_line);
  return ptr;
}

void *
debugging_malloc0 (size_t size, const char *source_file, int source_line)
{
  void *ptr = checking_malloc0 (size);
  ++malloc_count;
  register_ptr (ptr, source_file, source_line);
  return ptr;
}

void *
debugging_realloc (void *ptr, size_t newsize, const char *source_file, int source_line)
{
  void *newptr = checking_realloc (ptr, newsize);
  if (!ptr)
    {
      ++malloc_count;
      register_ptr (newptr, source_file, source_line);
    }
  else if (newptr != ptr)
    {
      unregister_ptr (ptr);
      register_ptr (newptr, source_file, source_line);
    }
  return newptr;
}

char *
debugging_strdup (const char *s, const char *source_file, int source_line)
{
  char *copy = checking_strdup (s);
  ++malloc_count;
  register_ptr (copy, source_file, source_line);
  return copy;
}

void
debugging_free (void *ptr, const char *source_file, int source_line)
{
  /* See checking_free for rationale of this abort.  We repeat it here
     because we can print the file and the line where the offending
     free occurred.  */
  if (ptr == NULL)
    {
      fprintf (stderr, "%s: xfree(NULL) at %s:%d\n",
               exec_name, source_file, source_line);
      abort ();
    }
  if (!unregister_ptr (ptr))
    {
      fprintf (stderr, "%s: bad xfree(0x%0*lx) at %s:%d\n",
               exec_name, PTR_FORMAT (ptr), source_file, source_line);
      abort ();
    }
  ++free_count;

  checking_free (ptr);
}

#endif /* DEBUG_MALLOC */
