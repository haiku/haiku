/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KHASH_H
#define _KERNEL_KHASH_H

#include <pools.h>

typedef struct hash_entry			hash_entry;
typedef struct new_hash_table		new_hash_table;
typedef struct hash_index			hash_index;

struct hash_entry {
	hash_entry      *next;
	int              hash;
	const void      *key;
	ssize_t          klen;
	const void      *val;
};

struct hash_index {
	new_hash_table	*nh;
	hash_entry	    *this_idx;
	hash_entry      *next;
	int		         index;
};

struct new_hash_table {
	hash_entry	   **array;
	hash_index	     iterator;
	int		         count;	
	int		         max;
	struct pool_ctl	*pool;
};

new_hash_table *hash_make(void);
void           *hash_get(new_hash_table *, const void *, ssize_t);
void            hash_set(new_hash_table *, const void *, ssize_t , const void *);


struct hash_iterator {
	void *ptr;
	int bucket;
};

void *hash_init(unsigned int table_size, int next_ptr_offset,
	int compare_func(void *a, const void *key),
	unsigned int hash_func(void *a, const void *key, unsigned int range));
int hash_uninit(void *_hash_table);
int hash_insert(void *_hash_table, void *_elem);
int hash_remove(void *_hash_table, void *_elem);
void *hash_find(void *_hash_table, void *e);
void *hash_lookup(void *_hash_table, const void *key);
struct hash_iterator *hash_open(void *_hash_table, struct hash_iterator *i);
void hash_close(void *_hash_table, struct hash_iterator *i, bool free_iterator);
void *hash_next(void *_hash_table, struct hash_iterator *i);
void hash_rewind(void *_hash_table, struct hash_iterator *i);

/* function ptrs must look like this:
	// hash function should calculate hash on either e or key,
	// depending on which one is not NULL
unsigned int hash_func(void *e, const void *key, unsigned int range);
	// compare function should compare the element with
	// the key, returning 0 if equal, other if not
	// NOTE: compare func can be null, in which case the hash
	// code will compare the key pointer with the target
int compare_func(void *e, const void *key);
*/

unsigned int hash_hash_str( const char *str );

#endif

