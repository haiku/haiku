/* File System File and Block Caches
 *
 * Copyright 2004-2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_HAIKU_FS_CACHE_H
#define USERLAND_FS_HAIKU_FS_CACHE_H


#include <fs_cache.h>


namespace UserlandFS {
namespace HaikuKernelEmu {

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
extern status_t cache_next_block_in_transaction(void *_cache, int32 id,
					uint32 *_cookie, off_t *_blockNumber, void **_data,
					void **_unchangedData);
extern int32 cache_blocks_in_transaction(void *_cache, int32 id);
extern int32 cache_blocks_in_sub_transaction(void *_cache, int32 id);

/* block cache */
extern void block_cache_delete(void *_cache, bool allowWrites);
extern void *block_cache_create(int fd, off_t numBlocks, size_t blockSize,
					bool readOnly);
extern status_t block_cache_sync(void *_cache);

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
extern void *file_cache_create(mount_id mountID, vnode_id vnodeID, off_t size,
					int fd);
extern void file_cache_delete(void *_cacheRef);
extern status_t file_cache_set_size(void *_cacheRef, off_t size);
extern status_t file_cache_sync(void *_cache);
extern status_t file_cache_invalidate_file_map(void *_cacheRef, off_t offset,
					off_t size);

extern status_t file_cache_read_pages(void *_cacheRef, off_t offset,
					const iovec *vecs, size_t count, size_t *_numBytes);
extern status_t file_cache_write_pages(void *_cacheRef, off_t offset,
					const iovec *vecs, size_t count, size_t *_numBytes);
extern status_t file_cache_read(void *_cacheRef, off_t offset, void *bufferBase,
					size_t *_size);
extern status_t file_cache_write(void *_cacheRef, off_t offset,
					const void *buffer, size_t *_size);

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS

#endif	/* USERLAND_FS_HAIKU_FS_CACHE_H */
