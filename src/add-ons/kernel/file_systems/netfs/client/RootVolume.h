// RootVolume.h

#ifndef NET_FS_ROOT_VOLUME_H
#define NET_FS_ROOT_VOLUME_H

#include <fsproto.h>

#include "ServerManager.h"
#include "VirtualVolume.h"

class ServerVolume;
class VirtualNode;

class RootVolume : public VirtualVolume, private ServerManager::Listener {
public:
								RootVolume(VolumeManager* volumeManager);
								~RootVolume();

			status_t			Init();
			void				Uninit();

	virtual	void				PrepareToUnmount();

			// FS
			status_t			Mount(const char* device, uint32 flags,
									const char* parameters, int32 len);
	virtual	status_t			Unmount();
	virtual	status_t			Sync();
	virtual	status_t			ReadFSStat(fs_info* info);
	virtual	status_t			WriteFSStat(struct fs_info* info, int32 mask);

			// files
	virtual	status_t			IOCtl(Node* node, void* cookie, int cmd,
									void* buffer, size_t bufferSize);

private:
	virtual	void				ServerAdded(ExtendedServerInfo* serverInfo);
	virtual	void				ServerUpdated(ExtendedServerInfo* oldInfo,
									ExtendedServerInfo* newInfo);
	virtual	void				ServerRemoved(ExtendedServerInfo* serverInfo);


			ServerVolume*		_GetServerVolume(const char* name);
			ServerVolume*		_GetServerVolume(const NetAddress& address);

protected:
			ServerManager*		fServerManager;
};

#endif	// NET_FS_ROOT_VOLUME_H
