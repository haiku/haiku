// ServerConnectionProvider.h

#ifndef NET_FS_SERVER_CONNECTON_PROVIDER_H
#define NET_FS_SERVER_CONNECTON_PROVIDER_H

#include <fsproto.h>

#include <Referenceable.h>

#include "Locker.h"

class ExtendedServerInfo;
class ServerConnection;
class VolumeManager;

class ServerConnectionProvider : public BReferenceable {
public:
								ServerConnectionProvider(
									VolumeManager* volumeManager,
									ExtendedServerInfo* serverInfo,
									vnode_id connectionBrokenTarget);
								~ServerConnectionProvider();

			status_t			Init();

			status_t			GetServerConnection(
									ServerConnection** serverConnection);
			ServerConnection*	GetExistingServerConnection();

			void				CloseServerConnection();

private:
			Locker				fLock;
			VolumeManager*		fVolumeManager;
			ExtendedServerInfo*	fServerInfo;
			ServerConnection*	fServerConnection;
			vnode_id			fConnectionBrokenTarget;
};

#endif	// NET_FS_SERVER_CONNECTON_PROVIDER_H
