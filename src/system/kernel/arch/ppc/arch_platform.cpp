/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <arch/generic/debug_uart.h>
#include <boot/kernel_args.h>
#include <platform/openfirmware/openfirmware.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>

#include "fdt_serial.h"

void *gFDT;
static PPCPlatform *sPPCPlatform;


// constructor
PPCPlatform::PPCPlatform(ppc_platform_type platformType)
	: fPlatformType(platformType)
{
}

// destructor
PPCPlatform::~PPCPlatform()
{
}

// Default
PPCPlatform *
PPCPlatform::Default()
{
	return sPPCPlatform;
}


// #pragma mark - Open Firmware


namespace BPrivate {

class PPCOpenFirmware : public PPCPlatform {
public:
	PPCOpenFirmware();
	virtual ~PPCOpenFirmware();

	virtual status_t Init(struct kernel_args *kernelArgs);
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs);
	virtual status_t InitPostVM(struct kernel_args *kernelArgs);
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data);

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	virtual	void SetHardwareRTC(uint32 seconds);
	virtual	uint32 GetHardwareRTC();

	virtual	void ShutDown(bool reboot);

private:
	int	fInput;
	int	fOutput;
	int	fRTC;
};

}	// namespace BPrivate

using BPrivate::PPCOpenFirmware;


// OF debugger commands

// debug_command_of_exit
static int
debug_command_of_exit(int argc, char **argv)
{
	of_exit();
	kprintf("of_exit() failed!\n");
	return 0;
}

// debug_command_of_enter
static int
debug_command_of_enter(int argc, char **argv)
{
	of_call_client_function("enter", 0, 0);
	return 0;
}


// constructor
PPCOpenFirmware::PPCOpenFirmware()
	: PPCPlatform(PPC_PLATFORM_OPEN_FIRMWARE),
	  fInput(-1),
	  fOutput(-1),
	  fRTC(-1)
{
}

// destructor
PPCOpenFirmware::~PPCOpenFirmware()
{
}

