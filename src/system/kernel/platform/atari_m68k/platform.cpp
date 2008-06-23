/*
	Just a dummy. No BIOS services are required in the kernel.
*/

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
//#include <platform/openfirmware/openfirmware.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>

using BPrivate::M68KAtari;


// constructor
M68KAtari::M68KAtari()
	: M68KPlatform(M68K_PLATFORM_ATARI),
	  fRTC(-1)
{
}


// destructor
M68KAtari::~M68KAtari()
{
}


status_t
M68KAtari::Init(struct kernel_args *kernelArgs)
{
	return B_NO_INIT;
	return B_OK;
}


status_t
M68KAtari::InitSerialDebug(struct kernel_args *kernelArgs)
{
	return B_NO_INIT;
	return B_OK;
}


status_t
M68KAtari::InitPostVM(struct kernel_args *kernelArgs)
{
#if 0
	add_debugger_command("of_exit", &debug_command_of_exit,
		"Exit to the Open Firmware prompt. No way to get back into the OS!");
	add_debugger_command("of_enter", &debug_command_of_enter,
		"Enter a subordinate Open Firmware interpreter. Quitting it returns "
		"to KDL.");
#endif
	return B_NO_INIT;
	return B_OK;
}


status_t
M68KAtari::InitPIC(struct kernel_args *kernelArgs)
{
	panic("WRITEME");
	return B_NO_INIT;
}


status_t
M68KAtari::InitRTC(struct kernel_args *kernelArgs,
	struct real_time_data *data)
{
	panic("WRITEME");
	return B_NO_INIT;
	return B_OK;
}


status_t
M68KAtari::InitTimer(struct kernel_args *kernelArgs)
{
	panic("WRITEME");
	return B_NO_INIT;
}


char
M68KAtari::SerialDebugGetChar()
{
	panic("WRITEME");
	return 0;
//	return (char)key;
}


void
M68KAtari::SerialDebugPutChar(char c)
{
	panic("WRITEME");
}


void
M68KAtari::SetHardwareRTC(uint32 seconds)
{
}


uint32
M68KAtari::GetHardwareRTC()
{
	return 0;
}


void
M68KAtari::SetHardwareTimer(bigtime_t timeout)
{
}


void
M68KAtari::ClearHardwareTimer(void)
{
}


void
M68KAtari::ShutDown(bool reboot)
{
	panic("WRITEME");
}
