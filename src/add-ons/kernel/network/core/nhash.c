/* nhash.c
 * net hash
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_malloc.h"
#include "nhash.h"

#define MAX_INITIAL 15;

net_hash *nhash_make(void)
{
	net_hash *nn;
	
	 nn = (net_hash *)malloc(sizeof(net_hash));

	if (!nn)
		return NULL;

	nn->count = 0;
	nn->max = MAX_INITIAL;

	nn->array = (net_hash_entry **)malloc(sizeof(net_hash_entry) * (nn->max + 1));
	memset(nn->array, 0, sizeof(net_hash_entry) * (nn->max +1));
	pool_init(&nn->pool, sizeof(net_hash_entry));
	if (!nn->pool)
		return NULL;
	return nn;
}

net_hash_index *nhash_next(net_hash_index *hi)
{
	hi->this = hi->next;
	while (!hi->this) {
		if (hi->index > hi->nh->max)
			return NULL;
		hi->this = hi->nh->array[hi->index++];
	}
	hi->next = hi->this->next;
	return hi;
}

net_hash_index *nhash_first(net_hash *nh)
{
        net_hash_index *hi = &nh->iterator;
        hi->nh = nh;
        hi->index = 0;
        hi->this = hi->next = NULL;
        return nhash_next(hi);
}

static void expand_array(net_hash *nh)
{
	net_hash_index *hi;
	net_hash_entry **new_array;
	int new_max = nh->max * 2 +1;
	int i;

	new_array = (net_hash_entry **)malloc(sizeof(net_hash_entry) * new_max);
	memset(new_array, 0, sizeof(net_hash_entry) * new_max);
	for (hi = nhash_first(nh); hi; hi = nhash_next(hi)) {
		i = hi->this->hash & new_max;
		hi->this->next = new_array[i];
		new_array[i] = hi->this;
	}
	free(nh->array);
	nh->array = new_array;
	nh->max = new_max;
}

void nhash_this(net_hash_index *hi, const void **key, ssize_t *klen,
		void **val)
{
	if (key)	*key = hi->this->key;
	if (klen)	*klen = hi->this->klen;
	if (val)	*val = (void*)hi->this->val;
}

static net_hash_entry **find_entry(net_hash *nh, const void *key,
				   ssize_t klen, const void *val)
{
	net_hash_entry **hep;
	net_hash_entry *he;
	const unsigned char *p;
	int hash = 0;
	ssize_t i;

	if (!nh)
		return NULL;

	for (p=key, i=klen; i; i--, p++) 
		hash = hash * 33 + *p;

	for (hep = &nh->array[hash & nh->max], he = *hep; he; 
				hep = &he->next, he = *hep) {
		if (he->hash == hash && he->klen == klen
			&& memcmp(he->key, key, klen) == 0) {
				break;
		}
	}

	if (he || !val)
		return hep;

	/* add a new linked-list entry */
	he = (net_hash_entry *)pool_get(nh->pool);
	he->next = NULL;
	he->hash = hash;
	he->key  = key;
	he->klen = klen;
	he->val = val;
	*hep = he;
	nh->count++;
	return hep;
}

void *nhash_get(net_hash *nh, const void *key, ssize_t klen)
{
	net_hash_entry *he;
	he = *find_entry(nh, key, klen, NULL);
	if (he)
		return (void*)he->val;
	else
		return NULL;
}

void nhash_set(net_hash *nh, const void *key, ssize_t klen, const void *val)
{
	net_hash_entry **hep;
	net_hash_entry *old;
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

