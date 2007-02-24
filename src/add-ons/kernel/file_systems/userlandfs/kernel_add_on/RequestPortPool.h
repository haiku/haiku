// RequestPortPool.h

#ifndef USERLAND_FS_REQUEST_PORT_POOL_H
#define USERLAND_FS_REQUEST_PORT_POOL_H

#include <OS.h>

#include "Locker.h"

namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

class RequestPortPool : public Locker {
public:
								RequestPortPool();
								~RequestPortPool();

			status_t			InitCheck() const;

			bool				IsDisconnected() const;

			status_t			AddPort(RequestPort* port);

			RequestPort*		AcquirePort();
			void				ReleasePort(RequestPort* port);

private:
			friend class KernelDebug;

			struct PortAcquirationInfo {
				RequestPort*	port;
				thread_id		owner;
				int32			count;
			};

			PortAcquirationInfo*	fPorts;
			int32				fPortCount;
			int32				fFreePorts;
			sem_id				fFreePortSemaphore;
			volatile bool		fDisconnected;
};

#endif	// USERLAND_FS_REQUEST_PORT_POOL_H
