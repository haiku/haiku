/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: RealtimeAlloc.cpp
 *  DESCR: 
 ***********************************************************************/

#define NDEBUG

#include <SupportDefs.h>
#include <RealtimeAlloc.h>
#include <malloc.h>
#include "debug.h"


struct rtm_pool 
{
};

extern "C" {
	rtm_pool * _rtm_pool; 
};

status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name)
{
	BROKEN();
	*out_pool = (rtm_pool *) 0x55557777;
	TRACE("  new pool = 0x%08x\n",(int)*out_pool);
	/* If out_pool is NULL, the default pool will be created if it isn't already. */
	/* If the default pool is already created, it will return EALREADY. */
	return B_OK;
}

status_t rtm_delete_pool(rtm_pool * pool)
{
	BROKEN();
	TRACE("  pool = 0x%08x\n",(int)pool);
	return B_OK;
}

void * rtm_alloc(rtm_pool * pool, size_t size)
{
	BROKEN();
	TRACE("  pool = 0x%08x\n",(int)pool);
	/* If NULL is passed for pool, the default pool is used (if created). */
	void *p = malloc(size);
	TRACE("  returning ptr = 0x%08x\n",(int)p);
	return p;
}

status_t rtm_free(void * data)
{
	BROKEN();
	TRACE("  ptr = 0x%08x\n",(int)data);
	free(data);
	return B_OK;
}

status_t rtm_realloc(void ** data, size_t new_size)
{
	BROKEN();
	TRACE("  ptr = 0x%08x\n",(int)*data);
	void * newptr = realloc(*data, new_size);
	if (newptr) {
		*data = newptr;
		TRACE("  new ptr = 0x%08x\n",(int)*data);
		return B_OK;
	} else
		return B_ERROR;
}

status_t rtm_size_for(void * data)
{
	UNIMPLEMENTED();
	TRACE("  ptr = 0x%08x\n",(int)data);
	return 0;
}

status_t rtm_phys_size_for(void * data)
{
	UNIMPLEMENTED();
	TRACE("  ptr = 0x%08x\n",(int)data);
	return 0;
}

rtm_pool * rtm_default_pool()
{
	BROKEN();
	/* Return the default pool, or NULL if not yet initialized */
	TRACE("  returning pool = 0x%08x\n",0x22229999);
	return (rtm_pool *) 0x22229999;
}


/****************************************************************************/
/* undocumented symboles that libmedia.so exports */
/* the following function declarations are guessed and are still wrong */
/****************************************************************************/

extern "C" {

status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, int32 param4, int32 param5, ...);

void rtm_get_pool(rtm_pool *pool,void *data,int32 param3,int32 param4, ...);

}

/*
param5 of rtm_create_pool_etc matches 
param3 of rtm_get_pool
and might be a pointer into some structure 

param4 of rtm_create_pool_etc is 0 in the Doom game,
and might be a Flags field

param4 of rtm_get_pool is 0x00000003 in the Doom game,
and might be a Flags field
  
*/

status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, int32 param4, int32 param5, ...)
{
	BROKEN();
	*out_pool = (rtm_pool *) 0x44448888;
	TRACE("  new pool = 0x%08x\n",(int)*out_pool);
	TRACE("  size = %d\n",(int)total_size);
	TRACE("  name = %s\n",name);
	TRACE("  param4 = 0x%08x\n",(int)param4);
	TRACE("  param5 = 0x%08x\n",(int)param5);
	return B_OK;
}


void rtm_get_pool(rtm_pool *pool,void *data,int32 param3, int32 param4, ...)
{
	UNIMPLEMENTED();
	TRACE("  pool = 0x%08x\n",(int)pool);
	TRACE("  ptr = 0x%08x\n",(int)data);
	TRACE("  param3 = 0x%08x\n",(int)param3);
	TRACE("  param4 = 0x%08x\n",(int)param4);
}

