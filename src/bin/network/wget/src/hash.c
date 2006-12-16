/* Hash tables.
   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

/* With -DSTANDALONE, this file can be compiled outside Wget source
   tree.  To test, also use -DTEST.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
# ifdef HAVE_STRING_H
#  include <string.h>
# else
#  include <strings.h>
# endif
# ifdef HAVE_LIMITS_H
#  include <limits.h>
# endif
#else
/* If running without Autoconf, go ahead and assume presence of
   standard C89 headers.  */
# include <string.h>
# include <limits.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef STANDALONE
/* Get Wget's utility headers. */
# include "wget.h"
# include "utils.h"
#else
/* Make do without them. */
# define xnew(x) xmalloc (sizeof (x))
# define xnew_array(type, x) xmalloc (sizeof (type) * (x))
# define xmalloc malloc		/* or something that exits
				   if not enough memory */
# define xfree free
# define countof(x) (sizeof (x) / sizeof ((x)[0]))
# define TOLOWER(x) ('A' <= (x) && (x) <= 'Z' ? (x) - 32 : (x))
# define PARAMS(x) x
#endif

#include "hash.h"

/* INTERFACE:

   Hash tables are a technique used to implement mapping between
   objects with near-constant-time access and storage.  The table
   associates keys to values, and a value can be very quickly
   retrieved by providing the key.  Fast lookup tables are typically
   implemented as hash tables.

   The entry points are
     hash_table_new       -- creates the table.
     hash_table_destroy   -- destroys the table.
     hash_table_put       -- establishes or updates key->value mapping.
     hash_table_get       -- retrieves value of key.
     hash_table_get_pair  -- get key/value pair for key.
     hash_table_contains  -- test whether the table contains key.
     hash_table_remove    -- remove the key->value mapping for key.
     hash_table_map       -- iterate through table mappings.
     hash_table_clear     -- clear hash table contents.
     hash_table_count     -- return the number of entries in the table.

   The hash table grows internally as new entries are added and is not
   limited in size, except by available memory.  The table doubles
   with each resize, which ensures that the amortized time per
   operation remains constant.

   By default, tables created by hash_table_new consider the keys to
   be equal if their pointer values are the same.  You can use
   make_string_hash_table to create tables whose keys are considered
   equal if their string contents are the same.  In the general case,
   the criterion of equality used to compare keys is specified at
   table creation time with two callback functions, "hash" and "test".
   The hash function transforms the key into an arbitrary number that
   must be the same for two equal keys.  The test function accepts two
   keys and returns non-zero if they are to be considered equal.

   Note that neither keys nor values are copied when inserted into the
   hash table, so they must exist for the lifetime of the table.  This
   means that e.g. the use of static strings is OK, but objects with a
   shorter life-time need to be copied (with strdup() or the like in
   the case of strings) before being inserted.  */

/* IMPLEMENTATION:

   The hash table is implemented as an open-addressed table with
   linear probing collision resolution.

   The above means that all the hash entries (pairs of pointers, key
   and value) are stored in a contiguous array.  The position of each
   mapping is determined by the hash value of its key and the size of
   the table: location := hash(key) % size.  If two different keys end
   up on the same position (collide), the one that came second is
   placed at the next empty position following the occupied place.
   This collision resolution technique is called "linear probing".

   There are more advanced collision resolution methods (quadratic
   probing, double hashing), but we don't use them because they incur
   more non-sequential access to the array, which results in worse CPU
   cache behavior.  Linear probing works well as long as the
   count/size ratio (fullness) is kept below 75%.  We make sure to
   grow and rehash the table whenever this threshold is exceeded.

   Collisions make deletion tricky because clearing a position
   followed by a colliding entry would make the position seem empty
   and the colliding entry not found.  One solution is to leave a
   "tombstone" instead of clearing the entry, and another is to
   carefully rehash the entries immediately following the deleted one.
   We use the latter method because it results in less bookkeeping and
   faster retrieval at the (slight) expense of deletion.  */

/* Maximum allowed fullness: when hash table's fullness exceeds this
   value, the table is resized.  */
#define HASH_MAX_FULLNESS 0.75

/* The hash table size is multiplied by this factor (and then rounded
   to the next prime) with each resize.  This guarantees infrequent
   resizes.  */
#define HASH_RESIZE_FACTOR 2

struct mapping {
  void *key;
  void *value;
};

typedef unsigned long (*hashfun_t) PARAMS ((const void *));
typedef int (*testfun_t) PARAMS ((const void *, const void *));

struct hash_table {
  hashfun_t hash_function;
  testfun_t test_function;

