/*
 * Copyright 2004-2020, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FS_CACHE_H
#define _FSSH_FS_CACHE_H

//! File System File and Block Caches


#include "fssh_fs_interface.h"


enum {
	FSSH_TRANSACTION_WRITTEN	= 0x01,
	FSSH_TRANSACTION_ABORTED	= 0x02,
	FSSH_TRANSACTION_ENDED		= 0x04,
	FSSH_TRANSACTION_IDLE		= 0x08
};

/* file map modes */
enum {
	FSSH_FILE_MAP_CACHE_ON_DEMAND	= 0x01,	/* default mode */
	FSSH_FILE_MAP_CACHE_ALL			= 0x02
};

typedef void (*fssh_transaction_notification_hook)(int32_t id, int32_t event,
	void *data);

#ifdef __cplusplus
extern "C" {
#endif

/* transactions */
extern int32_t			fssh_cache_start_transaction(void *_cache);
extern fssh_status_t	fssh_cache_sync_transaction(void *_cache, int32_t id);
extern fssh_status_t	fssh_cache_end_transaction(void *_cache, int32_t id,
							fssh_transaction_notification_hook hook,
							void *data);
extern fssh_status_t	fssh_cache_abort_transaction(void *_cache, int32_t id);
extern int32_t			fssh_cache_detach_sub_transaction(void *_cache,
							int32_t id, fssh_transaction_notification_hook hook,
							void *data);
extern fssh_status_t	fssh_cache_abort_sub_transaction(void *_cache,
							int32_t id);
extern fssh_status_t	fssh_cache_start_sub_transaction(void *_cache,
							int32_t id);
extern fssh_status_t	fssh_cache_add_transaction_listener(void *_cache,
							int32_t id, int32_t events,
							fssh_transaction_notification_hook hook,
							void *data);
extern fssh_status_t	fssh_cache_remove_transaction_listener(void *_cache,
							int32_t id, fssh_transaction_notification_hook hook,
							void *data);
extern fssh_status_t	fssh_cache_next_block_in_transaction(void *_cache,
							int32_t id, bool mainOnly, long *_cookie,
							fssh_off_t *_blockNumber, void **_data,
							void **_unchangedData);
extern int32_t			fssh_cache_blocks_in_transaction(void *_cache,
							int32_t id);
extern int32_t			fssh_cache_blocks_in_main_transaction(void *_cache,
							int32_t id);
extern int32_t			fssh_cache_blocks_in_sub_transaction(void *_cache,
							int32_t id);
extern bool				fssh_cache_has_block_in_transaction(void *_cache,
							int32_t id, fssh_off_t blockNumber);

/* block cache */
extern void				fssh_block_cache_delete(void *_cache, bool allowWrites);
extern void *			fssh_block_cache_create(int fd, fssh_off_t numBlocks,
							fssh_size_t blockSize, bool readOnly);
extern fssh_status_t	fssh_block_cache_sync(void *_cache);
extern fssh_status_t	fssh_block_cache_sync_etc(void *_cache,
							fssh_off_t blockNumber, fssh_size_t numBlocks);
extern void				fssh_block_cache_discard(void *_cache,
							fssh_off_t blockNumber, fssh_size_t numBlocks);
extern fssh_status_t	fssh_block_cache_make_writable(void *_cache,
							fssh_off_t blockNumber, int32_t transaction);
extern fssh_status_t	fssh_block_cache_get_writable_etc(void *_cache,
							fssh_off_t blockNumber, fssh_off_t base,
							fssh_off_t length, int32_t transaction,
							void **_block);
extern void *			fssh_block_cache_get_writable(void *_cache,
							fssh_off_t blockNumber, int32_t transaction);
extern void *			fssh_block_cache_get_empty(void *_cache,
							fssh_off_t blockNumber, int32_t transaction);
extern fssh_status_t	fssh_block_cache_get_etc(void *_cache,
							fssh_off_t blockNumber, fssh_off_t base,
							fssh_off_t length, const void **_block);
extern const void *		fssh_block_cache_get(void *_cache,
							fssh_off_t blockNumber);
extern fssh_status_t	fssh_block_cache_set_dirty(void *_cache,
							fssh_off_t blockNumber, bool isDirty,
							int32_t transaction);
extern void				fssh_block_cache_put(void *_cache,
							fssh_off_t blockNumber);

/* file cache */
extern void *			fssh_file_cache_create(fssh_mount_id mountID,
							fssh_vnode_id vnodeID, fssh_off_t size);
extern void				fssh_file_cache_delete(void *_cacheRef);
extern void				fssh_file_cache_enable(void *_cacheRef);
extern fssh_status_t	fssh_file_cache_disable(void *_cacheRef);
extern bool				fssh_file_cache_is_enabled(void *_cacheRef);
extern fssh_status_t	fssh_file_cache_set_size(void *_cacheRef,
							fssh_off_t size);
extern fssh_status_t	fssh_file_cache_sync(void *_cache);

extern fssh_status_t	fssh_file_cache_read(void *_cacheRef, void *cookie,
							fssh_off_t offset, void *bufferBase,
							fssh_size_t *_size);
extern fssh_status_t	fssh_file_cache_write(void *_cacheRef, void *cookie,
							fssh_off_t offset, const void *buffer,
							fssh_size_t *_size);

/* file map */
extern void *			fssh_file_map_create(fssh_mount_id mountID,
							fssh_vnode_id vnodeID, fssh_off_t size);
extern void				fssh_file_map_delete(void *_map);
extern void				fssh_file_map_set_size(void *_map, fssh_off_t size);
extern void				fssh_file_map_invalidate(void *_map, fssh_off_t offset,
							fssh_off_t size);
extern fssh_status_t	fssh_file_map_set_mode(void *_map, uint32_t mode);
extern fssh_status_t	fssh_file_map_translate(void *_map, fssh_off_t offset,
							fssh_size_t size, struct fssh_file_io_vec *vecs,
							fssh_size_t *_count, fssh_size_t align);

/* entry cache */
extern fssh_status_t	fssh_entry_cache_add(fssh_dev_t mountID,
							fssh_ino_t dirID, const char* name,
							fssh_ino_t nodeID);
extern fssh_status_t	fssh_entry_cache_add_missing(fssh_dev_t mountID,
							fssh_ino_t dirID, const char* name);
extern fssh_status_t	fssh_entry_cache_remove(fssh_dev_t mountID,
							fssh_ino_t dirID, const char* name);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_FS_CACHE_H */
