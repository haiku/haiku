/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * A pool of kernel ports
 ***********************************************************************/
#include <OS.h>
#include <malloc.h>
#include "PortPool.h"
#include "debug.h"

PortPool _ThePortPool;
PortPool *_PortPool = &_ThePortPool;

PortPool::PortPool()
{
	locker_atom = 0;
	locker_sem = create_sem(0,"port pool lock");
	count = 0;
	maxcount = 0;
	pool = 0;
}

PortPool::~PortPool()
{
	for (int i = 0; i < maxcount; i++)
		delete_port(pool[i].port);
	delete_sem(locker_sem);
	if (pool)
		free(pool);
}

port_id 
PortPool::GetPort()
{
	Lock();
	port_id port;
	if (count == maxcount) {
		maxcount += 3;
		pool = (PortInfo *)realloc(pool,sizeof(PortInfo) * maxcount);
		if (pool == NULL)
			debugger("out of memory in PortPool::GetPort()\n");
		for (int i = count; i < maxcount; i++) {
			pool[i].used = false;
			pool[i].port = create_port(1,"some reply port");
		}
	}
	count++;
	for (int i = 0; i < maxcount; i++)
		if (pool[i].used == false) {
			port = pool[i].port;
			pool[i].used = true;
			break;
		}
	Unlock();
	return port;
}

void
PortPool::PutPort(port_id port)
{
	Lock();
	count--;
	for (int i = 0; i < maxcount; i++)
		if (pool[i].port == port) {
			pool[i].used = false;
			break;
		}
	Unlock();
}

void
PortPool::Lock()
{ 
	if (atomic_add(&locker_atom, 1) > 0) {
		while (B_INTERRUPTED == acquire_sem(locker_sem))
			;
	}
}

void
PortPool::Unlock()
{ 
	if (atomic_add(&locker_atom, -1) > 1)
		release_sem(locker_sem);
}
