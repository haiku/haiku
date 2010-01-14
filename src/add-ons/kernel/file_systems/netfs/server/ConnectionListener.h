// ConnectionListener.h

#ifndef NET_FS_CONNECTION_LISTENER_H
#define NET_FS_CONNECTION_LISTENER_H

#include <OS.h>

class Connection;
class SecurityContext;
class User;

class ConnectionListener {
protected:
								ConnectionListener();

public:
	virtual						~ConnectionListener();

	virtual	status_t			Init(const char* parameters) = 0;

	virtual	status_t			Listen(Connection** connection) = 0;
	virtual	void				StopListening() = 0;

	virtual	status_t			FinishInitialization(Connection* connection,
									SecurityContext* securityContext,
									User** user) = 0;
};

#endif	// NET_FS_CONNECTION_LISTENER_H
