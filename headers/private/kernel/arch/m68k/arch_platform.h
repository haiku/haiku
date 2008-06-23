/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_M68K_ARCH_PLATFORM_H
#define _KERNEL_M68K_ARCH_PLATFORM_H

#include <arch/platform.h>

struct real_time_data;

enum m68k_platform_type {
	M68K_PLATFORM_AMIGA = 0,
	M68K_PLATFORM_ATARI,		/* TT, Falcon, Hades, Milan... */
	M68K_PLATFORM_MAC,
	M68K_PLATFORM_NEXT
};

namespace BPrivate {

// implemented in src/system/kernel/arch/m68k/arch_platform.cpp

class M68KPlatform {
public:
	M68KPlatform(m68k_platform_type platformType);
	virtual ~M68KPlatform();

	static M68KPlatform *Default();

	inline m68k_platform_type PlatformType() const	{ return fPlatformType; }

	virtual status_t Init(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPostVM(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPIC(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data) = 0;
	virtual status_t InitTimer(struct kernel_args *kernelArgs) = 0;

	virtual char SerialDebugGetChar() = 0;
	virtual void SerialDebugPutChar(char c) = 0;

	virtual	void SetHardwareRTC(uint32 seconds) = 0;
	virtual	uint32 GetHardwareRTC() = 0;

	virtual void SetHardwareTimer(bigtime_t timeout) = 0;
	virtual void ClearHardwareTimer(void) = 0;

	virtual	void ShutDown(bool reboot) = 0;

private:
	m68k_platform_type	fPlatformType;
};


// #pragma mark - Amiga



// #pragma mark - Atari (Falcon)
// implemented in src/system/kernel/platform/atari_m68k/platform.cpp

class M68KAtari : public M68KPlatform {
public:
	M68KAtari();
	virtual ~M68KAtari();

	virtual status_t Init(struct kernel_args *kernelArgs);
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs);
	virtual status_t InitPostVM(struct kernel_args *kernelArgs);
	virtual status_t InitPIC(struct kernel_args *kernelArgs);
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data);
	virtual status_t InitTimer(struct kernel_args *kernelArgs);

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	virtual	void SetHardwareRTC(uint32 seconds);
	virtual	uint32 GetHardwareRTC();

	virtual void SetHardwareTimer(bigtime_t timeout);
	virtual void ClearHardwareTimer(void);

	virtual	void ShutDown(bool reboot);

private:
	int	fRTC;
};

// #pragma mark - Mac

}	// namespace BPrivate

using BPrivate::M68KPlatform;


#endif	// _KERNEL_M68K_ARCH_PLATFORM_H
