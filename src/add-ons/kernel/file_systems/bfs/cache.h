/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <BeBuild.h>

#define ALLOW_WRITES  1
#define NO_WRITES     0

#ifdef __cplusplus
extern "C" {
#endif

extern int   init_block_cache(int max_blocks, int flags);
extern void  shutdown_block_cache(void);

extern void  force_cache_flush(int dev, int prefer_log_blocks);
extern int   flush_blocks(int dev, off_t bnum, int nblocks);
extern int   flush_device(int dev, int warn_locked);

extern int   init_cache_for_device(int fd, off_t max_blocks);
extern int   remove_cached_device_blocks(int dev, int allow_write);

extern void *get_block(int dev, off_t bnum, int bsize);
extern void *get_empty_block(int dev, off_t bnum, int bsize);
extern int   release_block(int dev, off_t bnum);
extern int   mark_blocks_dirty(int dev, off_t bnum, int nblocks);


extern int  cached_read(int dev, off_t bnum, void *data, off_t num_blocks, int bsize);
extern int  cached_write(int dev, off_t bnum, const void *data,
				off_t num_blocks, int bsize);
extern int  cached_write_locked(int dev, off_t bnum, const void *data,
				off_t num_blocks, int bsize);
extern int  set_blocks_info(int dev, off_t *blocks, int nblocks,
				void (*func)(off_t bnum, size_t nblocks, void *arg),
				void *arg);

extern size_t read_phys_blocks (int fd, off_t bnum, void *data, uint num_blocks, int bsize);
extern size_t write_phys_blocks(int fd, off_t bnum, void *data, uint num_blocks, int bsize);

#ifdef __cplusplus
}
#endif

#endif /* _CACHE_H_ */
