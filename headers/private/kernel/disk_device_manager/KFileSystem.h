// KFileSystem.h
//
// KFileSystem implements the KDiskSystem interface for file systems.
// It works with the FS API.

#ifndef _K_FILE_DISK_DEVICE_SYSTEM_H
#define _K_FILE_DISK_DEVICE_SYSTEM_H

#include "KDiskSystem.h"

struct file_system_module_info;

namespace BPrivate {
namespace DiskDevice {

class KFileSystem : public KDiskSystem {
public:
	KFileSystem(const char *name);
	virtual ~KFileSystem();

	virtual status_t Init();

	// Scanning

	virtual float Identify(KPartition *partition, void **cookie);
	virtual status_t Scan(KPartition *partition, void *cookie);
	virtual void FreeIdentifyCookie(KPartition *partition, void *cookie);
	virtual void FreeContentCookie(KPartition *partition);

	// Querying

	virtual bool SupportsDefragmenting(KPartition *partition,
									   bool *whileMounted);
	virtual bool SupportsRepairing(KPartition *partition, bool checkOnly,
								   bool *whileMounted);
	virtual bool SupportsResizing(KPartition *partition, bool *whileMounted);
	virtual bool SupportsMoving(KPartition *partition, bool *isNoOp);
	virtual bool SupportsSettingContentName(KPartition *partition,
											bool *whileMounted);
	virtual bool SupportsSettingContentParameters(KPartition *partition,
												  bool *whileMounted);
	virtual bool SupportsInitializing(KPartition *partition);

	virtual bool ValidateResize(KPartition *partition, off_t *size);
	virtual bool ValidateMove(KPartition *partition, off_t *start);
	virtual bool ValidateSetContentName(KPartition *partition, char *name);
	virtual bool ValidateSetContentParameters(KPartition *partition,
											  const char *parameters);
	virtual bool ValidateInitialize(KPartition *partition, char *name,
									const char *parameters);

	// Shadow partition modification

	virtual status_t ShadowPartitionChanged(KPartition *partition,
											uint32 operation);

	// Writing

	virtual status_t Defragment(KPartition *partition, KDiskDeviceJob *job);
	virtual status_t Repair(KPartition *partition, bool checkOnly,
							KDiskDeviceJob *job);
	virtual status_t Resize(KPartition *partition, off_t size,
							KDiskDeviceJob *job);
	virtual status_t Move(KPartition *partition, off_t offset,
						  KDiskDeviceJob *job);
	virtual status_t SetContentName(KPartition *partition, char *name,
									KDiskDeviceJob *job);
	virtual status_t SetContentParameters(KPartition *partition,
										  const char *parameters,
										  KDiskDeviceJob *job);
	virtual status_t Initialize(KPartition *partition, const char *name,
								const char *parameters, KDiskDeviceJob *job);

protected:
	virtual status_t LoadModule();
	virtual void UnloadModule();

private:
	file_system_module_info	*fModule;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KFileSystem;

#endif	// _K_FILE_DISK_DEVICE_SYSTEM_H
