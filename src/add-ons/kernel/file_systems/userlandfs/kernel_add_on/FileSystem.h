// FileSystem.h

#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fs_interface.h>

#include "LazyInitializable.h"
#include "Locker.h"
#include "Referencable.h"
#include "RequestPort.h"
#include "RequestPortPool.h"
#include "String.h"
#include "Vector.h"


struct IOCtlInfo;
class Settings;
class Volume;

class FileSystem {
public:
								FileSystem();
								~FileSystem();

			status_t			Init(const char* name, Port::Info* infos,
									int32 infoCount);

			const char*			GetName() const;

			RequestPortPool*	GetPortPool();

			status_t			Mount(mount_id id, const char* device,
									ulong flags, const char* parameters,
									Volume** volume);
//			status_t		 	Initialize(const char* deviceName,
//									const char* parameters, size_t len);
			void				VolumeUnmounted(Volume* volume);

			Volume*				GetVolume(mount_id id);

			const IOCtlInfo*	GetIOCtlInfo(int command) const;

			status_t			AddSelectSyncEntry(selectsync* sync);
			void				RemoveSelectSyncEntry(selectsync* sync);
			bool				KnowsSelectSyncEntry(selectsync* sync);

			bool				IsUserlandServerThread() const;

private:
	static	int32				_NotificationThreadEntry(void* data);
			int32				_NotificationThread();

private:
			friend class KernelDebug;
			struct SelectSyncEntry;
			struct SelectSyncMap;

			Vector<Volume*>		fVolumes;
			Locker				fVolumeLock;
			String				fName;
			RequestPort*		fNotificationPort;
			thread_id			fNotificationThread;
			RequestPortPool		fPortPool;
			SelectSyncMap*		fSelectSyncs;
			Settings*			fSettings;
			team_id				fUserlandServerTeam;
			bool				fInitialized;
	volatile bool				fTerminating;
};

#endif	// USERLAND_FS_FILE_SYSTEM_H
