//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITION_H
#define _PARTITION_H

#include <ObjectList.h>

class BDiskDevice;

// partition statuses
enum {
	B_PARTITION_VALID,
	B_PARTITION_CORRUPT,
	B_PARTITION_UNRECOGNIZED,
}

class BPartition {
public:
	// Partition Info
	
	off_t Offset() const;		// 0 for devices
	off_t Size() const;
	int32 BlockSize() const;
	int32 Index() const;		// 0 for devices
	uint32 Status() const;
	
	bool IsMountable() const;
	bool IsPartitionable() const;

	bool IsDevice() const;
	bool IsReadOnly() const;
	bool IsMounted() const;
	
	const char* Name() const;
	const char* Type() const;   // See DiskDeviceTypes.h
	int32 UniqueID() const;
	uint32 Flags() const;		
	
	status_t GetPath(BPath *path) const;
	status_t GetVolume(BVolume *volume) const;
	status_t GetIcon(BBitmap *icon, icon_size which) const;

	status_t Mount(uint32 mountFlags = 0, const char *parameters = NULL);
	status_t Unmount();
	
	// Hierarchy Info

	BDiskDevice* Device() const;
	BPartition* Parent() const;
	BPartition* ChildAt(int32 index) const;
	int32 CountChildren() const;

	BPartitionableSpace* PartitionableSpaceAt(int32 index) const;
	int32 CountPartitionableSpaces() const;
	
	BPartition* VisitEachChild(BDiskDeviceVisitor *visitor);
	BPartition* VisitSubtree(BDiskDeviceVisitor *visitor);

	// Self Modification

	bool IsLocked() const;
	status_t Lock();	// to be non-blocking
	status_t Unlock();
	
	bool CanDefragment() const;
	status_t Defragment() const;
	
	bool CanRepair(bool checkOnly) const;
	status_t Repair(bool checkOnly) const;

	bool CanResize() const;
	bool CanResizeWhileMounted() const;
	status_t ValidateResize(off_t*) const;
	status_t Resize(off_t);

	bool CanMove() const;
	status_t ValidateMove(off_t*) const;
	status_t Move(off_t);

	bool CanEditParameters() const;
	status_t GetParameterEditor(
           BDiskScannerParameterEditor **editor,
           BDiskScannerParameterEditor **parentEditor);
    status_t ValidateSetParameters(const char **parameters) const;
    status_t SetParameters(const char *parameters);

	bool CanInitialize() const;
	status_t GetInitializationParameterEditor(const char *system,       
               BDiskScannerParameterEditor **editor) const;
    status_t ValidateInitialize(const char *diskSystem, const char **parameters) const;
	status_t Initialize(const char *diskSystem,
	                 const char *parameters);
	
	// Modification of child partitions

	bool CanCreateChild() const;
	status_t ValidateCreateChild(
	           off_t *start, 
	           off_t *size) const;
	status_t CreateChild(off_t start, off_t size);
	
	bool CanDeleteChild(int32 index) const;
	status_t DeleteChild();
	
protected:
	BObjectList<BPartition>	fChildren;
	int32					fUniqueID;
	int32					fChangeCounter;
	off_t					fOffset;
	off_t					fSize;
	int32					fBlockSize;
	int32					fIndex;

	bool					fIsMountable;
	bool					fIsDevice;
	bool					fIsReadOnly;
	
	char					fName[B_FILE_NAME_LENGTH];
	char					fType[B_FILE_NAME_LENGTH];
}

#endif	// _PARTITION_H
