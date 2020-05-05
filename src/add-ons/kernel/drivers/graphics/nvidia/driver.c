/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson;
	Rudolf Cornelissen 3/2002-1/2016.
*/


#include "AGP.h"
#include "DriverInterface.h"
#include "nv_macros.h"

#include <graphic_driver.h>
#include <KernelExport.h>
#include <ISA.h>
#include <PCI.h>
#include <OS.h>
#include <directories.h>
#include <driver_settings.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))

#define MAX_DEVICES	  8

/* Tell the kernel what revision of the driver API we support */
int32 api_version = B_CUR_DRIVER_API_VERSION;

/* these structures are private to the kernel driver */
typedef struct device_info device_info;

typedef struct {
	timer		te;				/* timer entry for add_timer() */
	device_info	*di;			/* pointer to the owning device */
	bigtime_t	when_target;	/* when we're supposed to wake up */
} timer_info;

struct device_info {
	uint32		is_open;			/* a count of how many times the devices has been opened */
	area_id		shared_area;		/* the area shared between the driver and all of the accelerants */
	shared_info	*si;				/* a pointer to the shared area, for convenience */
	vuint32		*regs;				/* kernel's pointer to memory mapped registers */
	pci_info	pcii;					/* a convenience copy of the pci info for this device */
	char		name[B_OS_NAME_LENGTH];	/* where we keep the name of the device for publishing and comparing */
};

typedef struct {
	uint32		count;				/* number of devices actually found */
	benaphore	kernel;				/* for serializing opens/closes */
	char		*device_names[MAX_DEVICES+1];	/* device name pointer storage */
	device_info	di[MAX_DEVICES];	/* device specific stuff */
} DeviceData;

/* prototypes for our private functions */
static status_t open_hook(const char* name, uint32 flags, void** cookie);
static status_t close_hook(void* dev);
static status_t free_hook(void* dev);
static status_t read_hook(void* dev, off_t pos, void* buf, size_t* len);
static status_t write_hook(void* dev, off_t pos, const void* buf, size_t* len);
static status_t control_hook(void* dev, uint32 msg, void *buf, size_t len);
static status_t map_device(device_info *di);
static void unmap_device(device_info *di);
static void probe_devices(void);
static int32 nv_interrupt(void *data);

static DeviceData		*pd;
static isa_module_info	*isa_bus = NULL;
static pci_module_info	*pci_bus = NULL;
static agp_gart_module_info *agp_bus = NULL;
static device_hooks graphics_device_hooks = {
	open_hook,
	close_hook,
	free_hook,
	control_hook,
	read_hook,
	write_hook,
	NULL,
	NULL,
	NULL,
	NULL
};

#define VENDOR_ID_NVIDIA	0x10de /* Nvidia */
#define VENDOR_ID_ELSA		0x1048 /* Elsa GmbH */
#define VENDOR_ID_NVSTBSGS	0x12d2 /* Nvidia STB/SGS-Thompson */
#define VENDOR_ID_VARISYS	0x1888 /* Varisys Limited */