  struct mapping *mappings;	/* pointer to the table entries. */
  int size;			/* size of the array. */

  int count;			/* number of non-empty entries. */
  int resize_threshold;		/* after size exceeds this number of
				   entries, resize the table.  */
  int prime_offset;		/* the offset of the current prime in
				   the prime table. */
};

/* We use the all-bits-set constant (INVALID_PTR) marker to mean that
   a mapping is empty.  It is unaligned and therefore illegal as a
   pointer.  INVALID_PTR_BYTE (0xff) is the one-byte value used to
   initialize the mappings array as empty.

   The all-bits-set value is a better choice than NULL because it
   allows the use of NULL/0 keys.  Since the keys are either integers
   or pointers, the only key that cannot be used is the integer value
   -1.  This is acceptable because it still allows the use of
   nonnegative integer keys.  */

#define INVALID_PTR ((void *) ~(unsigned long)0)
#ifndef UCHAR_MAX
# define UCHAR_MAX 0xff
#endif
#define INVALID_PTR_BYTE UCHAR_MAX

#define NON_EMPTY(mp) ((mp)->key != INVALID_PTR)
#define MARK_AS_EMPTY(mp) ((mp)->key = INVALID_PTR)

/* "Next" mapping is the mapping after MP, but wrapping back to
   MAPPINGS when MP would reach MAPPINGS+SIZE.  */
#define NEXT_MAPPING(mp, mappings, size) (mp != mappings + (size - 1)	\
					  ? mp + 1 : mappings)

/* Loop over non-empty mappings starting at MP. */
#define LOOP_NON_EMPTY(mp, mappings, size)				\
  for (; NON_EMPTY (mp); mp = NEXT_MAPPING (mp, mappings, size))

/* Return the position of KEY in hash table SIZE large, hash function
   being HASHFUN.  */
#define HASH_POSITION(key, hashfun, size) ((hashfun) (key) % size)

/* Find a prime near, but greather than or equal to SIZE.  The primes
   are looked up from a table with a selection of primes convenient
   for this purpose.

   PRIME_OFFSET is a minor optimization: it specifies start position
   for the search for the large enough prime.  The final offset is
   stored in the same variable.  That way the list of primes does not
   have to be scanned from the beginning each time around.  */

static int
prime_size (int size, int *prime_offset)
{
  static const int primes[] = {
    13, 19, 29, 41, 59, 79, 107, 149, 197, 263, 347, 457, 599, 787, 1031,
    1361, 1777, 2333, 3037, 3967, 5167, 6719, 8737, 11369, 14783,
    19219, 24989, 32491, 42257, 54941, 71429, 92861, 120721, 156941,
    204047, 265271, 344857, 448321, 582821, 757693, 985003, 1280519,
    1664681, 2164111, 2813353, 3657361, 4754591, 6180989, 8035301,
    10445899, 13579681, 17653589, 22949669, 29834603, 38784989,
    50420551, 65546729, 85210757, 110774011, 144006217, 187208107,
    243370577, 316381771, 411296309, 534685237, 695090819, 903618083,
    1174703521, 1527114613, 1837299131, 2147483647
  };
  int i;

  for (i = *prime_offset; i < countof (primes); i++)
    if (primes[i] >= size)
      {
	/* Set the offset to the next prime.  That is safe because,
	   next time we are called, it will be with a larger SIZE,
	   which means we could never return the same prime anyway.
	   (If that is not the case, the caller can simply reset
	   *prime_offset.)  */
	*prime_offset = i + 1;
	return primes[i];
      }

  abort ();
}

static int cmp_pointer PARAMS ((const void *, const void *));

/* Create a hash table with hash function HASH_FUNCTION and test
   function TEST_FUNCTION.  The table is empty (its count is 0), but
   pre-allocated to store at least ITEMS items.

   ITEMS is the number of items that the table can accept without
   needing to resize.  It is useful when creating a table that is to
   be immediately filled with a known number of items.  In that case,
   the regrows are a waste of time, and specifying ITEMS correctly
   will avoid them altogether.

   Note that hash tables grow dynamically regardless of ITEMS.  The
   only use of ITEMS is to preallocate the table and avoid unnecessary
   dynamic regrows.  Don't bother making ITEMS prime because it's not
   used as size unchanged.  To start with a small table that grows as
   needed, simply specify zero ITEMS.

   If hash and test callbacks are not specified, identity mapping is
   assumed, i.e. pointer values are used for key comparison.  (Common
   Lisp calls such tables EQ hash tables, and Java calls them
   IdentityHashMaps.)  If your keys require different comparison,
   specify hash and test functions.  For easy use of C strings as hash
   keys, you can use the convenience functions make_string_hash_table
   and make_nocase_string_hash_table.  */

