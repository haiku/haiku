/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SELECT_SYNC_POOL_H
#define _KERNEL_SELECT_SYNC_POOL_H

#include <SupportDefs.h>

struct selectsync;
typedef struct select_sync_pool select_sync_pool;


#ifdef __cplusplus
extern "C" {
#endif


status_t add_select_sync_pool_entry(select_sync_pool **pool, selectsync *sync,
			uint8 event);
status_t remove_select_sync_pool_entry(select_sync_pool **pool,
			selectsync *sync, uint8 event);
void delete_select_sync_pool(select_sync_pool *pool);
void notify_select_event_pool(select_sync_pool *pool, uint8 event);


#ifdef __cplusplus
}
#endif


#endif	// _KERNEL_SELECT_SYNC_POOL_H