static uint16 nvidia_device_list[] = {
	0x0020, /* Nvidia TNT1 */
	0x0028, /* Nvidia TNT2 (pro) */
	0x0029, /* Nvidia TNT2 Ultra */
	0x002a, /* Nvidia TNT2 */
	0x002b, /* Nvidia TNT2 */
	0x002c, /* Nvidia Vanta (Lt) */
	0x002d, /* Nvidia TNT2-M64 (Pro) */
	0x002e, /* Nvidia NV06 Vanta */
	0x002f, /* Nvidia NV06 Vanta */
	0x0040, /* Nvidia Geforce FX 6800 Ultra */
	0x0041, /* Nvidia Geforce FX 6800 */
	0x0042, /* Nvidia Geforce FX 6800LE */
	0x0043, /* Nvidia Geforce 6800 XE */
	0x0045, /* Nvidia Geforce FX 6800 GT */
	0x0046, /* Nvidia Geforce FX 6800 GT */
	0x0047, /* Nvidia Geforce 6800 GS */
	0x0048, /* Nvidia Geforce FX 6800 XT */
	0x0049, /* Nvidia unknown FX */
	0x004d, /* Nvidia Quadro FX 4400 */
	0x004e, /* Nvidia Quadro FX 4000 */
	0x0091, /* Nvidia Geforce 7800 GTX PCIe */
	0x0092, /* Nvidia Geforce 7800 GT PCIe */
	0x0098, /* Nvidia Geforce 7800 Go PCIe */
	0x0099, /* Nvidia Geforce 7800 GTX Go PCIe */
	0x009d, /* Nvidia Quadro FX 4500 */
	0x00a0, /* Nvidia Aladdin TNT2 */
	0x00c0,	/* Nvidia Geforce 6800 GS */
	0x00c1, /* Nvidia Geforce FX 6800 */
	0x00c2, /* Nvidia Geforce FX 6800LE */
	0x00c3, /* Nvidia Geforce FX 6800 XT */
	0x00c8, /* Nvidia Geforce FX 6800 Go */
	0x00c9, /* Nvidia Geforce FX 6800 Ultra Go */
	0x00cc, /* Nvidia Quadro FX 1400 Go */
	0x00cd, /* Nvidia Quadro FX 3450/4000 SDI */
	0x00ce, /* Nvidia Quadro FX 1400 */
	0x00f0, /* Nvidia Geforce FX 6800 (Ultra) AGP(?) */
	0x00f1, /* Nvidia Geforce FX 6600 GT AGP */
	0x00f2, /* Nvidia Geforce FX 6600 AGP */
	0x00f3, /* Nvidia Geforce 6200 */
	0x00f4, /* Nvidia Geforce 6600 LE */
	0x00f5, /* Nvidia Geforce FX 7800 GS AGP */
	0x00f6, /* Nvidia Geforce 6800 GS */
	0x00f8, /* Nvidia Quadro FX 3400/4400 PCIe */
	0x00f9,	/* Nvidia Geforce PCX 6800 PCIe */
	0x00fa,	/* Nvidia Geforce PCX 5750 PCIe */
	0x00fb,	/* Nvidia Geforce PCX 5900 PCIe */
	0x00fc, /* Nvidia Geforce PCX 5300 PCIe */
	0x00fd,	/* Nvidia Quadro PCX PCIe */
	0x00fe,	/* Nvidia Quadro FX 1300 PCIe(?) */
	0x00ff, /* Nvidia Geforce PCX 4300 PCIe */
	0x0100, /* Nvidia Geforce256 SDR */
	0x0101, /* Nvidia Geforce256 DDR */
	0x0102, /* Nvidia Geforce256 Ultra */
	0x0103, /* Nvidia Quadro */
	0x0110, /* Nvidia Geforce2 MX/MX400 */
	0x0111, /* Nvidia Geforce2 MX100/MX200 DDR */
	0x0112, /* Nvidia Geforce2 Go */
	0x0113, /* Nvidia Quadro2 MXR/EX/Go */
	0x0140, /* Nvidia Geforce FX 6600 GT */
	0x0141, /* Nvidia Geforce FX 6600 */
	0x0142, /* Nvidia Geforce FX 6600LE */
	0x0143, /* Nvidia Geforce 6600 VE */
	0x0144, /* Nvidia Geforce FX 6600 Go */
	0x0145, /* Nvidia Geforce FX 6610 XL */
	0x0146, /* Nvidia Geforce FX 6600 TE Go / 6200 TE Go */
	0x0147, /* Nvidia Geforce FX 6700 XL */
	0x0148, /* Nvidia Geforce FX 6600 Go */
	0x0149, /* Nvidia Geforce FX 6600 GT Go */
	0x014b, /* Nvidia unknown FX */
	0x014c, /* Nvidia Quadro FX 540 MXM */
	0x014d, /* Nvidia unknown FX */
	0x014e, /* Nvidia Quadro FX 540 */
	0x014f, /* Nvidia Geforce 6200 PCIe (128Mb) */
	0x0150, /* Nvidia Geforce2 GTS/Pro */
	0x0151, /* Nvidia Geforce2 Ti DDR */
	0x0152, /* Nvidia Geforce2 Ultra */
	0x0153, /* Nvidia Quadro2 Pro */
	0x0160, /* Nvidia Geforce 6500 Go */
	0x0161, /* Nvidia Geforce 6200 TurboCache */
	0x0162, /* Nvidia Geforce 6200SE TurboCache */
	0x0163, /* Nvidia Geforce 6200LE */
	0x0164, /* Nvidia Geforce FX 6200 Go */
	0x0165, /* Nvidia Quadro FX NVS 285 */
	0x0166, /* Nvidia Geforce 6400 Go */
	0x0167, /* Nvidia Geforce 6200 Go */
	0x0168, /* Nvidia Geforce 6400 Go */
	0x0169, /* Nvidia Geforce 6250 Go */
	0x016a, /* Nvidia Geforce 7100 GS */
	0x016b, /* Nvidia unknown FX Go */
	0x016c, /* Nvidia unknown FX Go */
	0x016d, /* Nvidia unknown FX Go */
	0x016e, /* Nvidia unknown FX */
	0x0170, /* Nvidia Geforce4 MX 460 */
	0x0171, /* Nvidia Geforce4 MX 440 */
	0x0172, /* Nvidia Geforce4 MX 420 */
	0x0173, /* Nvidia Geforce4 MX 440SE */
	0x0174, /* Nvidia Geforce4 440 Go */
	0x0175, /* Nvidia Geforce4 420 Go */
	0x0176, /* Nvidia Geforce4 420 Go 32M */
	0x0177, /* Nvidia Geforce4 460 Go */
	0x0178, /* Nvidia Quadro4 500 XGL/550 XGL */
	0x0179, /* Nvidia Geforce4 440 Go 64M (PPC: Geforce4 MX) */
	0x017a, /* Nvidia Quadro4 200 NVS/400 NVS */
	0x017c, /* Nvidia Quadro4 500 GoGL */
	0x017d, /* Nvidia Geforce4 410 Go 16M */
	0x0181, /* Nvidia Geforce4 MX 440 AGP8X */
	0x0182, /* Nvidia Geforce4 MX 440SE AGP8X */
	0x0183, /* Nvidia Geforce4 MX 420 AGP8X */
	0x0185, /* Nvidia Geforce4 MX 4000 AGP8X */
	0x0186, /* Nvidia Geforce4 448 Go */
	0x0187, /* Nvidia Geforce4 488 Go */
	0x0188, /* Nvidia Quadro4 580 XGL */
	0x0189,	/* Nvidia Geforce4 MX AGP8X (PPC) */
	0x018a, /* Nvidia Quadro4 280 NVS AGP8X */
	0x018b, /* Nvidia Quadro4 380 XGL */
	0x018c, /* Nvidia Quadro4 NVS 50 PCI */
	0x018d, /* Nvidia Geforce4 448 Go */
	0x01a0, /* Nvidia Geforce2 Integrated GPU */
	0x01d1, /* Nvidia Geforce 7300 LE */
	0x01d3, /* Nvidia Geforce 7300 SE */
	0x01d7,	/* Nvidia Quadro NVS 110M/Geforce 7300 Go */
	0x01d8,	/* Nvidia Geforce 7400 GO */
	0x01dd, /* Nvidia Geforce 7500 LE */
	0x01df, /* Nvidia Geforce 7300 GS */
	0x01f0, /* Nvidia Geforce4 MX Integrated GPU */
	0x0200, /* Nvidia Geforce3 */
	0x0201, /* Nvidia Geforce3 Ti 200 */
	0x0202, /* Nvidia Geforce3 Ti 500 */
	0x0203, /* Nvidia Quadro DCC */
	0x0211, /* Nvidia Geforce FX 6800 */
	0x0212, /* Nvidia Geforce FX 6800LE */
	0x0215, /* Nvidia Geforce FX 6800 GT */
	0x0218, /* Nvidia Geforce 6800 XT */
	0x0220, /* Nvidia unknown FX */
	0x0221, /* Nvidia Geforce 6200 AGP (256Mb - 128bit) */
	0x0222, /* Nvidia unknown FX */
	0x0228, /* Nvidia unknown FX Go */
	0x0240, /* Nvidia Geforce 6150 (NFORCE4 Integr.GPU) */
	0x0241, /* Nvidia Geforce 6150 LE (NFORCE4 Integr.GPU) */
	0x0242, /* Nvidia Geforce 6100 (NFORCE4 Integr.GPU) */
	0x0244, /* Nvidia Geforce Go 6150 (NFORCE4 Integr.GPU) */
	0x0245, /* Nvidia Quadro NVS 210S / Geforce 6150LE */
	0x0247, /* Nvidia Geforce 6100 Go (NFORCE4 Integr.GPU) */
	0x0250, /* Nvidia Geforce4 Ti 4600 */
	0x0251, /* Nvidia Geforce4 Ti 4400 */
	0x0252, /* Nvidia Geforce4 Ti 4600 */
	0x0253, /* Nvidia Geforce4 Ti 4200 */
	0x0258, /* Nvidia Quadro4 900 XGL */
	0x0259, /* Nvidia Quadro4 750 XGL */
	0x025b, /* Nvidia Quadro4 700 XGL */
	0x0280, /* Nvidia Geforce4 Ti 4800 AGP8X */
	0x0281, /* Nvidia Geforce4 Ti 4200 AGP8X */
	0x0282, /* Nvidia Geforce4 Ti 4800SE */
	0x0286, /* Nvidia Geforce4 4200 Go */
	0x0288, /* Nvidia Quadro4 980 XGL */
	0x0289, /* Nvidia Quadro4 780 XGL */
	0x028c, /* Nvidia Quadro4 700 GoGL */
	0x0290, /* Nvidia Geforce 7900 GTX */
	0x0291, /* Nvidia Geforce 7900 GT */
	0x0292, /* Nvidia Geforce 7900 GS */
	0x0293, /* Nvidia Geforce 7900 GX2 */
	0x0294, /* Nvidia Geforce 7950 GX2 */
	0x0295, /* Nvidia Geforce 7950 GT */
	0x0298, /* Nvidia Geforce Go 7900 GS */
	0x0299, /* Nvidia Geforce Go 7900 GTX */
	0x029c, /* Nvidia Quadro FX 5500 */
	0x029f, /* Nvidia Quadro FX 4500 X2 */
	0x02a0, /* Nvidia Geforce3 Integrated GPU */
	0x02e0,	/* Nvidia Geforce 7600 GT */
	0x02e1,	/* Nvidia Geforce 7600 GS */
	0x02e2, /* Nvidia Geforce 7300 GT */
	0x0301, /* Nvidia Geforce FX 5800 Ultra */
	0x0302, /* Nvidia Geforce FX 5800 */
	0x0308, /* Nvidia Quadro FX 2000 */
	0x0309, /* Nvidia Quadro FX 1000 */
	0x0311, /* Nvidia Geforce FX 5600 Ultra */
	0x0312, /* Nvidia Geforce FX 5600 */
	0x0313, /* Nvidia unknown FX */
	0x0314, /* Nvidia Geforce FX 5600XT */
	0x0316, /* Nvidia unknown FX Go */
	0x0317, /* Nvidia unknown FX Go */
	0x031a, /* Nvidia Geforce FX 5600 Go */
	0x031b, /* Nvidia Geforce FX 5650 Go */
	0x031c, /* Nvidia Quadro FX 700 Go */
	0x031d, /* Nvidia unknown FX Go */
	0x031e, /* Nvidia unknown FX Go */
	0x031f, /* Nvidia unknown FX Go */
	0x0320, /* Nvidia Geforce FX 5200 */
	0x0321, /* Nvidia Geforce FX 5200 Ultra */
	0x0322, /* Nvidia Geforce FX 5200 */
	0x0323, /* Nvidia Geforce FX 5200LE */
	0x0324, /* Nvidia Geforce FX 5200 Go */
	0x0325, /* Nvidia Geforce FX 5250 Go */
	0x0326, /* Nvidia Geforce FX 5500 */
	0x0327, /* Nvidia Geforce FX 5100 */
	0x0328, /* Nvidia Geforce FX 5200 Go 32M/64M */
	0x0329, /* Nvidia Geforce FX 5200 (PPC) */
	0x032a, /* Nvidia Quadro NVS 280 PCI */
	0x032b, /* Nvidia Quadro FX 500/600 PCI */
	0x032c, /* Nvidia Geforce FX 5300 Go */
	0x032d, /* Nvidia Geforce FX 5100 Go */
	0x032e, /* Nvidia unknown FX Go */
	0x032f, /* Nvidia unknown FX Go */
	0x0330, /* Nvidia Geforce FX 5900 Ultra */
	0x0331, /* Nvidia Geforce FX 5900 */
	0x0332, /* Nvidia Geforce FX 5900 XT */
	0x0333, /* Nvidia Geforce FX 5950 Ultra */
	0x0334, /* Nvidia Geforce FX 5900 ZT */
	0x0338, /* Nvidia Quadro FX 3000 */
	0x033f, /* Nvidia Quadro FX 700 */
	0x0341, /* Nvidia Geforce FX 5700 Ultra */
	0x0342, /* Nvidia Geforce FX 5700 */
	0x0343, /* Nvidia Geforce FX 5700LE */
	0x0344, /* Nvidia Geforce FX 5700VE */
	0x0345, /* Nvidia unknown FX */
	0x0347, /* Nvidia Geforce FX 5700 Go */
	0x0348, /* Nvidia Geforce FX 5700 Go */
	0x0349, /* Nvidia unknown FX Go */
	0x034b, /* Nvidia unknown FX Go */
	0x034c, /* Nvidia Quadro FX 1000 Go */
	0x034e, /* Nvidia Quadro FX 1100 */
	0x034f, /* Nvidia unknown FX */
	0x0391, /* Nvidia Geforce 7600 GT */
	0x0392, /* Nvidia Geforce 7600 GS */
	0x0393, /* Nvidia Geforce 7300 GT */
	0x0394, /* Nvidia Geforce 7600 LE */
	0x0398, /* Nvidia Geforce 7600 GO */
	0x03d0, /* Nvidia Geforce 6100 nForce 430 */
	0x03d1, /* Nvidia Geforce 6100 nForce 405 */
	0x03d2, /* Nvidia Geforce 6100 nForce 400 */
	0x03d5, /* Nvidia Geforce 6100 nForce 420 */
	0x03d6, /* Nvidia Geforce 7025 / nForce 630a */
	0x07e1, /* Nvidia Geforce 7100 / nForce 630i */
	0
};

