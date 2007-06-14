// InsecureConnection.h

#ifndef NET_FS_INSECURE_CONNECTION_LISTENER_H
#define NET_FS_INSECURE_CONNECTION_LISTENER_H

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#include "ConnectionListener.h"

class InsecureConnectionListener : public ConnectionListener {
public:
								InsecureConnectionListener();
	virtual						~InsecureConnectionListener();

	virtual	status_t			Init(const char* parameters);

	virtual	status_t			Listen(Connection** connection);
	virtual	void				StopListening();

	virtual	status_t			FinishInitialization(Connection* connection,
									SecurityContext* securityContext,
									User** user);

private:
			vint32				fSocket;
};

#endif	// NET_FS_INSECURE_CONNECTION_LISTENER_H
