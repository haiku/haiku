/* nhash.h
 */

#ifndef OBOS_NHASH_H
#define OBOS_NHASH_H

#include "pools.h"

typedef struct net_hash_entry	net_hash_entry;
typedef struct net_hash		net_hash;
typedef struct net_hash_index	net_hash_index;

struct net_hash_entry {
        net_hash_entry *next;
        int     	hash;
        const void      *key;
        ssize_t 	klen;
        const void      *val;
};

struct net_hash_index {
	net_hash	*nh;
	net_hash_entry	*this;
        net_hash_entry  *next;
	int		index;
};

struct net_hash {
	net_hash_entry	**array;
	net_hash_index	iterator;
	int		count;	
	int		max;
	struct pool_ctl	*pool;
};

net_hash *nhash_make(void);
void *nhash_get(net_hash *, const void *key, ssize_t klen);
void nhash_set(net_hash *, const void *, ssize_t , const void *);

#endif /* OBOS_NHASH_H */
