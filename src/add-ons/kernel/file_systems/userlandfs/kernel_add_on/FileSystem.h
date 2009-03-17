/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_FILE_SYSTEM_H
#define USERLAND_FS_FILE_SYSTEM_H

#include <fs_interface.h>

#include <util/OpenHashTable.h>

#include <lock.h>

#include "FSCapabilities.h"
#include "LazyInitializable.h"
#include "Locker.h"
#include "RequestPort.h"
#include "RequestPortPool.h"
#include "String.h"
#include "Vector.h"


struct IOCtlInfo;
class Settings;
class Volume;


struct VNodeOps : HashTableLink<VNodeOps> {
	int32				refCount;
	FSVNodeCapabilities	capabilities;
	fs_vnode_ops*		ops;

	VNodeOps(const FSVNodeCapabilities& capabilities, fs_vnode_ops* ops)
		:
		refCount(1),
		capabilities(capabilities),
		ops(ops)
	{
	}

	~VNodeOps()
	{
		delete ops;
	}
};


struct VNodeOpsHashDefinition {
	typedef FSVNodeCapabilities	KeyType;
	typedef	VNodeOps			ValueType;

	size_t HashKey(const FSVNodeCapabilities& key) const
		{ return key.GetHashCode(); }
	size_t Hash(const VNodeOps* value) const
		{ return HashKey(value->capabilities); }
	bool Compare(const FSVNodeCapabilities& key, const VNodeOps* value) const
		{ return value->capabilities == key; }
	HashTableLink<VNodeOps>* GetLink(VNodeOps* value) const
		{ return value; }
};


class FileSystem {
public:
								FileSystem();
								~FileSystem();

			status_t			Init(const char* name, team_id team,
									Port::Info* infos, int32 infoCount,
									const FSCapabilities& capabilities);

			const char*			GetName() const;
			team_id				GetTeam() const	{ return fTeam; }

			const FSCapabilities& GetCapabilities() const;
	inline	bool				HasCapability(uint32 capability) const;

			RequestPortPool*	GetPortPool();

			status_t			Mount(fs_volume* fsVolume, const char* device,
									ulong flags, const char* parameters,
									Volume** volume);
//			status_t		 	Initialize(const char* deviceName,
//									const char* parameters, size_t len);
			void				VolumeUnmounted(Volume* volume);

			Volume*				GetVolume(dev_t id);

			const IOCtlInfo*	GetIOCtlInfo(int command) const;

			status_t			AddSelectSyncEntry(selectsync* sync);
			void				RemoveSelectSyncEntry(selectsync* sync);
			bool				KnowsSelectSyncEntry(selectsync* sync);

			VNodeOps*			GetVNodeOps(
									const FSVNodeCapabilities& capabilities);
			void				PutVNodeOps(VNodeOps* ops);

			bool				IsUserlandServerThread() const;

private:
			void				_InitVNodeOpsVector(fs_vnode_ops* ops,
									const FSVNodeCapabilities& capabilities);

	static	int32				_NotificationThreadEntry(void* data);
			int32				_NotificationThread();

private:
			friend class KernelDebug;
			struct SelectSyncEntry;
			struct SelectSyncMap;
			typedef OpenHashTable<VNodeOpsHashDefinition> VNodeOpsMap;

			Vector<Volume*>		fVolumes;
			mutex				fVolumeLock;
			VNodeOpsMap			fVNodeOps;
			mutex				fVNodeOpsLock;
			String				fName;
			team_id				fTeam;
			FSCapabilities		fCapabilities;
			RequestPort*		fNotificationPort;
			thread_id			fNotificationThread;
			RequestPortPool		fPortPool;
			SelectSyncMap*		fSelectSyncs;
			Settings*			fSettings;
			team_id				fUserlandServerTeam;
			bool				fInitialized;
	volatile bool				fTerminating;
};


// HasCapability
inline bool
FileSystem::HasCapability(uint32 capability) const
{
	return fCapabilities.Get(capability);
}

#endif	// USERLAND_FS_FILE_SYSTEM_H
