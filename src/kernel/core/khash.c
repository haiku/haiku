/* Generic hash table
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <malloc.h>
#include <debug.h>
#include <Errors.h>
#include <string.h>
#include <khash.h>

// ToDo: this file apparently contains two different hash implementations
//		get rid of one of them, and update the external code.

struct hash_table {
	struct hash_elem **table;
	int				next_ptr_offset;
	unsigned int	table_size;
	int				num_elems;
	int				flags;
	int				(*compare_func)(void *e, const void *key);
	unsigned int	(*hash_func)(void *e, const void *key, unsigned int range);
};

// XXX gross hack
#define NEXT_ADDR(t, e) ((void *)(((unsigned long)(e)) + (t)->next_ptr_offset))
#define NEXT(t, e) ((void *)(*(unsigned long *)NEXT_ADDR(t, e)))
#define PUT_IN_NEXT(t, e, val) (*(unsigned long *)NEXT_ADDR(t, e) = (long)(val))


void *
hash_init(unsigned int table_size, int next_ptr_offset,
	int compare_func(void *e, const void *key),
	unsigned int hash_func(void *e, const void *key, unsigned int range))
{
	struct hash_table *t;
	unsigned int i;

	t = (struct hash_table *)malloc(sizeof(struct hash_table));
	if (t == NULL)
		return NULL;

	t->table = (struct hash_elem **)malloc(sizeof(void *) * table_size);
	for (i = 0; i<table_size; i++)
		t->table[i] = NULL;
	t->table_size = table_size;
	t->next_ptr_offset = next_ptr_offset;
	t->flags = 0;
	t->num_elems = 0;
	t->compare_func = compare_func;
	t->hash_func = hash_func;

//	dprintf("hash_init: created table 0x%x, next_ptr_offset %d, compare_func 0x%x, hash_func 0x%x\n",
//		t, next_ptr_offset, compare_func, hash_func);

	return t;
}


int
hash_uninit(void *_hash_table)
{
	struct hash_table *t = (struct hash_table *)_hash_table;

#if 0
	if(t->num_elems > 0) {
		return -1;
	}
#endif

	free(t->table);
	free(t);

	return 0;
}


int
hash_insert(void *_hash_table, void *e)
{
	struct hash_table *t = (struct hash_table *)_hash_table;
	unsigned int hash;

//	dprintf("hash_insert: table 0x%x, element 0x%x\n", t, e);

	hash = t->hash_func(e, NULL, t->table_size);
	PUT_IN_NEXT(t, e, t->table[hash]);
	t->table[hash] = (struct hash_elem *)e;
	t->num_elems++;

	return 0;
}


int hash_remove(void *_hash_table, void *e)
{
	struct hash_table *t = (struct hash_table *)_hash_table;
	void *i, *last_i;
	unsigned int hash;

	hash = t->hash_func(e, NULL, t->table_size);
	last_i = NULL;
	for (i = t->table[hash]; i != NULL; last_i = i, i = NEXT(t, i)) {
		if (i == e) {
			if (last_i != NULL)
				PUT_IN_NEXT(t, last_i, NEXT(t, i));
			else
				t->table[hash] = (struct hash_elem *)NEXT(t, i);
			t->num_elems--;
			return B_NO_ERROR;
		}
	}

	return B_ERROR;
}


void *
hash_find(void *_hash_table, void *e)
{
	struct hash_table *t = (struct hash_table *)_hash_table;
	void *i;
	unsigned int hash;

	hash = t->hash_func(e, NULL, t->table_size);
	for (i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if (i == e)
			return i;
	}

	return NULL;
}


void *
hash_lookup(void *_hash_table, const void *key)
{
	struct hash_table *t = (struct hash_table *)_hash_table;
	void *i;
	unsigned int hash;

	if (t->compare_func == NULL)
		return NULL;

	hash = t->hash_func(NULL, key, t->table_size);
	for (i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if (t->compare_func(i, key) == 0)
			return i;
	}

	return NULL;
}


struct hash_iterator *
hash_open(void *_hash_table, struct hash_iterator *i)
{
	struct hash_table *t = (struct hash_table *)_hash_table;

	if (i == NULL) {
		i = (struct hash_iterator *)malloc(sizeof(struct hash_iterator));
		if (i == NULL)
			return NULL;
	}

	hash_rewind(t, i);

	return i;
}


void
hash_close(void *_hash_table, struct hash_iterator *i, bool freeIterator)
{
	if (freeIterator)
		free(i);
}


void
hash_rewind(void *_hash_table, struct hash_iterator *i)
{
	i->ptr = NULL;
	i->bucket = -1;
}


void *
hash_next(void *_hash_table, struct hash_iterator *i)
{
	struct hash_table *t = (struct hash_table *)_hash_table;
	unsigned int index;

restart:
	if (!i->ptr) {
		for (index = (unsigned int)(i->bucket + 1); index < t->table_size; index++) {
			if (t->table[index]) {
				i->bucket = index;
				i->ptr = t->table[index];
				break;
			}
		}
	} else {
		i->ptr = NEXT(t, i->ptr);
		if (!i->ptr)
			goto restart;
	}

	return i->ptr;
}


unsigned int
hash_hash_str( const char *str )
{
	char ch;
	unsigned int hash = 0;

	// we assume hash to be at least 32 bits
	while ((ch = *str++) != 0) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= ch;
	}

	return hash;
}

#define MAX_INITIAL 15;

/*
static void nhash_this(hash_table_index *hi, const void **key, ssize_t *klen,
                       void **val)
{
	if (key)	*key = hi->this->key;
	if (klen)	*klen = hi->this->klen;
	if (val)	*val = (void*)hi->this->val;
}
*/


