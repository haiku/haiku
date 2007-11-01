/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_DISK_DEVICE_SYSTEM_H
#define _K_DISK_DEVICE_SYSTEM_H

#include "disk_device_manager.h"

struct user_disk_system_info;

namespace BPrivate {
namespace DiskDevice {

//class KDiskDeviceJob;
class KPartition;

//!	\brief Common ancestor for disk system add-on wrappers
class KDiskSystem {
public:
	KDiskSystem(const char *name);
	virtual ~KDiskSystem();

	virtual status_t Init();

//	void SetID(disk_system_id id);
	disk_system_id ID() const;
	const char *Name() const;
	const char *PrettyName();
	uint32 Flags() const;

	bool IsFileSystem() const;
	bool IsPartitioningSystem() const;

	void GetInfo(user_disk_system_info *info);

	// manager will be locked
	status_t Load();		// load/unload -- can be nested
	void Unload();			//
	bool IsLoaded() const;

	// Scanning
	// Device must be write locked.

	virtual float Identify(KPartition *partition, void **cookie);
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeCookie(KPartition *partition);
	virtual void FreeContentCookie(KPartition *partition);

	// Writing
	// Device should not be locked.

#if 0
	virtual status_t Defragment(KPartition *partition, KDiskDeviceJob *job);
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
	virtual status_t Initialize(KPartition *partition, const char *name,
								const char *parameters, KDiskDeviceJob *job);
	virtual status_t CreateChild(KPartition *partition, off_t offset,
								 off_t size, const char *type,
								 const char *parameters, KDiskDeviceJob *job,
								 KPartition **child = NULL,
								 partition_id childID = -1);
	virtual status_t DeleteChild(KPartition *child, KDiskDeviceJob *job);
// The KPartition* parameters for the writing methods are a bit `volatile',
// since the device will not be locked, when they are called. The KPartition
// is registered though, so that it is at least guaranteed that the object
// won't go away.
#endif // 0

protected:
	virtual status_t LoadModule();
	virtual void UnloadModule();

	status_t SetPrettyName(const char *name);
	void SetFlags(uint32 flags);

	static int32 _NextID();

private:
	disk_system_id	fID;
	char			*fName;
	char			*fPrettyName;
	uint32			fFlags;
	int32			fLoadCounter;

	static int32	fNextID;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskSystem;

#endif	// _K_DISK_DEVICE_SYSTEM_H
