// KDiskDeviceUserDataWriter.h

#ifndef _K_DISK_DEVICE_USER_DATA_WRITER_H
#define _K_DISK_DEVICE_USER_DATA_WRITER_H

#include <SupportDefs.h>

struct user_disk_device_data;
struct user_partition_data;

namespace BPrivate {
namespace DiskDevice {

class UserDataWriter {
public:
	UserDataWriter();
	UserDataWriter(user_disk_device_data *buffer, size_t bufferSize);
	~UserDataWriter();

	status_t SetTo(user_disk_device_data *buffer, size_t bufferSize);
	void Unset();

	void *AllocateData(size_t size, size_t align = 1);
	user_partition_data *AllocatePartitionData(size_t childCount);
	user_disk_device_data *AllocateDeviceData(size_t childCount);

	char *PlaceString(const char *str);

	size_t AllocatedSize() const;

	status_t AddRelocationEntry(void *address);
	status_t Relocate(void *address);

private:
	struct RelocationEntryList;

	user_disk_device_data	*fBuffer;
	size_t					fBufferSize;
	size_t					fAllocatedSize;
	RelocationEntryList		*fRelocationEntries;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::UserDataWriter;

#endif	// _K_DISK_DEVICE_USER_DATA_WRITER_H
