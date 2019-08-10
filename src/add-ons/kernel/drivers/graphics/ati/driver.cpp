/*
	Copyright 2007-2011 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/

#include <KernelExport.h>
#include <PCI.h>
#include <drivers/bios.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <graphic_driver.h>
#include <boot_item.h>

#include "DriverInterface.h"


#undef TRACE

#ifdef ENABLE_DEBUG_TRACE
#	define TRACE(x...) dprintf("ati: " x)
#else
#	define TRACE(x...) ;
#endif


#define ATI_ACCELERANT_NAME    "ati.accelerant"

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

#define VESA_MODES_BOOT_INFO "vesa_modes/v1"

#define SKD_HANDLER_INSTALLED 0x80000000
#define MAX_DEVICES		4
#define DEVICE_FORMAT	"%04X_%04X_%02X%02X%02X"

#define M64_BIOS_SIZE		0x10000		// 64KB
#define R128_BIOS_SIZE		0x10000		// 64KB

int32 api_version = B_CUR_DRIVER_API_VERSION;	// revision of driver API used

#define VENDOR_ID 0x1002	// ATI vendor ID

// Mach64 register definitions.
#define M64_CLOCK_INTERNAL	4
#define M64_CONFIG_CHIP_ID	0x0CE0		// offset in register area
#define M64_CFG_CHIP_TYPE	0x0000FFFF


struct ChipInfo {
	uint16		chipID;			// PCI device id of the chip
	ChipType	chipType;		// assigned chip type identifier
	const char*	chipName;		// user recognizable name for chip
								//   (must be < 32 chars)
};


// Names for chip types.

static char sRage128_GL[]		 = "RAGE 128 GL";
static char sRage128_VR[]		 = "RAGE 128 VR";
static char sRage128_Pro_GL[]	 = "RAGE 128 PRO GL";
static char sRage128_Pro_VR[]	 = "RAGE 128 PRO VR";
static char sRage128_Pro_Ultra[] = "RAGE 128 PRO Ultra";

// This table maps a PCI device ID to a chip type identifier and the chip name.
// The table is split into two groups of chips, the Mach64 and Rage128 chips,
// with each group ordered by the chip ID.

static const ChipInfo chipTable[] = {
	{ 0x4742, MACH64_264GTPRO,		"3D RAGE PRO, AGP"		},		// GB
	{ 0x4744, MACH64_264GTPRO,		"3D RAGE PRO, AGP"		},		// GD
	{ 0x4749, MACH64_264GTPRO,		"3D RAGE PRO, PCI"		},		// GI
	{ 0x474C, MACH64_264XL,			"3D RAGE XC, PCI"		},		// GL
	{ 0x474D, MACH64_264XL,			"3D RAGE XL, AGP"		},		// GM
	{ 0x474E, MACH64_264XL,			"3D RAGE XC, AGP"		},		// GN
	{ 0x474F, MACH64_264XL,			"3D RAGE XL, PCI"		},		// GO
	{ 0x4750, MACH64_264GTPRO,		"3D RAGE PRO, PCI"		},		// GP
	{ 0x4751, MACH64_264GTPRO,		"3D RAGE PRO, PCI"		},		// GQ
	{ 0x4752, MACH64_264XL,			"3D RAGE XL, PCI"		},		// GR
	{ 0x4753, MACH64_264XL,			"3D RAGE XC, PCI"		},		// GS
	{ 0x4754, MACH64_264GT,			"3D RAGE II"			},		// GT
	{ 0x4755, MACH64_264GTDVD,		"3D RAGE II+"			},		// GU
	{ 0x4756, MACH64_264GT2C,		"3D RAGE IIC, PCI"		},		// GV
	{ 0x4757, MACH64_264GT2C,		"3D RAGE IIC, AGP"		},		// GW
	{ 0x4759, MACH64_264GT2C,		"3D RAGE IIC, PCI"		},		// GY
	{ 0x475A, MACH64_264GT2C,		"3D RAGE IIC, AGP"		},		// GZ
	{ 0x4C42, MACH64_264LTPRO,		"3D RAGE LT PRO, AGP"	},		// LB
	{ 0x4C44, MACH64_264LTPRO,		"3D RAGE LT PRO, AGP"	},		// LD
	{ 0x4C47, MACH64_264LT,			"3D RAGE LT"			},		// LG
	{ 0x4C49, MACH64_264LTPRO,		"3D RAGE LT PRO, PCI"	},		// LI
	{ 0x4C4D, MACH64_MOBILITY,		"3D RAGE Mobility, AGP"	},		// LM
	{ 0x4C4E, MACH64_MOBILITY,		"3D RAGE Mobility, AGP"	},		// LN
	{ 0x4C50, MACH64_264LTPRO,		"3D RAGE LT PRO, PCI"	},		// LP
	{ 0x4C51, MACH64_264LTPRO,		"3D RAGE LT PRO, PCI"	},		// LQ
	{ 0x4C52, MACH64_MOBILITY,		"3D RAGE Mobility, PCI"	},		// LR
	{ 0x4C53, MACH64_MOBILITY,		"3D RAGE Mobility, PCI"	},		// LS
	{ 0x5654, MACH64_264VT,			"264VT2"				},		// VT
	{ 0x5655, MACH64_264VT3,		"264VT3"				},		// VU
	{ 0x5656, MACH64_264VT4,		"264VT4"				},		// VV

	{ 0x4C45, RAGE128_MOBILITY,		"RAGE 128 Mobility 3"	},		// LE
	{ 0x4C46, RAGE128_MOBILITY,		"RAGE 128 Mobility 3"	},		// LF
	{ 0x4D46, RAGE128_MOBILITY,		"RAGE 128 Mobility 4"	},		// MF
	{ 0x4D4C, RAGE128_MOBILITY,		"RAGE 128 Mobility 4"	},		// ML
	{ 0x5041, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PA
	{ 0x5042, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PB
	{ 0x5043, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PC
	{ 0x5044, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PD
	{ 0x5045, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PE
	{ 0x5046, RAGE128_PRO_GL,		sRage128_Pro_GL			},		// PF
	{ 0x5047, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PG
	{ 0x5048, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PH
	{ 0x5049, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PI
	{ 0x504A, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PJ
	{ 0x504B, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PK
	{ 0x504C, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PL
	{ 0x504D, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PM
	{ 0x504E, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PN
	{ 0x504F, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PO
	{ 0x5050, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PP
	{ 0x5051, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PQ
	{ 0x5052, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PR
	{ 0x5053, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PS
	{ 0x5054, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PT
	{ 0x5055, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PU
	{ 0x5056, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PV
	{ 0x5057, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PW
	{ 0x5058, RAGE128_PRO_VR,		sRage128_Pro_VR			},		// PX
	{ 0x5245, RAGE128_GL,			sRage128_GL				},		// RE
	{ 0x5246, RAGE128_GL,			sRage128_GL				},		// RF
	{ 0x5247, RAGE128_GL,			sRage128_GL				},		// RG
	{ 0x524B, RAGE128_VR,			sRage128_VR				},		// RK
	{ 0x524C, RAGE128_VR,			sRage128_VR				},		// RL
	{ 0x5345, RAGE128_VR,			sRage128_VR				},		// SE
	{ 0x5346, RAGE128_VR,			sRage128_VR				},		// SF
	{ 0x5347, RAGE128_VR,			sRage128_VR				},		// SG
	{ 0x5348, RAGE128_VR,			sRage128_VR				},		// SH
	{ 0x534B, RAGE128_GL,			sRage128_GL				},		// SK
	{ 0x534C, RAGE128_GL,			sRage128_GL				},		// SL
	{ 0x534D, RAGE128_GL,			sRage128_GL				},		// SM
	{ 0x534E, RAGE128_GL,			sRage128_GL				},		// SN
	{ 0x5446, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TF
	{ 0x544C, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TL
	{ 0x5452, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TR
	{ 0x5453, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TS
	{ 0x5454, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TT
	{ 0x5455, RAGE128_PRO_ULTRA,	sRage128_Pro_Ultra		},		// TU
	{ 0,	  ATI_NONE,				NULL }
};


struct DeviceInfo {
	uint32			openCount;		// count of how many times device has been opened
	int32			flags;
	area_id 		sharedArea;		// area shared between driver and all accelerants
	SharedInfo* 	sharedInfo;		// pointer to shared info area memory
	vuint8*	 		regs;			// pointer to memory mapped registers
	const ChipInfo*	pChipInfo;		// info about the selected chip
	pci_info		pciInfo;		// copy of pci info for this device
	char			name[B_OS_NAME_LENGTH]; // name of device
};


static Benaphore		gLock;
static DeviceInfo		gDeviceInfo[MAX_DEVICES];
static char*			gDeviceNames[MAX_DEVICES + 1];
static pci_module_info*	gPCI;


// Prototypes for device hook functions.

static status_t device_open(const char* name, uint32 flags, void** cookie);
static status_t device_close(void* dev);
static status_t device_free(void* dev);
static status_t device_read(void* dev, off_t pos, void* buf, size_t* len);
static status_t device_write(void* dev, off_t pos, const void* buf, size_t* len);
static status_t device_ioctl(void* dev, uint32 msg, void* buf, size_t len);

static device_hooks gDeviceHooks =
{
	device_open,
	device_close,
	device_free,
	device_ioctl,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL
};



static inline uint32
GetPCI(pci_info& info, uint8 offset, uint8 size)
{
	return gPCI->read_pci_config(info.bus, info.device, info.function, offset,
		size);
}


static inline void
SetPCI(pci_info& info, uint8 offset, uint8 size, uint32 value)
{
	gPCI->write_pci_config(info.bus, info.device, info.function, offset, size,
		value);
}


// Functions for dealing with Vertical Blanking Interrupts.  Currently, I do
// not know the commands to handle these operations;  thus, these functions
// currently do nothing.

static bool
InterruptIsVBI()
{
	// return true only if a vertical blanking interrupt has occured
	return false;
}


static void
ClearVBI()
{
}

static void
EnableVBI()
{
}

static void
DisableVBI()
{
}



static status_t
GetEdidFromBIOS(edid1_raw& edidRaw)
{
	// Get the EDID info from the video BIOS, and return B_OK if successful.

#define ADDRESS_SEGMENT(address) ((addr_t)(address) >> 4)
#define ADDRESS_OFFSET(address) ((addr_t)(address) & 0xf)

	bios_module_info* biosModule;
	status_t status = get_module(B_BIOS_MODULE_NAME, (module_info**)&biosModule);
	if (status != B_OK) {
		TRACE("GetEdidFromBIOS(): failed to get BIOS module: 0x%" B_PRIx32 "\n",
			status);
		return status;
	}

	bios_state* state;
	status = biosModule->prepare(&state);
	if (status != B_OK) {
		TRACE("GetEdidFromBIOS(): bios_prepare() failed: 0x%" B_PRIx32 "\n",
			status);
		put_module(B_BIOS_MODULE_NAME);
		return status;
	}

	bios_regs regs = {};
	regs.eax = 0x4f15;
	regs.ebx = 0;			// 0 = report DDC service
	regs.ecx = 0;
	regs.es = 0;
	regs.edi = 0;

	status = biosModule->interrupt(state, 0x10, &regs);
	if (status == B_OK) {
		// AH contains the error code, and AL determines whether or not the
		// function is supported.
		if (regs.eax != 0x4f)
			status = B_NOT_SUPPORTED;

		// Test if DDC is supported by the monitor.
		if ((regs.ebx & 3) == 0)
			status = B_NOT_SUPPORTED;
	}

	if (status == B_OK) {
		edid1_raw* edid = (edid1_raw*)biosModule->allocate_mem(state,
			sizeof(edid1_raw));
		if (edid == NULL) {
			status = B_NO_MEMORY;
			goto out;
		}

		regs.eax = 0x4f15;
		regs.ebx = 1;		// 1 = read EDID
		regs.ecx = 0;
		regs.edx = 0;
		regs.es  = ADDRESS_SEGMENT(edid);
		regs.edi = ADDRESS_OFFSET(edid);

		status = biosModule->interrupt(state, 0x10, &regs);
		if (status == B_OK) {
			if (regs.eax != 0x4f) {
				status = B_NOT_SUPPORTED;
			} else {
				// Copy the EDID info to the caller's location, and compute the
				// checksum of the EDID info while copying.

				uint8 sum = 0;
				uint8 allOr = 0;
				uint8* dest = (uint8*)&edidRaw;
				uint8* src = (uint8*)edid;

				for (uint32 j = 0; j < sizeof(edidRaw); j++) {
					sum += *src;
					allOr |= *src;
					*dest++ = *src++;
				}

				if (allOr == 0) {
					TRACE("GetEdidFromBIOS(); EDID info contains only zeros\n");
					status = B_ERROR;
				} else if (sum != 0) {
					TRACE("GetEdidFromBIOS(); Checksum error in EDID info\n");
					status = B_ERROR;
				}
			}
		}
	}

out:
	biosModule->finish(state);
	put_module(B_BIOS_MODULE_NAME);
	return status;
}


static status_t
SetVesaDisplayMode(uint16 mode)
{
	// Set the VESA display mode, and return B_OK if successful.

#define SET_MODE_MASK				0x01ff
#define SET_MODE_LINEAR_BUFFER		(1 << 14)

	bios_module_info* biosModule;
	status_t status = get_module(B_BIOS_MODULE_NAME, (module_info**)&biosModule);
	if (status != B_OK) {
		TRACE("SetVesaDisplayMode(0x%x): failed to get BIOS module: 0x%" B_PRIx32
			"\n", mode, status);
		return status;
	}

	bios_state* state;
	status = biosModule->prepare(&state);
	if (status != B_OK) {
		TRACE("SetVesaDisplayMode(0x%x): bios_prepare() failed: 0x%" B_PRIx32
			"\n", mode, status);
		put_module(B_BIOS_MODULE_NAME);
		return status;
	}

	bios_regs regs = {};
	regs.eax = 0x4f02;
	regs.ebx = (mode & SET_MODE_MASK) | SET_MODE_LINEAR_BUFFER;

	status = biosModule->interrupt(state, 0x10, &regs);
	if (status != B_OK) {
		TRACE("SetVesaDisplayMode(0x%x): BIOS interrupt failed\n", mode);
	}

	if (status == B_OK && (regs.eax & 0xffff) != 0x4f) {
		TRACE("SetVesaDisplayMode(0x%x): BIOS returned 0x%04" B_PRIx32 "\n",
			mode, regs.eax & 0xffff);
		status = B_ERROR;
	}

	biosModule->finish(state);
	put_module(B_BIOS_MODULE_NAME);
	return status;
}



// Macros for accessing BIOS info.

#define BIOS8(v)  (romAddr[v])
#define BIOS16(v) (romAddr[v] | \
				  (romAddr[(v) + 1] << 8))
#define BIOS32(v) (romAddr[v] | \
				  (romAddr[(v) + 1] << 8) | \
				  (romAddr[(v) + 2] << 16) | \
				  (romAddr[(v) + 3] << 24))


static status_t
Mach64_GetBiosParameters(DeviceInfo& di, uint8& clockType)
{
	// Get some clock parameters from the video BIOS, and if Mobility chip,
	// also get the LCD panel width & height.

	// In case mapping the ROM area fails or other error occurs, set default
	// values for the parameters which will be obtained from the BIOS ROM.

	clockType = M64_CLOCK_INTERNAL;

	SharedInfo& si = *(di.sharedInfo);
	M64_Params& params = si.m64Params;
	params.clockNumberToProgram = 3;

	si.panelX = 0;
	si.panelY = 0;

	// Map the ROM area.  The Mach64 chips do not assign a ROM address in the
	// PCI info;  thus, access the ROM via the ISA legacy memory map.

	uint8* romAddr;
	area_id romArea = map_physical_memory("ATI Mach64 ROM",
		0x000c0000,
		M64_BIOS_SIZE,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA,
		(void**)&(romAddr));

	if (romArea < 0) {
		TRACE("Mach64_GetBiosParameters(), ROM mapping error: %ld\n", romArea);
		return romArea;		// ROM mapping failed; return error code
	}

	// Check if we have the BIOS signature (might fail on laptops..).

	if (BIOS8(0) != 0x55 || BIOS8(1) != 0xaa) {
		TRACE("Mach64_GetBiosParameters(), ROM does not contain BIOS signature\n");
		delete_area(romArea);
		return B_ERROR;
	}

	// Get clock info from BIOS.

	uint32 romTable = BIOS16(0x48);
	uint32 clockTable = BIOS16(romTable + 16);
	clockType = BIOS8(clockTable);
	params.clockNumberToProgram = BIOS8(clockTable + 6);
	params.maxPixelClock = BIOS16(clockTable + 4) * 10;
	params.refFreq = BIOS16(clockTable + 8);
	params.refDivider = BIOS16(clockTable + 10);

	// If Mobility chip, get the LCD panel width & height.

	if (si.chipType == MACH64_MOBILITY) {
		uint32 lcdTable = BIOS16(0x78);
		if (BIOS32(lcdTable) == 0x544d5224) {	// is LCD table signature correct?
			uint32 lcdPanelInfo = BIOS16(lcdTable + 10);
			si.panelX = BIOS16(lcdPanelInfo + 25);
			si.panelY = BIOS16(lcdPanelInfo + 27);
			TRACE("Mobility LCD Panel size: %dx%d\n", si.panelX, si.panelY);
		} else {
			TRACE("Mobility LCD table signature 0x%x in BIOS is incorrect\n",
				 BIOS32(lcdTable));
		}
	}

	delete_area(romArea);

	return B_OK;
}



static status_t
Rage128_GetBiosParameters(DeviceInfo& di)
{
	// Get the PLL parameters from the video BIOS, and if Mobility chips, also
	// get the LCD panel width & height and a few other related parameters.

	// In case mapping the ROM area fails or other error occurs, set default
	// values for the parameters which will be obtained from the BIOS ROM.
	// The default PLL parameters values probably will not work for all chips.
	// For example, reference freq can be 29.50MHz, 28.63MHz, or 14.32MHz.

	SharedInfo& si = *(di.sharedInfo);
	R128_PLLParams& pll = si.r128PLLParams;
	pll.reference_freq = 2950;
	pll.reference_div = 65;
	pll.min_pll_freq = 12500;
	pll.max_pll_freq = 25000;
	pll.xclk = 10300;

	si.panelX = 0;
	si.panelY = 0;
	si.panelPowerDelay = 1;

	// Map the ROM area.  The Rage128 chips do not assign a ROM address in the
	// PCI info;  thus, access the ROM via the ISA legacy memory map.

	uint8* romAddr;
	area_id romArea = map_physical_memory("ATI Rage128 ROM",
		0x000c0000,
		R128_BIOS_SIZE,
		B_ANY_KERNEL_ADDRESS,
		B_READ_AREA,
		(void**)&(romAddr));

	if (romArea < 0) {
		TRACE("Rage128_GetBiosParameters(), ROM mapping error: %ld\n", romArea);
		return romArea;		// ROM mapping failed; return error code
	}

	// Check if we got the BIOS signature (might fail on laptops..).

	if (BIOS8(0) != 0x55 || BIOS8(1) != 0xaa) {
		TRACE("Rage128_GetBiosParameters(), ROM does not contain BIOS signature\n");
		delete_area(romArea);
		return B_ERROR;
	}

	// Get the PLL values from the mapped ROM area.

	uint16 biosHeader = BIOS16(0x48);
	uint16 pllInfoBlock = BIOS16(biosHeader + 0x30);

	pll.reference_freq = BIOS16(pllInfoBlock + 0x0e);
	pll.reference_div = BIOS16(pllInfoBlock + 0x10);
	pll.min_pll_freq = BIOS32(pllInfoBlock + 0x12);
	pll.max_pll_freq = BIOS32(pllInfoBlock + 0x16);
	pll.xclk = BIOS16(pllInfoBlock + 0x08);

	TRACE("PLL parameters: rf=%d rd=%d min=%ld max=%ld; xclk=%d\n",
		pll.reference_freq, pll.reference_div, pll.min_pll_freq,
		pll.max_pll_freq, pll.xclk);

	// If Mobility chip, get the LCD panel width & height and a few other
	// related parameters.

	if (si.chipType == RAGE128_MOBILITY) {
		// There should be direct access to the start of the FP info table, but
		// until we find out where that offset is stored, we must search for
		// the ATI signature string: "M3      ".

		int i;
		for (i = 4; i < R128_BIOS_SIZE - 8; i++) {
			if (BIOS8(i) == 'M' &&
					BIOS8(i + 1) == '3' &&
					BIOS8(i + 2) == ' ' &&
					BIOS8(i + 3) == ' ' &&
					BIOS8(i + 4) == ' ' &&
					BIOS8(i + 5) == ' ' &&
					BIOS8(i + 6) == ' ' &&
					BIOS8(i + 7) == ' ') {
				int fpHeader = i - 2;

				// Assume that only one panel is attached and supported.

				for (i = fpHeader + 20; i < fpHeader + 84; i += 2) {
					if (BIOS16(i) != 0) {
						int fpStart = BIOS16(i);
						si.panelX = BIOS16(fpStart + 25);
						si.panelY = BIOS16(fpStart + 27);
						si.panelPowerDelay = BIOS8(fpStart + 56);
						TRACE("LCD Panel size: %dx%d  Panel type: 0x%x   power delay: %d\n",
							si.panelX, si.panelY, BIOS16(fpStart + 29),
							si.panelPowerDelay);
						break;
					}
				}

				break;
			}
		}
	}

	delete_area(romArea);

	return B_OK;
}


static status_t
MapDevice(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);
	pci_info& pciInfo = di.pciInfo;

	// Enable memory mapped IO and bus master.

	SetPCI(pciInfo, PCI_command, 2, GetPCI(pciInfo, PCI_command, 2)
		| PCI_command_io | PCI_command_memory | PCI_command_master);

	// Map the video memory.

	phys_addr_t videoRamAddr = pciInfo.u.h0.base_registers[0];
	uint32 videoRamSize = pciInfo.u.h0.base_register_sizes[0];
	si.videoMemPCI = videoRamAddr;
	char frameBufferAreaName[] = "ATI frame buffer";

	si.videoMemArea = map_physical_memory(
		frameBufferAreaName,
		videoRamAddr,
		videoRamSize,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA + B_WRITE_AREA,
		(void**)&(si.videoMemAddr));

	if (si.videoMemArea < 0) {
		// Try to map this time without write combining.
		si.videoMemArea = map_physical_memory(
			frameBufferAreaName,
			videoRamAddr,
			videoRamSize,
			B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA + B_WRITE_AREA,
			(void**)&(si.videoMemAddr));
	}

	if (si.videoMemArea < 0)
		return si.videoMemArea;

	// Map the MMIO register area.

	phys_addr_t regsBase = pciInfo.u.h0.base_registers[2];
	uint32 regAreaSize = pciInfo.u.h0.base_register_sizes[2];

	// If the register area address or size is not in the PCI info, it should
	// be at the end of the video memory.  Check if it is there.

	if (MACH64_FAMILY(si.chipType) && (regsBase == 0 || regAreaSize == 0)) {
		uint32 regsOffset = 0x7ff000;	// offset to regs area in video memory
		uint32 regs = uint32(si.videoMemAddr) + regsOffset;
		uint32 chipInfo = *((vuint32*)(regs + M64_CONFIG_CHIP_ID));

		if (si.deviceID != (chipInfo & M64_CFG_CHIP_TYPE)) {
			// Register area not found;  delete any other areas that were
			// created.
			delete_area(si.videoMemArea);
			si.videoMemArea = -1;
			TRACE("Mach64 register area not found\n");
			return B_ERROR;
		}

		// Adjust params for creating register area below.

		regsBase = videoRamAddr + regsOffset;
		regAreaSize = 0x1000;
		TRACE("Register address is at end of frame buffer memory at 0x%lx\n",
			uint32(regsBase));
	}

	si.regsArea = map_physical_memory("ATI mmio registers",
		regsBase,
		regAreaSize,
		B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA,
		(void**)&di.regs);

	// If there was an error, delete other areas.
	if (si.regsArea < 0) {
		delete_area(si.videoMemArea);
		si.videoMemArea = -1;
	}

	return si.regsArea;
}


static void
UnmapDevice(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);

	if (si.regsArea >= 0)
		delete_area(si.regsArea);
	if (si.videoMemArea >= 0)
		delete_area(si.videoMemArea);

	si.regsArea = si.videoMemArea = -1;
	si.videoMemAddr = (addr_t)NULL;
	di.regs = NULL;
}


static int32
InterruptHandler(void* data)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	DeviceInfo& di = *((DeviceInfo*)data);
	int32* flags = &(di.flags);

	// Is someone already handling an interrupt for this device?
	if (atomic_or(flags, SKD_HANDLER_INSTALLED) & SKD_HANDLER_INSTALLED)
		return B_UNHANDLED_INTERRUPT;

	if (InterruptIsVBI()) {	// was interrupt a VBI?
		ClearVBI();			// clear interrupt

		handled = B_HANDLED_INTERRUPT;

		// Release vertical blanking semaphore.
		sem_id& sem = di.sharedInfo->vertBlankSem;

		if (sem >= 0) {
			int32 blocked;
			if ((get_sem_count(sem, &blocked) == B_OK) && (blocked < 0)) {
				release_sem_etc(sem, -blocked, B_DO_NOT_RESCHEDULE);
				handled = B_INVOKE_SCHEDULER;
			}
		}
	}

	atomic_and(flags, ~SKD_HANDLER_INSTALLED);	// note we're not in handler anymore

	return handled;
}


static void
InitInterruptHandler(DeviceInfo& di)
{
	SharedInfo& si = *(di.sharedInfo);

	DisableVBI();					// disable & clear any pending interrupts
	si.bInterruptAssigned = false;	// indicate interrupt not assigned yet

	// Create a semaphore for vertical blank management.
	si.vertBlankSem = create_sem(0, di.name);
	if (si.vertBlankSem < 0)
		return;

	// Change the owner of the semaphores to the calling team (usually the
	// app_server).  This is required because apps can't aquire kernel
	// semaphores.

	thread_id threadID = find_thread(NULL);
	thread_info threadInfo;
	status_t status = get_thread_info(threadID, &threadInfo);
	if (status == B_OK)
		status = set_sem_owner(si.vertBlankSem, threadInfo.team);

	// If there is a valid interrupt assigned, set up interrupts.

	if (status == B_OK && di.pciInfo.u.h0.interrupt_pin != 0x00
		&& di.pciInfo.u.h0.interrupt_line != 0xff) {
		// We have a interrupt line to use.

		status = install_io_interrupt_handler(di.pciInfo.u.h0.interrupt_line,
			InterruptHandler, (void*)(&di), 0);

		if (status == B_OK)
			si.bInterruptAssigned = true;	// we can use interrupt related functions
	}

	if (status != B_OK) {
		// Interrupt does not exist; thus delete semaphore as it won't be used.
		delete_sem(si.vertBlankSem);
		si.vertBlankSem = -1;
	}
}


static status_t
InitDevice(DeviceInfo& di)
{
	// Perform initialization and mapping of the device, and return B_OK if
	// sucessful;  else, return error code.

	// Get the table of VESA modes that the chip supports.  Note that we will
	// need this table only for chips that are currently connected to a laptop
	// display or a monitor connected via a DVI interface.

	size_t vesaModeTableSize = 0;
	VesaMode* vesaModes = (VesaMode*)get_boot_item(VESA_MODES_BOOT_INFO,
		&vesaModeTableSize);

	size_t sharedSize = (sizeof(SharedInfo) + 7) & ~7;

	// Create the area for shared info with NO user-space read or write
	// permissions, to prevent accidental damage.

	di.sharedArea = create_area("ATI shared info",
		(void**) &(di.sharedInfo),
		B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize + vesaModeTableSize),
		B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (di.sharedArea < 0)
		return di.sharedArea;	// return error code

	SharedInfo& si = *(di.sharedInfo);

	memset(&si, 0, sharedSize);

	if (vesaModes != NULL) {
		si.vesaModeTableOffset = sharedSize;
		si.vesaModeCount = vesaModeTableSize / sizeof(VesaMode);

		memcpy((uint8*)&si + si.vesaModeTableOffset, vesaModes,
			vesaModeTableSize);
	}

	pci_info& pciInfo = di.pciInfo;

	si.vendorID = pciInfo.vendor_id;
	si.deviceID = pciInfo.device_id;
	si.revision = pciInfo.revision;
	si.chipType = di.pChipInfo->chipType;
	strcpy(si.chipName, di.pChipInfo->chipName);

	TRACE("Chip revision: 0x%x\n", si.revision);

	// 264GT has two chip versions.  If version is non-zero, chip is 264GTB.

	if (si.chipType == MACH64_264GT && si.revision & 0x7)
		si.chipType = MACH64_264GTB;

	// 264VT has two chip versions.  If version is non-zero, chip is 264VTB.

	if (si.chipType == MACH64_264VT && si.revision & 0x7)
		si.chipType = MACH64_264VTB;

	status_t status = MapDevice(di);

	// If device mapped without any error, get the bios parameters from the
	// chip's BIOS ROM.

	if (status >= 0) {
		if (MACH64_FAMILY(si.chipType)) {
			uint8 clockType;
			Mach64_GetBiosParameters(di, clockType);

			// All chips supported by this driver should have an internal clock.
			// If the clock is not an internal clock, the video chip is not
			// supported.

			if (clockType != M64_CLOCK_INTERNAL) {
				TRACE("Video chip clock type %d not supported\n", clockType);
				status = B_UNSUPPORTED;
			}
		}
		else if (RAGE128_FAMILY(si.chipType))
			Rage128_GetBiosParameters(di);
	}

	if (status < 0) {
		delete_area(di.sharedArea);
		di.sharedArea = -1;
		di.sharedInfo = NULL;
		return status;		// return error code
	}

	InitInterruptHandler(di);

	TRACE("Interrupt assigned:  %s\n", si.bInterruptAssigned ? "yes" : "no");
	return B_OK;
}


static const ChipInfo*
GetNextSupportedDevice(uint32& pciIndex, pci_info& pciInfo)
{
	// Search the PCI devices for a device that is supported by this driver.
	// The search starts at the device specified by argument pciIndex, and
	// continues until a supported device is found or there are no more devices
	// to examine.  Argument pciIndex is incremented after each device is
	// examined.

	// If a supported device is found, return a pointer to the struct containing
	// the chip info; else return NULL.

	while (gPCI->get_nth_pci_info(pciIndex, &pciInfo) == B_OK) {

		if (pciInfo.vendor_id == VENDOR_ID) {

			// Search the table of supported devices to find a chip/device that
			// matches device ID of the current PCI device.

			const ChipInfo* pDevice = chipTable;

			while (pDevice->chipID != 0) {	// end of table?
				if (pDevice->chipID == pciInfo.device_id) {
					// Matching device/chip found.  If chip is 264VT, reject it
					// if its version is zero since the mode can not be set on
					// that chip.

					if (pDevice->chipType == MACH64_264VT
							&& (pciInfo.revision & 0x7) == 0)
						break;

					return pDevice;		// matching device/chip found
				}

				pDevice++;
			}
		}

		pciIndex++;
	}

	return NULL;		// no supported device found
}



//	#pragma mark - Kernel Interface


status_t
init_hardware(void)
{
	// Return B_OK if a device supported by this driver is found; otherwise,
	// return B_ERROR so the driver will be unloaded.

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;		// unable to access PCI bus

	// Check pci devices for a device supported by this driver.

	uint32 pciIndex = 0;
	pci_info pciInfo;
	const ChipInfo* pDevice = GetNextSupportedDevice(pciIndex, pciInfo);

	TRACE("init_hardware() - %s\n",
		pDevice == NULL ? "no supported devices" : "device supported");

	put_module(B_PCI_MODULE_NAME);		// put away the module manager

	return (pDevice == NULL ? B_ERROR : B_OK);
}


status_t
init_driver(void)
{
	// Get handle for the pci bus.

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI) != B_OK)
		return B_ERROR;

	status_t status = gLock.Init("ATI driver lock");
	if (status < B_OK)
		return status;

	// Get info about all the devices supported by this driver.

	uint32 pciIndex = 0;
	uint32 count = 0;

	while (count < MAX_DEVICES) {
		DeviceInfo& di = gDeviceInfo[count];

		const ChipInfo* pDevice = GetNextSupportedDevice(pciIndex, di.pciInfo);
		if (pDevice == NULL)
			break;			// all supported devices have been obtained

		// Compose device name.
		sprintf(di.name, "graphics/" DEVICE_FORMAT,
				  di.pciInfo.vendor_id, di.pciInfo.device_id,
				  di.pciInfo.bus, di.pciInfo.device, di.pciInfo.function);
		TRACE("init_driver() match found; name: %s\n", di.name);

		gDeviceNames[count] = di.name;
		di.openCount = 0;		// mark driver as available for R/W open
		di.sharedArea = -1;		// indicate shared area not yet created
		di.sharedInfo = NULL;
		di.pChipInfo = pDevice;
		count++;
		pciIndex++;
	}

	gDeviceNames[count] = NULL;	// terminate list with null pointer

	TRACE("init_driver() %ld supported devices\n", count);

	return B_OK;
}


void
uninit_driver(void)
{
	// Free the driver data.

	gLock.Delete();
	put_module(B_PCI_MODULE_NAME);	// put the pci module away
}


const char**
publish_devices(void)
{
	return (const char**)gDeviceNames;	// return list of supported devices
}


device_hooks*
find_device(const char* name)
{
	int i = 0;
	while (gDeviceNames[i] != NULL) {
		if (strcmp(name, gDeviceNames[i]) == 0)
			return &gDeviceHooks;
		i++;
	}

	return NULL;
}



//	#pragma mark - Device Hooks


static status_t
device_open(const char* name, uint32 /*flags*/, void** cookie)
{
	status_t status = B_OK;

	TRACE("device_open() - name: %s\n", name);

	// Find the device name in the list of devices.

	int32 i = 0;
	while (gDeviceNames[i] != NULL && (strcmp(name, gDeviceNames[i]) != 0))
		i++;

	if (gDeviceNames[i] == NULL)
		return B_BAD_VALUE;		// device name not found in list of devices

	DeviceInfo& di = gDeviceInfo[i];

	gLock.Acquire();	// make sure no one else has write access to common data

	if (di.openCount == 0)
		status = InitDevice(di);

	gLock.Release();

	if (status == B_OK) {
		di.openCount++;		// mark device open
		*cookie = &di;		// send cookie to opener
	}

	TRACE("device_open() returning 0x%lx,  open count: %ld\n", status,
		di.openCount);
	return status;
}


