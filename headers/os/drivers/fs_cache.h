/*
 * Copyright 2004-2008, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FS_CACHE_H
#define _FS_CACHE_H

/*! File System File and Block Caches */


#include <fs_interface.h>


enum {
	TRANSACTION_WRITTEN,
	TRANSACTION_ABORTED,
	TRANSACTION_ENDED,
	TRANSACTION_IDLE
};

typedef void (*transaction_notification_hook)(int32 id, int32 event,
	void *data);

#ifdef __cplusplus
extern "C" {
#endif

/* transactions */
extern int32 cache_start_transaction(void *_cache);
extern status_t cache_sync_transaction(void *_cache, int32 id);
extern status_t cache_end_transaction(void *_cache, int32 id,
					transaction_notification_hook hook, void *data);
extern status_t cache_abort_transaction(void *_cache, int32 id);
extern int32 cache_detach_sub_transaction(void *_cache, int32 id,
					transaction_notification_hook hook, void *data);
extern status_t cache_abort_sub_transaction(void *_cache, int32 id);
extern status_t cache_start_sub_transaction(void *_cache, int32 id);
extern status_t cache_add_transaction_listener(void *_cache, int32 id,
					transaction_notification_hook hook, void *data);
extern status_t cache_remove_transaction_listener(void *_cache, int32 id,
					transaction_notification_hook hook, void *data);
extern status_t cache_next_block_in_transaction(void *_cache, int32 id,
					bool mainOnly, long *_cookie, off_t *_blockNumber,
					void **_data, void **_unchangedData);
extern int32 cache_blocks_in_transaction(void *_cache, int32 id);
extern int32 cache_blocks_in_main_transaction(void *_cache, int32 id);
extern int32 cache_blocks_in_sub_transaction(void *_cache, int32 id);

/* block cache */
extern void block_cache_delete(void *_cache, bool allowWrites);
extern void *block_cache_create(int fd, off_t numBlocks, size_t blockSize,
					bool readOnly);
extern status_t block_cache_sync(void *_cache);
extern status_t block_cache_sync_etc(void *_cache, off_t blockNumber,
					size_t numBlocks);
extern status_t block_cache_make_writable(void *_cache, off_t blockNumber,
					int32 transaction);
extern void *block_cache_get_writable_etc(void *_cache, off_t blockNumber,
					off_t base, off_t length, int32 transaction);
extern void *block_cache_get_writable(void *_cache, off_t blockNumber,
					int32 transaction);
extern void *block_cache_get_empty(void *_cache, off_t blockNumber,
					int32 transaction);
extern const void *block_cache_get_etc(void *_cache, off_t blockNumber,
					off_t base, off_t length);
extern const void *block_cache_get(void *_cache, off_t blockNumber);
extern status_t block_cache_set_dirty(void *_cache, off_t blockNumber,
					bool isDirty, int32 transaction);
extern void block_cache_put(void *_cache, off_t blockNumber);

/* file cache */
extern void *file_cache_create(dev_t mountID, ino_t vnodeID, off_t size);
extern void file_cache_delete(void *_cacheRef);
extern status_t file_cache_set_size(void *_cacheRef, off_t size);
extern status_t file_cache_sync(void *_cache);

extern status_t file_cache_read(void *_cacheRef, void *cookie, off_t offset,
					void *bufferBase, size_t *_size);
extern status_t file_cache_write(void *_cacheRef, void *cookie, off_t offset,
					const void *buffer, size_t *_size);

/* file map */
extern void *file_map_create(dev_t mountID, ino_t vnodeID, off_t size);
extern void file_map_delete(void *_map);
extern void file_map_set_size(void *_map, off_t size);
extern void file_map_invalidate(void *_map, off_t offset, off_t size);
extern status_t file_map_translate(void *_map, off_t offset, size_t size,
					struct file_io_vec *vecs, size_t *_count);

#ifdef __cplusplus
}
#endif

#endif	/* _FS_CACHE_H */
