// ServerConnection.h

#ifndef NET_FS_SERVER_CONNECTION_H
#define NET_FS_SERVER_CONNECTION_H

#include <fsproto.h>

#include <Referenceable.h>

#include "Locker.h"
#include "RequestHandler.h"

class ConnectionBrokenEvent;
class ExtendedServerInfo;
class RequestConnection;
class ShareVolume;
class VolumeManager;

class ServerConnection : public BReferenceable, private RequestHandler {
public:
								ServerConnection(VolumeManager* volumeManager,
									ExtendedServerInfo* serverInfo);
								~ServerConnection();

			status_t			Init(vnode_id connectionBrokenTarget);
			void				Close();

			bool				IsConnected();

			RequestConnection*	GetRequestConnection() const;

			status_t			AddVolume(ShareVolume* volume);
			void				RemoveVolume(ShareVolume* volume);
			ShareVolume*		GetVolume(int32 volumeID);

private:
	virtual	status_t			VisitConnectionBrokenRequest(
									ConnectionBrokenRequest* request);
	virtual	status_t			VisitNodeMonitoringRequest(
									NodeMonitoringRequest* request);

private:
			struct VolumeMap;

			Locker				fLock;
			VolumeManager*		fVolumeManager;
			ExtendedServerInfo*	fServerInfo;
			RequestConnection*	fConnection;
			VolumeMap*			fVolumes;
			ConnectionBrokenEvent* fConnectionBrokenEvent;
			volatile bool		fConnected;
};

#endif	// NET_FS_SERVER_CONNECTION_H
