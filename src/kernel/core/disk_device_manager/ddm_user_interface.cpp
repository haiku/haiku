// ddm_userland_interface.cpp

#include <KDiskDevice.h>
#include <KDiskDeviceManager.h>
#include <KDiskDeviceUtils.h>

#include "ddm_userland_interface.h"
#include "UserDataWriter.h"

// _kern_get_next_disk_device_id
partition_id
_kern_get_next_disk_device_id(int32 *cookie, size_t *neededSize)
{
	if (!cookie)
		return B_BAD_VALUE;
	partition_id id = B_ERROR;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterNextDevice(cookie)) {
		PartitionRegistrar _(device, true);
		id = device->ID();
		if (neededSize) {
			if (DeviceReadLocker locker = device) {
				UserDataWriter writer;
				device->WriteUserData(writer, false);
				*neededSize = writer.AllocatedSize();
			} else
				return B_ERROR;
		}
	}
	return id;
}

// _kern_get_disk_device_data
status_t
_kern_get_disk_device_data(partition_id deviceID, bool shadow,
						   user_disk_device_data *buffer, size_t bufferSize,
						   size_t *neededSize)
{
	if (!buffer && bufferSize > 0)
		return B_BAD_VALUE;
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	if (KDiskDevice *device = manager->RegisterDevice(deviceID)) {
		PartitionRegistrar _(device, true);
		if (DeviceReadLocker locker = device) {
			UserDataWriter writer(buffer, bufferSize);
			device->WriteUserData(writer, shadow);
			if (neededSize)
				*neededSize = writer.AllocatedSize();
			if (writer.AllocatedSize() <= bufferSize)
				return B_OK;
		}
	}
	return B_ERROR;
}

