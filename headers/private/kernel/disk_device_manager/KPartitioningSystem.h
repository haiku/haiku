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

	virtual float Identify(KPartition *partition, void **cookie);
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeCookie(KPartition *partition);
	virtual void FreeContentCookie(KPartition *partition);

	// Querying

	virtual uint32 GetSupportedOperations(KPartition* partition, uint32 mask);
	virtual uint32 GetSupportedChildOperations(KPartition* child, uint32 mask);

	virtual bool SupportsInitializingChild(KPartition *child,
		const char *diskSystem);
	virtual bool IsSubSystemFor(KPartition *partition);

	virtual bool ValidateResize(KPartition *partition, off_t *size);
	virtual bool ValidateResizeChild(KPartition *child, off_t *size);
	virtual bool ValidateMove(KPartition *partition, off_t *start);
	virtual bool ValidateMoveChild(KPartition *child, off_t *start);
	virtual bool ValidateSetName(KPartition *partition, char *name);
	virtual bool ValidateSetContentName(KPartition *partition, char *name);
	virtual bool ValidateSetType(KPartition *partition, const char *type);
	virtual bool ValidateSetParameters(KPartition *partition,
									   const char *parameters);
	virtual bool ValidateSetContentParameters(KPartition *parameters,
											  const char *parameters);
	virtual bool ValidateInitialize(KPartition *partition, char *name,
									const char *parameters);
	virtual bool ValidateCreateChild(KPartition *partition, off_t *start,
									 off_t *size, const char *type,
									 const char *parameters, int32 *index);
	virtual int32 CountPartitionableSpaces(KPartition *partition);
	virtual status_t GetPartitionableSpaces(KPartition *partition,
											partitionable_space_data *buffer,
											int32 count,
											int32 *actualCount = NULL);

	virtual status_t GetNextSupportedType(KPartition *partition, int32 *cookie,
										  char *type);
	virtual status_t GetTypeForContentType(const char *contentType,
										   char *type);

	// Shadow partition modification

	virtual status_t ShadowPartitionChanged(KPartition *partition,
											uint32 operation);

	// Writing

	virtual status_t Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job);
	virtual status_t Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job);
	virtual status_t ResizeChild(KPartition *child, off_t size,
								 KDiskDeviceJob *job);
	virtual status_t Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job);
	virtual status_t MoveChild(KPartition *child, off_t offset,
							   KDiskDeviceJob *job);
	virtual status_t SetName(KPartition *partition, char *name,
							 KDiskDeviceJob *job);
	virtual status_t SetContentName(KPartition *partition, char *name,
									KDiskDeviceJob *job);
	virtual status_t SetType(KPartition *partition, char *type,
							 KDiskDeviceJob *job);
	virtual status_t SetParameters(KPartition *partition,
								   const char *parameters,
								   KDiskDeviceJob *job);
	virtual status_t SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job);
	virtual status_t CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *type,
								 const char *parameters, KDiskDeviceJob *job,
								 KPartition **child = NULL,
								 partition_id childID = -1);
	virtual status_t DeleteChild(KPartition *child, KDiskDeviceJob *job);
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
