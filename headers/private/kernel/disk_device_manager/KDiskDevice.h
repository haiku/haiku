// KDiskDevice.h

#ifndef _K_DISK_DEVICE_H
#define _K_DISK_DEVICE_H

#include <OS.h>

#include "KPhysicalPartition.h"
#include "RWLocker.h"

namespace BPrivate {
namespace DiskDevice {

class UserDataWriter;

class KDiskDevice : public KPhysicalPartition {
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
	bool IsReadLocked(bool orWriteLocked = true);
	bool WriteLock();
	void WriteUnlock();
	bool IsWriteLocked();

	virtual bool PrepareForRemoval();

	virtual void SetID(partition_id id);

	virtual status_t PublishDevice();
	virtual status_t UnpublishDevice();

	void SetDeviceFlags(uint32 flags);	// comprises the ones below
	uint32 DeviceFlags() const;
	bool IsReadOnlyMedia() const;
	bool IsWriteOnce() const;
	bool IsRemovable() const;
	bool HasMedia() const;

	status_t SetPath(const char *path);
		// TODO: Remove this method or make it private. Once initialized the
		// path must not be changed.
	const char *Path() const;
	virtual status_t GetPath(KPath *path) const;

	// File descriptor: Set only from a kernel thread, valid only for
	// kernel threads.
	void SetFD(int fd);
	int FD() const;

	// access to C style device data
	disk_device_data *DeviceData();
	const disk_device_data *DeviceData() const;

	status_t CreateShadowDevice(team_id team);
	status_t DeleteShadowDevice();
	void SetShadowOwner(team_id team);
	team_id ShadowOwner() const;

	void WriteUserData(UserDataWriter &writer, bool shadow);

	virtual void Dump(bool deep = true, int32 level = 0);

protected:
	virtual status_t GetMediaStatus(status_t *mediaStatus);
	virtual status_t GetGeometry(device_geometry *geometry);

private:
	void _InitPartitionData();

	disk_device_data	fDeviceData;
	RWLocker			fLocker;
	int					fFD;
	status_t			fMediaStatus;
	team_id				fShadowOwner;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDevice;

#endif	// _K_DISK_DEVICE_H
