/* xmalloc.c declarations.
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

#ifndef XMALLOC_H
#define XMALLOC_H

/* Define this to use Wget's builtin malloc debugging, which is crude
   but occasionally useful.  It will make Wget a lot slower and
   larger, and susceptible to aborting if malloc_table overflows, so
   it should be used by developers only.  */
#undef DEBUG_MALLOC

/* When DEBUG_MALLOC is not defined (which is normally the case), the
   allocator identifiers are mapped to checking_* wrappers, which exit
   Wget if malloc/realloc/strdup return NULL

   In DEBUG_MALLOC mode, the allocators are mapped to debugging_*
   wrappers, which also record the file and line from which the
   allocation was attempted.  At the end of the program, a detailed
   summary of unfreed allocations is displayed.

   *Note*: xfree(NULL) aborts in both modes.  If the pointer you're
   freeing can be NULL, use xfree_null instead.  */

#ifndef DEBUG_MALLOC

#define xmalloc  checking_malloc
#define xmalloc0 checking_malloc0
#define xrealloc checking_realloc
#define xstrdup  checking_strdup
#define xfree    checking_free

void *checking_malloc (size_t);
void *checking_malloc0 (size_t);
void *checking_realloc (void *, size_t);
char *checking_strdup (const char *);
void checking_free (void *);

#else  /* DEBUG_MALLOC */

#define xmalloc(s)     debugging_malloc (s, __FILE__, __LINE__)
#define xmalloc0(s)    debugging_malloc0 (s, __FILE__, __LINE__)
#define xrealloc(p, s) debugging_realloc (p, s, __FILE__, __LINE__)
#define xstrdup(p)     debugging_strdup (p, __FILE__, __LINE__)
#define xfree(p)       debugging_free (p, __FILE__, __LINE__)

void *debugging_malloc (size_t, const char *, int);
void *debugging_malloc0 (size_t, const char *, int);
void *debugging_realloc (void *, size_t, const char *, int);
char *debugging_strdup (const char *, const char *, int);
void debugging_free (void *, const char *, int);

#endif /* DEBUG_MALLOC */

/* Macros that interface to malloc, but know about type sizes, and
   cast the result to the appropriate type.  The casts are not
   necessary in standard C, but Wget performs them anyway for the sake
   of pre-standard environments and possibly C++.  */

#define xnew(type) (xmalloc (sizeof (type)))
#define xnew0(type) (xmalloc0 (sizeof (type)))
#define xnew_array(type, len) (xmalloc ((len) * sizeof (type)))
#define xnew0_array(type, len) (xmalloc0 ((len) * sizeof (type)))

#define alloca_array(type, size) ((type *) alloca ((size) * sizeof (type)))

/* Free P if it is non-NULL.  C requires free() to behaves this way by
   default, but Wget's code is historically careful not to pass NULL
   to free.  This allows us to assert p!=NULL in xfree to check
   additional errors.  (But we currently don't do that!)  */
#define xfree_null(p) if (!(p)) ; else xfree (p)

#endif /* XMALLOC_H */
