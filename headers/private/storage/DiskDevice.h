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

	void Unset();

	off_t Size() const;
	int32 BlockSize() const;

	int32 CountSessions() const;
	BSession *SessionAt(int32 index) const;

	int32 CountPartitions() const;

	const char *Path() const;
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

	BSession *SessionWithID(int32 id);
	BPartition *PartitionWithID(int32 id);

private:
	BDiskDevice(const BDiskDevice &);
	BDiskDevice &operator=(const BDiskDevice &);

	status_t _Unarchive(BMessage *archive);

	bool _AddSession(BSession *session);

private:
	friend class BDiskDeviceRoster;

	BObjectList<BSession>	fSessions;
	int32					fUniqueID;
	int32					fChangeCounter;
	status_t				fMediaStatus;
	off_t					fSize;
	int32					fBlockSize;
	uint8					fType;
	bool					fReadOnly;
	bool					fRemovable;
//	bool					fIsFloppy;
	char					fPath[B_FILE_NAME_LENGTH];
};

#endif	// _DISK_DEVICE_H
