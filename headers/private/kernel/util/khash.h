/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_UTIL_KHASH_H
#define _KERNEL_UTIL_KHASH_H


#include <SupportDefs.h>

// The use of offsetof() on non-PODs is invalid. Since many structs use
// templated members (i.e. DoublyLinkedList) which makes them non-PODs we
// can't use offsetof() anymore. This macro does the same, but requires an
// instance of the object in question.
#define offset_of_member(OBJECT, MEMBER) \
	((size_t)((char*)&OBJECT.MEMBER - (char*)&OBJECT))

// can be allocated on the stack
typedef struct hash_iterator {
	void *current;
	int bucket;
} hash_iterator;

typedef struct hash_table hash_table;

#ifdef __cplusplus
extern "C" {
#endif

struct hash_table *hash_init(uint32 table_size, int next_ptr_offset,
	int compare_func(void *element, const void *key),
	uint32 hash_func(void *element, const void *key, uint32 range));
int hash_uninit(struct hash_table *table);
status_t hash_insert(struct hash_table *table, void *_element);
status_t hash_insert_grow(struct hash_table *table, void *_element);
status_t hash_remove(struct hash_table *table, void *_element);
void hash_remove_current(struct hash_table *table, struct hash_iterator *iterator);
void *hash_remove_first(struct hash_table *table, uint32 *_cookie);
void *hash_find(struct hash_table *table, void *e);
void *hash_lookup(struct hash_table *table, const void *key);
struct hash_iterator *hash_open(struct hash_table *table, struct hash_iterator *i);
void hash_close(struct hash_table *table, struct hash_iterator *i, bool free_iterator);
void *hash_next(struct hash_table *table, struct hash_iterator *i);
void hash_rewind(struct hash_table *table, struct hash_iterator *i);
uint32 hash_count_elements(struct hash_table *table);
uint32 hash_count_used_slots(struct hash_table *table);
void hash_dump_table(struct hash_table* table);

/* function pointers must look like this:
 *
 * uint32 hash_func(void *e, const void *key, uint32 range);
 *		hash function should calculate hash on either e or key,
 *		depending on which one is not NULL - they also need
 *		to make sure the returned value is within range.
 * int compare_func(void *e, const void *key);
 *		compare function should compare the element with
 *		the key, returning 0 if equal, other if not
 */

uint32 hash_hash_string(const char *str);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_UTIL_KHASH_H */
