// KFileDiskDevice.h

#ifndef _K_FILE_DISK_DEVICE_H
#define _K_FILE_DISK_DEVICE_H

#include <OS.h>

#include "KDiskDevice.h"

namespace BPrivate {
namespace DiskDevice {

class KFileDiskDevice : public KDiskDevice {
public:
	KFileDiskDevice(partition_id id = -1);
	virtual ~KFileDiskDevice();

	status_t SetTo(const char *filePath, const char *devicePath = NULL);
	void Unset();
	virtual status_t InitCheck() const;
		// TODO: probably superfluous

	const char *FilePath() const;

	virtual KPartition *CreateShadowPartition();

//	virtual void Dump(bool deep = true, int32 level = 0);

protected:
	virtual status_t GetMediaStatus(status_t *mediaStatus);
	virtual status_t GetGeometry(device_geometry *geometry);

private:
	char			*fFilePath;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KFileDiskDevice;

#endif	// _K_FILE_DISK_DEVICE_H
