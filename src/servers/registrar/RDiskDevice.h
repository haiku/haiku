//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef DISK_DEVICE_H
#define DISK_DEVICE_H

#include <Drivers.h>
#include <ObjectList.h>
#include <String.h>

#include "RChangeCounter.h"

struct session_info;

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

	status_t MediaChanged();
	status_t SessionLayoutChanged();

	int32 ID() const { return fID; }
	int32 ChangeCounter() const { return fChangeCounter.Count(); }
	void Changed() { fChangeCounter.Increment(); }

	void SetTouched(bool touched) { fTouched = touched; }
	bool Touched() const { return fTouched; }

	int FD() const { return fFD; }
	off_t Size() const;
	int32 BlockSize() const { return fGeometry.bytes_per_sector; }
	bool IsReadOnly() { return fGeometry.read_only; }

	const char *Path() const { return fPath.String(); }

	status_t AddSession(const session_info *sessionInfo,
						uint32 cause/* = B_DEVICE_CAUSE_UNKNOWN*/);
	bool AddSession(RSession *session,
					uint32 cause/* = B_DEVICE_CAUSE_UNKNOWN*/);
	bool RemoveSession(int32 index,
					   uint32 cause/* = B_DEVICE_CAUSE_UNKNOWN*/);
	bool RemoveSession(RSession *session,
					   uint32 cause/* = B_DEVICE_CAUSE_UNKNOWN*/);
	int32 CountSessions() const { return fSessions.CountItems(); }
	RSession *SessionAt(int32 index) const { return fSessions.ItemAt(index); }
	int32 IndexOfSession(const RSession *session) const
		{ return fSessions.IndexOf(session); }

	status_t Update();

	status_t Archive(BMessage *archive) const;

	void Dump() const;

private:
	status_t _RescanSessions(uint32 cause);

	static int32 _NextID();

private:
	BObjectList<RSession>	fSessions;
	RDiskDeviceList			*fDeviceList;
	int32					fID;
	RChangeCounter			fChangeCounter;
	bool					fTouched;
	BString					fPath;
	int						fFD;
	device_geometry			fGeometry;
	status_t				fMediaStatus;

	static vint32			fNextID;
};

#endif	// DISK_DEVICE_H
