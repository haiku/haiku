/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
//#include <platform/openfirmware/openfirmware.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>


static M68KPlatform *sM68KPlatform;


// constructor
M68KPlatform::M68KPlatform(m68k_platform_type platformType)
	: fPlatformType(platformType)
{
}

// destructor
M68KPlatform::~M68KPlatform()
{
}

// Default
M68KPlatform *
M68KPlatform::Default()
{
	return sM68KPlatform;
}


// #pragma mark - Amiga


// #pragma mark - Apple

namespace BPrivate {

class M68KApple : public M68KPlatform {
public:
	M68KApple();
	virtual ~M68KApple();

	virtual status_t Init(struct kernel_args *kernelArgs);
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs);
	virtual status_t InitPostVM(struct kernel_args *kernelArgs);
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data);

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	//virtual	void SetHardwareRTC(uint32 seconds);
	//virtual	uint32 GetHardwareRTC();

	virtual	void ShutDown(bool reboot);

private:
	int	fRTC;
};

}	// namespace BPrivate

using BPrivate::M68KApple;

// constructor
M68KApple::M68KApple()
	: M68KPlatform(M68K_PLATFORM_OPEN_FIRMWARE),
	  fRTC(-1)
{
}

// destructor
M68KApple::~M68KApple()
{
}

// Init
status_t
M68KApple::Init(struct kernel_args *kernelArgs)
{
	return of_init(
		(int(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}

// InitSerialDebug
status_t
M68KApple::InitSerialDebug(struct kernel_args *kernelArgs)
{
	return B_OK;
}

// InitPostVM
status_t
M68KApple::InitPostVM(struct kernel_args *kernelArgs)
{
	return B_OK;
}

// InitRTC
status_t
M68KApple::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
	return B_OK;
}

// DebugSerialGetChar
char
M68KApple::SerialDebugGetChar()
{
	int key;
	return (char)key;
}

// DebugSerialPutChar
void
M68KApple::SerialDebugPutChar(char c)
{
}

// ShutDown
void
M68KApple::ShutDown(bool reboot)
{
	if (reboot) {
		of_interpret("reset-all", 0, 0);
	} else {
		// not standardized, so it might fail
		of_interpret("shut-down", 0, 0);
	}
}


// #pragma mark - Atari (Falcon)


namespace BPrivate {

class M68KAtari : public M68KPlatform {
public:
	M68KAtari();
	virtual ~M68KAtari();

	virtual status_t Init(struct kernel_args *kernelArgs);
	virtual status_t InitSerialDebug(struct kernel_args *kernelArgs);
	virtual status_t InitPostVM(struct kernel_args *kernelArgs);
	virtual status_t InitRTC(struct kernel_args *kernelArgs,
		struct real_time_data *data);

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	//virtual	void SetHardwareRTC(uint32 seconds);
	//virtual	uint32 GetHardwareRTC();

	virtual	void ShutDown(bool reboot);

private:
	int	fRTC;
};

}	// namespace BPrivate

using BPrivate::M68KAtari;

// constructor
M68KAtari::M68KAtari()
	: M68KPlatform(M68K_PLATFORM_OPEN_FIRMWARE),
	  fRTC(-1)
{
}

// destructor
M68KAtari::~M68KAtari()
{
}

// Init
status_t
M68KAtari::Init(struct kernel_args *kernelArgs)
{
	return of_init(
		(int(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}

// InitSerialDebug
status_t
M68KAtari::InitSerialDebug(struct kernel_args *kernelArgs)
{
	if (of_getprop(gChosen, "stdin", &fInput, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	if (of_getprop(gChosen, "stdout", &fOutput, sizeof(int)) == OF_FAILED)
		return B_ERROR;

	return B_OK;
}

// InitPostVM
status_t
M68KAtari::InitPostVM(struct kernel_args *kernelArgs)
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
M68KAtari::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
	// open RTC
	fRTC = of_open(kernelArgs->platform_args.rtc_path);
	if (fRTC == OF_FAILED) {
		dprintf("M68KAtari::InitRTC(): Failed open RTC device!\n");
		return B_ERROR;
	}

	return B_OK;
}

// DebugSerialGetChar
char
M68KAtari::SerialDebugGetChar()
{
	int key;
	if (of_interpret("key", 0, 1, &key) == OF_FAILED)
		return 0;
	return (char)key;
}

// DebugSerialPutChar
void
M68KAtari::SerialDebugPutChar(char c)
{
	if (c == '\n')
		of_write(fOutput, "\r\n", 2);
	else
		of_write(fOutput, &c, 1);
}

// ShutDown
void
M68KAtari::ShutDown(bool reboot)
{
	if (reboot) {
		of_interpret("reset-all", 0, 0);
	} else {
		// not standardized, so it might fail
		of_interpret("shut-down", 0, 0);
	}
}


// # pragma mark -


// static buffer for constructing the actual M68KPlatform
static char *sM68KPlatformBuffer[sizeof(M68KAtari)];

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
#warning M68K: switch platform from kernel args
	// only Atari supported for now
	if (true)
		sM68KPlatform = new(sM68KPlatformBuffer) M68KAtari;

	return sM68KPlatform->Init(kernelArgs);
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return sM68KPlatform->InitPostVM(kernelArgs);
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
