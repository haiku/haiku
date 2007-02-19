// FileSystem.h

#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fsproto.h>

#include "LazyInitializable.h"
#include "Locker.h"
#include "Referencable.h"
#include "RequestPortPool.h"
#include "String.h"
#include "Vector.h"

namespace UserlandFSUtil {

class RequestPort;

}

using UserlandFSUtil::RequestPort;

struct IOCtlInfo;
class Settings;
class Volume;

class FileSystem : public LazyInitializable, public Referencable {
public:
								FileSystem(const char* name,
									RequestPort* initPort,
									status_t* error);
								~FileSystem();

			const char*			GetName() const;

			RequestPortPool*	GetPortPool();

			status_t			Mount(nspace_id id, const char* device,
									ulong flags, const char* parameters,
									int32 len, Volume** volume);
			status_t		 	Initialize(const char* deviceName,
									const char* parameters, size_t len);
			void				VolumeUnmounted(Volume* volume);

			Volume*				GetVolume(nspace_id id);

			const IOCtlInfo*	GetIOCtlInfo(int command) const;

			status_t			AddSelectSyncEntry(selectsync* sync);
			void				RemoveSelectSyncEntry(selectsync* sync);
			bool				KnowsSelectSyncEntry(selectsync* sync);

			bool				IsUserlandServerThread() const;

protected:
	virtual	status_t			FirstTimeInit();

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
			RequestPort*		fInitPort;
			RequestPort*		fNotificationPort;
			thread_id			fNotificationThread;
			RequestPortPool		fPortPool;
			SelectSyncMap*		fSelectSyncs;
			Settings*			fSettings;
			team_id				fUserlandServerTeam;
	volatile bool				fTerminating;
};

#endif	// USERLAND_FS_FILE_SYSTEM_H
