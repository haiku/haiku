//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_H
#define DISK_DEVICE_H

#include <Drivers.h>
#include <ObjectList.h>
#include <String.h>

class RDiskDeviceList;
class RPartition;
class RSession;

class RDiskDevice {
public:
	RDiskDevice();
	~RDiskDevice();

	status_t SetTo(const char *path, int fd, const device_geometry *geometry,
				   status_t mediaStatus);
	void Unset();

	void SetDeviceList(RDiskDeviceList *deviceList)
		{ fDeviceList = deviceList ;}
	RDiskDeviceList *DeviceList() const { return fDeviceList; }

	int32 ID() const { return fID; }
	int32 ChangeCounter() const { return fChangeCounter; }

	void SetTouched(bool touched) { fTouched = touched; }
	bool Touched() const { return fTouched; }

	const char *Path() const { return fPath.String(); }

	bool AddSession(RSession *session);
	bool RemoveSession(int32 index);
	bool RemoveSession(RSession *session);
	int32 CountSessions() const { return fSessions.CountItems(); }
	RSession *SessionAt(int32 index) const { return fSessions.ItemAt(index); }

	status_t Update();

	void Dump() const;

private:
	static int32 _NextID();

private:
	BObjectList<RSession>	fSessions;
	RDiskDeviceList			*fDeviceList;
	int32					fID;
	int32					fChangeCounter;
	bool					fTouched;
	BString					fPath;
	int						fFD;
	device_geometry			fGeometry;
	status_t				fMediaStatus;

	static vint32			fNextID;
};

#endif	// DISK_DEVICE_H