static status_t
device_read(void* dev, off_t pos, void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter"
	// warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_write(void* dev, off_t pos, const void* buf, size_t* len)
{
	// Following 3 lines of code are here to eliminate "unused parameter"
	// warnings.
	(void)dev;
	(void)pos;
	(void)buf;

	*len = 0;
	return B_NOT_ALLOWED;
}


static status_t
device_close(void* dev)
{
	(void)dev;		// avoid compiler warning for unused arg

	return B_NO_ERROR;
}


static status_t
device_free(void* dev)
{
	DeviceInfo& di = *((DeviceInfo*)dev);
	SharedInfo& si = *(di.sharedInfo);
	pci_info& pciInfo = di.pciInfo;

	TRACE("enter device_free()\n");

	gLock.Acquire();		// lock driver

	// If opened multiple times, merely decrement the open count and exit.

	if (di.openCount <= 1) {
		DisableVBI();		// disable & clear any pending interrupts

		if (si.bInterruptAssigned) {
			remove_io_interrupt_handler(pciInfo.u.h0.interrupt_line,
				InterruptHandler, &di);
		}

		// Delete the semaphores, ignoring any errors because the owning team
		// may have died.
		if (si.vertBlankSem >= 0)
			delete_sem(si.vertBlankSem);
		si.vertBlankSem = -1;

		UnmapDevice(di);	// free regs and frame buffer areas

		delete_area(di.sharedArea);
		di.sharedArea = -1;
		di.sharedInfo = NULL;
	}

	if (di.openCount > 0)
		di.openCount--;		// mark device available

	gLock.Release();	// unlock driver

	TRACE("exit device_free() openCount: %ld\n", di.openCount);
	return B_OK;
}


static status_t
device_ioctl(void* dev, uint32 msg, void* buffer, size_t bufferLength)
{
	DeviceInfo& di = *((DeviceInfo*)dev);

//	TRACE("device_ioctl(); ioctl: %lu, buffer: 0x%08lx, bufLen: %lu\n", msg,
//		(uint32)buffer, bufferLength);

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char*)buffer, ATI_ACCELERANT_NAME);
			return B_OK;

		case ATI_DEVICE_NAME:
			strncpy((char*)buffer, di.name, B_OS_NAME_LENGTH);
			((char*)buffer)[B_OS_NAME_LENGTH -1] = '\0';
			return B_OK;

		case ATI_GET_SHARED_DATA:
			if (bufferLength != sizeof(area_id))
				return B_BAD_DATA;

			*((area_id*)buffer) = di.sharedArea;
			return B_OK;

		case ATI_GET_EDID:
		{
			if (bufferLength != sizeof(edid1_raw))
				return B_BAD_DATA;

			edid1_raw rawEdid;
			status_t status = GetEdidFromBIOS(rawEdid);
			if (status == B_OK)
				user_memcpy((edid1_raw*)buffer, &rawEdid, sizeof(rawEdid));
			return status;
		}

		case ATI_SET_VESA_DISPLAY_MODE:
			if (bufferLength != sizeof(uint16))
				return B_BAD_DATA;

			return SetVesaDisplayMode(*((uint16*)buffer));

		case ATI_RUN_INTERRUPTS:
			if (bufferLength != sizeof(bool))
				return B_BAD_DATA;

			if (*((bool*)buffer))
				EnableVBI();
			else
				DisableVBI();

			return B_OK;
	}

	return B_DEV_INVALID_IOCTL;
}
