/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PARTITION_DELEGATE_H
#define _PARTITION_DELEGATE_H

#include <MutablePartition.h>
#include <Partition.h>


class BDiskSystemAddOn;
class BPartitionHandle;


class BPartition::Delegate {
public:
								Delegate(BPartition* partition);
								~Delegate();

			BPartition*			Partition() const	{ return fPartition; }

			BMutablePartition*	MutablePartition();
			const BMutablePartition* MutablePartition() const;

			status_t			InitHierarchy(
									const user_partition_data* partitionData,
									Delegate* parent);
			status_t			InitAfterHierarchy();

			const user_partition_data* PartitionData() const;

			Delegate*			ChildAt(int32 index) const;
			int32				CountChildren() const;

			uint32				SupportedOperations(uint32 mask);
			uint32				SupportedChildOperations(Delegate* child,
									uint32 mask);

			// Self Modification

			status_t			Defragment();
			status_t			Repair(bool checkOnly);

			status_t			ValidateResize(off_t* size) const;
			status_t			ValidateResizeChild(Delegate* child,
									off_t* size) const;
			status_t			Resize(off_t size);
			status_t			ResizeChild(Delegate* child, off_t size);

			status_t			ValidateMove(off_t* offset) const;
			status_t			ValidateMoveChild(Delegate* child,
									off_t* offset) const;
			status_t			Move(off_t offset);
			status_t			MoveChild(Delegate* child, off_t offset);

			status_t			ValidateSetContentName(BString* name) const;
			status_t			ValidateSetName(Delegate* child,
									BString* name) const;
			status_t			SetContentName(const char* name);
			status_t			SetName(Delegate* child, const char* name);

			status_t			ValidateSetType(Delegate* child,
									const char* type) const;
			status_t			SetType(Delegate* child, const char* type);

			status_t			GetContentParameterEditor(
									BDiskDeviceParameterEditor** editor) const;
			status_t			GetParameterEditor(Delegate* child,
									BDiskDeviceParameterEditor** editor) const;
			status_t			SetContentParameters(const char* parameters);
			status_t			SetParameters(Delegate* child,
									const char* parameters);

			bool				CanInitialize(const char* diskSystem) const;
			status_t			GetInitializationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor) const;
			status_t			ValidateInitialize(const char* diskSystem,
									BString* name, const char* parameters);
			status_t			Initialize(const char* diskSystem,
									const char* name,
									const char* parameters);
			status_t			Uninitialize();
	
			// Modification of child partitions

			status_t			GetPartitioningInfo(BPartitioningInfo* info);

			status_t			GetChildCreationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor) const;
			status_t			ValidateCreateChild(off_t* start, off_t* size,
									const char* type, BString* name,
									const char* parameters) const;
			status_t			CreateChild(off_t start, off_t size,
									const char* type, const char* name,
									const char* parameters, BPartition** child);
	
			status_t			DeleteChild(Delegate* child);

private:
			void				_FreeHandle();

private:
			BPartition*			fPartition;
			BMutablePartition	fMutablePartition;
			BDiskSystemAddOn*	fDiskSystem;
			BPartitionHandle*	fPartitionHandle;
};


#endif	// _PARTITION_DELEGATE_H
