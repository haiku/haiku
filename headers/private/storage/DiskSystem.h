//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_SYSTEM_H
#define _DISK_SYSTEM_H

class BDiskSystem {
public:
	const char *Name() const;

	bool SupportsDefragmenting(BPartition *partition) const;
	bool SupportsRepairing(BPartition *partition, bool checkOnly) const;
	bool SupportsResizing(BPartition *partition) const;
	bool SupportsResizingChild(BPartition *child) const;
	bool SupportsMoving(BPartition *partition) const;
	bool SupportsMovingChild(BPartition *child) const;
	bool SupportsParentSystem(const char *system) const;
		// True in most cases. NULL == raw device.
	bool SupportsChildSystem(const char *system) const;
		// False for most file systems, true for most partitioning
		// systems.

	bool ValidateResize(BPartition *partition, off_t *size) const;
	bool ValidateMove(BPartition *partition, off_t *start) const;
	bool ValidateResizeChild(BPartition *partition, off_t *size) const;
	bool ValidateMoveChild(BPartition *partition, off_t *start) const;
	bool ValidateCreateChild(BPartition *partition, off_t *start, off_t *size) const;
	bool ValidateInitialize(BPartition *partition) const;

	bool IsPartitioningSystem() const;
	bool IsFileSystem() const;
};

#endif	// _DISK_SYSTEM_H
