// NetFSServerRoster.h

#ifndef NETFS_SERVER_ROSTER_H
#define NETFS_SERVER_ROSTER_H

#include <Messenger.h>
#include <OS.h>

class NetFSServerRoster {
public:
								NetFSServerRoster();
								~NetFSServerRoster();

			// server
			bool				IsServerRunning();
			status_t			LaunchServer();
			status_t			TerminateServer(bool force = false,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			SaveServerSettings();

			// users
			status_t			AddUser(const char* user, const char* password);
			status_t			RemoveUser(const char* user);
			status_t			GetUsers(BMessage* users);
			status_t			GetUserStatistics(const char* user,
									BMessage* statistics);

			// shares
			status_t			AddShare(const char* share, const char* path);
			status_t			RemoveShare(const char* share);
			status_t			GetShares(BMessage* shares);
			status_t			GetShareUsers(const char* share,
									BMessage* users);
			status_t			GetShareStatistics(const char* share,
									BMessage* statistics);

			// permissions
			status_t			SetUserPermissions(const char* share,
									const char* user, uint32 permissions);
			status_t			GetUserPermissions(const char* share,
									const char* user, uint32* permissions);

private:
			status_t			_InitMessenger();
			status_t			_SendRequest(BMessage* request,
									BMessage* reply = NULL);

private:
			BMessenger			fServerMessenger;
};

#endif	// NETFS_SERVER_ROSTER_H
