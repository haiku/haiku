/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <BeBuild.h>

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
	struct lock		lock;
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

extern _IMPEXP_KERNEL int   init_block_cache(int max_blocks, int flags);
extern _IMPEXP_KERNEL void  shutdown_block_cache(void);

extern _IMPEXP_KERNEL void  force_cache_flush(int dev, int prefer_log_blocks);
extern _IMPEXP_KERNEL int   flush_blocks(int dev, off_t bnum, int nblocks);
extern _IMPEXP_KERNEL int   flush_device(int dev, int warn_locked);

extern _IMPEXP_KERNEL int   init_cache_for_device(int fd, off_t max_blocks);
extern _IMPEXP_KERNEL int   remove_cached_device_blocks(int dev, int allow_write);

extern _IMPEXP_KERNEL void *get_block(int dev, off_t bnum, int bsize);
extern _IMPEXP_KERNEL void *get_empty_block(int dev, off_t bnum, int bsize);
extern _IMPEXP_KERNEL int   release_block(int dev, off_t bnum);
extern _IMPEXP_KERNEL int   mark_blocks_dirty(int dev, off_t bnum, int nblocks);


extern _IMPEXP_KERNEL int  cached_read(int dev, off_t bnum, void *data, off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  cached_write(int dev, off_t bnum, const void *data,
				  off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  cached_write_locked(int dev, off_t bnum, const void *data,
						 off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  set_blocks_info(int dev, off_t *blocks, int nblocks,
					 void (*func)(off_t bnum, size_t nblocks, void *arg),
					 void *arg);


extern _IMPEXP_KERNEL size_t read_phys_blocks (int fd, off_t bnum, void *data, uint num_blocks, int bsize);
extern _IMPEXP_KERNEL size_t write_phys_blocks(int fd, off_t bnum, void *data, uint num_blocks, int bsize);

#endif /* _CACHE_H_ */