struct hash_table *
hash_table_new (int items,
		unsigned long (*hash_function) (const void *),
		int (*test_function) (const void *, const void *))
{
  int size;
  struct hash_table *ht = xnew (struct hash_table);

  ht->hash_function = hash_function ? hash_function : hash_pointer;
  ht->test_function = test_function ? test_function : cmp_pointer;

  /* If the size of struct hash_table ever becomes a concern, this
     field can go.  (Wget doesn't create many hashes.)  */
  ht->prime_offset = 0;

  /* Calculate the size that ensures that the table will store at
     least ITEMS keys without the need to resize.  */
  size = 1 + items / HASH_MAX_FULLNESS;
  size = prime_size (size, &ht->prime_offset);
  ht->size = size;
  ht->resize_threshold = size * HASH_MAX_FULLNESS;
  /*assert (ht->resize_threshold >= items);*/

  ht->mappings = xnew_array (struct mapping, ht->size);

  /* Mark mappings as empty.  We use 0xff rather than 0 to mark empty
     keys because it allows us to use NULL/0 as keys.  */
  memset (ht->mappings, INVALID_PTR_BYTE, size * sizeof (struct mapping));

  ht->count = 0;

  return ht;
}

/* Free the data associated with hash table HT. */

void
hash_table_destroy (struct hash_table *ht)
{
  xfree (ht->mappings);
  xfree (ht);
}

/* The heart of most functions in this file -- find the mapping whose
   KEY is equal to key, using linear probing.  Returns the mapping
   that matches KEY, or the first empty mapping if none matches.  */

static inline struct mapping *
find_mapping (const struct hash_table *ht, const void *key)
{
  struct mapping *mappings = ht->mappings;
  int size = ht->size;
  struct mapping *mp = mappings + HASH_POSITION (key, ht->hash_function, size);
  testfun_t equals = ht->test_function;

  LOOP_NON_EMPTY (mp, mappings, size)
    if (equals (key, mp->key))
      break;
  return mp;
}

/* Get the value that corresponds to the key KEY in the hash table HT.
   If no value is found, return NULL.  Note that NULL is a legal value
   for value; if you are storing NULLs in your hash table, you can use
   hash_table_contains to be sure that a (possibly NULL) value exists
   in the table.  Or, you can use hash_table_get_pair instead of this
   function.  */

void *
hash_table_get (const struct hash_table *ht, const void *key)
{
  struct mapping *mp = find_mapping (ht, key);
  if (NON_EMPTY (mp))
    return mp->value;
  else
    return NULL;
}

/* Like hash_table_get, but writes out the pointers to both key and
   value.  Returns non-zero on success.  */

int
hash_table_get_pair (const struct hash_table *ht, const void *lookup_key,
		     void *orig_key, void *value)
{
  struct mapping *mp = find_mapping (ht, lookup_key);
  if (NON_EMPTY (mp))
    {
      if (orig_key)
	*(void **)orig_key = mp->key;
      if (value)
	*(void **)value = mp->value;
      return 1;
    }
  else
    return 0;
}

/* Return 1 if HT contains KEY, 0 otherwise. */

int
hash_table_contains (const struct hash_table *ht, const void *key)
{
  struct mapping *mp = find_mapping (ht, key);
  return NON_EMPTY (mp);
}

/* Grow hash table HT as necessary, and rehash all the key-value
   mappings.  */

static void
grow_hash_table (struct hash_table *ht)
{
  hashfun_t hasher = ht->hash_function;
  struct mapping *old_mappings = ht->mappings;
  struct mapping *old_end      = ht->mappings + ht->size;
  struct mapping *mp, *mappings;
  int newsize;

  newsize = prime_size (ht->size * HASH_RESIZE_FACTOR, &ht->prime_offset);
#if 0
  printf ("growing from %d to %d; fullness %.2f%% to %.2f%%\n",
	  ht->size, newsize,
	  100.0 * ht->count / ht->size,
	  100.0 * ht->count / newsize);
#endif

  ht->size = newsize;
  ht->resize_threshold = newsize * HASH_MAX_FULLNESS;

  mappings = xnew_array (struct mapping, newsize);
  memset (mappings, INVALID_PTR_BYTE, newsize * sizeof (struct mapping));
  ht->mappings = mappings;

  for (mp = old_mappings; mp < old_end; mp++)
    if (NON_EMPTY (mp))
      {
	struct mapping *new_mp;
	/* We don't need to test for uniqueness of keys because they
	   come from the hash table and are therefore known to be
	   unique.  */
	new_mp = mappings + HASH_POSITION (mp->key, hasher, newsize);
	LOOP_NON_EMPTY (new_mp, mappings, newsize)
	  ;
	*new_mp = *mp;
      }

  xfree (old_mappings);
}

