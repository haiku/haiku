/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_PPC_ARCH_PLATFORM_H
#define _KERNEL_PPC_ARCH_PLATFORM_H

#include <arch/platform.h>

struct real_time_data;

namespace BPrivate {

class PPCPlatform {
public:
	PPCPlatform();
	virtual ~PPCPlatform();

	static PPCPlatform *Default();

	virtual status_t Init(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPostVM(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data) = 0;

	virtual char SerialDebugGetChar() = 0;
	virtual void SerialDebugPutChar(char c) = 0;

	virtual	void SetHardwareRTC(uint32 seconds) = 0;
	virtual	uint32 GetHardwareRTC() = 0;

	virtual	void ShutDown(bool reboot) = 0;
};

}	// namespace BPrivate

using BPrivate::PPCPlatform;


#endif	// _KERNEL_PPC_ARCH_PLATFORM_H
