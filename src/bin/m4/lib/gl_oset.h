/* Abstract ordered set data type.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _GL_OSET_H
#define _GL_OSET_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* gl_oset is an abstract ordered set data type.  It can contain any number
   of objects ('void *' or 'const void *' pointers) in the order of a given
   comparator function.  Duplicates (in the sense of the comparator) are
   forbidden.

   There are several implementations of this ordered set datatype, optimized
   for different operations or for memory.  You can start using the simplest
   ordered set implementation, GL_ARRAY_OSET, and switch to a different
   implementation later, when you realize which operations are performed
   the most frequently.  The API of the different implementations is exactly
   the same; when switching to a different implementation, you only have to
   change the gl_oset_create call.

   The implementations are:
     GL_ARRAY_OSET        a growable array
     GL_AVLTREE_OSET      a binary tree (AVL tree)
     GL_RBTREE_OSET       a binary tree (red-black tree)

   The memory consumption is asymptotically the same: O(1) for every object
   in the set.  When looking more closely at the average memory consumed
   for an object, GL_ARRAY_OSET is the most compact representation, and
   GL_AVLTREE_OSET, GL_RBTREE_OSET need more memory.

   The guaranteed average performance of the operations is, for a set of
   n elements:

   Operation                  ARRAY     TREE

   gl_oset_size                O(1)     O(1)
   gl_oset_add                 O(n)   O(log n)
   gl_oset_remove              O(n)   O(log n)
   gl_oset_search            O(log n) O(log n)
   gl_oset_search_atleast    O(log n) O(log n)
   gl_oset_iterator            O(1)   O(log n)
   gl_oset_iterator_next       O(1)   O(log n)
 */

/* -------------------------- gl_oset_t Data Type -------------------------- */

/* Type of function used to compare two elements.  Same as for qsort().
   NULL denotes pointer comparison.  */
typedef int (*gl_setelement_compar_fn) (const void *elt1, const void *elt2);

/* Type of function used to compare an element with a threshold.
   Return true if the element is greater or equal than the threshold.  */
typedef bool (*gl_setelement_threshold_fn) (const void *elt, const void *threshold);

struct gl_oset_impl;
/* Type representing an entire ordered set.  */
typedef struct gl_oset_impl * gl_oset_t;

struct gl_oset_implementation;
/* Type representing a ordered set datatype implementation.  */
typedef const struct gl_oset_implementation * gl_oset_implementation_t;

/* Create an empty set.
   IMPLEMENTATION is one of GL_ARRAY_OSET, GL_AVLTREE_OSET, GL_RBTREE_OSET.
   COMPAR_FN is an element comparison function or NULL.  */
extern gl_oset_t gl_oset_create_empty (gl_oset_implementation_t implementation,
				       gl_setelement_compar_fn compar_fn);

/* Return the current number of elements in an ordered set.  */
extern size_t gl_oset_size (gl_oset_t set);

/* Search whether an element is already in the ordered set.
   Return true if found, or false if not present in the set.  */
extern bool gl_oset_search (gl_oset_t set, const void *elt);

/* Search the least element in the ordered set that compares greater or equal
   to the given THRESHOLD.  The representation of the THRESHOLD is defined
   by the THRESHOLD_FN.
   Return true and store the found element in *ELTP if found, otherwise return
   false.  */
extern bool gl_oset_search_atleast (gl_oset_t set,
				    gl_setelement_threshold_fn threshold_fn,
				    const void *threshold,
				    const void **eltp);

/* Add an element to an ordered set.
   Return true if it was not already in the set and added.  */
extern bool gl_oset_add (gl_oset_t set, const void *elt);

/* Remove an element from an ordered set.
   Return true if it was found and removed.  */
extern bool gl_oset_remove (gl_oset_t set, const void *elt);

/* Free an entire ordered set.
   (But this call does not free the elements of the set.)  */
extern void gl_oset_free (gl_oset_t set);

/* --------------------- gl_oset_iterator_t Data Type --------------------- */

/* Functions for iterating through an ordered set.  */

/* Type of an iterator that traverses an ordered set.
   This is a fixed-size struct, so that creation of an iterator doesn't need
   memory allocation on the heap.  */
typedef struct
{
  /* For fast dispatch of gl_oset_iterator_next.  */
  const struct gl_oset_implementation *vtable;
  /* For detecting whether the last returned element was removed.  */
  gl_oset_t set;
  size_t count;
  /* Other, implementation-private fields.  */
  void *p; void *q;
  size_t i; size_t j;
} gl_oset_iterator_t;

