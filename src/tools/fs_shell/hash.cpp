/* Generic hash table
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "hash.h"

#include <stdlib.h>

#include "fssh_errors.h"
#include "fssh_kernel_export.h"


#undef TRACE
#define TRACE_HASH 0
#if TRACE_HASH
#	define TRACE(x) fssh_dprintf x
#else
#	define TRACE(x) ;
#endif

#undef ASSERT
#define ASSERT(x)


namespace FSShell {


// TODO: the hashtable is not expanded when necessary (no load factor, nothing)
//		resizing should be optional, though, in case the hash is used at times
//		that forbid resizing.

struct hash_table {
	struct		hash_element **table;
	int			next_ptr_offset;
	uint32_t	table_size;
	int			num_elements;
	int			flags;
	int			(*compare_func)(void *e, const void *key);
	uint32_t	(*hash_func)(void *e, const void *key, uint32_t range);
};

// XXX gross hack
#define NEXT_ADDR(t, e) ((void *)(((unsigned long)(e)) + (t)->next_ptr_offset))
#define NEXT(t, e) ((void *)(*(unsigned long *)NEXT_ADDR(t, e)))
#define PUT_IN_NEXT(t, e, val) (*(unsigned long *)NEXT_ADDR(t, e) = (long)(val))


static inline void *
next_element(hash_table *table, void *element)
{
	// ToDo: should we use this instead of the NEXT() macro?
	return (void *)(*(unsigned long *)NEXT_ADDR(table, element));
}


struct hash_table *
hash_init(uint32_t table_size, int next_ptr_offset,
	int compare_func(void *e, const void *key),
	uint32_t hash_func(void *e, const void *key, uint32_t range))
{
	struct hash_table *t;
	unsigned int i;

	if (compare_func == NULL || hash_func == NULL) {
		fssh_dprintf("hash_init() called with NULL function pointer\n");
		return NULL;
	}

	t = (struct hash_table *)malloc(sizeof(struct hash_table));
	if (t == NULL)
		return NULL;

	t->table = (struct hash_element **)malloc(sizeof(void *) * table_size);
	if (t->table == NULL) {
		free(t);
		return NULL;
	}

	for (i = 0; i < table_size; i++)
		t->table[i] = NULL;

	t->table_size = table_size;
	t->next_ptr_offset = next_ptr_offset;
	t->flags = 0;
	t->num_elements = 0;
	t->compare_func = compare_func;
	t->hash_func = hash_func;

	TRACE(("hash_init: created table %p, next_ptr_offset %d, compare_func %p, hash_func %p\n",
		t, next_ptr_offset, compare_func, hash_func));

	return t;
}


int
hash_uninit(struct hash_table *table)
{
	ASSERT(table->num_elements == 0);

	free(table->table);
	free(table);

	return 0;
}


fssh_status_t
hash_insert(struct hash_table *table, void *element)
{
	uint32_t hash;

	ASSERT(table != NULL && element != NULL);
	TRACE(("hash_insert: table 0x%x, element 0x%x\n", table, element));

	hash = table->hash_func(element, NULL, table->table_size);
	PUT_IN_NEXT(table, element, table->table[hash]);
	table->table[hash] = (struct hash_element *)element;
	table->num_elements++;

	// ToDo: resize hash table if it's grown too much!

	return FSSH_B_OK;
}


fssh_status_t
hash_remove(struct hash_table *table, void *_element)
{
	uint32_t hash = table->hash_func(_element, NULL, table->table_size);
	void *element, *lastElement = NULL;

	for (element = table->table[hash]; element != NULL;
			lastElement = element, element = NEXT(table, element)) {
		if (element == _element) {
			if (lastElement != NULL) {
				// connect the previous entry with the next one
				PUT_IN_NEXT(table, lastElement, NEXT(table, element));
			} else
				table->table[hash] = (struct hash_element *)NEXT(table, element);
			table->num_elements--;

			return FSSH_B_OK;
		}
	}

	return FSSH_B_ERROR;
}


void
hash_remove_current(struct hash_table *table, struct hash_iterator *iterator)
{
	uint32_t index = iterator->bucket;
	void *element;

	if (iterator->current == NULL)
		fssh_panic("hash_remove_current() called too early.");

	for (element = table->table[index]; index < table->table_size; index++) {
		void *lastElement = NULL;

		while (element != NULL) {
			if (element == iterator->current) {
				iterator->current = lastElement;

				if (lastElement != NULL) {
					// connect the previous entry with the next one
					PUT_IN_NEXT(table, lastElement, NEXT(table, element));
				} else {
					table->table[index] = (struct hash_element *)NEXT(table,
						element);
				}

				table->num_elements--;
				return;
			}

			element = NEXT(table, element);
		}
	}
}


void *
hash_remove_first(struct hash_table *table, uint32_t *_cookie)
{
	uint32_t index;

	for (index = _cookie ? *_cookie : 0; index < table->table_size; index++) {
		void *element = table->table[index];
		if (element != NULL) {
			// remove the first element we find
			table->table[index] = (struct hash_element *)NEXT(table, element);
			table->num_elements--;
			if (_cookie)
				*_cookie = index;
			return element;
		}
	}

	return NULL;
}


void *
hash_find(struct hash_table *table, void *searchedElement)
{
	uint32_t hash = table->hash_func(searchedElement, NULL, table->table_size);
	void *element;

	for (element = table->table[hash]; element != NULL; element = NEXT(table, element)) {
		if (element == searchedElement)
			return element;
	}

	return NULL;
}


void *
hash_lookup(struct hash_table *table, const void *key)
{
	uint32_t hash = table->hash_func(NULL, key, table->table_size);
	void *element;

	for (element = table->table[hash]; element != NULL; element = NEXT(table, element)) {
		if (table->compare_func(element, key) == 0)
			return element;
	}

	return NULL;
}


struct hash_iterator *
hash_open(struct hash_table *table, struct hash_iterator *iterator)
{
	if (iterator == NULL) {
		iterator = (struct hash_iterator *)malloc(sizeof(struct hash_iterator));
		if (iterator == NULL)
			return NULL;
	}

	hash_rewind(table, iterator);

	return iterator;
}


void
hash_close(struct hash_table *table, struct hash_iterator *iterator, bool freeIterator)
{
	if (freeIterator)
		free(iterator);
}


void
hash_rewind(struct hash_table *table, struct hash_iterator *iterator)
{
	iterator->current = NULL;
	iterator->bucket = -1;
}


void *
hash_next(struct hash_table *table, struct hash_iterator *iterator)
{
	uint32_t index;

restart:
	if (iterator->current == NULL) {
		// get next bucket
		for (index = (uint32_t)(iterator->bucket + 1); index < table->table_size; index++) {
			if (table->table[index]) {
				iterator->bucket = index;
				iterator->current = table->table[index];
				break;
			}
		}
	} else {
		iterator->current = NEXT(table, iterator->current);
		if (!iterator->current)
			goto restart;
	}

	return iterator->current;
}


uint32_t
hash_hash_string(const char *string)
{
	uint32_t hash = 0;
	char c;

	// we assume hash to be at least 32 bits
	while ((c = *string++) != 0) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= c;
	}

	return hash;
}

}	// namespace FSShell
