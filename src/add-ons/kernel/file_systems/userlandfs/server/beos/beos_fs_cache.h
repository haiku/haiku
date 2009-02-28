/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef USERLAND_FS_BEOS_FS_CACHE_H
#define USERLAND_FS_BEOS_FS_CACHE_H

#include <BeBuild.h>

#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif

#ifdef __cplusplus
	extern "C" {
#endif

extern _IMPEXP_KERNEL int   beos_init_block_cache(int max_blocks, int flags);
extern _IMPEXP_KERNEL void  beos_shutdown_block_cache(void);

extern _IMPEXP_KERNEL void  beos_force_cache_flush(int dev,
								int prefer_log_blocks);
extern _IMPEXP_KERNEL int   beos_flush_blocks(int dev, off_t bnum, int nblocks);
extern _IMPEXP_KERNEL int   beos_flush_device(int dev, int warn_locked);

extern _IMPEXP_KERNEL int   beos_init_cache_for_device(int fd,
								off_t max_blocks);
extern _IMPEXP_KERNEL int   beos_remove_cached_device_blocks(int dev,
								int allow_write);

extern _IMPEXP_KERNEL void *beos_get_block(int dev, off_t bnum, int bsize);
extern _IMPEXP_KERNEL void *beos_get_empty_block(int dev, off_t bnum,
								int bsize);
extern _IMPEXP_KERNEL int   beos_release_block(int dev, off_t bnum);
extern _IMPEXP_KERNEL int   beos_mark_blocks_dirty(int dev, off_t bnum,
								int nblocks);


extern _IMPEXP_KERNEL int  beos_cached_read(int dev, off_t bnum, void *data,
								off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  beos_cached_write(int dev, off_t bnum,
								const void *data, off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  beos_cached_write_locked(int dev, off_t bnum,
								const void *data, off_t num_blocks, int bsize);
extern _IMPEXP_KERNEL int  beos_set_blocks_info(int dev, off_t *blocks,
								int nblocks, void (*func)(off_t bnum,
									size_t nblocks, void *arg), void *arg);


extern _IMPEXP_KERNEL size_t beos_read_phys_blocks (int fd, off_t bnum,
								void *data, uint num_blocks, int bsize);
extern _IMPEXP_KERNEL size_t beos_write_phys_blocks(int fd, off_t bnum,
								void *data, uint num_blocks, int bsize);

#ifdef __cplusplus
	}	// extern "C"
#endif

#endif /* USERLAND_FS_BEOS_FS_CACHE_H */
