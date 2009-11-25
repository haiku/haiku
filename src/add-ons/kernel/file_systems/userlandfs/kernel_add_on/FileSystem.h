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
class KMessage;
class Settings;
class Volume;


struct VNodeOps {
	int32				refCount;
	FSVNodeCapabilities	capabilities;
	fs_vnode_ops*		ops;
	VNodeOps*			hash_link;

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
	VNodeOps*& GetLink(VNodeOps* value) const
		{ return value->hash_link; }
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

			status_t			AddNodeListener(dev_t device, ino_t node,
									uint32 flags, void* listener);
			status_t			RemoveNodeListener(dev_t device, ino_t node,
									void* listener);

			VNodeOps*			GetVNodeOps(
									const FSVNodeCapabilities& capabilities);
			void				PutVNodeOps(VNodeOps* ops);

			bool				IsUserlandServerThread() const;

private:
			struct SelectSyncMap;
			struct NodeListenerKey;
			struct NodeListenerProxy;
			struct NodeListenerHashDefinition;

			friend class KernelDebug;
			friend struct NodeListenerProxy;

			typedef BOpenHashTable<VNodeOpsHashDefinition> VNodeOpsMap;
			typedef BOpenHashTable<NodeListenerHashDefinition> NodeListenerMap;


private:
			void				_InitVNodeOpsVector(fs_vnode_ops* ops,
									const FSVNodeCapabilities& capabilities);

			void				_NodeListenerEventOccurred(
									NodeListenerProxy* proxy,
									const KMessage* event);

	static	int32				_NotificationThreadEntry(void* data);
			int32				_NotificationThread();

private:
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
			mutex				fNodeListenersLock;
			NodeListenerMap*	fNodeListeners;
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
