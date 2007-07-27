/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_PARTITIONING_DISK_DEVICE_SYSTEM_H
#define _K_PARTITIONING_DISK_DEVICE_SYSTEM_H

#include "KDiskSystem.h"

struct partition_module_info;

namespace BPrivate {
namespace DiskDevice {

/**
 * 	\brief Wrapper for the C interface of a partitioning system add-on.
 *
 * 	See \ref ddm_modules.h for better description of the interface.
 */
class KPartitioningSystem : public KDiskSystem {
public:
	KPartitioningSystem(const char *name);
	virtual ~KPartitioningSystem();

	virtual status_t Init();

	// Scanning

	//! Try to identify a given partition
	virtual float Identify(KPartition *partition, void **cookie);
	//! Scan the partition
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeCookie(KPartition *partition);
	virtual void FreeContentCookie(KPartition *partition);

	// Querying

	//! Check whether the add-on supports repairing this partition.
	virtual bool SupportsRepairing(KPartition *partition, bool checkOnly,
								   bool *whileMounted);
	//! Check whether the add-on supports resizing this partition.
	virtual bool SupportsResizing(KPartition *partition, bool *whileMounted);
	//! Check whether the add-on supports resizing children of this partition.
	virtual bool SupportsResizingChild(KPartition *child);
	//! Check whether the add-on supports moving this partition.
	virtual bool SupportsMoving(KPartition *partition, bool *isNoOp);
	//! Check whether the add-on supports moving children of this partition.
	virtual bool SupportsMovingChild(KPartition *child);
	//! Check whether the add-on supports setting name of this partition.
	virtual bool SupportsSettingName(KPartition *partition);
	//! Check whether the add-on supports setting name to content of this partition.
	virtual bool SupportsSettingContentName(KPartition *partition,
											bool *whileMounted);
	//! Check whether the add-on supports setting type of this partition.
	virtual bool SupportsSettingType(KPartition *partition);
	//! Check whether the add-on supports setting parameters of this partition.
	virtual bool SupportsSettingParameters(KPartition *partition);
	//! Check whether the add-on supports setting parameters to content of this partition.
	virtual bool SupportsSettingContentParameters(KPartition *partition,
												  bool *whileMounted);
	//! Check whether the add-on supports initializing this partition.
	virtual bool SupportsInitializing(KPartition *partition);
	//! Check whether the add-on supports initializing a child of this partition.
	virtual bool SupportsInitializingChild(KPartition *child,
										   const char *diskSystem);
	//! Check whether the add-on supports creating children of this partition.
	virtual bool SupportsCreatingChild(KPartition *partition);
	//! Check whether the add-on supports deleting children of this partition.
	virtual bool SupportsDeletingChild(KPartition *child);
	//! Check whether the add-on is a subsystem for a given partition.
	virtual bool IsSubSystemFor(KPartition *partition);

	//! Validates parameters for resizing a partition
	virtual bool ValidateResize(KPartition *partition, off_t *size);
	//! Validates parameters for resizing a child partition
	virtual bool ValidateResizeChild(KPartition *child, off_t *size);
	//! Validates parameters for moving a partition
	virtual bool ValidateMove(KPartition *partition, off_t *start);
	//! Validates parameters for moving a child partition
	virtual bool ValidateMoveChild(KPartition *child, off_t *start);
	//! Validates parameters for setting name of a partition
	virtual bool ValidateSetName(KPartition *partition, char *name);
	//! Validates parameters for setting name to content of a partition
	virtual bool ValidateSetContentName(KPartition *partition, char *name);
	//! Validates parameters for setting type of a partition
	virtual bool ValidateSetType(KPartition *partition, const char *type);
	//! Validates parameters for setting parameters of a partition
	virtual bool ValidateSetParameters(KPartition *partition,
									   const char *parameters);
	//! Validates parameters for setting parameters to content of a partition
	virtual bool ValidateSetContentParameters(KPartition *parameters,
											  const char *parameters);
	//! Validates parameters for initializing a partition
	virtual bool ValidateInitialize(KPartition *partition, char *name,
									const char *parameters);
	//! Validates parameters for creating child of a partition
	virtual bool ValidateCreateChild(KPartition *partition, off_t *start,
									 off_t *size, const char *type,
									 const char *parameters, int32 *index);
	//! Counts partitionable spaces on a partition
	virtual int32 CountPartitionableSpaces(KPartition *partition);
	//! Retrieves a list of partitionable spaces on a partition
	virtual status_t GetPartitionableSpaces(KPartition *partition,
											partitionable_space_data *buffer,
											int32 count,
											int32 *actualCount = NULL);

	//! Iterates through supported partition types
	virtual status_t GetNextSupportedType(KPartition *partition, int32 *cookie,
										  char *type);
	//! Translates the "pretty" content type to an internal type
	virtual status_t GetTypeForContentType(const char *contentType,
										   char *type);

	// Shadow partition modification

	//! Calls for additional modifications when shadow partition is changed
	virtual status_t ShadowPartitionChanged(KPartition *partition,
											uint32 operation);

	// Writing

	//! Repairs a partition
	virtual status_t Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job);
	//! Resizes a partition
	virtual status_t Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job);
	//! Resizes child of a partition
	virtual status_t ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job);
	//! Moves a partition
	virtual status_t Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job);
	//! Moves child of a partition
	virtual status_t MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job);
	//! Sets name of a partition
	virtual status_t SetName(KPartition *partition, char *name,
							 KDiskDeviceJob *job);
	//! Sets name of the content of a partition
	virtual status_t SetContentName(KPartition *partition, char *name,
									KDiskDeviceJob *job);
	//! Sets type of a partition
	virtual status_t SetType(KPartition *partition, char *type,
							 KDiskDeviceJob *job);
	//! Sets parameters of a partition
	virtual status_t SetParameters(KPartition *partition,
								   const char *parameters,
								   KDiskDeviceJob *job);
	//! Sets parameters to content of a partition
	virtual status_t SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job);
	//! Creates a child partition
	virtual status_t CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *type,
								 const char *parameters, KDiskDeviceJob *job,
								 KPartition **child = NULL,
								 partition_id childID = -1);
	//! Deletes a child partition
	virtual status_t DeleteChild(KPartition *child, KDiskDeviceJob *job);
	//! Initializes a partition with this partitioning system
	virtual status_t Initialize(KPartition *partition, const char *name,
								const char *parameters, KDiskDeviceJob *job);

protected:
	virtual status_t LoadModule();
	virtual void UnloadModule();

private:
	partition_module_info	*fModule;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartitioningSystem;

#endif	// _K_PARTITIONING_DISK_DEVICE_SYSTEM_H
