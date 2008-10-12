/*
	Atari kernel platform code.
*/

#include <arch_platform.h>

#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <arch/cpu.h>
//#include <platform/openfirmware/openfirmware.h>
#include <platform/atari_m68k/MFP.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>

#include "debugger_keymaps.h"

#define MFP0_BASE	0xFFFFFA00
#define MFP1_BASE	0xFFFFFA80

#define MFP0_VECTOR_BASE	64
#define MFP1_VECTOR_BASE	(MFP0_VECTOR_BASE+16)
// ?
#define SCC_C0_VECTOR_BASE	(MFP1_VECTOR_BASE+16)
// ??
#define SCC_C1_VECTOR_BASE	(0x1BC/4)

#define IKBD_BASE	0xFFFFFC00
#define IKBD_CTRL	0
#define IKBD_DATA	2
#define IKBD_STATUS_READ_BUFFER_FULL	0x01

// keyboard scancodes, very much like PC ones
// see
// http://www.classiccmp.org/dunfield/atw800/h/atw800k.jpg
// ST Mag Nr 57 page 55
enum keycodes {
	LEFT_SHIFT		= 42,
	RIGHT_SHIFT		= 54,

	LEFT_CONTROL	= 29,

	LEFT_ALT		= 56,

	CURSOR_LEFT		= 75,
	CURSOR_RIGHT	= 77,
	CURSOR_UP		= 72,
	CURSOR_DOWN		= 80,
	CURSOR_HOME		= 71,
	CURSOR_END		= 79,	// not on real atari keyboard
	PAGE_UP			= 73,	// not on real atari keyboard XXX remap Help ?
	PAGE_DOWN		= 81,	// not on real atari keyboard XXX remap Undo ?

	DELETE			= 83,
	F12				= 88,	// but it's shifted

};

#define in8(a)  (*(volatile uint8 *)(a))
#define out8(a, v)  (*(volatile uint8 *)(a) = v)


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
		bool AcknowledgeIOInterrupt(int irq);

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

	virtual char BlueScreenGetChar();

	virtual char SerialDebugGetChar();
	virtual void SerialDebugPutChar(char c);

	virtual void EnableIOInterrupt(int irq);
	virtual void DisableIOInterrupt(int irq);
	virtual bool AcknowledgeIOInterrupt(int irq);

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
	uint8 val = in8(reg);
	if (val & bit == 0) {
		val |= bit;
		out8(reg, val);
	}
}


void
M68KAtari::MFP::DisableIOInterrupt(int irq)
{
	uint8 bit = 1 << (irq % 8);
	// I*B[0] is vector+0, I*A[0] is vector+8
	uint32 reg = Base() + ((irq > 8) ? (MFP_IERA) : (MFP_IERB));
	uint8 val = in8(reg);
	if (val & bit) {
		val &= ~bit;
		out8(reg, val);
	}
}


bool
M68KAtari::MFP::AcknowledgeIOInterrupt(int irq)
{
	uint8 bit = 1 << (irq % 8);
	// I*B[0] is vector+0, I*A[0] is vector+8
	uint32 reg = Base() + ((irq > 8) ? (MFP_ISRA) : (MFP_ISRB));
	uint8 val = in8(reg);
	if (val & bit) {
		val &= ~bit;
		out8(reg, val);
		return true;
	}
	return false;
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

	//out8(IKBD_BASE+IKBD_DATA, 0x11);

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
	
	out8(fMFP[0]->Base() + MFP_TACR, 0); // stop it
	install_io_interrupt_handler(fMFP[0]->Vector()+13, &pit_timer_interrupt, NULL, 0);
	return B_OK;
}


char
M68KAtari::BlueScreenGetChar()
{
	/* polling the keyboard, similar to code in keyboard
	 * driver, but without using an interrupt
	 * taken almost straight from x86 code
	 * XXX: maybe use the keymap from the _AKP cookie instead ?
	 */
	static bool shiftPressed = false;
	static bool controlPressed = false;
	static bool altPressed = false;
	static uint8 special = 0;
	static uint8 special2 = 0;
	uint8 key = 0;

	if (special & 0x80) {
		special &= ~0x80;
		return '[';
	}
	if (special != 0) {
		key = special;
		special = 0;
		return key;
	}
	if (special2 != 0) {
		key = special2;
		special2 = 0;
		return key;
	}

	while (true) {
		uint8 status = in8(IKBD_BASE+IKBD_CTRL);

		if ((status & IKBD_STATUS_READ_BUFFER_FULL) == 0) {
			// no data in keyboard buffer
			spin(200);
			//kprintf("no key\n");
			continue;
		}

		spin(200);
		key = in8(IKBD_BASE+IKBD_DATA);
		/*
		kprintf("key: %02x, %sshift %scontrol %salt\n",
			key,
			shiftPressed?"":"!",
			controlPressed?"":"!",
			altPressed?"":"!");
		*/

		if (key & 0x80) {
			// key up
			switch (key & ~0x80) {
				case LEFT_SHIFT:
				case RIGHT_SHIFT:
					shiftPressed = false;
					break;
				case LEFT_CONTROL:
					controlPressed = false;
					break;
				case LEFT_ALT:
					altPressed = false;
					break;
			}
		} else {
			// key down
			switch (key) {
				case LEFT_SHIFT:
				case RIGHT_SHIFT:
					shiftPressed = true;
					break;

				case LEFT_CONTROL:
					controlPressed = true;
					break;

				case LEFT_ALT:
					altPressed = true;
					break;

				// start escape sequence for cursor movement
				case CURSOR_UP:
					special = 0x80 | 'A';
					return '\x1b';
				case CURSOR_DOWN:
					special = 0x80 | 'B';
					return '\x1b';
				case CURSOR_RIGHT:
					special = 0x80 | 'C';
					return '\x1b';
				case CURSOR_LEFT:
					special = 0x80 | 'D';
					return '\x1b';
				case CURSOR_HOME:
					special = 0x80 | 'H';
					return '\x1b';
				case CURSOR_END:
					special = 0x80 | 'F';
					return '\x1b';
				case PAGE_UP:
					special = 0x80 | '5';
					special2 = '~';
					return '\x1b';
				case PAGE_DOWN:
					special = 0x80 | '6';
					special2 = '~';
					return '\x1b';


				case DELETE:
					if (controlPressed && altPressed)
						arch_cpu_shutdown(true);

					special = 0x80 | '3';
					special2 = '~';
					return '\x1b';

				default:
					if (controlPressed) {
						char c = kShiftedKeymap[key];
						if (c >= 'A' && c <= 'Z')
							return 0x1f & c;
					}

					if (altPressed)
						return kAltedKeymap[key];

					return shiftPressed
						? kShiftedKeymap[key] : kUnshiftedKeymap[key];
			}
		}
	}
}


char
M68KAtari::SerialDebugGetChar()
{
	//WRITEME
	return BlueScreenGetChar();
	//return 0;
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


bool
M68KAtari::AcknowledgeIOInterrupt(int irq)
{
	MFP *mfp = MFPForIrq(irq);

	if (mfp)
		return mfp->AcknowledgeIOInterrupt(irq - mfp->Vector());
	return false;
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
	uint8 counts = (uint8)(timeout & 0x0ff);
	//XXX: SCALE
	out8(fMFP[0]->Base() + MFP_TADR, counts);
	out8(fMFP[0]->Base() + MFP_TACR, 0x01); // delay mode, device by 4
}


void
M68KAtari::ClearHardwareTimer(void)
{
	out8(fMFP[0]->Base() + MFP_TACR, 0); // stop it
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