/* Put VALUE in the hash table HT under the key KEY.  This regrows the
   table if necessary.  */

void
hash_table_put (struct hash_table *ht, const void *key, void *value)
{
  struct mapping *mp = find_mapping (ht, key);
  if (NON_EMPTY (mp))
    {
      /* update existing item */
      mp->key   = (void *)key; /* const? */
      mp->value = value;
      return;
    }

  /* If adding the item would make the table exceed max. fullness,
     grow the table first.  */
  if (ht->count >= ht->resize_threshold)
    {
      grow_hash_table (ht);
      mp = find_mapping (ht, key);
    }

  /* add new item */
  ++ht->count;
  mp->key   = (void *)key;	/* const? */
  mp->value = value;
}

/* Remove a mapping that matches KEY from HT.  Return 0 if there was
   no such entry; return 1 if an entry was removed.  */

int
hash_table_remove (struct hash_table *ht, const void *key)
{
  struct mapping *mp = find_mapping (ht, key);
  if (!NON_EMPTY (mp))
    return 0;
  else
    {
      int size = ht->size;
      struct mapping *mappings = ht->mappings;
      hashfun_t hasher = ht->hash_function;

      MARK_AS_EMPTY (mp);
      --ht->count;

      /* Rehash all the entries following MP.  The alternative
	 approach is to mark the entry as deleted, i.e. create a
	 "tombstone".  That speeds up removal, but leaves a lot of
	 garbage and slows down hash_table_get and hash_table_put.  */

      mp = NEXT_MAPPING (mp, mappings, size);
      LOOP_NON_EMPTY (mp, mappings, size)
	{
	  const void *key2 = mp->key;
	  struct mapping *mp_new;

	  /* Find the new location for the key. */
	  mp_new = mappings + HASH_POSITION (key2, hasher, size);
	  LOOP_NON_EMPTY (mp_new, mappings, size)
	    if (key2 == mp_new->key)
	      /* The mapping MP (key2) is already where we want it (in
		 MP_NEW's "chain" of keys.)  */
	      goto next_rehash;

	  *mp_new = *mp;
	  MARK_AS_EMPTY (mp);

	next_rehash:
	  ;
	}
      return 1;
    }
}

/* Clear HT of all entries.  After calling this function, the count
   and the fullness of the hash table will be zero.  The size will
   remain unchanged.  */

void
hash_table_clear (struct hash_table *ht)
{
  memset (ht->mappings, INVALID_PTR_BYTE, ht->size * sizeof (struct mapping));
  ht->count = 0;
}

/* Map MAPFUN over all the mappings in hash table HT.  MAPFUN is
   called with three arguments: the key, the value, and MAPARG.

   It is undefined what happens if you add or remove entries in the
   hash table while hash_table_map is running.  The exception is the
   entry you're currently mapping over; you may remove or change that
   entry.  */

void
hash_table_map (struct hash_table *ht,
		int (*mapfun) (void *, void *, void *),
		void *maparg)
{
  struct mapping *mp  = ht->mappings;
  struct mapping *end = ht->mappings + ht->size;

  for (; mp < end; mp++)
    if (NON_EMPTY (mp))
      {
	void *key;
      repeat:
	key = mp->key;
	if (mapfun (key, mp->value, maparg))
	  return;
	/* hash_table_remove might have moved the adjacent
	   mappings. */
	if (mp->key != key && NON_EMPTY (mp))
	  goto repeat;
      }
}

/* Return the number of elements in the hash table.  This is not the
   same as the physical size of the hash table, which is always
   greater than the number of elements.  */

int
hash_table_count (const struct hash_table *ht)
{
  return ht->count;
}

/* Functions from this point onward are meant for convenience and
   don't strictly belong to this file.  However, this is as good a
   place for them as any.  */

