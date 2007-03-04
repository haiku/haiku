// RequestPortPool.cpp

#include "RequestPortPool.h"

#include <stdlib.h>

#include "AutoLocker.h"
#include "Debug.h"
#include "RequestPort.h"

typedef AutoLocker<RequestPortPool> PoolLocker;

// constructor
RequestPortPool::RequestPortPool()
	: fPorts(NULL),
	  fPortCount(0),
	  fFreePorts(0),
	  fFreePortSemaphore(-1),
	  fDisconnected(false)
{
	fFreePortSemaphore = create_sem(0, "request port pool");
}

// destructor
RequestPortPool::~RequestPortPool()
{
	delete_sem(fFreePortSemaphore);
	free(fPorts);
}

// InitCheck
status_t
RequestPortPool::InitCheck() const
{
	if (fFreePortSemaphore < 0)
		return fFreePortSemaphore;
	return B_OK;
}

// IsDisconnected
bool
RequestPortPool::IsDisconnected() const
{
	return fDisconnected;
}

// AddPort
status_t
RequestPortPool::AddPort(RequestPort* port)
{
	if (!port)
		return B_BAD_VALUE;
	PoolLocker _(this);
	// resize the port array
	PortAcquirationInfo* ports = (PortAcquirationInfo*)realloc(fPorts,
		(fPortCount + 1) * sizeof(PortAcquirationInfo));
	if (!ports)
		return B_NO_MEMORY;
	fPorts = ports;
	// add the port as used port and let AcquirePort() free it
	fPorts[fPortCount].port = port;
	fPorts[fPortCount].owner = -1;
	fPorts[fPortCount].count = 1;
	fPortCount++;
	ReleasePort(port);
	return B_OK;
}

// AcquirePort
RequestPort*
RequestPortPool::AcquirePort()
{
	// first check whether the thread does already own a port
	thread_id thread = find_thread(NULL);
	{
		PoolLocker _(this);
		if (fDisconnected)
			return NULL;
		for (int32 i = fFreePorts; i < fPortCount; i++) {
			PortAcquirationInfo& info = fPorts[i];
			if (info.owner == thread) {
				info.count++;
				return info.port;
			}
		}
	}
	// the thread doesn't own a port yet, find a free one
	status_t error = acquire_sem(fFreePortSemaphore);
	if (error != B_OK)
		return NULL;
	PoolLocker _(this);
	if (fDisconnected)
		return NULL;
	if (fFreePorts < 1) {
		FATAL(("Inconsistent request port pool: We acquired the free port "
			"semaphore, but there are no free ports.\n"));
		return NULL;
	}
	PortAcquirationInfo& info = fPorts[--fFreePorts];
	info.owner = find_thread(NULL);
	info.count = 1;
	return info.port;
}

// ReleasePort
void
RequestPortPool::ReleasePort(RequestPort* port)
{
	if (!port)
		return;
	PoolLocker _(this);
	// find the port
	for (int32 i = fFreePorts; i < fPortCount; i++) {
		PortAcquirationInfo& info = fPorts[i];
		if (info.port == port) {
			if (--info.count == 0) {
				// swap with first used port
				if (i != fFreePorts) {
					fPorts[i] = fPorts[fFreePorts];
					fPorts[fFreePorts].port = port;
				}
				fFreePorts++;
				release_sem(fFreePortSemaphore);
			}
			if (port->InitCheck() != B_OK)
				fDisconnected = true;
			return;
		}
	}
	WARN(("RequestPortPool::ReleasePort(%p): port not found\n", port));
	// Not found!
}