// Init
status_t
PPCOpenFirmware::Init(struct kernel_args *kernelArgs)
{
	return of_init(
		(intptr_t(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}

// InitSerialDebug
status_t
PPCOpenFirmware::InitSerialDebug(struct kernel_args *kernelArgs)
{
	if (of_getprop(gChosen, "stdin", &fInput, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	if (!kernelArgs->frame_buffer.enabled) {
		if (of_getprop(gChosen, "stdout", &fOutput, sizeof(int)) == OF_FAILED)
			return B_ERROR;
	}

	return B_OK;
}

// InitPostVM
status_t
PPCOpenFirmware::InitPostVM(struct kernel_args *kernelArgs)
{
	add_debugger_command("of_exit", &debug_command_of_exit,
		"Exit to the Open Firmware prompt. No way to get back into the OS!");
	add_debugger_command("of_enter", &debug_command_of_enter,
		"Enter a subordinate Open Firmware interpreter. Quitting it returns "
		"to KDL.");

	return B_OK;
}

// InitRTC
status_t
PPCOpenFirmware::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
	// open RTC
	fRTC = of_open(kernelArgs->platform_args.rtc_path);
	if (fRTC == OF_FAILED) {
		dprintf("PPCOpenFirmware::InitRTC(): Failed open RTC device!\n");
		return B_ERROR;
	}

	return B_OK;
}

// DebugSerialGetChar
char
PPCOpenFirmware::SerialDebugGetChar()
{
	int key;
	if (of_interpret("key", 0, 1, &key) == OF_FAILED)
		return 0;
	return (char)key;
}

// DebugSerialPutChar
void
PPCOpenFirmware::SerialDebugPutChar(char c)
{
	if (fOutput == -1)
		return;

	if (c == '\n')
		of_write(fOutput, "\r\n", 2);
	else
		of_write(fOutput, &c, 1);
}

// SetHardwareRTC
void
PPCOpenFirmware::SetHardwareRTC(uint32 seconds)
{
	struct tm t;
	rtc_secs_to_tm(seconds, &t);

	t.tm_year += RTC_EPOCH_BASE_YEAR;
	t.tm_mon++;

	if (of_call_method(fRTC, "set-time", 6, 0, t.tm_year, t.tm_mon, t.tm_mday,
			t.tm_hour, t.tm_min, t.tm_sec) == OF_FAILED) {
		dprintf("PPCOpenFirmware::SetHardwareRTC(): Failed to set RTC!\n");
	}
}

// GetHardwareRTC
uint32
PPCOpenFirmware::GetHardwareRTC()
{
	struct tm t;
	if (of_call_method(fRTC, "get-time", 0, 6, &t.tm_year, &t.tm_mon,
			&t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == OF_FAILED) {
		dprintf("PPCOpenFirmware::GetHardwareRTC(): Failed to get RTC!\n");
		return 0;
	}

	t.tm_year -= RTC_EPOCH_BASE_YEAR;
	t.tm_mon--;

	return rtc_tm_to_secs(&t);
}

// ShutDown
void
PPCOpenFirmware::ShutDown(bool reboot)
{
	if (reboot) {
		of_interpret("reset-all", 0, 0);
	} else {
		// not standardized, so it might fail
		of_interpret("shut-down", 0, 0);
	}
}


// #pragma mark - U-Boot + FDT


namespace BPrivate {

class PPCUBoot : public PPCPlatform {
public:
	PPCUBoot();
	virtual ~PPCUBoot();

	virtual status_t Init(struct kernel_args *kernelArgs);
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs);
	virtual status_t InitPostVM(struct kernel_args *kernelArgs);
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data);

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	virtual	void SetHardwareRTC(uint32 seconds);
	virtual	uint32 GetHardwareRTC();

	virtual	void ShutDown(bool reboot);

private:
	int	fInput;
	int	fOutput;
	int	fRTC;
	DebugUART *fDebugUART;
};

}	// namespace BPrivate

using BPrivate::PPCUBoot;


// constructor
PPCUBoot::PPCUBoot()
	: PPCPlatform(PPC_PLATFORM_U_BOOT),
	  fInput(-1),
	  fOutput(-1),
	  fRTC(-1),
	  fDebugUART(NULL)
{
}

// destructor
PPCUBoot::~PPCUBoot()
{
}

// Init
status_t
PPCUBoot::Init(struct kernel_args *kernelArgs)
{
	gFDT = kernelArgs->platform_args.fdt;
	// XXX: do we error out if no FDT?
	return B_OK;
}

// InitSerialDebug
status_t
PPCUBoot::InitSerialDebug(struct kernel_args *kernelArgs)
{
	fDebugUART = debug_uart_from_fdt(gFDT);
	if (fDebugUART == NULL)
		return B_ERROR;
	return B_OK;
}

// InitPostVM
status_t
PPCUBoot::InitPostVM(struct kernel_args *kernelArgs)
{
	return B_ERROR;
}

// InitRTC
status_t
PPCUBoot::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
	return B_ERROR;
}

// DebugSerialGetChar
char
PPCUBoot::SerialDebugGetChar()
{
	if (fDebugUART)
		return fDebugUART->GetChar(false);
	return 0;
}

// DebugSerialPutChar
void
PPCUBoot::SerialDebugPutChar(char c)
{
	if (fDebugUART)
		fDebugUART->PutChar(c);
}

// SetHardwareRTC
void
PPCUBoot::SetHardwareRTC(uint32 seconds)
{
}

// GetHardwareRTC
uint32
PPCUBoot::GetHardwareRTC()
{
	return 0;
}

// ShutDown
void
PPCUBoot::ShutDown(bool reboot)
{
}


// # pragma mark -


#define PLATFORM_BUFFER_SIZE MAX(sizeof(PPCOpenFirmware),sizeof(PPCUBoot))
// static buffer for constructing the actual PPCPlatform
static char *sPPCPlatformBuffer[PLATFORM_BUFFER_SIZE];

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	// only OpenFirmware supported for now
	switch (kernelArgs->arch_args.platform) {
		case PPC_PLATFORM_OPEN_FIRMWARE:
			sPPCPlatform = new(sPPCPlatformBuffer) PPCOpenFirmware;
			break;
		case PPC_PLATFORM_U_BOOT:
			sPPCPlatform = new(sPPCPlatformBuffer) PPCUBoot;
			break;
		default:
			return B_ERROR;
	}

	return sPPCPlatform->Init(kernelArgs);
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return sPPCPlatform->InitPostVM(kernelArgs);
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
