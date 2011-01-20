/*
 * Copyright 2003-2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003, Tyler Akidau, haiku@akidau.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PARTITION_H
#define _PARTITION_H


#include <DiskDeviceDefs.h>
#include <Messenger.h>
#include <Mime.h>


class BBitmap;
class BDiskDevice;
class BPartitionParameterEditor;
class BDiskDeviceVisitor;
class BDiskSystem;
class BMutablePartition;
template <class T> class BObjectList;
class BPartitioningInfo;
class BPath;
class BVolume;
struct user_partition_data;


namespace BPrivate {
	class DiskDeviceJobGenerator;
}

class BPartition {
public:
	// Partition Info

			off_t				Offset() const;		// 0 for devices
			off_t				Size() const;
			off_t				ContentSize() const;	// 0 if uninitialized
			uint32				BlockSize() const;
			int32				Index() const;		// 0 for devices
			uint32				Status() const;

			bool				ContainsFileSystem() const;
			bool				ContainsPartitioningSystem() const;

			bool				IsDevice() const;
			bool				IsReadOnly() const;
			bool				IsMounted() const;
			bool				IsBusy() const;

			uint32				Flags() const;

			const char*			Name() const;
			const char*			ContentName() const;
			const char*			Type() const;			// See DiskDeviceTypes.h
			const char*			ContentType() const;	// See DiskDeviceTypes.h
			partition_id		ID() const;
			const char*			Parameters() const;
			const char*			ContentParameters() const;

			status_t			GetDiskSystem(BDiskSystem* diskSystem) const;

	virtual	status_t			GetPath(BPath* path) const;
			status_t			GetVolume(BVolume* volume) const;
			status_t			GetIcon(BBitmap* icon, icon_size which) const;
			status_t			GetIcon(uint8** _data, size_t* _size,
									type_code* _type) const;
			status_t			GetMountPoint(BPath* mountPoint) const;

			dev_t				Mount(const char* mountPoint = NULL,
									uint32 mountFlags = 0,
									const char* parameters = NULL);
			status_t			Unmount(uint32 unmountFlags = 0);

	// Hierarchy Info

			BDiskDevice*		Device() const;
			BPartition*			Parent() const;
			BPartition*			ChildAt(int32 index) const;
			int32				CountChildren() const;
			int32				CountDescendants() const;
			BPartition*			FindDescendant(partition_id id) const;

			status_t			GetPartitioningInfo(
									BPartitioningInfo* info) const;

			BPartition*			VisitEachChild(BDiskDeviceVisitor* visitor)
									const;
	virtual BPartition*			VisitEachDescendant(
									BDiskDeviceVisitor* visitor) const;

	// Self Modification

			bool				CanDefragment(bool* whileMounted = NULL) const;
			status_t			Defragment() const;

			bool				CanRepair(bool checkOnly,
									bool* whileMounted = NULL) const;
			status_t			Repair(bool checkOnly) const;

			bool				CanResize(bool* canResizeContents = NULL,
									bool* whileMounted = NULL) const;
			status_t			ValidateResize(off_t* size) const;
			status_t			Resize(off_t size);

			bool				CanMove(BObjectList<BPartition>*
										unmovableDescendants = NULL,
									BObjectList<BPartition>*
										movableOnlyIfUnmounted = NULL) const;
			status_t			ValidateMove(off_t* newOffset) const;
			status_t			Move(off_t newOffset);

			bool				CanSetName() const;
			status_t			ValidateSetName(BString* name) const;
									// adjusts name to be suitable
			status_t			SetName(const char* name);

			bool				CanSetContentName(
									bool* whileMounted = NULL) const;
			status_t			ValidateSetContentName(BString* name) const;
									// adjusts name to be suitable
			status_t			SetContentName(const char* name);

			bool				CanSetType() const;
			status_t			ValidateSetType(const char* type) const;
									// type must be one the parent disk system's
									// GetNextSupportedType() returns.
			status_t			SetType(const char* type);

			bool				CanEditParameters() const;
			status_t			GetParameterEditor(
									B_PARAMETER_EDITOR_TYPE type,
									BPartitionParameterEditor** editor);
			status_t			SetParameters(const char* parameters);

			bool				CanEditContentParameters(
									bool* whileMounted = NULL) const;
			status_t			SetContentParameters(const char* parameters);

			status_t			GetNextSupportedType(int32 *cookie,
									BString* type) const;
									// Returns all partition types for this
									// partition supported by the parent disk
									// system.
			status_t			GetNextSupportedChildType(int32 *cookie,
									BString* type) const;
									// Returns all partition types for a child
									// of this partition supported by its disk
									// system.
			bool				IsSubSystem(const char* diskSystem) const;

			bool				CanInitialize(const char* diskSystem) const;
			status_t			ValidateInitialize(const char* diskSystem,
									BString* name, const char* parameters);
			status_t			Initialize(const char* diskSystem,
									const char* name, const char* parameters);
			status_t			Uninitialize();

	// Modification of child partitions

			bool				CanCreateChild() const;
			status_t			ValidateCreateChild(off_t* start, off_t* size,
									const char* type, BString* name,
									const char* parameters) const;
			status_t			CreateChild(off_t start, off_t size,
									const char* type, const char* name,
									const char* parameters,
									BPartition** child = NULL);

			bool				CanDeleteChild(int32 index) const;
			status_t			DeleteChild(int32 index);

			bool				SupportsChildName() const;

private:
			class Delegate;

								BPartition();
								BPartition(const BPartition&);
	virtual						~BPartition();

			BPartition&			operator=(const BPartition&);

			status_t			_SetTo(BDiskDevice* device, BPartition* parent,
									user_partition_data* data);
			void				_Unset();
			status_t			_RemoveObsoleteDescendants(
									user_partition_data* data, bool* updated);
			status_t			_Update(user_partition_data* data,
									bool* updated);
			void				_RemoveChild(int32 index);

			BPartition*			_ChildAt(int32 index) const;
			int32				_CountChildren() const;
			int32				_CountDescendants() const;

			int32				_Level() const;
			virtual	bool		_AcceptVisitor(BDiskDeviceVisitor* visitor,
									int32 level);
			BPartition*			_VisitEachDescendant(
									BDiskDeviceVisitor* visitor,
									int32 level = -1);

			const user_partition_data* _PartitionData() const;

			bool				_HasContent() const;
			bool				_SupportsOperation(uint32 flag,
									uint32 whileMountedFlag,
									bool* whileMounted) const;
			bool				_SupportsChildOperation(const BPartition* child,
									uint32 flag) const;

			status_t			_CreateDelegates();
			status_t			_InitDelegates();
			void				_DeleteDelegates();
			bool				_IsModified() const;

			friend class BDiskDevice;
			friend class BDiskSystem;
			friend class BMutablePartition;
			friend class BPrivate::DiskDeviceJobGenerator;

			BDiskDevice*		fDevice;
			BPartition*			fParent;
			user_partition_data* fPartitionData;
			Delegate*			fDelegate;
};


#endif	// _PARTITION_H
