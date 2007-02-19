// PortConnection.h

#ifndef NET_FS_PORT_CONNECTION_LISTENER_H
#define NET_FS_PORT_CONNECTION_LISTENER_H

#include <OS.h>

#include "ConnectionListener.h"

class PortConnectionListener : public ConnectionListener {
public:
								PortConnectionListener();
	virtual						~PortConnectionListener();

	virtual	status_t			Init(const char* parameters);

	virtual	status_t			Listen(Connection** connection);
	virtual	void				StopListening();

	virtual	status_t			FinishInitialization(Connection* connection,
									SecurityContext* securityContext,
									User** user);

private:
			port_id				fPort;
};

#endif	// NET_FS_PORT_CONNECTION_LISTENER_H
