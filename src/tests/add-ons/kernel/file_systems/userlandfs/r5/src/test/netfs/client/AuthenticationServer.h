// AuthenticationServer.h

#ifndef NET_FS_AUTHENTICATION_SERVER_H
#define NET_FS_AUTHENTICATION_SERVER_H

#include <OS.h>

class AuthenticationServer {
public:
								AuthenticationServer();
								~AuthenticationServer();

			status_t			InitCheck() const;

			status_t			GetAuthentication(const char* context,
									const char* server, const char* share,
									uid_t uid, bool badPassword,
									bool* cancelled, char* foundUser,
									int32 foundUserSize, char* foundPassword,
									int32 foundPasswordSize);

private:
			port_id				fServerPort;
};

#endif	// NET_FS_AUTHENTICATION_SERVER_H
