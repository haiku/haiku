// PortReleaser.h

#ifndef USERLAND_FS_PORT_RELEASER_H
#define USERLAND_FS_PORT_RELEASER_H

#include "FileSystem.h"
#include "RequestPortPool.h"

// PortReleaser
class PortReleaser {
public:
	PortReleaser(RequestPortPool* portPool, RequestPort* port)
		: fPortPool(portPool),
		  fPort(port)
	{
	}

	~PortReleaser()
	{
		if (fPort && fPortPool)
			fPortPool->ReleasePort(fPort);
	}

private:
	RequestPortPool*	fPortPool;
	RequestPort*		fPort;
};

#endif	// USERLAND_FS_PORT_RELEASER_H
