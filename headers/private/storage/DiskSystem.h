//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_SYSTEM_H
#define _DISK_SYSTEM_H

#include <DiskDeviceDefs.h>
#include <String.h>

class BPartition;
struct user_disk_system_info;

class BDiskSystem {
public:
	BDiskSystem();
	~BDiskSystem();

	status_t InitCheck() const;

	const char *Name() const;
	const char *PrettyName() const;

	bool SupportsDefragmenting(BPartition *partition,
							   bool *whileMounted) const;
	bool SupportsRepairing(BPartition *partition, bool checkOnly,
						   bool *whileMounted) const;
	bool SupportsResizing(BPartition *partition, bool *whileMounted) const;
	bool SupportsResizingChild(BPartition *child) const;
	bool SupportsMoving(BPartition *partition, bool *whileMounted) const;
	bool SupportsMovingChild(BPartition *child) const;
	bool SupportsSettingName(BPartition *partition) const;
	bool SupportsSettingContentName(BPartition *partition,
									bool *whileMounted) const;
	bool SupportsSettingType(BPartition *partition) const;
	bool SupportsCreatingChild(BPartition *partition) const;
	bool SupportsDeletingChild(BPartition *child) const;
	bool SupportsInitializing(BPartition *partition) const;
	bool SupportsInitializingChild(BPartition *child,
								   const char *diskSystem) const;

	bool SupportsParentSystem(BPartition *child) const;
	bool SupportsParentSystem(const char *system) const;
		// True in most cases. NULL == raw device.
	bool SupportsChildSystem(BPartition *child, const char *system) const;
	bool SupportsChildSystem(const char *system) const;
		// False for most file systems, true for most partitioning
		// systems.

	status_t ValidateResize(BPartition *partition, off_t *size) const;
	status_t ValidateResizeChild(BPartition *partition, off_t *size) const;
	status_t ValidateMove(BPartition *partition, off_t *start) const;
	status_t ValidateMoveChild(BPartition *partition, off_t *start) const;
	status_t ValidateSetName(BPartition *partition, char *name) const;
	status_t ValidateSetContentName(BPartition *partition, char *name) const;
	status_t ValidateSetType(BPartition *partition, const char *type) const;
	status_t ValidateCreateChild(BPartition *partition, off_t *start,
								 off_t *size, const char *type,
								 const char *parameters) const;

	status_t GetNextSupportedType(BPartition *partition, int32 *cookie,
								  char *type) const;
		// Returns all types the disk system supports for children of the
		// supplied partition.
	status_t GetTypeForContentType(const char *contentType, char *type) const;

	bool IsPartitioningSystem() const;
	bool IsFileSystem() const;
	bool IsSubSystemFor(BPartition *parent) const;

private:
	status_t _SetTo(user_disk_system_info *info);
	void _Unset();

	friend class BDiskDeviceRoster;

	disk_system_id	fID;
	BString			fName;
	BString			fPrettyName;
	bool			fFileSystem;
};

#endif	// _DISK_SYSTEM_H
