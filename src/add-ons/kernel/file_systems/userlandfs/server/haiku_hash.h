/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef USERLAND_FS_HAIKU_HASH_H
#define USERLAND_FS_HAIKU_HASH_H

#include <SupportDefs.h>

namespace UserlandFS {
namespace HaikuKernelEmu {

// can be allocated on the stack
typedef struct hash_iterator {
	void *current;
	int bucket;
} hash_iterator;

typedef struct hash_table hash_table;

struct hash_table *hash_init(uint32 table_size, int next_ptr_offset,
	int compare_func(void *element, const void *key),
	uint32 hash_func(void *element, const void *key, uint32 range));
int hash_uninit(struct hash_table *table);
status_t hash_insert(struct hash_table *table, void *_element);
status_t hash_remove(struct hash_table *table, void *_element);
void *hash_remove_first(struct hash_table *table, uint32 *_cookie);
void *hash_find(struct hash_table *table, void *e);
void *hash_lookup(struct hash_table *table, const void *key);
struct hash_iterator *hash_open(struct hash_table *table, struct hash_iterator *i);
void hash_close(struct hash_table *table, struct hash_iterator *i, bool free_iterator);
void *hash_next(struct hash_table *table, struct hash_iterator *i);
void hash_rewind(struct hash_table *table, struct hash_iterator *i);

/* function ptrs must look like this:
 *
 * uint32 hash_func(void *e, const void *key, uint32 range);
 *		hash function should calculate hash on either e or key,
 *		depending on which one is not NULL
 * int compare_func(void *e, const void *key);
 *		compare function should compare the element with
 *		the key, returning 0 if equal, other if not
 *		NOTE: compare func can be null, in which case the hash
 *		code will compare the key pointer with the target
 * ToDo: check this!
 */

uint32 hash_hash_string(const char *str);

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS

#endif	/* USERLAND_FS_HAIKU_HASH_H */