new_hash_table *
hash_make(void)
{
	new_hash_table *nn;

	nn = (new_hash_table *)malloc(sizeof(new_hash_table));
	if (!nn)
		return NULL;

	nn->count = 0;
	nn->max = MAX_INITIAL;

	nn->array = (hash_entry **)malloc(sizeof(hash_entry) * (nn->max + 1));
	memset(nn->array, 0, sizeof(hash_entry) * (nn->max +1));
	pool_init(&nn->pool, sizeof(hash_entry));
	if (!nn->pool)
		return NULL;
	return nn;
}


static hash_index *
new_hash_next(hash_index *hi)
{
	hi->this_idx = hi->next;
	while (!hi->this_idx) {
		if (hi->index > hi->nh->max)
			return NULL;
		hi->this_idx = hi->nh->array[hi->index++];
	}
	hi->next = hi->this_idx->next;
	return hi;
}


static hash_index *
new_hash_first(new_hash_table *nh)
{
        hash_index *hi = &nh->iterator;
        hi->nh = nh;
        hi->index = 0;
        hi->this_idx = hi->next = NULL;
        return new_hash_next(hi);
}


static void
expand_array(new_hash_table *nh)
{
	hash_index *hi;
	hash_entry **new_array;
	int new_max = nh->max * 2 +1;
	int i;

	new_array = (hash_entry **)malloc(sizeof(hash_entry) * new_max);
	memset(new_array, 0, sizeof(hash_entry) * new_max);
	for (hi = new_hash_first(nh); hi; hi = new_hash_next(hi)) {
		i = hi->this_idx->hash & new_max;
		hi->this_idx->next = new_array[i];
		new_array[i] = hi->this_idx;
	}
	free(nh->array);
	nh->array = new_array;
	nh->max = new_max;
}


static hash_entry **
find_entry(new_hash_table *nh, const void *key, ssize_t klen, const void *val)
{
	hash_entry **hep;
	hash_entry *he;
	const unsigned char *p;
	int hash = 0;
	ssize_t i;

	if (!nh)
		return NULL;

	for (p = key, i = klen; i; i--, p++) 
		hash = hash * 33 + *p;

	for (hep = &nh->array[hash & nh->max], he = *hep; he; hep = &he->next, he = *hep) {
		if (he->hash == hash && he->klen == klen
			&& memcmp(he->key, key, klen) == 0) {
				break;
		}
	}

	if (he || !val)
		return hep;

	/* add a new linked-list entry */
	he = (hash_entry *)pool_get(nh->pool);
	he->next = NULL;
	he->hash = hash;
	he->key  = key;
	he->klen = klen;
	he->val = val;
	*hep = he;
	nh->count++;
	return hep;
}


void *
hash_get(new_hash_table *nh, const void *key, ssize_t klen)
{
	hash_entry *he;
	he = *find_entry(nh, key, klen, NULL);
	if (he)
		return (void*)he->val;

	return NULL;
}


void
hash_set(new_hash_table *nh, const void *key, ssize_t klen, const void *val)
{
	hash_entry **hep;
	hash_entry *old;
	hep = find_entry(nh, key, klen, val);

	if (*hep) {
		if (!val) {
			/* delete it */
			old = *hep;
			*hep = (*hep)->next;
			--nh->count;
			pool_put(nh->pool, old);
		} else {
			/* replace it */
			(*hep)->val = val;
			if (nh->count > nh->max) 
				expand_array(nh);
		}
	}
}
