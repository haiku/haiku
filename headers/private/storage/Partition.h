//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITION_H
#define _PARTITION_H

#include <DiskDeviceDefs.h>
#include <Messenger.h>
#include <Mime.h>

class BBitmap;
class BDiskDevice;
class BDiskDeviceVisitor;
class BPartitioningInfo;
class BDiskScannerParameterEditor;	// TODO: Rename!
class BDiskSystem;
class BVolume;
struct user_partition_data;

class BPartition {
public:
	// Partition Info
	
	off_t Offset() const;		// 0 for devices
	off_t Size() const;
	uint32 BlockSize() const;
	int32 Index() const;		// 0 for devices
	uint32 Status() const;
	
	bool ContainsFileSystem() const;
	bool ContainsPartitioningSystem() const;

	bool IsDevice() const;
	bool IsReadOnly() const;
	bool IsMounted() const;

	uint32 Flags() const;		
	
	const char *Name() const;
	const char *ContentName() const;
	const char *Type() const;			// See DiskDeviceTypes.h
	const char *ContentType() const;	// See DiskDeviceTypes.h
	partition_id UniqueID() const;
		// TODO: just ID() ?

	status_t GetDiskSystem(BDiskSystem *diskSystem) const;
	
	virtual status_t GetPath(BPath *path) const;
	status_t GetVolume(BVolume *volume) const;
	status_t GetIcon(BBitmap *icon, icon_size which) const;

	status_t Mount(uint32 mountFlags = 0, const char *parameters = NULL);
	status_t Unmount();
	
	// Hierarchy Info

	BDiskDevice *Device() const;
	BPartition *Parent() const;
	BPartition *ChildAt(int32 index) const;
	int32 CountChildren() const;

	status_t GetPartitioningInfo(BPartitioningInfo *info) const;
	
	BPartition *VisitEachChild(BDiskDeviceVisitor *visitor);
	virtual BPartition *VisitEachDescendent(BDiskDeviceVisitor *visitor);

	// Self Modification

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
	
private:
	BPartition();
	BPartition(const Partition &);
	virtual ~BPartition();

	status_t SetTo(BDiskDevice *device, BPartition *parent,
				   user_partition_data *data);
	void Unset();

	friend class BDiskDevice;

	BDiskDevice				*fDevice;
	BPartition				*fParent;
	user_partition_data		*fPartitionData;
};

#endif	// _PARTITION_H
