/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_M68K_ARCH_PLATFORM_H
#define _KERNEL_M68K_ARCH_PLATFORM_H

#include <OS.h>
#include <arch/platform.h>

struct real_time_data;

typedef enum m68k_platform_types {
	M68K_PLATFORM_AMIGA = 0,
	M68K_PLATFORM_ATARI,		/* TT, Falcon, Hades, Milan... */
	M68K_PLATFORM_MAC,
	M68K_PLATFORM_NEXT
} m68k_platform_type;

namespace BPrivate {

// implemented in src/system/kernel/arch/m68k/arch_platform.cpp

class M68KPlatform {
public:
	M68KPlatform(platform_type platformType, m68k_platform_type m68kPlatformType);
	virtual ~M68KPlatform();

	static M68KPlatform *Default();

	inline platform_type PlatformType() const	{ return fPlatformType; }
	inline m68k_platform_type M68KPlatformType() const	{ return fM68KPlatformType; }

	virtual status_t Init(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPostVM(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitPIC(struct kernel_args *kernelArgs) = 0;
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data) = 0;
	virtual status_t InitTimer(struct kernel_args *kernelArgs) = 0;

	virtual char BlueScreenGetChar() = 0;

	virtual char SerialDebugGetChar() = 0;
	virtual void SerialDebugPutChar(char c) = 0;

	virtual void EnableIOInterrupt(int irq) = 0;
	virtual void DisableIOInterrupt(int irq) = 0;
	virtual bool AcknowledgeIOInterrupt(int irq) = 0;

	// mimic the PC CMOS
	virtual uint8 ReadRTCReg(uint8 reg) = 0;
	virtual void WriteRTCReg(uint8 reg, uint8 val) = 0;
	virtual	void SetHardwareRTC(uint32 seconds) = 0;
	virtual	uint32 GetHardwareRTC() = 0;

	virtual void SetHardwareTimer(bigtime_t timeout) = 0;
	virtual void ClearHardwareTimer(void) = 0;

	virtual	void ShutDown(bool reboot) = 0;

private:
	m68k_platform_type	fM68KPlatformType;
	platform_type	fPlatformType;
};


}	// namespace BPrivate

using BPrivate::M68KPlatform;

//extern "C" M68KPlatform *instanciate_m68k_platform_amiga();
extern "C" M68KPlatform *instanciate_m68k_platform_atari();
//extern "C" M68KPlatform *instanciate_m68k_platform_mac();
//extern "C" M68KPlatform *instanciate_m68k_platform_next();


#endif	// _KERNEL_M68K_ARCH_PLATFORM_H