static uint16 elsa_device_list[] = {
	0x0c60, /* Elsa Gladiac Geforce2 MX */
	0
};

static uint16 nvstbsgs_device_list[] = {
	0x0020, /* Nvidia STB/SGS-Thompson TNT1 */
	0x0028, /* Nvidia STB/SGS-Thompson TNT2 (pro) */
	0x0029, /* Nvidia STB/SGS-Thompson TNT2 Ultra */
	0x002a, /* Nvidia STB/SGS-Thompson TNT2 */
	0x002b, /* Nvidia STB/SGS-Thompson TNT2 */
	0x002c, /* Nvidia STB/SGS-Thompson Vanta (Lt) */
	0x002d, /* Nvidia STB/SGS-Thompson TNT2-M64 (Pro) */
	0x002e, /* Nvidia STB/SGS-Thompson NV06 Vanta */
	0x002f, /* Nvidia STB/SGS-Thompson NV06 Vanta */
	0x00a0, /* Nvidia STB/SGS-Thompson Aladdin TNT2 */
	0
};

static uint16 varisys_device_list[] = {
	0x3503, /* Varisys Geforce4 MX440 */
	0x3505, /* Varisys Geforce4 Ti 4200 */
	0
};

static struct {
	uint16	vendor;
	uint16	*devices;
} SupportedDevices[] = {
	{VENDOR_ID_NVIDIA, nvidia_device_list},
	{VENDOR_ID_ELSA, elsa_device_list},
	{VENDOR_ID_NVSTBSGS, nvstbsgs_device_list},
	{VENDOR_ID_VARISYS, varisys_device_list},
	{0x0000, NULL}
};

static nv_settings sSettings = { // see comments in nvidia.settings
	/* for driver */
	DRIVER_PREFIX ".accelerant",
	"none",					// primary
	false,      			// dumprom
	/* for accelerant */
	0x00000000, 			// logmask
	0,          			// memory
	0,						// tv_output
	true,       			// usebios
	true,       			// hardcursor
	false,					// switchhead
	false,					// force_pci
	false,					// unhide_fw
	false,					// pgm_panel
	true,					// dma_acc
	false,					// vga_on_tv
	false,					// force_sync
	false,					// force_ws
	false,					// block_acc
	0,						// gpu_clk
	0,						// ram_clk
	true,					// check_edid
};


