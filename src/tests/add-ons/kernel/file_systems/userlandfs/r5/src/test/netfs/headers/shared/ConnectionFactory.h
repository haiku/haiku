// ConnectionFactory.h

#ifndef NET_FS_CONNECTION_FACTORY_H
#define NET_FS_CONNECTION_FACTORY_H

#include <SupportDefs.h>

class Connection;
class ConnectionListener;

class ConnectionFactory {
public:
								ConnectionFactory();
								~ConnectionFactory();

			status_t			CreateConnection(const char* type,
									const char* parameters,
									Connection** connection);
};

#endif	// NET_FS_CONNECTION_FACTORY_H
