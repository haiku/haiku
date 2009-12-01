/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _REALTIME_ALLOC_H
#define _REALTIME_ALLOC_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtm_pool rtm_pool;

#ifdef __cplusplus
status_t rtm_create_pool(rtm_pool** _pool, size_t totalSize,
	const char* name = NULL);
#else
status_t rtm_create_pool(rtm_pool** _pool, size_t totalSize, const char* name);
#endif

status_t rtm_delete_pool(rtm_pool* pool);

void* rtm_alloc(rtm_pool* pool, size_t size);
status_t rtm_free(void* data);
status_t rtm_realloc(void** data, size_t new_size);
status_t rtm_size_for(void* data);
status_t rtm_phys_size_for(void* data);
size_t rtm_available(rtm_pool* pool);

rtm_pool* rtm_default_pool();

#ifdef __cplusplus
}
#endif

#endif // _REALTIME_ALLOC_H
