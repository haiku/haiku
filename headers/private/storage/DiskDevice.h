//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include <DiskDeviceVisitor.h>
#include <ObjectList.h>

class BPartition;
class BSession;

class BDiskDevice {
public:
	BDiskDevice();
	~BDiskDevice();

	off_t Size() const;
	int32 BlockSize() const;

	int32 CountSessions() const;
	BSession *SessionAt(int32 index) const;

	int32 CountPartitions() const;

	const char *DevicePath() const;
	void GetName(BString *name, bool includeBusID = true, 
				 bool includeLUN = false) const;
	void GetName(char *name, bool includeBusID = true, 
				 bool includeLUN = false) const;

	bool IsReadOnly() const;
	bool IsRemovable() const;
	bool HasMedia() const;
	bool IsFloppy() const;
	uint8 Type() const;

	int32 UniqueID() const;

	status_t Eject();
	status_t LowLevelFormat();	// TODO: remove?

	status_t Update();

	BSession *VisitEachSession(BDiskDeviceVisitor *visitor);
	BPartition *VisitEachPartition(BDiskDeviceVisitor *visitor);
	bool Traverse(BDiskDeviceVisitor *visitor);

private:
	BDiskDevice(const BDiskDevice &);
	BDiskDevice &operator=(const BDiskDevice &);

private:
	BObjectList<Session *>	fSessions;
	int32					fUniqueID;
	int32					fBlockSize;
	char					fPath[B_FILE_NAME_LENGTH];
	bool					fReadOnly;
	bool					fRemovable;
	bool					fIsFloppy;
};

#endif	// _DISK_DEVICE_H
