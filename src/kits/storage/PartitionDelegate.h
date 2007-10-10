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
	virtual						~Delegate();

	virtual	status_t			Init(const user_partition_data* partitionData)
									= 0;

			BPartition*			Partition() const	{ return fPartition; }

	virtual	const user_partition_data* PartitionData() const = 0;

	virtual	Delegate*			ChildAt(int32 index) const = 0;
	virtual	int32				CountChildren() const = 0;

	virtual	uint32				SupportedOperations(uint32 mask) = 0;
	virtual	uint32				SupportedChildOperations(Delegate* child,
									uint32 mask) = 0;

	// Self Modification

	virtual	status_t			Defragment() = 0;
	virtual	status_t			Repair(bool checkOnly) = 0;

	virtual	status_t			ValidateResize(off_t* size) const = 0;
	virtual	status_t			ValidateResizeChild(Delegate* child,
									off_t* size) const = 0;
	virtual	status_t			Resize(off_t size) = 0;
	virtual	status_t			ResizeChild(Delegate* child, off_t size) = 0;

	virtual	status_t			ValidateMove(off_t* offset) const = 0;
	virtual	status_t			ValidateMoveChild(Delegate* child,
									off_t* offset) const = 0;
	virtual	status_t			Move(off_t offset) = 0;
	virtual	status_t			MoveChild(Delegate* child, off_t offset) = 0;

	virtual	status_t			ValidateSetContentName(BString* name) const = 0;
	virtual	status_t			ValidateSetName(Delegate* child,
									BString* name) const = 0;
	virtual	status_t			SetContentName(const char* name) = 0;
	virtual	status_t			SetName(Delegate* child, const char* name) = 0;

	virtual	status_t			ValidateSetType(Delegate* child,
									const char* type) const = 0;
	virtual	status_t			SetType(Delegate* child, const char* type) = 0;

	virtual	status_t			GetContentParameterEditor(
									BDiskDeviceParameterEditor** editor)
									const = 0;
	virtual	status_t			GetParameterEditor(Delegate* child,
									BDiskDeviceParameterEditor** editor)
									const = 0;
	virtual	status_t			SetContentParameters(
									const char* parameters) = 0;
	virtual	status_t			SetParameters(Delegate* child,
									const char* parameters) = 0;

	virtual	bool				CanInitialize(const char* diskSystem) const = 0;
	virtual	status_t			GetInitializationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor)
									const = 0;
	virtual	status_t			ValidateInitialize(const char* diskSystem,
									BString* name, const char* parameters) = 0;
	virtual	status_t			Initialize(const char* diskSystem,
									const char* name,
									const char* parameters) = 0;
	virtual	status_t			Uninitialize() = 0;
	
	// Modification of child partitions

	virtual	status_t			GetChildCreationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor)
									const = 0;
	virtual	status_t			ValidateCreateChild(off_t* start, off_t* size,
									const char* type,
									const char* parameters) const = 0;
	virtual	status_t			CreateChild(off_t start, off_t size,
									const char* type, const char* parameters,
									BPartition** child) = 0;
	
	virtual	status_t			DeleteChild(Delegate* child) = 0;


protected:
			BPartition*			fPartition;
};


class BPartition::MutableDelegate : BPartition::Delegate {
public:
								MutableDelegate(BPartition* partition);
	virtual						~MutableDelegate();

			BMutablePartition*	MutablePartition();
			const BMutablePartition* MutablePartition() const;

	virtual	status_t			Init(const user_partition_data* partitionData);

	virtual	const user_partition_data* PartitionData() const;

	virtual	Delegate*			ChildAt(int32 index) const;
	virtual	int32				CountChildren() const;

	virtual	uint32				SupportedOperations(uint32 mask);
	virtual	uint32				SupportedChildOperations(Delegate* child,
									uint32 mask);

	// Self Modification

	virtual	status_t			Defragment();
	virtual	status_t			Repair(bool checkOnly);

	virtual	status_t			ValidateResize(off_t* size) const = 0;
	virtual	status_t			ValidateResizeChild(Delegate* child,
									off_t* size) const = 0;
	virtual	status_t			Resize(off_t size) = 0;
	virtual	status_t			ResizeChild(Delegate* child, off_t size) = 0;

	virtual	status_t			ValidateMove(off_t* offset) const = 0;
	virtual	status_t			ValidateMoveChild(Delegate* child,
									off_t* offset) const = 0;
	virtual	status_t			Move(off_t offset) = 0;
	virtual	status_t			MoveChild(Delegate* child, off_t offset) = 0;

	virtual	status_t			ValidateSetContentName(BString* name) const;
	virtual	status_t			ValidateSetName(Delegate* child,
									BString* name) const;
	virtual	status_t			SetContentName(const char* name);
	virtual	status_t			SetName(Delegate* child, const char* name);

	virtual	status_t			ValidateSetType(Delegate* child,
									const char* type) const;
	virtual	status_t			SetType(Delegate* child, const char* type);

	virtual	status_t			GetContentParameterEditor(
									BDiskDeviceParameterEditor** editor) const;
	virtual	status_t			GetParameterEditor(Delegate* child,
									BDiskDeviceParameterEditor** editor) const;
	virtual	status_t			SetContentParameters(const char* parameters);
	virtual	status_t			SetParameters(Delegate* child,
									const char* parameters);

	virtual	bool				CanInitialize(const char* diskSystem) const;
	virtual	status_t			GetInitializationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor) const;
	virtual	status_t			ValidateInitialize(const char* diskSystem,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(const char* diskSystem,
									const char* name,
									const char* parameters);
	virtual	status_t			Uninitialize();
	
	// Modification of child partitions

	virtual	status_t			GetChildCreationParameterEditor(
									const char* system,
									BDiskDeviceParameterEditor** editor) const;
	virtual	status_t			ValidateCreateChild(off_t* start, off_t* size,
									const char* type,
									const char* parameters) const;
	virtual	status_t			CreateChild(off_t start, off_t size,
									const char* type, const char* parameters,
									BPartition** child);
	
	virtual	status_t			DeleteChild(Delegate* child);

private:
			void				_FreeHandle();

private:
//			friend class BMutablePartition;

			BMutablePartition	fMutablePartition;
			BDiskSystemAddOn*	fDiskSystem;
			BPartitionHandle*	fPartitionHandle;
};


#endif	// _PARTITION_DELEGATE_H
