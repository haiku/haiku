/* Copyright (C) 1995-1999, 2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _SIMPLE_HASH_H
#define _SIMPLE_HASH_H

#include <obstack.h>

typedef struct hash_table
{
  unsigned long int size;
  unsigned long int filled;
  void *first;
  void *table;
  struct obstack mem_pool;
}
hash_table;


extern int init_hash (hash_table *htab, unsigned long int init_size) __THROW;
extern int delete_hash (hash_table *htab) __THROW;
extern int insert_entry (hash_table *htab, const void *key, size_t keylen,
			 void *data) __THROW;
extern int find_entry (const hash_table *htab, const void *key, size_t keylen,
		       void **result) __THROW;
extern int set_entry (hash_table *htab, const void *key, size_t keylen,
		      void *newval) __THROW;

extern int iterate_table (const hash_table *htab, void **ptr,
			  const void **key, size_t *keylen, void **data)
     __THROW;

extern unsigned long int compute_hashval (const void *key, size_t keylen)
     __THROW;
extern unsigned long int next_prime (unsigned long int seed) __THROW;

#endif /* simple-hash.h */
