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


namespace BPrivate {

//class MfpPIC;

// #pragma mark - Atari (Falcon)

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
	// native features (ARAnyM emulator)
	uint32 (*nfGetID)(const char *name);
	int32 (*nfCall)(uint32 ID, ...);
	char *nfPage;
	uint32 nfDebugPrintfID;
	
};


}	// namespace BPrivate

using BPrivate::M68KAtari;


// constructor
M68KAtari::M68KAtari()
	: M68KPlatform(B_ATARI_PLATFORM, M68K_PLATFORM_ATARI),
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
	nfGetID =
		kernelArgs->arch_args.plat_args.atari.nat_feat.nf_get_id;
	nfCall = 
		kernelArgs->arch_args.plat_args.atari.nat_feat.nf_call;
	nfPage = (char *)
		kernelArgs->arch_args.plat_args.atari.nat_feat.nf_page;

	return B_OK;
}


status_t
M68KAtari::InitSerialDebug(struct kernel_args *kernelArgs)
{
	nfDebugPrintfID =
		kernelArgs->arch_args.plat_args.atari.nat_feat.nf_dprintf_id;

#warning M68K: add real serial debug output someday

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
	if (nfCall && nfDebugPrintfID) {
#if 0
		static char buffer[2] = { '\0', '\0' };
		buffer[0] = c;
		
		nfCall(nfDebugPrintfID /*| 0*/, buffer);
#endif
		nfPage[0] = c;
		nfPage[1] = '\0';
		nfCall(nfDebugPrintfID /*| 0*/, nfPage);
	}

#warning M68K: WRITEME
	// real serial
	//panic("WRITEME");
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
	panic("Bombs!");
	panic("WRITEME");
}

// static buffer for constructing the actual M68KPlatform
static char *sM68KPlatformBuffer[sizeof(M68KAtari)];
#warning PTR HERE ???


M68KPlatform *instanciate_m68k_platform_atari()
{
	return new(sM68KPlatformBuffer) M68KAtari;
}