static void
dumprom(void *rom, uint32 size, pci_info pcii)
{
	int fd;
	uint32 cnt;
	char fname[64];

	/* determine the romfile name: we need split-up per card in the system */
	sprintf (fname, kUserDirectory "//" DRIVER_PREFIX "." DEVICE_FORMAT ".rom",
		pcii.vendor_id, pcii.device_id, pcii.bus, pcii.device, pcii.function);

	fd = open (fname, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) return;

	/* apparantly max. 32kb may be written at once;
	 * the ROM size is a multiple of that anyway. */
	for (cnt = 0; (cnt < size); cnt += 32768)
		write (fd, ((void *)(((uint8 *)rom) + cnt)), 32768);
	close (fd);
}


/*! return 1 if vblank interrupt has occured */
static int
caused_vbi_crtc1(vuint32 * regs)
{
	return (NV_REG32(NV32_CRTC_INTS) & 0x00000001);
}


/*! clear the vblank interrupt */
static void
clear_vbi_crtc1(vuint32 * regs)
{
	NV_REG32(NV32_CRTC_INTS) = 0x00000001;
}


static void
enable_vbi_crtc1(vuint32 * regs)
{
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC_INTS) = 0x00000001;
	/* enable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC_INTE) |= 0x00000001;
	/* enable nVidia interrupt system hardware (b0-1) */
	NV_REG32(NV32_MAIN_INTE) = 0x00000001;
}


static void
disable_vbi_crtc1(vuint32 * regs)
{
	/* disable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC_INTE) &= 0xfffffffe;
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC_INTS) = 0x00000001;
}


/*! return 1 if vblank interrupt has occured */
static int
caused_vbi_crtc2(vuint32 * regs)
{
	return (NV_REG32(NV32_CRTC2_INTS) & 0x00000001);
}


/*! clear the vblank interrupt */
static void
clear_vbi_crtc2(vuint32 * regs)
{
	NV_REG32(NV32_CRTC2_INTS) = 0x00000001;
}


static void
enable_vbi_crtc2(vuint32 * regs)
{
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC2_INTS) = 0x00000001;
	/* enable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC2_INTE) |= 0x00000001;
	/* enable nVidia interrupt system hardware (b0-1) */
	NV_REG32(NV32_MAIN_INTE) = 0x00000001;
}


static void
disable_vbi_crtc2(vuint32 * regs)
{
	/* disable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC2_INTE) &= 0xfffffffe;
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC2_INTS) = 0x00000001;
}


//fixme:
//dangerous code, on singlehead cards better not try accessing secondary head
//registers (card might react in unpredictable ways, though there's only a small
//chance we actually run into this).
//fix requires (some) card recognition code to be moved from accelerant to
//kerneldriver...
static void
disable_vbi_all(vuint32 * regs)
{
	/* disable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC_INTE) &= 0xfffffffe;
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC_INTS) = 0x00000001;

	/* disable nVidia interrupt source vblank */
	NV_REG32(NV32_CRTC2_INTE) &= 0xfffffffe;
	/* clear the vblank interrupt */
	NV_REG32(NV32_CRTC2_INTS) = 0x00000001;

	/* disable nVidia interrupt system hardware (b0-1) */
	NV_REG32(NV32_MAIN_INTE) = 0x00000000;
}


