// AuthenticationServer.h

#ifndef NETFS_AUTHENTICATION_SERVER_H
#define NETFS_AUTHENTICATION_SERVER_H

#include <Application.h>
#include <Locker.h>

#include "HashString.h"

class AuthenticationServer : public BApplication {
public:
								AuthenticationServer();
								~AuthenticationServer();

			status_t			Init();
private:
	static	int32				_RequestThreadEntry(void* data);
			int32				_RequestThread();

			bool				_GetAuthentication(const char* context,
									const char* server, const char* share,
									HashString* user, HashString* password);
			status_t			_AddAuthentication(const char* context,
									const char* server, const char* share,
									const char* user, const char* password,
									bool makeDefault);
			status_t			_SendRequestReply(port_id port, int32 token,
									status_t error, bool cancelled,
									const char* user, const char* password);

private:
			class Authentication;
			class ServerKey;
			class ServerEntry;
			class ServerEntryMap;
			struct AuthenticationRequest;
			class UserDialogTask;
			friend class UserDialogTask;

			BLocker				fLock;
			port_id				fRequestPort;
			thread_id			fRequestThread;
			ServerEntryMap*		fServerEntries;
			bool				fTerminating;
};

#endif	// NETFS_AUTHENTICATION_SERVER_H
