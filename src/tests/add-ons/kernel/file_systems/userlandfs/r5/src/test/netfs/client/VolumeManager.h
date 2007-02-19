// VolumeManager.h

#ifndef NET_FS_VOLUME_MANAGER_H
#define NET_FS_VOLUME_MANAGER_H

#include "Locker.h"
#include "Volume.h"

class QueryManager;
class RootVolume;
class Volume;
class VolumeEvent;

// VolumeManager
class VolumeManager : private Locker {
public:
								VolumeManager(nspace_id id, uint32 flags);
								~VolumeManager();

			nspace_id			GetID() const			{ return fID; }
			uint32				GetMountFlags() const	{ return fMountFlags; }
			uid_t				GetMountUID() const		{ return fMountUID; }
			uid_t				GetMountGID() const		{ return fMountGID; }

			status_t			MountRootVolume(const char* device,
									const char* parameters, int32 len,
									Volume** volume);
			void				UnmountRootVolume();

			QueryManager*		GetQueryManager() const;

			Volume*				GetRootVolume();
			status_t			AddVolume(Volume* volume);
			Volume*				GetVolume(vnode_id nodeID);
private:
			Volume*				GetVolume(Volume* volume);
				// TODO: This is actually a bit dangerous, since the volume
				// might be delete and another one reallocated at the same
				// location. (Use the node ID of its root node instead?!)
public:
			void				PutVolume(Volume* volume);

			vnode_id			NewNodeID(Volume* volume);
			void				RemoveNodeID(vnode_id nodeID);

			void				SendVolumeEvent(VolumeEvent* event);

private:
			struct VolumeSet;
			struct NodeIDVolumeMap;
			class VolumeEventQueue;

	static	int32				_EventDelivererEntry(void* data);
			int32				_EventDeliverer();

private:
			nspace_id			fID;
			uint32				fMountFlags;
			uid_t				fMountUID;
			gid_t				fMountGID;
			vnode_id			fRootID;
			volatile vnode_id	fNextNodeID;
			QueryManager*		fQueryManager;
			VolumeSet*			fVolumes;
			NodeIDVolumeMap*	fNodeIDs2Volumes;
			VolumeEventQueue*	fVolumeEvents;
			thread_id			fEventDeliverer;
};

// VolumePutter
class VolumePutter {
public:
	VolumePutter(Volume* volume)
		: fVolume(volume)
	{
	}

	~VolumePutter()
	{
		Put();
	}

	void Put()
	{
		if (fVolume) {
			fVolume->PutVolume();
			fVolume = NULL;
		}
	}

	void Detach()
	{
		fVolume = NULL;
	}

private:
	Volume*		fVolume;
};

#endif	// NET_FS_VOLUME_MANAGER_H
