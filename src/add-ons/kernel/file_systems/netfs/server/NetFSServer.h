// NetFSServer.h

#ifndef NET_FS_SERVER_H
#define NET_FS_SERVER_H

#include <Application.h>
#include <List.h>

#include "ClientConnection.h"
#include "ConnectionListenerFactory.h"
#include "Locker.h"

class BPath;

class Connection;
class ConnectionListener;
class SecurityContext;
class ServerInfo;

class NetFSServer : public BApplication, private ClientConnectionListener {
public:
								NetFSServer(bool useBroadcasting);
								~NetFSServer();

			status_t			Init();
	virtual	thread_id			Run();

	virtual	void				MessageReceived(BMessage* message);

			VolumeManager*		GetVolumeManager() const;
			SecurityContext*	GetSecurityContext() const;

private:
			class ConnectionInitializer;
			friend class ConnectionInitializer;
			class ServerInfoSender;

			status_t			_AddClientConnection(
									ClientConnection* connection);

	virtual	void				ClientConnectionClosed(
									ClientConnection* connection,
									bool broken);

			status_t			_LoadSecurityContext(
									SecurityContext** securityContext);
			status_t			_InitSettings();
			status_t			_LoadSettings();
			status_t			_SaveSettings();
			status_t			_GetSettingsDirPath(BPath* path,
									bool create = false);
			status_t			_GetSettingsFilePath(BPath* path,
									bool createDir = false);

			status_t			_InitServerInfoConnectionListener();
			void				_ExitServerInfoConnectionListener();

	static	int32				_ConnectionListenerEntry(void* data);
			int32				_ConnectionListener();

	static	int32				_ConnectionDeleterEntry(void* data);
			int32				_ConnectionDeleter();

	static	int32				_BroadcasterEntry(void* data);
			int32				_Broadcaster();

	static	int32				_ServerInfoConnectionListenerEntry(void* data);
			int32				_ServerInfoConnectionListener();
			status_t			_GetServerInfo(ServerInfo& serverInfo);

			void				_ServerInfoUpdated();

			void				_SendReply(BMessage* message, BMessage* reply,
									status_t error = B_OK);
			void				_SendReply(BMessage* message,
									status_t error = B_OK);

private:
			SecurityContext*	fSecurityContext;
			ConnectionListenerFactory fConnectionListenerFactory;
			ConnectionListener*	fConnectionListener;
			Locker				fLock;
			BList				fClientConnections;
			VolumeManager*		fVolumeManager;
			BList				fClosedConnections;
			sem_id				fClosedConnectionsSemaphore;
			thread_id			fConnectionListenerThread;
			thread_id			fConnectionDeleter;
			thread_id			fBroadcaster;
			vint32				fBroadcastingSocket;
			sem_id				fBroadcasterSemaphore;
			thread_id			fServerInfoConnectionListener;
			vint32				fServerInfoConnectionListenerSocket;
			vint32				fServerInfoUpdated;
			bool				fUseBroadcasting;
			volatile bool		fTerminating;
};

#endif	// NET_FS_SERVER_H
