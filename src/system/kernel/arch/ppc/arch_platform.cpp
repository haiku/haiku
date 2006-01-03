/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <platform/openfirmware/openfirmware.h>
#include <util/kernel_cpp.h>


static PPCPlatform *sPPCPlatform;


// constructor
PPCPlatform::PPCPlatform()
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

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

private:
	int fInput;
	int fOutput;
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
	: fInput(-1),
	  fOutput(-1)
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
		(int(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}

// InitSerialDebug
status_t
PPCOpenFirmware::InitSerialDebug(struct kernel_args *kernelArgs)
{
	if (of_getprop(gChosen, "stdin", &fInput, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	if (of_getprop(gChosen, "stdout", &fOutput, sizeof(int)) == OF_FAILED)
		return B_ERROR;

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
	if (c == '\n')
		of_write(fOutput, "\r\n", 2);
	else
		of_write(fOutput, &c, 1);
}


// # pragma mark -


// static buffer for constructing the actual PPCPlatform
static char *sPPCPlatformBuffer[sizeof(PPCOpenFirmware)];

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	// only OpenFirmware supported for now
	if (true)
		sPPCPlatform = new(sPPCPlatformBuffer) PPCOpenFirmware;

	return sPPCPlatform->Init(kernelArgs);
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return sPPCPlatform->InitPostVM(kernelArgs);
}
