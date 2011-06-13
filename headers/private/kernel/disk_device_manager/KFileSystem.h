/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *
 * KFileSystem implements the KDiskSystem interface for file systems.
 */
#ifndef _K_FILE_DISK_DEVICE_SYSTEM_H
#define _K_FILE_DISK_DEVICE_SYSTEM_H

#include "KDiskSystem.h"


struct file_system_module_info;


namespace BPrivate {
namespace DiskDevice {


//!	\brief Wrapper for the C interface of a filesystem add-on.
class KFileSystem : public KDiskSystem {
public:
								KFileSystem(const char* name);
	virtual						~KFileSystem();

	virtual	status_t			Init();

	// Scanning

	virtual	float				Identify(KPartition* partition, void** cookie);
	virtual	status_t			Scan(KPartition* partition, void* cookie);
	virtual	void				FreeIdentifyCookie(KPartition* partition,
									void* cookie);
	virtual	void				FreeContentCookie(KPartition* partition);

	// Writing

	virtual	status_t			Defragment(KPartition* partition,
									disk_job_id job);
	virtual	status_t			Repair(KPartition* partition, bool checkOnly,
									disk_job_id job);
	virtual	status_t			Resize(KPartition* partition, off_t size,
									disk_job_id job);
	virtual	status_t			Move(KPartition* partition, off_t offset,
									disk_job_id job);
	virtual	status_t			SetContentName(KPartition* partition,
									const char* name, disk_job_id job);
	virtual	status_t			SetContentParameters(KPartition* partition,
									const char* parameters, disk_job_id job);
	virtual	status_t			Initialize(KPartition* partition,
									const char* name, const char* parameters,
									disk_job_id job);
	virtual	status_t			Uninitialize(KPartition* partition,
									disk_job_id job);

protected:
	virtual	status_t			LoadModule();
	virtual	void				UnloadModule();

private:
			file_system_module_info* fModule;
};


} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KFileSystem;

#endif	// _K_FILE_DISK_DEVICE_SYSTEM_H