/* Create an iterator traversing an ordered set.
   The set's contents must not be modified while the iterator is in use,
   except for removing the last returned element.  */
extern gl_oset_iterator_t gl_oset_iterator (gl_oset_t set);

/* If there is a next element, store the next element in *ELTP, advance the
   iterator and return true.  Otherwise, return false.  */
extern bool gl_oset_iterator_next (gl_oset_iterator_t *iterator,
				   const void **eltp);

/* Free an iterator.  */
extern void gl_oset_iterator_free (gl_oset_iterator_t *iterator);

/* ------------------------ Implementation Details ------------------------ */

struct gl_oset_implementation
{
  /* gl_oset_t functions.  */
  gl_oset_t (*create_empty) (gl_oset_implementation_t implementation,
			     gl_setelement_compar_fn compar_fn);
  size_t (*size) (gl_oset_t set);
  bool (*search) (gl_oset_t set, const void *elt);
  bool (*search_atleast) (gl_oset_t set,
			  gl_setelement_threshold_fn threshold_fn,
			  const void *threshold, const void **eltp);
  bool (*add) (gl_oset_t set, const void *elt);
  bool (*remove) (gl_oset_t set, const void *elt);
  void (*oset_free) (gl_oset_t set);
  /* gl_oset_iterator_t functions.  */
  gl_oset_iterator_t (*iterator) (gl_oset_t set);
  bool (*iterator_next) (gl_oset_iterator_t *iterator, const void **eltp);
  void (*iterator_free) (gl_oset_iterator_t *iterator);
};

struct gl_oset_impl_base
{
  const struct gl_oset_implementation *vtable;
  gl_setelement_compar_fn compar_fn;
};

#if HAVE_INLINE

/* Define all functions of this file as inline accesses to the
   struct gl_oset_implementation.
   Use #define to avoid a warning because of extern vs. static.  */

# define gl_oset_create_empty gl_oset_create_empty_inline
static inline gl_oset_t
gl_oset_create_empty (gl_oset_implementation_t implementation,
		      gl_setelement_compar_fn compar_fn)
{
  return implementation->create_empty (implementation, compar_fn);
}

# define gl_oset_size gl_oset_size_inline
static inline size_t
gl_oset_size (gl_oset_t set)
{
  return ((const struct gl_oset_impl_base *) set)->vtable->size (set);
}

# define gl_oset_search gl_oset_search_inline
static inline bool
gl_oset_search (gl_oset_t set, const void *elt)
{
  return ((const struct gl_oset_impl_base *) set)->vtable->search (set, elt);
}

# define gl_oset_search_atleast gl_oset_search_atleast_inline
static inline bool
gl_oset_search_atleast (gl_oset_t set,
			gl_setelement_threshold_fn threshold_fn,
			const void *threshold, const void **eltp)
{
  return ((const struct gl_oset_impl_base *) set)->vtable
	 ->search_atleast (set, threshold_fn, threshold, eltp);
}

# define gl_oset_add gl_oset_add_inline
static inline bool
gl_oset_add (gl_oset_t set, const void *elt)
{
  return ((const struct gl_oset_impl_base *) set)->vtable->add (set, elt);
}

# define gl_oset_remove gl_oset_remove_inline
static inline bool
gl_oset_remove (gl_oset_t set, const void *elt)
{
  return ((const struct gl_oset_impl_base *) set)->vtable->remove (set, elt);
}

# define gl_oset_free gl_oset_free_inline
static inline void
gl_oset_free (gl_oset_t set)
{
  ((const struct gl_oset_impl_base *) set)->vtable->oset_free (set);
}

# define gl_oset_iterator gl_oset_iterator_inline
static inline gl_oset_iterator_t
gl_oset_iterator (gl_oset_t set)
{
  return ((const struct gl_oset_impl_base *) set)->vtable->iterator (set);
}

# define gl_oset_iterator_next gl_oset_iterator_next_inline
static inline bool
gl_oset_iterator_next (gl_oset_iterator_t *iterator, const void **eltp)
{
  return iterator->vtable->iterator_next (iterator, eltp);
}

# define gl_oset_iterator_free gl_oset_iterator_free_inline
static inline void
gl_oset_iterator_free (gl_oset_iterator_t *iterator)
{
  iterator->vtable->iterator_free (iterator);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* _GL_OSET_H */
