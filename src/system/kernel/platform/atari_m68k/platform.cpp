/*
	Just a dummy. No BIOS services are required in the kernel.
*/

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
//#include <platform/openfirmware/openfirmware.h>
#include <platform/atari_m68k/MFP.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>

#define MFP0_BASE	0xFFFFFA00
#define MFP1_BASE	0xFFFFFA80

#define MFP0_VECTOR_BASE	64
#define MFP1_VECTOR_BASE	(MFP0_VECTOR_BASE+16)
// ?
#define SCC_C0_VECTOR_BASE	(MFP1_VECTOR_BASE+16)
// ??
#define SCC_C1_VECTOR_BASE	(0x1BC/4)

#define inb(a)  (*(volatile uint8 *)(a))
#define outb(a, v)  (*(volatile uint8 *)(a) = v)


namespace BPrivate {

//class MfpPIC;

// #pragma mark - Atari (Falcon)


class M68KAtari : public M68KPlatform {
public:
	class MFP {
	public:
		MFP(uint32 base, int vector);
		~MFP();

		uint32 Base() const { return fBase; };
		int Vector() const { return fVector; };

		void EnableIOInterrupt(int irq);
		void DisableIOInterrupt(int irq);
		void AcknowledgeIOInterrupt(int irq);

	private:
		uint32 fBase;
		int fVector;
	};

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

	virtual void EnableIOInterrupt(int irq);
	virtual void DisableIOInterrupt(int irq);
	virtual void AcknowledgeIOInterrupt(int irq);

	virtual	void SetHardwareRTC(uint32 seconds);
	virtual	uint32 GetHardwareRTC();

	virtual void SetHardwareTimer(bigtime_t timeout);
	virtual void ClearHardwareTimer(void);

	virtual	void ShutDown(bool reboot);

private:
	MFP	*MFPForIrq(int irq);
	int	fRTC;

	MFP	*fMFP[2];

	// native features (ARAnyM emulator)
	uint32 (*nfGetID)(const char *name);
	int32 (*nfCall)(uint32 ID, ...);
	char *nfPage;
	uint32 nfDebugPrintfID;
	
};


}	// namespace BPrivate

using BPrivate::M68KAtari;


// #pragma mark - M68KAtari::MFP


static char sMFP0Buffer[sizeof(M68KAtari::MFP)];
static char sMFP1Buffer[sizeof(M68KAtari::MFP)];

// constructor
M68KAtari::MFP::MFP(uint32 base, int vector)
{
	fBase = base;
	fVector = vector;
}


M68KAtari::MFP::~MFP()
{
}

#warning M68K: use enable or mark register ?

void
M68KAtari::MFP::EnableIOInterrupt(int irq)
{
	uint8 bit = 1 << (irq % 8);
	// I*B[0] is vector+0, I*A[0] is vector+8
	uint32 reg = Base() + ((irq > 8) ? (MFP_IERA) : (MFP_IERB));
	uint8 val = inb(reg);
	if (val & bit == 0) {
		val |= bit;
		outb(reg, val);
	}
}


void
M68KAtari::MFP::DisableIOInterrupt(int irq)
{
	uint8 bit = 1 << (irq % 8);
	// I*B[0] is vector+0, I*A[0] is vector+8
	uint32 reg = Base() + ((irq > 8) ? (MFP_IERA) : (MFP_IERB));
	uint8 val = inb(reg);
	if (val & bit) {
		val &= ~bit;
		outb(reg, val);
	}
}


void
M68KAtari::MFP::AcknowledgeIOInterrupt(int irq)
{
	uint8 bit = 1 << (irq % 8);
	// I*B[0] is vector+0, I*A[0] is vector+8
	uint32 reg = Base() + ((irq > 8) ? (MFP_ISRA) : (MFP_ISRB));
	uint8 val = inb(reg);
	if (val & bit) {
		val &= ~bit;
		outb(reg, val);
	}
}


// #pragma mark - M68KAtari


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
	fMFP[0] = NULL;
	fMFP[1] = NULL;

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
	fMFP[0] = new(sMFP0Buffer) M68KAtari::MFP(MFP0_BASE, MFP0_VECTOR_BASE);
	//if (kernelArgs->arch_args.machine == /*TT*/) {
		fMFP[1] = new(sMFP1Buffer) M68KAtari::MFP(MFP1_BASE, MFP1_VECTOR_BASE);
	//}
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
M68KAtari::EnableIOInterrupt(int irq)
{
	MFP *mfp = MFPForIrq(irq);

	if (mfp)
		mfp->EnableIOInterrupt(irq - mfp->Vector());
}


void
M68KAtari::DisableIOInterrupt(int irq)
{
	MFP *mfp = MFPForIrq(irq);

	if (mfp)
		mfp->DisableIOInterrupt(irq - mfp->Vector());
}


void
M68KAtari::AcknowledgeIOInterrupt(int irq)
{
	MFP *mfp = MFPForIrq(irq);

	if (mfp)
		mfp->AcknowledgeIOInterrupt(irq - mfp->Vector());
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


M68KAtari::MFP *
M68KAtari::MFPForIrq(int irq)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (fMFP[i]) {
			if (irq >= fMFP[i]->Vector() && irq < fMFP[i]->Vector() + 16)
				return fMFP[i];
		}
	}
	return NULL;
}


// static buffer for constructing the actual M68KPlatform
static char *sM68KPlatformBuffer[sizeof(M68KAtari)];
#warning PTR HERE ???


M68KPlatform *instanciate_m68k_platform_atari()
{
	return new(sM68KPlatformBuffer) M68KAtari;
}
