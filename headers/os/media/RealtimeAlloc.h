/*******************************************************************************
/
/	File:			RealtimeAlloc.h
/
/   Description:    Allocation from separate "pools" of memory. Those pools
/					will be locked in RAM if realtime allocators are turned
/					on in the BMediaRoster, so don't waste this memory unless
/					it's needed. Also, the shared pool is a scarce resource,
/					so it's better if you create your own pool for your own
/					needs and leave the shared pool for BMediaNode instances
/					and the needs of the Media Kit.
/
/	Copyright 1998-99, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_REALTIME_ALLOC_H)
#define _REALTIME_ALLOC_H

#include <SupportDefs.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct rtm_pool rtm_pool;

/* If out_pool is NULL, the default pool will be created if it isn't already. */
/* If the default pool is already created, it will return EALREADY. */
#if defined(__cplusplus)
status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name=NULL);
#else
status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name);
#endif
status_t rtm_delete_pool(rtm_pool * pool);
/* If NULL is passed for pool, the default pool is used (if created). */
void * rtm_alloc(rtm_pool * pool, size_t size);
status_t rtm_free(void * data);
status_t rtm_realloc(void ** data, size_t new_size);
status_t rtm_size_for(void * data);
status_t rtm_phys_size_for(void * data);

/* Return the default pool, or NULL if not yet initialized */
rtm_pool * rtm_default_pool();

#if defined(__cplusplus)
}
#endif

#endif // _REALTIME_ALLOC_H

