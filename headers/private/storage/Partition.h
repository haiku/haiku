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
	uint32 BlockSize() const;
	int32 Index() const;		// 0 for devices
	uint32 Status() const;
	
	bool IsMountable() const;
	bool IsPartitionable() const;

	bool IsDevice() const;
	bool IsReadOnly() const;
	bool IsMounted() const;
	
	const char* Name() const;
	const char* ContentName() const;
	const char* Type() const;			// See DiskDeviceTypes.h
	const char* ContentType() const;	// See DiskDeviceTypes.h
	partition_id UniqueID() const;
	uint32 Flags() const;		

	status_t GetDiskSystem(BDiskSystem *diskSystem) const;
	
	status_t GetPath(BPath *path) const;
	status_t GetVolume(BVolume *volume) const;
	status_t GetIcon(BBitmap *icon, icon_size which) const;

	status_t Mount(uint32 mountFlags = 0, const char *parameters = NULL);
	status_t Unmount();
	
	// Hierarchy Info

	BDiskDevice *Device() const;
	BPartition *Parent() const;
	BPartition *ChildAt(int32 index) const;
	int32 CountChildren() const;

	BPartitioningInfo* GetPartitioningInfo() const;
	
	BPartition* VisitEachChild(BDiskDeviceVisitor *visitor);
	BPartition* VisitEachDescendent(BDiskDeviceVisitor *visitor);

	// Self Modification

	bool IsLocked() const;
	status_t Lock();	// to be non-blocking
	status_t Unlock();
	
	bool CanDefragment(bool *whileMounted = NULL) const;
	status_t Defragment() const;
	
	bool CanRepair(bool *checkOnly, bool *whileMounted = NULL) const;
	status_t Repair(bool checkOnly) const;

	bool CanResize(bool *whileMounted = NULL) const;
	status_t ValidateResize(off_t*) const;
	status_t Resize(off_t);

	bool CanMove(bool *whileMounted = NULL) const;
	status_t ValidateMove(off_t*) const;
	status_t Move(off_t);

	bool CanEditParameters(bool *whileMounted = NULL) const;
	status_t GetParameterEditor(
               BDiskScannerParameterEditor **editor,
               BDiskScannerParameterEditor **contentEditor);
    status_t SetParameters(const char *parameters, const char *contentParameters);

// TODO: Add name/content name editing methods, Also in BDiskSystem.

	bool CanInitialize(const char *diskSystem) const;
	status_t GetInitializationParameterEditor(const char *system,       
               BDiskScannerParameterEditor **editor) const;
	status_t Initialize(const char *diskSystem, const char *parameters);
	
	// Modification of child partitions

	bool CanCreateChild() const;
	status_t GetChildCreationParameterEditor(const char *system,
               BDiskScannerParameterEditor **editor) const;
	status_t ValidateCreateChild(off_t *start, off_t *size,
				const char *type, const char *parameters) const;
	status_t CreateChild(off_t start, off_t size, const char *type,
				const char *parameters, BPartition** child = NULL);
	
	bool CanDeleteChild(int32 index) const;
	status_t DeleteChild(int32 index);
	
protected:
	off_t					fOffset;
	off_t					fSize;
	uint32					fBlockSize;
	int32					fIndex;
	uint32					fStatus;

	bool					fIsMountable;
	bool					fIsPartitionable;

	bool					fIsDevice;
	bool					fIsReadOnly;
	bool					fIsMounted;
	
	char					fName[B_FILE_NAME_LENGTH];
	char					fType[B_FILE_NAME_LENGTH];
	char					fContentType[B_FILE_NAME_LENGTH];

	partition_id			fUniqueID;
	uint32 					fFlags;

	BObjectList<BPartition>	fChildren;

	int32					fChangeCounter;
}

#endif	// _PARTITION_H