/* Guidelines for creating custom hash and test functions:

   - The test function returns non-zero for keys that are considered
     "equal", zero otherwise.

   - The hash function returns a number that represents the
     "distinctness" of the object.  In more precise terms, it means
     that for any two objects that test "equal" under the test
     function, the hash function MUST produce the same result.

     This does not mean that all different objects must produce
     different values (that would be "perfect" hashing), only that
     non-distinct objects must produce the same values!  For instance,
     a hash function that returns 0 for any given object is a
     perfectly valid (albeit extremely bad) hash function.  A hash
     function that hashes a string by adding up all its characters is
     another example of a valid (but quite bad) hash function.

     It is not hard to make hash and test functions agree about
     equality.  For example, if the test function compares strings
     case-insensitively, the hash function can lower-case the
     characters when calculating the hash value.  That ensures that
     two strings differing only in case will hash the same.

   - If you care about performance, choose a hash function with as
     good "spreading" as possible.  A good hash function will use all
     the bits of the input when calculating the hash, and will react
     to even small changes in input with a completely different
     output.  Finally, don't make the hash function itself overly
     slow, because you'll be incurring a non-negligible overhead to
     all hash table operations.  */

/*
 * Support for hash tables whose keys are strings.
 *
 */
   
/* 31 bit hash function.  Taken from Gnome's glib, modified to use
   standard C types.

   We used to use the popular hash function from the Dragon Book, but
   this one seems to perform much better.  */

static unsigned long
hash_string (const void *key)
{
  const char *p = key;
  unsigned int h = *p;
  
  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;
  
  return h;
}

/* Frontend for strcmp usable for hash tables. */

static int
cmp_string (const void *s1, const void *s2)
{
  return !strcmp ((const char *)s1, (const char *)s2);
}

/* Return a hash table of preallocated to store at least ITEMS items
   suitable to use strings as keys.  */

struct hash_table *
make_string_hash_table (int items)
{
  return hash_table_new (items, hash_string, cmp_string);
}

/*
 * Support for hash tables whose keys are strings, but which are
 * compared case-insensitively.
 *
 */

/* Like hash_string, but produce the same hash regardless of the case. */

static unsigned long
hash_string_nocase (const void *key)
{
  const char *p = key;
  unsigned int h = TOLOWER (*p);
  
  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + TOLOWER (*p);
  
  return h;
}

/* Like string_cmp, but doing case-insensitive compareison. */

static int
string_cmp_nocase (const void *s1, const void *s2)
{
  return !strcasecmp ((const char *)s1, (const char *)s2);
}

/* Like make_string_hash_table, but uses string_hash_nocase and
   string_cmp_nocase.  */

struct hash_table *
make_nocase_string_hash_table (int items)
{
  return hash_table_new (items, hash_string_nocase, string_cmp_nocase);
}

/* Hashing of numeric values, such as pointers and integers.

   This implementation is the Robert Jenkins' 32 bit Mix Function,
   with a simple adaptation for 64-bit values.  It offers excellent
   spreading of values and doesn't need to know the hash table size to
   work (unlike the very popular Knuth's multiplication hash).  */

unsigned long
hash_pointer (const void *ptr)
{
  unsigned long key = (unsigned long)ptr;
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);
#if SIZEOF_LONG > 4
  key += (key << 44);
  key ^= (key >> 54);
  key += (key << 36);
  key ^= (key >> 41);
  key += (key << 42);
  key ^= (key >> 34);
  key += (key << 39);
  key ^= (key >> 44);
#endif
  return key;
}

static int
cmp_pointer (const void *ptr1, const void *ptr2)
{
  return ptr1 == ptr2;
}

#ifdef TEST

#include <stdio.h>
#include <string.h>

int
print_hash_table_mapper (void *key, void *value, void *count)
{
  ++*(int *)count;
  printf ("%s: %s\n", (const char *)key, (char *)value);
  return 0;
}

void
print_hash (struct hash_table *sht)
{
  int debug_count = 0;
  hash_table_map (sht, print_hash_table_mapper, &debug_count);
  assert (debug_count == sht->count);
}

int
main (void)
{
  struct hash_table *ht = make_string_hash_table (0);
  char line[80];
  while ((fgets (line, sizeof (line), stdin)))
    {
      int len = strlen (line);
      if (len <= 1)
	continue;
      line[--len] = '\0';
      if (!hash_table_contains (ht, line))
	hash_table_put (ht, strdup (line), "here I am!");
#if 1
      if (len % 5 == 0)
	{
	  char *line_copy;
	  if (hash_table_get_pair (ht, line, &line_copy, NULL))
	    {
	      hash_table_remove (ht, line);
	      xfree (line_copy);
	    }
	}
#endif
    }
#if 0
  print_hash (ht);
#endif
#if 1
  printf ("%d %d\n", ht->count, ht->size);
#endif
  return 0;
}
#endif /* TEST */
