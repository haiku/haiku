// ConnectionListenerFactory.h

#ifndef NET_FS_CONNECTION_LISTENER_FACTORY_H
#define NET_FS_CONNECTION_LISTENER_FACTORY_H

#include <SupportDefs.h>

class Connection;
class ConnectionListener;

class ConnectionListenerFactory {
public:
								ConnectionListenerFactory();
								~ConnectionListenerFactory();

			status_t			CreateConnectionListener(const char* type,
									const char* parameters,
									ConnectionListener** connectionListener);
};

#endif	// NET_FS_CONNECTION_LISTENER_FACTORY_H
