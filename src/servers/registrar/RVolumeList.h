//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef VOLUME_LIST_H
#define VOLUME_LIST_H

#include <Messenger.h>
#include <Node.h>
#include <ObjectList.h>
#include <VolumeRoster.h>

#include "MessageHandler.h"

class BLocker;

// RVolume
class RVolume {
public:
	RVolume();
	~RVolume() {}

	status_t SetTo(dev_t id);
	void Unset();

	dev_t ID() const { return fID; }
	ino_t RootNode() const { return fRootNode; }
	void GetRootNodeRef(node_ref *ref) const
		{ ref->device = fID; ref->node = fRootNode; }
	const char *DevicePath() const { return fDevicePath; }

	status_t StartWatching(BMessenger target) const;
	status_t StopWatching(BMessenger target) const;

private:
	dev_t	fID;
	ino_t	fRootNode;
	char	fDevicePath[B_FILE_NAME_LENGTH];
};

// RVolumeListListener
class RVolumeListListener {
public:
	RVolumeListListener();
	virtual ~RVolumeListListener();

	virtual void VolumeMounted(const RVolume *volume);
	virtual void VolumeUnmounted(const RVolume *volume);
	virtual void MountPointMoved(const RVolume *volume);
};

// RVolumeList
class RVolumeList : public MessageHandler {
public:
	RVolumeList(BMessenger target, BLocker &lock);
	virtual ~RVolumeList();

	virtual void HandleMessage(BMessage *message);

	status_t Rescan();

	const RVolume *VolumeForDevicePath(const char *devicePath) const;

	bool Lock();
	void Unlock();

	void SetListener(RVolumeListListener *listener);

	void Dump() const;

private:
	RVolume *_AddVolume(dev_t id);
	void _DeviceMounted(BMessage *message);
	void _DeviceUnmounted(BMessage *message);
	void _MountPointMoved(BMessage *message);

private:
	mutable BLocker			&fLock;
	BMessenger				fTarget;
	BObjectList<RVolume>	fVolumes;
	BVolumeRoster			fRoster;
	RVolumeListListener		*fListener;
};

#endif	// VOLUME_LIST_H
