// KDiskDevice.h

#ifndef _K_DISK_DEVICE_H
#define _K_DISK_DEVICE_H

#include <OS.h>

#include "KPartition.h"
#include "RWLocker.h"

// disk device flags
// TODO: move to another header (must be accessible from userland API impl.)
enum {
	B_DISK_DEVICE_REMOVABLE		= 0x01,
	B_DISK_DEVICE_HAS_MEDIA		= 0x02,
};

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice : public KPartition {
public:
	KDiskDevice(partition_id id = -1);
	virtual ~KDiskDevice();

	status_t SetTo(const char *path);
	void Unset();
	status_t InitCheck() const;
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

	void SetDeviceFlags(uint32 flags);	// comprises the ones below
	uint32 DeviceFlags() const;
	bool IsRemovable() const;
	bool HasMedia() const;

	status_t SetPath(const char *path);
		// TODO: Remove this method or make it private. Once initialized the
		// path must not be changed.
	const char *Path() const;
	virtual status_t GetPath(char *path) const;

	// File descriptor: Set only from a kernel thread, valid only for
	// kernel threads.
	void SetFD(int fd);
	int FD() const;

	// access to C style device data
	disk_device_data *DeviceData();
	const disk_device_data *DeviceData() const;

	virtual KPartition *CreateShadowPartition();

	void SetShadowOwner(team_id team);
	team_id ShadowOwner() const;

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
