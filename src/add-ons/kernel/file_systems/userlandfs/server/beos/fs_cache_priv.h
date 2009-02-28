/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef USERLAND_FS_BEOS_FS_CACHE_PRIV_H
#define USERLAND_FS_BEOS_FS_CACHE_PRIV_H

#include "lock.h"

#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif

typedef struct hash_ent {
	int              dev;
	off_t            bnum;
	off_t            hash_val;
	void            *data;
	struct hash_ent *next;
} hash_ent;


typedef struct hash_table {
    hash_ent **table;
    int        max;
    int        mask;          /* == max - 1 */
    int        num_elements;
} hash_table;


#define HT_DEFAULT_MAX   128


typedef struct cache_ent {
	int               dev;
	off_t             block_num;
	int               bsize;
	volatile int      flags;

	void             *data;
	void             *clone;         /* copy of data by set_block_info() */
	int               lock;

	void            (*func)(off_t bnum, size_t num_blocks, void *arg);
	off_t             logged_bnum;
	void             *arg;

	struct cache_ent *next,          /* points toward mru end of list */
	                 *prev;          /* points toward lru end of list */

} cache_ent;

#define CE_NORMAL    0x0000     /* a nice clean pristine page */
#define CE_DIRTY     0x0002     /* needs to be written to disk */
#define CE_BUSY      0x0004     /* this block has i/o happening, don't touch it */


typedef struct cache_ent_list {
	cache_ent *lru;              /* tail of the list */
	cache_ent *mru;              /* head of the list */
} cache_ent_list;


typedef struct block_cache {
	struct beos_lock lock;
	int			 	flags;
    int				cur_blocks;
	int				max_blocks;
	hash_table		ht;

	cache_ent_list	normal,       /* list of "normal" blocks (clean & dirty) */
					locked;       /* list of clean and locked blocks */
} block_cache;

#if 0   /* XXXdbg -- need to deal with write through caches */
#define DC_WRITE_THROUGH    0x0001  /* cache is write-through (for floppies) */
#endif

#define ALLOW_WRITES  1
#define NO_WRITES     0

#endif /* USERLAND_FS_BEOS_FS_CACHE_PRIV_H */
