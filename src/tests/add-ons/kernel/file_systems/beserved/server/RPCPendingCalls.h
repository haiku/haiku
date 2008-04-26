#ifndef _RPCPENDINGCALLS_H

#define _RPCPENDINGCALLS_H

#include <malloc.h>
#include <OS.h>
#include <socket.h>

struct SemaphorePool
{
	sem_id *fPool;
	int32 fPoolCount;
	int32 fPoolSize;
	sem_id fPoolSem;
};

void SemaphorePoolInit (struct SemaphorePool *pool);
void SemaphorePoolDestroy (struct SemaphorePool *pool);
sem_id SemaphorePoolGet(struct SemaphorePool *pool);
void SemaphorePoolPut (struct SemaphorePool *pool, sem_id sem);

struct PendingCall
{
	struct PendingCall *next;
	
	sem_id sem;
	struct sockaddr_in addr;
	int32 xid;
	uint8 *buffer;	
};

void PendingCallInit (struct PendingCall *call);
void PendingCallDestroy (struct PendingCall *call);

struct RPCPendingCalls
{
	struct PendingCall *fFirst;
	sem_id fSem;		
	struct SemaphorePool fPool;	
};

void RPCPendingCallsInit (struct RPCPendingCalls *calls);
void RPCPendingCallsDestroy (struct RPCPendingCalls *calls);

struct PendingCall *RPCPendingCallsAddPendingCall (struct RPCPendingCalls *calls,
													int32 xid, const struct sockaddr_in *addr);

struct PendingCall *RPCPendingCallsFindAndRemovePendingCall (struct RPCPendingCalls *calls,
													int32 xid, const struct sockaddr_in *addr);

#endif
