//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef VOLUME_LIST_H
#define VOLUME_LIST_H

#include <Handler.h>
#include <Node.h>
#include <ObjectList.h>
#include <VolumeRoster.h>

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

private:
	dev_t	fID;
	ino_t	fRootNode;
	char	fDevicePath[B_FILE_NAME_LENGTH];
};

// RVolumeList
class RVolumeList : public BHandler {
public:
	RVolumeList();
	virtual ~RVolumeList();

	virtual void MessageReceived(BMessage *message);
	virtual void SetNextHandler(BHandler *handler);

	status_t Rescan();

	const RVolume *VolumeForDevicePath(const char *devicePath) const;

	bool Lock();
	void Unlock();

	void Dump() const;

private:
	void _AddVolume(dev_t id);
	void _DeviceMounted(BMessage *message);
	void _DeviceUnmounted(BMessage *message);

private:
	BObjectList<RVolume>	fVolumes;
	BVolumeRoster			fRoster;
};

#endif	// VOLUME_LIST_H
