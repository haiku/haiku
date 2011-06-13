/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
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
								KPartitioningSystem(const char* name);
	virtual						~KPartitioningSystem();

	virtual	status_t			Init();

	// Scanning

	virtual	float				Identify(KPartition* partition, void** cookie);
	virtual	status_t			Scan(KPartition* partition, void* cookie);
	virtual	void				FreeIdentifyCookie(KPartition* partition,
									void* cookie);
	virtual	void				FreeCookie(KPartition* partition);
	virtual	void				FreeContentCookie(KPartition* partition);

	// Writing

	virtual	status_t			Repair(KPartition* partition, bool checkOnly,
									disk_job_id job);
	virtual	status_t			Resize(KPartition* partition, off_t size,
									disk_job_id job);
	virtual	status_t			ResizeChild(KPartition* child, off_t size,
									disk_job_id job);
	virtual	status_t			Move(KPartition* partition, off_t offset,
									disk_job_id job);
	virtual	status_t			MoveChild(KPartition* child, off_t offset,
									disk_job_id job);
	virtual	status_t			SetName(KPartition* partition, const char* name,
									disk_job_id job);
	virtual	status_t			SetContentName(KPartition* partition,
									const char* name, disk_job_id job);
	virtual	status_t			SetType(KPartition* partition, const char* type,
									disk_job_id job);
	virtual	status_t			SetParameters(KPartition* partition,
									const char* parameters, disk_job_id job);
	virtual	status_t			SetContentParameters(KPartition* partition,
									const char* parameters, disk_job_id job);
	virtual	status_t			Initialize(KPartition* partition,
									const char* name, const char* parameters,
									disk_job_id job);
	virtual	status_t			Uninitialize(KPartition* partition,
									disk_job_id job);
	virtual	status_t			CreateChild(KPartition* partition, off_t offset,
									off_t size, const char* type,
									const char* name, const char* parameters,
									disk_job_id job, KPartition** child = NULL,
									partition_id childID = -1);
	virtual	status_t			DeleteChild(KPartition* child, disk_job_id job);

protected:
	virtual	status_t			LoadModule();
	virtual	void				UnloadModule();

private:
			partition_module_info* fModule;
};


} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartitioningSystem;

#endif	// _K_PARTITIONING_DISK_DEVICE_SYSTEM_H
