// DiskDevice.h

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

	int32 CountSessions() const;
	BSession *SessionAt(int32 index) const;

	int32 CountPartitions() const;
	
	int32 BlockSize() const;

	const char *DevicePath() const;
	void GetName(BString *name, bool includeBusID = true, 
				 bool includeLUN = false) const;
	void GetName(char *name, bool includeBusID = true, 
				 bool includeLUN = false) const;

	bool IsReadOnly() const;
	bool IsRemovable() const;
	bool HasMedia() const;
	bool IsFloppy() const;
	uint8 Type() const;		// cf. device_geometry::device_type

	int32 UniqueID() const;

	status_t Eject();
	status_t LowLevelFormat();	// TODO: remove?

	status_t Update();		// sync this object with reality

	BSession *VisitEachSession(BDiskDeviceVisitor *visitor);
		// return BSession* if terminated early
	BPartition *VisitEachPartition(BDiskDeviceVisitor *visitor);
		// return BPartition* if terminated early
	bool Traverse(BDiskDeviceVisitor *visitor);
		// return true if terminated early

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
