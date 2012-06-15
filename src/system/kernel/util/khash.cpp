/*
 * Copyright 2002-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

//! Generic hash table


#include <KernelExport.h>
#include <debug.h>
#include <util/khash.h>

#include <stdlib.h>
#include <string.h>

#define TRACE_HASH 0
#if TRACE_HASH
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// TODO: the hashtable is not expanded when necessary (no load factor, nothing)
//		resizing should be optional, though, in case the hash is used at times
//		that forbid resizing.

struct hash_table {
	struct hash_element **table;
	int		next_ptr_offset;
	uint32	table_size;
	int		num_elements;
	int		flags;
	int		(*compare_func)(void *e, const void *key);
	uint32	(*hash_func)(void *e, const void *key, uint32 range);
};

// XXX gross hack
#define NEXT_ADDR(t, e) ((void *)(((unsigned long)(e)) + (t)->next_ptr_offset))
#define NEXT(t, e) ((void *)(*(unsigned long *)NEXT_ADDR(t, e)))
#define PUT_IN_NEXT(t, e, val) (*(unsigned long *)NEXT_ADDR(t, e) = (long)(val))


const uint32 kPrimes [] = {
	13, 31, 61, 127, 251,
	509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139,
	524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859,
	134217689, 268435399, 536870909, 1073741789, 2147483647, 0
};


static uint32
get_prime_table_size(uint32 size)
{
	int i;
	for (i = 0; kPrimes[i] != 0; i++) {
		if (kPrimes[i] > size)
			return kPrimes[i];
	}

	return kPrimes[i - 1];
}


static inline void *
next_element(hash_table *table, void *element)
{
	// ToDo: should we use this instead of the NEXT() macro?
	return (void *)(*(unsigned long *)NEXT_ADDR(table, element));
}


static status_t
hash_grow(struct hash_table *table)
{
	uint32 newSize = get_prime_table_size(table->num_elements);
	struct hash_element **newTable;
	uint32 index;

	if (table->table_size >= newSize)
		return B_OK;

	newTable = (struct hash_element **)malloc(sizeof(void *) * newSize);
	if (newTable == NULL)
		return B_NO_MEMORY;

	memset(newTable, 0, sizeof(void *) * newSize);

	// rehash all the entries and add them to the new table
	for (index = 0; index < table->table_size; index++) {
		void *element;
		void *next;

		for (element = table->table[index]; element != NULL; element = next) {
			uint32 hash = table->hash_func(element, NULL, newSize);
			next = NEXT(table, element);
			PUT_IN_NEXT(table, element, newTable[hash]);
			newTable[hash] = (struct hash_element *)element;
		}
	}

	free(table->table);

	table->table = newTable;
	table->table_size = newSize;

	TRACE(("hash_grow: grown table %p, new size %lu\n", table, newSize));
	return B_OK;
}


//	#pragma mark - kernel private API


struct hash_table *
hash_init(uint32 tableSize, int nextPointerOffset,
	int compareFunc(void *e, const void *key),
	uint32 hashFunc(void *e, const void *key, uint32 range))
{
	struct hash_table *t;
	uint32 i;

	tableSize = get_prime_table_size(tableSize);

	if (compareFunc == NULL || hashFunc == NULL) {
		dprintf("hash_init() called with NULL function pointer\n");
		return NULL;
	}

	t = (struct hash_table *)malloc(sizeof(struct hash_table));
	if (t == NULL)
		return NULL;

	t->table = (struct hash_element **)malloc(sizeof(void *) * tableSize);
	if (t->table == NULL) {
		free(t);
		return NULL;
	}

	for (i = 0; i < tableSize; i++)
		t->table[i] = NULL;

	t->table_size = tableSize;
	t->next_ptr_offset = nextPointerOffset;
	t->flags = 0;
	t->num_elements = 0;
	t->compare_func = compareFunc;
	t->hash_func = hashFunc;

	TRACE(("hash_init: created table %p, next_ptr_offset %d, compare_func %p, hash_func %p\n",
		t, nextPointerOffset, compareFunc, hashFunc));

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


status_t
hash_insert(struct hash_table *table, void *element)
{
	uint32 hash;

	ASSERT(table != NULL && element != NULL);
	TRACE(("hash_insert: table %p, element %p\n", table, element));

	hash = table->hash_func(element, NULL, table->table_size);
	PUT_IN_NEXT(table, element, table->table[hash]);
	table->table[hash] = (struct hash_element *)element;
	table->num_elements++;

	return B_OK;
}


status_t
hash_insert_grow(struct hash_table *table, void *element)
{
	uint32 hash;

	ASSERT(table != NULL && element != NULL);
	TRACE(("hash_insert_grow: table %p, element %p\n", table, element));

	hash = table->hash_func(element, NULL, table->table_size);
	PUT_IN_NEXT(table, element, table->table[hash]);
	table->table[hash] = (struct hash_element *)element;
	table->num_elements++;

	if ((uint32)table->num_elements > table->table_size) {
		//dprintf("hash_insert: table has grown too much: %d in %d\n", table->num_elements, (int)table->table_size);
		hash_grow(table);
	}

	return B_OK;
}


status_t
hash_remove(struct hash_table *table, void *_element)
{
	uint32 hash = table->hash_func(_element, NULL, table->table_size);
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

			return B_OK;
		}
	}

	return B_ERROR;
}


void
hash_remove_current(struct hash_table *table, struct hash_iterator *iterator)
{
	uint32 index = iterator->bucket;
	void *element;
	void *lastElement = NULL;

	if (iterator->current == NULL || (element = table->table[index]) == NULL) {
		panic("hash_remove_current(): invalid iteration state");
		return;
	}

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

		lastElement = element;
		element = NEXT(table, element);
	}

	panic("hash_remove_current(): current element not found!");
}


void *
hash_remove_first(struct hash_table *table, uint32 *_cookie)
{
	uint32 index;

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
	uint32 hash = table->hash_func(searchedElement, NULL, table->table_size);
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
	uint32 hash = table->hash_func(NULL, key, table->table_size);
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
	uint32 index;

restart:
	if (iterator->current == NULL) {
		// get next bucket
		for (index = (uint32)(iterator->bucket + 1); index < table->table_size; index++) {
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


uint32
hash_hash_string(const char *string)
{
	uint32 hash = 0;
	char c;

	// we assume hash to be at least 32 bits
	while ((c = *string++) != 0) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= c;
	}

	return hash;
}


uint32
hash_count_elements(struct hash_table *table)
{
	return table->num_elements;
}


uint32
hash_count_used_slots(struct hash_table *table)
{
	uint32 usedSlots = 0;
	uint32 i;
	for (i = 0; i < table->table_size; i++) {
		if (table->table[i] != NULL)
			usedSlots++;
	}

	return usedSlots;
}


void
hash_dump_table(struct hash_table* table)
{
	uint32 i;

	dprintf("hash table %p, table size: %" B_PRIu32 ", elements: %u\n", table,
		table->table_size, table->num_elements);

	for (i = 0; i < table->table_size; i++) {
		struct hash_element* element = table->table[i];
		if (element != NULL) {
			dprintf("%6" B_PRIu32 ":", i);
			while (element != NULL) {
				dprintf(" %p", element);
				element = (hash_element*)NEXT(table, element);
			}
			dprintf("\n");
		}
	}
}
