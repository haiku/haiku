// ServerVolume.h

#ifndef NET_FS_SERVER_VOLUME_H
#define NET_FS_SERVER_VOLUME_H

#include "Locker.h"
#include "NetAddress.h"
#include "ServerManager.h"
#include "VirtualVolume.h"

class ExtendedServerInfo;
class ExtendedShareInfo;
class ServerConnectionProvider;
class ShareVolume;
class VirtualNode;

class ServerVolume : public VirtualVolume {
public:
								ServerVolume(VolumeManager* volumeManager,
									ExtendedServerInfo* serverInfo);
								~ServerVolume();

			NetAddress			GetServerAddress();

			void				SetServerInfo(ExtendedServerInfo* serverInfo);

			status_t			Init(const char* name);
			void				Uninit();

	virtual	void				PrepareToUnmount();

	virtual	void				HandleEvent(VolumeEvent* event);

			// FS
	virtual	status_t			Unmount();

			// queries
	virtual	status_t			OpenQuery(const char* queryString,
									uint32 flags, port_id port, int32 token,
									QueryIterator** iterator);
	virtual	void				FreeQueryIterator(QueryIterator* iterator);
	virtual	status_t			ReadQuery(QueryIterator* iterator,
									struct dirent* buffer, size_t bufferSize,
									int32 count, int32* countRead);

private:
			status_t			_AddShare(ExtendedShareInfo* shareInfo);
			ShareVolume*		_GetShareVolume(int32 volumeID);

protected:
			ExtendedServerInfo*	fServerInfo;
			ServerConnectionProvider* fConnectionProvider;
};

#endif	// NET_FS_SERVER_VOLUME_H