static status_t
map_device(device_info *di)
{
	char buffer[B_OS_NAME_LENGTH]; /*memory for device name*/
	shared_info *si = di->si;
	uint32	tmpUlong, tmpROMshadow;
	pci_info *pcii = &(di->pcii);
	system_info sysinfo;

	/* variables for making copy of ROM */
	uint8* rom_temp;
	area_id rom_area = -1;

	/* Nvidia cards have registers in [0] and framebuffer in [1] */
	int registers = 0;
	int frame_buffer = 1;

	/* enable memory mapped IO, disable VGA I/O - this is defined in the PCI standard */
	tmpUlong = get_pci(PCI_command, 2);
	/* enable PCI access */
	tmpUlong |= PCI_command_memory;
	/* enable busmastering */
	tmpUlong |= PCI_command_master;
	/* disable ISA I/O access */
	tmpUlong &= ~PCI_command_io;
	set_pci(PCI_command, 2, tmpUlong);

 	/*work out which version of BeOS is running*/
 	get_system_info(&sysinfo);
 	if (0)//sysinfo.kernel_build_date[0]=='J')/*FIXME - better ID version*/
 	{
 		si->use_clone_bugfix = 1;
 	}
 	else
 	{
 		si->use_clone_bugfix = 0;
 	}

	/* work out a name for the register mapping */
	sprintf(buffer, DEVICE_FORMAT " regs",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* get a virtual memory address for the registers*/
	si->regs_area = map_physical_memory(
		buffer,
		/* WARNING: Nvidia needs to map regs as viewed from PCI space! */
		di->pcii.u.h0.base_registers_pci[registers],
		di->pcii.u.h0.base_register_sizes[registers],
		B_ANY_KERNEL_ADDRESS,
		B_CLONEABLE_AREA | (si->use_clone_bugfix ? B_READ_AREA|B_WRITE_AREA : 0),
		(void **)&(di->regs));
	si->clone_bugfix_regs = (uint32 *) di->regs;

	/* if mapping registers to vmem failed then pass on error */
	if (si->regs_area < 0) return si->regs_area;

	/* work out a name for the ROM mapping*/
	sprintf(buffer, DEVICE_FORMAT " rom",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* preserve ROM shadowing setting, we need to restore the current state later on. */
	/* warning:
	 * 'don't touch': (confirmed) NV04, NV05, NV05-M64, NV11 all shutoff otherwise.
	 * NV18, NV28 and NV34 keep working.
	 * confirmed NV28 and NV34 to use upper part of shadowed ROM for scratch purposes,
	 * however the actual ROM content (so the used part) is intact (confirmed). */
	tmpROMshadow = get_pci(NVCFG_ROMSHADOW, 4);
	/* temporary disable ROM shadowing, we want the guaranteed exact contents of the chip */
	set_pci(NVCFG_ROMSHADOW, 4, 0);

	/* get ROM memory mapped base adress - this is defined in the PCI standard */
	tmpUlong = get_pci(PCI_rom_base, 4);
	//fixme?: if (!tmpUlong) try to map the ROM ourselves. Confirmed a PCIe system not
	//having the ROM mapped on PCI and PCIe cards. Falling back to fetching from ISA
	//legacy space will get us into trouble if we aren't the primary graphics card!!
	//(as legacy space always has the primary card's ROM 'mapped'!)
	if (tmpUlong) {
		/* ROM was assigned an adress, so enable ROM decoding - see PCI standard */
		tmpUlong |= 0x00000001;
		set_pci(PCI_rom_base, 4, tmpUlong);

		rom_area = map_physical_memory(
			buffer,
			di->pcii.u.h0.rom_base_pci,
			di->pcii.u.h0.rom_size,
			B_ANY_KERNEL_ADDRESS,
			B_READ_AREA,
			(void **)&(rom_temp)
		);

		/* check if we got the BIOS and signature (might fail on laptops..) */
		if (rom_area >= 0) {
			if ((rom_temp[0] != 0x55) || (rom_temp[1] != 0xaa)) {
				/* apparantly no ROM is mapped here */
				delete_area(rom_area);
				rom_area = -1;
				/* force using ISA legacy map as fall-back */
				tmpUlong = 0x00000000;
			}
		} else {
			/* mapping failed: force using ISA legacy map as fall-back */
			tmpUlong = 0x00000000;
		}
	}

	if (!tmpUlong) {
		/* ROM was not assigned an adress, fetch it from ISA legacy memory map! */
		rom_area = map_physical_memory(buffer, 0x000c0000,
			65536, B_ANY_KERNEL_ADDRESS, B_READ_AREA, (void **)&(rom_temp));
	}

	/* if mapping ROM to vmem failed then clean up and pass on error */
	if (rom_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		return rom_area;
	}

	/* dump ROM to file if selected in nvidia.settings
	 * (ROM always fits in 64Kb: checked TNT1 - FX5950) */
	if (sSettings.dumprom)
		dumprom(rom_temp, 65536, di->pcii);

	/* make a copy of ROM for future reference */
	memcpy(si->rom_mirror, rom_temp, 65536);

	/* disable ROM decoding - this is defined in the PCI standard, and delete the area */
	tmpUlong = get_pci(PCI_rom_base, 4);
	tmpUlong &= 0xfffffffe;
	set_pci(PCI_rom_base, 4, tmpUlong);
	delete_area(rom_area);

	/* restore original ROM shadowing setting to prevent trouble starting (some) cards */
	set_pci(NVCFG_ROMSHADOW, 4, tmpROMshadow);

	/* work out a name for the framebuffer mapping*/
	sprintf(buffer, DEVICE_FORMAT " framebuffer",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);

	/* map the framebuffer into vmem, using Write Combining*/
	si->fb_area = map_physical_memory(buffer,
		/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
		di->pcii.u.h0.base_registers_pci[frame_buffer],
		di->pcii.u.h0.base_register_sizes[frame_buffer],
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA,
		&(si->framebuffer));

	/*if failed with write combining try again without*/
	if (si->fb_area < 0) {
		si->fb_area = map_physical_memory(buffer,
			/* WARNING: Nvidia needs to map framebuffer as viewed from PCI space! */
			di->pcii.u.h0.base_registers_pci[frame_buffer],
			di->pcii.u.h0.base_register_sizes[frame_buffer],
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA,
			&(si->framebuffer));
	}

	/* if there was an error, delete our other areas and pass on error*/
	if (si->fb_area < 0) {
		delete_area(si->regs_area);
		si->regs_area = -1;
		return si->fb_area;
	}

	//fixme: retest for card coldstart and PCI/virt_mem mapping!!
	/* remember the DMA address of the frame buffer for BDirectWindow?? purposes */
	si->framebuffer_pci = (void *) di->pcii.u.h0.base_registers_pci[frame_buffer];

	/* note the amount of memory mapped by the kerneldriver so we can make sure we
	 * don't attempt to adress more later on */
	si->ps.memory_size = di->pcii.u.h0.base_register_sizes[frame_buffer];

	// remember settings for use here and in accelerant
	si->settings = sSettings;

	/* in any case, return the result */
	return si->fb_area;
}


static void
unmap_device(device_info *di)
{
	shared_info *si = di->si;
	uint32	tmpUlong;
	pci_info *pcii = &(di->pcii);

	/* disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 4);
	tmpUlong &= 0xfffffffc;
	set_pci(PCI_command, 4, tmpUlong);
	/* delete the areas */
	if (si->regs_area >= 0)
		delete_area(si->regs_area);
	if (si->fb_area >= 0)
		delete_area(si->fb_area);
	si->regs_area = si->fb_area = -1;
	si->framebuffer = NULL;
	di->regs = NULL;
}


static void
probe_devices(void)
{
	uint32 pci_index = 0;
	uint32 count = 0;
	device_info *di = pd->di;
	char tmp_name[B_OS_NAME_LENGTH];

	/* while there are more pci devices */
	while (count < MAX_DEVICES
		&& (*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) == B_OK) {
		int vendor = 0;

		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == di->pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == di->pcii.device_id ) {
						/* publish the device name */
						sprintf(tmp_name, DEVICE_FORMAT,
							di->pcii.vendor_id, di->pcii.device_id,
							di->pcii.bus, di->pcii.device, di->pcii.function);
						/* tweak the exported name to show first in the alphabetically ordered /dev/
						 * hierarchy folder, so the system will use it as primary adaptor if requested
						 * via nvidia.settings. */
						if (strcmp(tmp_name, sSettings.primary) == 0)
							sprintf(tmp_name, "-%s", sSettings.primary);
						/* add /dev/ hierarchy path */
						sprintf(di->name, "graphics/%s", tmp_name);
						/* remember the name */
						pd->device_names[count] = di->name;
						/* mark the driver as available for R/W open */
						di->is_open = 0;
						/* mark areas as not yet created */
						di->shared_area = -1;
						/* mark pointer to shared data as invalid */
						di->si = NULL;
						/* inc pointer to device info */
						di++;
						/* inc count */
						count++;
						/* break out of these while loops */
						goto next_device;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
next_device:
		/* next pci_info struct, please */
		pci_index++;
	}
	/* propagate count */
	pd->count = count;
	/* terminate list of device names with a null pointer */
	pd->device_names[pd->count] = NULL;
}


static uint32
thread_interrupt_work(int32 *flags, vuint32 *regs, shared_info *si)
{
	uint32 handled = B_HANDLED_INTERRUPT;
	/* release the vblank semaphore */
	if (si->vblank >= 0) {
		int32 blocked;
		if ((get_sem_count(si->vblank, &blocked) == B_OK) && (blocked < 0)) {
			release_sem_etc(si->vblank, -blocked, B_DO_NOT_RESCHEDULE);
			handled = B_INVOKE_SCHEDULER;
		}
	}
	return handled;
}


static int32
nv_interrupt(void *data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	device_info *di = (device_info *)data;
	shared_info *si = di->si;
	int32 *flags = &(si->flags);
	vuint32 *regs;

	/* is someone already handling an interrupt for this device? */
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED) goto exit0;

	/* get regs */
	regs = di->regs;

	/* was it a VBI? */
	/* note: si->ps.secondary_head was cleared by kerneldriver earlier! (at least) */
	if (si->ps.secondary_head) {
		//fixme:
		//rewrite once we use one driver instance 'per head' (instead of 'per card')
		if (caused_vbi_crtc1(regs) || caused_vbi_crtc2(regs)) {
			/* clear the interrupt(s) */
			clear_vbi_crtc1(regs);
			clear_vbi_crtc2(regs);
			/* release the semaphore */
			handled = thread_interrupt_work(flags, regs, si);
		}
	} else {
		if (caused_vbi_crtc1(regs)) {
			/* clear the interrupt */
			clear_vbi_crtc1(regs);
			/* release the semaphore */
			handled = thread_interrupt_work(flags, regs, si);
		}
	}

	/* note that we're not in the handler any more */
	atomic_and(flags, ~SKD_HANDLER_INSTALLED);

exit0:
	return handled;
}


//	#pragma mark - device hooks


static status_t
open_hook(const char* name, uint32 flags, void** cookie)
{
	int32 index = 0;
	device_info *di;
	shared_info *si;
	thread_id	thid;
	thread_info	thinfo;
	status_t	result = B_OK;
	char shared_name[B_OS_NAME_LENGTH];
	physical_entry map[1];
	size_t net_buf_size;
	void *unaligned_dma_buffer;
	uint32 mem_size;

	/* find the device name in the list of devices */
	/* we're never passed a name we didn't publish */
	while (pd->device_names[index]
		&& (strcmp(name, pd->device_names[index]) != 0))
		index++;

	/* for convienience */
	di = &(pd->di[index]);

	/* make sure no one else has write access to the common data */
	AQUIRE_BEN(pd->kernel);

	/* if it's already open for writing */
	if (di->is_open) {
		/* mark it open another time */
		goto mark_as_open;
	}
	/* create the shared_info area */
	sprintf(shared_name, DEVICE_FORMAT " shared",
		di->pcii.vendor_id, di->pcii.device_id,
		di->pcii.bus, di->pcii.device, di->pcii.function);
	/* create this area with NO user-space read or write permissions, to prevent accidental damage */
	di->shared_area = create_area(shared_name, (void **)&(di->si), B_ANY_KERNEL_ADDRESS,
		((sizeof(shared_info) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1)), B_FULL_LOCK,
		B_CLONEABLE_AREA);
	if (di->shared_area < 0) {
		/* return the error */
		result = di->shared_area;
		goto done;
	}

	/* save a few dereferences */
	si = di->si;

	/* create the DMA command buffer area */
	//fixme? for R4.5 a workaround for cloning would be needed!
	/* we want to setup a 1Mb buffer (size must be multiple of B_PAGE_SIZE) */
	net_buf_size = ((1 * 1024 * 1024) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create the area that will hold the DMA command buffer */
	si->unaligned_dma_area =
		create_area("NV DMA cmd buffer",
			(void **)&unaligned_dma_buffer,
			B_ANY_KERNEL_ADDRESS,
			2 * net_buf_size, /* take twice the net size so we can have MTRR-WC even on old systems */
			B_32_BIT_CONTIGUOUS, /* GPU always needs access */
			B_CLONEABLE_AREA | B_READ_AREA | B_WRITE_AREA);
			// TODO: Physical aligning can be done without waste using the
			// private create_area_etc().
	/* on error, abort */
	if (si->unaligned_dma_area < 0)
	{
		/* free the already created shared_info area, and return the error */
		result = si->unaligned_dma_area;
		goto free_shared;
	}
	/* we (also) need the physical adress our DMA buffer is at, as this needs to be
	 * fed into the GPU's engine later on. Get an aligned adress so we can use MTRR-WC
	 * even on older CPU's. */
	get_memory_map(unaligned_dma_buffer, B_PAGE_SIZE, map, 1);
	si->dma_buffer_pci = (void*)
		((map[0].address + net_buf_size - 1) & ~(net_buf_size - 1));

	/* map the net DMA command buffer into vmem, using Write Combining */
	si->dma_area = map_physical_memory(
		"NV aligned DMA cmd buffer", (addr_t)si->dma_buffer_pci, net_buf_size,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, &(si->dma_buffer));
	/* if failed with write combining try again without */
	if (si->dma_area < 0) {
		si->dma_area = map_physical_memory("NV aligned DMA cmd buffer",
			(addr_t)si->dma_buffer_pci, net_buf_size,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA | B_WRITE_AREA, &(si->dma_buffer));
	}
	/* if there was an error, delete our other areas and pass on error*/
	if (si->dma_area < 0)
	{
		/* free the already created areas, and return the error */
		result = si->dma_area;
		goto free_shared_and_uadma;
	}

	/* save the vendor and device IDs */
	si->vendor_id = di->pcii.vendor_id;
	si->device_id = di->pcii.device_id;
	si->revision = di->pcii.revision;
	si->bus = di->pcii.bus;
	si->device = di->pcii.device;
	si->function = di->pcii.function;

	/* ensure that the accelerant's INIT_ACCELERANT function can be executed */
	si->accelerant_in_use = false;
	/* preset singlehead card to prevent early INT routine calls (once installed) to
	 * wrongly identify the INT request coming from us! */
	si->ps.secondary_head = false;

	/* map the device */
	result = map_device(di);
	if (result < 0) goto free_shared_and_alldma;

	/* we will be returning OK status for sure now */
	result = B_OK;

	/* note the amount of system RAM the system BIOS assigned to the card if applicable:
	 * unified memory architecture (UMA) */
	switch ((((uint32)(si->device_id)) << 16) | si->vendor_id)
	{
	case 0x01a010de: /* Nvidia Geforce2 Integrated GPU */
		/* device at bus #0, device #0, function #1 holds value at byte-index 0x7C */
		mem_size = 1024 * 1024 *
			(((((*pci_bus->read_pci_config)(0, 0, 1, 0x7c, 4)) & 0x000007c0) >> 6) + 1);
		/* don't attempt to adress memory not mapped by the kerneldriver */
		if (si->ps.memory_size > mem_size) si->ps.memory_size = mem_size;
		/* last 64kB RAM is used for the BIOS (or something else?) */
		si->ps.memory_size -= (64 * 1024);
		break;
	case 0x01f010de: /* Nvidia Geforce4 MX Integrated GPU */
		/* device at bus #0, device #0, function #1 holds value at byte-index 0x84 */
		mem_size = 1024 * 1024 *
			(((((*pci_bus->read_pci_config)(0, 0, 1, 0x84, 4)) & 0x000007f0) >> 4) + 1);
		/* don't attempt to adress memory not mapped by the kerneldriver */
		if (si->ps.memory_size > mem_size) si->ps.memory_size = mem_size;
		/* last 64kB RAM is used for the BIOS (or something else?) */
		si->ps.memory_size -= (64 * 1024);
		break;
	default:
		/* all other cards have own RAM: the amount of which is determined in the
		 * accelerant. */
		break;
	}

	/* disable and clear any pending interrupts */
	//fixme:
	//distinquish between crtc1/crtc2 once all heads get seperate driver instances!
	disable_vbi_all(di->regs);

	/* preset we can't use INT related functions */
	si->ps.int_assigned = false;

	/* create a semaphore for vertical blank management */
	si->vblank = create_sem(0, di->name);
	if (si->vblank < 0) goto mark_as_open;

	/* change the owner of the semaphores to the opener's team */
	/* this is required because apps can't aquire kernel semaphores */
	thid = find_thread(NULL);
	get_thread_info(thid, &thinfo);
	set_sem_owner(si->vblank, thinfo.team);

	/* If there is a valid interrupt line assigned then set up interrupts */
	if ((di->pcii.u.h0.interrupt_pin == 0x00) ||
	    (di->pcii.u.h0.interrupt_line == 0xff) || /* no IRQ assigned */
	    (di->pcii.u.h0.interrupt_line <= 0x02))   /* system IRQ assigned */
	{
		/* delete the semaphore as it won't be used */
		delete_sem(si->vblank);
		si->vblank = -1;
	}
	else
	{
		/* otherwise install our interrupt handler */
		result = install_io_interrupt_handler(di->pcii.u.h0.interrupt_line, nv_interrupt, (void *)di, 0);
		/* bail if we couldn't install the handler */
		if (result != B_OK)
		{
			/* delete the semaphore as it won't be used */
			delete_sem(si->vblank);
			si->vblank = -1;
		}
		else
		{
			/* inform accelerant(s) we can use INT related functions */
			si->ps.int_assigned = true;
		}
	}

mark_as_open:
	/* mark the device open */
	di->is_open++;

	/* send the cookie to the opener */
	*cookie = di;

	goto done;


free_shared_and_alldma:
	/* clean up our aligned DMA area */
	delete_area(si->dma_area);
	si->dma_area = -1;
	si->dma_buffer = NULL;

free_shared_and_uadma:
	/* clean up our unaligned DMA area */
	delete_area(si->unaligned_dma_area);
	si->unaligned_dma_area = -1;
	si->dma_buffer_pci = NULL;

free_shared:
	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

done:
	/* end of critical section */
	RELEASE_BEN(pd->kernel);

	/* all done, return the status */
	return result;
}


static status_t
read_hook(void* dev, off_t pos, void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
write_hook(void* dev, off_t pos, const void* buf, size_t* len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
close_hook(void* dev)
{
	/* we don't do anything on close: there might be dup'd fd */
	return B_NO_ERROR;
}


static status_t
free_hook(void* dev)
{
	device_info *di = (device_info *)dev;
	shared_info	*si = di->si;
	vuint32 *regs = di->regs;

	/* lock the driver */
	AQUIRE_BEN(pd->kernel);

	/* if opened multiple times, decrement the open count and exit */
	if (di->is_open > 1)
		goto unlock_and_exit;

	/* disable and clear any pending interrupts */
	//fixme:
	//distinquish between crtc1/crtc2 once all heads get seperate driver instances!
	disable_vbi_all(regs);

	if (si->ps.int_assigned) {
		/* remove interrupt handler */
		remove_io_interrupt_handler(di->pcii.u.h0.interrupt_line, nv_interrupt, di);

		/* delete the semaphores, ignoring any errors ('cause the owning
		   team may have died on us) */
		delete_sem(si->vblank);
		si->vblank = -1;
	}

	/* free regs and framebuffer areas */
	unmap_device(di);

	/* clean up our aligned DMA area */
	delete_area(si->dma_area);
	si->dma_area = -1;
	si->dma_buffer = NULL;

	/* clean up our unaligned DMA area */
	delete_area(si->unaligned_dma_area);
	si->unaligned_dma_area = -1;
	si->dma_buffer_pci = NULL;

	/* clean up our shared area */
	delete_area(di->shared_area);
	di->shared_area = -1;
	di->si = NULL;

unlock_and_exit:
	/* mark the device available */
	di->is_open--;
	/* unlock the driver */
	RELEASE_BEN(pd->kernel);
	/* all done */
	return B_OK;
}


static status_t
control_hook(void* dev, uint32 msg, void *buf, size_t len)
{
	device_info *di = (device_info *)dev;
	status_t result = B_DEV_INVALID_IOCTL;
	uint32 tmpUlong;

	switch (msg) {
		/* the only PUBLIC ioctl */
		case B_GET_ACCELERANT_SIGNATURE:
		{
			strcpy((char* )buf, sSettings.accelerant);
			result = B_OK;
			break;
		}

		/* PRIVATE ioctl from here on */
		case NV_GET_PRIVATE_DATA:
		{
			nv_get_private_data *gpd = (nv_get_private_data *)buf;
			if (gpd->magic == NV_PRIVATE_DATA_MAGIC) {
				gpd->shared_info_area = di->shared_area;
				result = B_OK;
			}
			break;
		}

		case NV_GET_PCI:
		{
			nv_get_set_pci *gsp = (nv_get_set_pci *)buf;
			if (gsp->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				gsp->value = get_pci(gsp->offset, gsp->size);
				result = B_OK;
			}
			break;
		}

		case NV_SET_PCI:
		{
			nv_get_set_pci *gsp = (nv_get_set_pci *)buf;
			if (gsp->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);
				set_pci(gsp->offset, gsp->size, gsp->value);
				result = B_OK;
			}
			break;
		}

		case NV_DEVICE_NAME:
		{
			nv_device_name *dn = (nv_device_name *)buf;
			if (dn->magic == NV_PRIVATE_DATA_MAGIC) {
				strcpy(dn->name, di->name);
				result = B_OK;
			}
			break;
		}

		case NV_RUN_INTERRUPTS:
		{
			nv_set_vblank_int *vi = (nv_set_vblank_int *)buf;
			if (vi->magic == NV_PRIVATE_DATA_MAGIC) {
				vuint32 *regs = di->regs;
				if (!(vi->crtc)) {
					if (vi->do_it) {
						enable_vbi_crtc1(regs);
					} else {
						disable_vbi_crtc1(regs);
					}
				} else {
					if (vi->do_it) {
						enable_vbi_crtc2(regs);
					} else {
						disable_vbi_crtc2(regs);
					}
				}
				result = B_OK;
			}
			break;
		}

		case NV_GET_NTH_AGP_INFO:
		{
			nv_nth_agp_info *nai = (nv_nth_agp_info *)buf;
			if (nai->magic == NV_PRIVATE_DATA_MAGIC) {
				nai->exist = false;
				nai->agp_bus = false;
				if (agp_bus) {
					nai->agp_bus = true;
					if ((*agp_bus->get_nth_agp_info)(nai->index, &(nai->agpi)) == B_NO_ERROR) {
						nai->exist = true;
					}
				}
				result = B_OK;
			}
			break;
		}

		case NV_ENABLE_AGP:
		{
			nv_cmd_agp *nca = (nv_cmd_agp *)buf;
			if (nca->magic == NV_PRIVATE_DATA_MAGIC) {
				if (agp_bus) {
					nca->agp_bus = true;
					nca->cmd = agp_bus->set_agp_mode(nca->cmd);
				} else {
					nca->agp_bus = false;
					nca->cmd = 0;
				}
				result = B_OK;
			}
			break;
		}

		case NV_ISA_OUT:
		{
			nv_in_out_isa *io_isa = (nv_in_out_isa *)buf;
			if (io_isa->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);

				/* lock the driver:
				 * no other graphics card may have ISA I/O enabled when we enter */
				AQUIRE_BEN(pd->kernel);

				/* enable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong |= PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				if (io_isa->size == 1)
  					isa_bus->write_io_8(io_isa->adress, (uint8)io_isa->data);
   				else
   					isa_bus->write_io_16(io_isa->adress, io_isa->data);
  				result = B_OK;

				/* disable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong &= ~PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				/* end of critical section */
				RELEASE_BEN(pd->kernel);
   			}
			break;
		}

		case NV_ISA_IN:
		{
			nv_in_out_isa *io_isa = (nv_in_out_isa *)buf;
			if (io_isa->magic == NV_PRIVATE_DATA_MAGIC) {
				pci_info *pcii = &(di->pcii);

				/* lock the driver:
				 * no other graphics card may have ISA I/O enabled when we enter */
				AQUIRE_BEN(pd->kernel);

				/* enable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong |= PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				if (io_isa->size == 1)
	   				io_isa->data = isa_bus->read_io_8(io_isa->adress);
	   			else
	   				io_isa->data = isa_bus->read_io_16(io_isa->adress);
   				result = B_OK;

				/* disable ISA I/O access */
				tmpUlong = get_pci(PCI_command, 2);
				tmpUlong &= ~PCI_command_io;
				set_pci(PCI_command, 2, tmpUlong);

				/* end of critical section */
				RELEASE_BEN(pd->kernel);
   			}
			break;
		}
	}

	return result;
}


//	#pragma mark - driver API


status_t
init_hardware(void)
{
	long index = 0;
	pci_info pcii;
	bool found = false;

	/* choke if we can't find the PCI bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* choke if we can't find the ISA bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK)
	{
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* while there are more pci devices */
	while ((*pci_bus->get_nth_pci_info)(index, &pcii) == B_NO_ERROR) {
		int vendor = 0;

		/* if we match a supported vendor */
		while (SupportedDevices[vendor].vendor) {
			if (SupportedDevices[vendor].vendor == pcii.vendor_id) {
				uint16 *devices = SupportedDevices[vendor].devices;
				/* while there are more supported devices */
				while (*devices) {
					/* if we match a supported device */
					if (*devices == pcii.device_id ) {

						found = true;
						goto done;
					}
					/* next supported device */
					devices++;
				}
			}
			vendor++;
		}
		/* next pci_info struct, please */
		index++;
	}

done:
	/* put away the module manager */
	put_module(B_PCI_MODULE_NAME);
	return found ? B_OK : B_ERROR;
}


status_t
init_driver(void)
{
	void *settings;

	// get driver/accelerant settings
	settings = load_driver_settings(DRIVER_PREFIX ".settings");
	if (settings != NULL) {
		const char *item;
		char *end;
		uint32 value;

		// for driver
		item = get_driver_parameter(settings, "accelerant", "", "");
		if (item[0] && strlen(item) < sizeof(sSettings.accelerant) - 1)
			strcpy (sSettings.accelerant, item);

		item = get_driver_parameter(settings, "primary", "", "");
		if (item[0] && strlen(item) < sizeof(sSettings.primary) - 1)
			strcpy(sSettings.primary, item);

		sSettings.dumprom = get_driver_boolean_parameter(settings,
			"dumprom", false, false);

		// for accelerant
		item = get_driver_parameter(settings, "logmask",
			"0x00000000", "0x00000000");
		value = strtoul(item, &end, 0);
		if (*end == '\0')
			sSettings.logmask = value;

		item = get_driver_parameter(settings, "memory", "0", "0");
		value = strtoul(item, &end, 0);
		if (*end == '\0')
			sSettings.memory = value;

		item = get_driver_parameter(settings, "tv_output", "0", "0");
		value = strtoul(item, &end, 0);
		if (*end == '\0')
			sSettings.tv_output = value;

		sSettings.hardcursor = get_driver_boolean_parameter(settings,
			"hardcursor", true, true);
		sSettings.usebios = get_driver_boolean_parameter(settings,
			"usebios", true, true);
		sSettings.switchhead = get_driver_boolean_parameter(settings,
			"switchhead", false, false);
		sSettings.force_pci = get_driver_boolean_parameter(settings,
			"force_pci", false, false);
		sSettings.unhide_fw = get_driver_boolean_parameter(settings,
			"unhide_fw", false, false);
		sSettings.pgm_panel = get_driver_boolean_parameter(settings,
			"pgm_panel", false, false);
		sSettings.dma_acc = get_driver_boolean_parameter(settings,
			"dma_acc", true, true);
		sSettings.vga_on_tv = get_driver_boolean_parameter(settings,
			"vga_on_tv", false, false);
		sSettings.force_sync = get_driver_boolean_parameter(settings,
			"force_sync", false, false);
		sSettings.force_ws = get_driver_boolean_parameter(settings,
			"force_ws", false, false);
		sSettings.block_acc = get_driver_boolean_parameter(settings,
			"block_acc", false, false);
		sSettings.check_edid = get_driver_boolean_parameter(settings,
			"check_edid", true, true);

		item = get_driver_parameter(settings, "gpu_clk", "0", "0");
		value = strtoul(item, &end, 0);
		if (*end == '\0')
			sSettings.gpu_clk = value;

		item = get_driver_parameter(settings, "ram_clk", "0", "0");
		value = strtoul(item, &end, 0);
		if (*end == '\0')
			sSettings.ram_clk = value;

		unload_driver_settings(settings);
	}

	/* get a handle for the pci bus */
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	/* get a handle for the isa bus */
	if (get_module(B_ISA_MODULE_NAME, (module_info **)&isa_bus) != B_OK) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* get a handle for the agp bus if it exists */
	get_module(B_AGP_GART_MODULE_NAME, (module_info **)&agp_bus);

	/* driver private data */
	pd = (DeviceData *)calloc(1, sizeof(DeviceData));
	if (!pd) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	/* initialize the benaphore */
	INIT_BEN(pd->kernel);
	/* find all of our supported devices */
	probe_devices();
	return B_OK;
}


const char **
publish_devices(void)
{
	/* return the list of supported devices */
	return (const char **)pd->device_names;
}


device_hooks *
find_device(const char *name)
{
	int index = 0;
	while (pd->device_names[index]) {
		if (strcmp(name, pd->device_names[index]) == 0)
			return &graphics_device_hooks;
		index++;
	}
	return NULL;

}


void
uninit_driver(void)
{
	/* free the driver data */
	DELETE_BEN(pd->kernel);
	free(pd);
	pd = NULL;

	/* put the pci module away */
	put_module(B_PCI_MODULE_NAME);
	put_module(B_ISA_MODULE_NAME);

	/* put the agp module away if it's there */
	if (agp_bus)
		put_module(B_AGP_GART_MODULE_NAME);
}

