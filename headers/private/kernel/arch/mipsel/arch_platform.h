/*
 * Copyright 2009 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_MIPSEL_PLATFORM_H
#define _KERNEL_ARCH_MIPSEL_PLATFORM_H

#include <arch/platform.h>

#warning IMPLEMENT arch_platform.h

struct real_time_data;

enum mipsel_platform_type {
	MIPSEL_PLATFORM_ROUTERBOARD = 0,
};

namespace BPrivate {

class MipselPlatform {
public:
	MipselPlatform(mipsel_platform_type platformType);
	virtual ~MipselPlatform();

	static MipselPlatform* Default();

	inline mipsel_platform_type PlatformType() const { return fPlatformType; }

	virtual status_t Init(struct kernel_args* kernelArgs) = 0;
	virtual status_t InitSerialDebug(struct kernel_args* kernelArgs) = 0;
	virtual status_t InitPostVM(struct kernel_args* kernelArgs) = 0;
	virtual status_t InitRTC(struct kernel_args* kernelArgs,
		struct real_time_data* data) = 0;

	virtual char SerialDebugGetChar() = 0;
	virtual void SerialDebugPutChar(char c) = 0;

	virtual	void SetHardwareRTC(uint32 seconds) = 0;
	virtual	uint32 GetHardwareRTC() = 0;

	virtual	void ShutDown(bool reboot) = 0;

private:
	mipsel_platform_type	fPlatformType;
};

}	// namespace BPrivate

using BPrivate::MipselPlatform;


#endif /* _KERNEL_ARCH_MIPSEL_PLATFORM_H */

