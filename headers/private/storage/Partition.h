//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITION_H
#define _PARTITION_H

#include <ObjectList.h>

class BPartition {
public:
	// General Info
	
	off_t Offset() const;		// 0 for devices
	off_t Size() const;
	int32 BlockSize() const;
	int32 Index() const;		// 0 for devices
	
	bool IsMountable();
	bool IsPartitionable();
	bool IsDevice();
	bool IsReadOnly() const;
	
	const char* Name() const;
	const char* Type() const;   // See DiskDeviceTypes.h
	int32 UniqueID() const;
	virtual char* Path() const; // Recursively obtained down to the parent device
	
	uint32 Flags() const;		// not sure if this is really necessary anymore
	

	// Hierarchy Info

	const BPartition* Parent();
	BPartition* PartitionAt(int32 index) const;
	BPartition* PartitionWithID(int32 id);

	int32 CountChildren() const;	// Immediate descendents
	int32 CountDescendents() const;	// All descendents
	
	BPartition* VisitEachChild(BDiskDeviceVisitor *visitor);
	bool Traverse(BDiskDeviceVisitor *visitor);


	// Functions relevant to manipulating *this* partition

	off_t MinimumSize();
	off_t MaximumSize();
		// Both based on information obtained from the system used for the
		// partition and the partitioning system used for its parent

	int32 MoveTo(off_t start, BMessenger progressMessenger);
	int32 Resize(off_t size, BMessenger progressMessenger);
	
	status_t GetParameterEditor(BDiskScannerParameterEditor **editor,
	                            BDiskScannerParameterEditor **parentEditor);
		// For changing parameters of the partition after it's been initialized,
		// i.e. file system block size, volume name, etc. "editor" is an editor
		// for the system with which this partition is initialized, "parentEditor"
		// is an editor for any settings the parent partitioning system may
		// allow to be edited ("*parentEditor" will be set to NULL if not
		// applicable).

	status_t GetInitializationParameterEditor(const char *system,
	                                          BDiskScannerParameterEditor **editor);
	int32 Initialize(const char *system,
	                 const char *parameters,
	                 BMessenger progressMessenger);
	                 	// Initialization is used to set up a partition with either
	                 	// an empty file system or an empty partitioning system


	// Functions relevant to manipluating *child* partitions

	status_t GetPartitionCreationParameterEditor(BDiskScannerParameterEditor **editor);
	int32 CreateChildPartition(off_t start,
	                           off_t length,
	                           const char *parameters,
	                           BMessenger progressMessenger);
	int32 DeleteChildPartition(int32 index,
	                           BMessenger progressMessenger);	
	
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
