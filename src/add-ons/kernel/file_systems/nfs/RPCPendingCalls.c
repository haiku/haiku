#include "RPCPendingCalls.h"
#include <string.h>

extern bool conf_no_check_ip_xid;

extern void 
PendingCallInit(struct PendingCall *call)
{
	call->buffer=NULL;
}

extern void 
PendingCallDestroy(struct PendingCall *call)
{
	free (call->buffer);
}

extern void 
RPCPendingCallsInit(struct RPCPendingCalls *calls)
{
	SemaphorePoolInit(&calls->fPool);
	
	calls->fFirst=NULL;
	calls->fSem=create_sem(1,"RPCPendingCalls");
	set_sem_owner (calls->fSem,B_SYSTEM_TEAM);
}

extern void 
RPCPendingCallsDestroy(struct RPCPendingCalls *calls)
{
	delete_sem(calls->fSem);
	
	while (calls->fFirst)
	{
		struct PendingCall *next=calls->fFirst->next;
		
		SemaphorePoolPut (&calls->fPool,calls->fFirst->sem);
		PendingCallDestroy (calls->fFirst);
		free (calls->fFirst);

		calls->fFirst=next;
	}
	
	SemaphorePoolDestroy (&calls->fPool);
}

extern struct PendingCall *
RPCPendingCallsAddPendingCall (struct RPCPendingCalls *calls,
								int32 xid, const struct sockaddr_in *addr)
{
	struct PendingCall *call=(struct PendingCall *)malloc(sizeof(struct PendingCall));
	PendingCallInit (call);
	
	call->sem=SemaphorePoolGet(&calls->fPool);
		
	memcpy(&call->addr,addr,sizeof(struct sockaddr_in));
	call->xid=xid;

	while (acquire_sem (calls->fSem)==B_INTERRUPTED);
	
	call->next=calls->fFirst;
	calls->fFirst=call;

	while (release_sem (calls->fSem)==B_INTERRUPTED);
	
	return call;
}

extern struct PendingCall *
RPCPendingCallsFindAndRemovePendingCall (struct RPCPendingCalls *calls,
										int32 xid, const struct sockaddr_in *addr)
{
	struct PendingCall *last=NULL;
	struct PendingCall *current;
	
	while (acquire_sem (calls->fSem)==B_INTERRUPTED);
	
	current=calls->fFirst; // mmu_man
	
	while (current)
	{
		if (current->xid==xid)
		{
			if (((current->addr.sin_addr.s_addr==addr->sin_addr.s_addr)&&
				(current->addr.sin_port==addr->sin_port)) || conf_no_check_ip_xid)
			{	
				if (last)
					last->next=current->next;
				else
					calls->fFirst=current->next;

				current->next=NULL;			
				
				while (release_sem (calls->fSem)==B_INTERRUPTED);
				return current;
			}
		}

		last=current;
		current=current->next;
	}
	
	while (release_sem (calls->fSem)==B_INTERRUPTED);
	
	return NULL;
}

extern void 
SemaphorePoolInit(struct SemaphorePool *pool)
{
	pool->fPool=NULL;
	pool->fPoolCount=0;
	pool->fPoolSize=0;
	
	pool->fPoolSem=create_sem(1,"semaphore_pool_sem");
	set_sem_owner (pool->fPoolSem,B_SYSTEM_TEAM);
}

extern void 
SemaphorePoolDestroy(struct SemaphorePool *pool)
{
	int32 i;
	
	for (i=0;i<pool->fPoolCount;i++)
		delete_sem (pool->fPool[i]);

	free (pool->fPool);
	
	delete_sem (pool->fPoolSem);
}

extern sem_id 
SemaphorePoolGet(struct SemaphorePool *pool)
{
	sem_id sem;
	
	while (acquire_sem(pool->fPoolSem)==B_INTERRUPTED)
	{
	}
	
	if (pool->fPoolCount==0)
	{
		sem=create_sem (0,"pending_call");
		
		while (release_sem(pool->fPoolSem)==B_INTERRUPTED)
		{
		}
		
		return sem;
	}

	sem=pool->fPool[pool->fPoolCount-1];
	pool->fPoolCount--;

	while (release_sem(pool->fPoolSem)==B_INTERRUPTED)
	{
	}
	
	return sem;
}

extern void 
SemaphorePoolPut(struct SemaphorePool *pool, sem_id sem)
{
	while (acquire_sem(pool->fPoolSem)==B_INTERRUPTED)
	{
	}

	if (pool->fPoolCount+1>pool->fPoolSize)
	{
		pool->fPoolSize+=8;
		pool->fPool=(sem_id *)realloc(pool->fPool,pool->fPoolSize*sizeof(sem_id));
	}

	pool->fPool[pool->fPoolCount]=sem;
	pool->fPoolCount++;
	
	while (release_sem(pool->fPoolSem)==B_INTERRUPTED)
	{
	}
}

