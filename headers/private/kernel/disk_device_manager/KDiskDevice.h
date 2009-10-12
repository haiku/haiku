/*
 * Copyright 2003-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_DISK_DEVICE_H
#define _K_DISK_DEVICE_H


#include <OS.h>

#include <lock.h>

#include "KPartition.h"


namespace BPrivate {
namespace DiskDevice {


class UserDataWriter;


class KDiskDevice : public KPartition {
public:
	KDiskDevice(partition_id id = -1);
	virtual ~KDiskDevice();

	status_t SetTo(const char *path);
	void Unset();
	virtual status_t InitCheck() const;
		// TODO: probably superfluous

	// A read lock owner can be sure that the device (incl. all of its
	// partitions won't be changed).
	// A write lock owner is moreover allowed to make changes.
	// The hierarchy is additionally protected by the disk device manager's
	// lock -- only a device write lock owner is allowed to change it, but
	// manager lock owners can be sure, that it won't change.
	bool ReadLock();
	void ReadUnlock();
	bool WriteLock();
	void WriteUnlock();

	virtual void SetID(partition_id id);

	virtual status_t PublishDevice();
	virtual status_t UnpublishDevice();
	virtual status_t RepublishDevice();

	void SetDeviceFlags(uint32 flags);	// comprises the ones below
	uint32 DeviceFlags() const;
	bool IsReadOnlyMedia() const;
	bool IsWriteOnce() const;
	bool IsRemovable() const;
	bool HasMedia() const;
	bool MediaChanged() const;

	void UpdateMediaStatusIfNeeded();
	void UninitializeMedia();

	void UpdateGeometry();

	status_t SetPath(const char *path);
		// TODO: Remove this method or make it private. Once initialized the
		// path must not be changed.
	const char *Path() const;
	virtual	status_t GetFileName(char* buffer, size_t size) const;
	virtual status_t GetPath(KPath *path) const;

	// File descriptor: Set only from a kernel thread, valid only for
	// kernel threads.
	void SetFD(int fd);
	int FD() const;

	// access to C style device data
	disk_device_data *DeviceData();
	const disk_device_data *DeviceData() const;

	virtual void WriteUserData(UserDataWriter &writer,
		user_partition_data *data);
	void WriteUserData(UserDataWriter &writer);

	virtual void Dump(bool deep = true, int32 level = 0);

protected:
	virtual status_t GetMediaStatus(status_t *mediaStatus);
	virtual status_t GetGeometry(device_geometry *geometry);

private:
	void _ResetGeometry();
	void _InitPartitionData();
	void _UpdateDeviceFlags();

	disk_device_data	fDeviceData;
	rw_lock				fLocker;
	int					fFD;
	status_t			fMediaStatus;
};


} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDevice;

#endif	// _K_DISK_DEVICE_H
