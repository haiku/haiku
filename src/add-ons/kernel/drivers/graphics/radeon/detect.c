/*
 * Copyright (c) 2002, Thomas Kurschel
 * Distributed under the terms of the MIT license.
 */

/**	Graphics card detection */


#include "radeon_driver.h"

#include <stdio.h>
#include <string.h>


// this table is gathered from different sources
// and may even contain some wrong entries
#define VENDOR_ID_ATI		0x1002

// R100
#define DEVICE_ID_RADEON_QD		0x5144
#define DEVICE_ID_RADEON_QE		0x5145
#define DEVICE_ID_RADEON_QF		0x5146
#define DEVICE_ID_RADEON_QG		0x5147

// RV100
#define DEVICE_ID_RADEON_QY		0x5159
#define DEVICE_ID_RADEON_QZ		0x515a

// M6
#define DEVICE_ID_RADEON_LY		0x4c59
#define DEVICE_ID_RADEON_LZ		0x4c5a

// RV200
#define DEVICE_ID_RADEON_QW		0x5157
#define DEVICE_ID_RADEON_QX		0x5158

// R200 mobility
#define DEVICE_ID_RADEON_LW		0x4c57
#define DEVICE_ID_RADEON_LX		0x4c58

// R200
#define DEVICE_ID_RADEON_QH		0x5148
#define DEVICE_ID_RADEON_QI		0x5149
#define DEVICE_ID_RADEON_QJ		0x514a
#define DEVICE_ID_RADEON_QK		0x514b
#define DEVICE_ID_RADEON_QL		0x514c
#define DEVICE_ID_RADEON_QM		0x514d

#define DEVICE_ID_RADEON_Qh		0x5168
#define DEVICE_ID_RADEON_Qi		0x5169
#define DEVICE_ID_RADEON_Qj		0x516a
#define DEVICE_ID_RADEON_Qk		0x516b

#define DEVICE_ID_RADEON_BB		0x4242

// RV250
#define DEVICE_ID_RADEON_Id     0x4964
#define DEVICE_ID_RADEON_Ie     0x4965
#define DEVICE_ID_RADEON_If     0x4966
#define DEVICE_ID_RADEON_Ig     0x4967

// M9
#define DEVICE_ID_RADEON_Ld     0x4c64
#define DEVICE_ID_RADEON_Le     0x4c65
#define DEVICE_ID_RADEON_Lf     0x4c66
#define DEVICE_ID_RADEON_Lg     0x4c67

// M9+
#define DEVICE_ID_RADEON_5c61   0x5c61

// RV280
#define DEVICE_ID_RADEON_Zprea	0x5960
#define DEVICE_ID_RADEON_Za		0x5961
#define DEVICE_ID_RADEON_Zd		0x5964

// r300
#define DEVICE_ID_RADEON_ND     0x4e44
#define DEVICE_ID_RADEON_NE     0x4e45
#define DEVICE_ID_RADEON_NF     0x4e46
#define DEVICE_ID_RADEON_NG     0x4e47

// r300-4P
#define DEVICE_ID_RADEON_AD     0x4144
#define DEVICE_ID_RADEON_AE     0x4145
#define DEVICE_ID_RADEON_AF     0x4146
#define DEVICE_ID_RADEON_AG     0x4147

// rv350
#define DEVICE_ID_RADEON_AP		0x4150
#define DEVICE_ID_RADEON_AQ		0x4151
#define DEVICE_ID_RADEON_AS		0x4153

// m10
#define DEVICE_ID_RADEON_NP		0x4e50
// Mobility Fire GL T2 - any idea about the chip?
#define DEVICE_ID_RADEON_NT		0x4e54

// rv360
#define DEVICE_ID_RADEON_AR		0x4152

// r350
#define DEVICE_ID_RADEON_AH		0x4148
#define DEVICE_ID_RADEON_NH		0x4e48
#define DEVICE_ID_RADEON_NI		0x4e49

// r360
#define DEVICE_ID_RADEON_NJ		0x4e4a

// rs100
#define DEVICE_ID_IGP320M		0x4336

// rs200
#define DEVICE_ID_RADEON_C7		0x4337
#define DEVICE_ID_RADEON_A7		0x4137

typedef struct {
	uint16 device_id;
	radeon_type asic;
	char *name;
} RadeonDevice;

// list of supported devices
RadeonDevice radeon_device_list[] = {
	// original Radeons, now called r100
	{ DEVICE_ID_RADEON_QD,	rt_r100,	"Radeon 7200 / Radeon / ALL-IN-WONDER Radeon" },
	{ DEVICE_ID_RADEON_QE,	rt_r100,	"Radeon QE" },
	{ DEVICE_ID_RADEON_QF,	rt_r100,	"Radeon QF" },
	{ DEVICE_ID_RADEON_QG,	rt_r100,	"Radeon QG" },

	// Radeon VE (low-cost, dual CRT, no TCL), now called RV100
	{ DEVICE_ID_RADEON_QY,	rt_ve,		"Radeon 7000 / Radeon VE" },
	{ DEVICE_ID_RADEON_QZ,	rt_ve, 		"Radeon QZ VE" },
	
	// mobility version of original Radeon (based on VE), now called M6
	{ DEVICE_ID_RADEON_LY,	rt_m6,		"Radeon Mobility" },
	{ DEVICE_ID_RADEON_LZ,	rt_m6, 		"Radeon Mobility M6 LZ" },
	
	// rs100 (integrated Radeon, seems to be a Radeon VE)
	{ DEVICE_ID_IGP320M,	rt_rs100,	"IGP320M" },
	
	// RV200 (dual CRT)
	{ DEVICE_ID_RADEON_QW,	rt_rv200,	"Radeon 7500 / ALL-IN-WONDER Radeon 7500" },
	{ DEVICE_ID_RADEON_QX,	rt_rv200,	"Radeon 7500 QX" },
	
	// M7 (based on RV200)
	{ DEVICE_ID_RADEON_LW,	rt_m7,		"Radeon Mobility 7500" },
	{ DEVICE_ID_RADEON_LX,	rt_m7,		"Radeon Mobility 7500 GL" },
	
	// R200
	{ DEVICE_ID_RADEON_QH,	rt_r200,	"ATI Fire GL E1" },	// chip type: fgl8800
	{ DEVICE_ID_RADEON_QI,	rt_r200,	"Radeon 8500 QI" },
	{ DEVICE_ID_RADEON_QJ,	rt_r200,	"Radeon 8500 QJ" },
	{ DEVICE_ID_RADEON_QK,	rt_r200,	"Radeon 8500 QK" },
	{ DEVICE_ID_RADEON_QL,	rt_r200,	"Radeon 8500 / 8500LE / ALL-IN-WONDER Radeon 8500" },
	{ DEVICE_ID_RADEON_QM,	rt_r200,	"Radeon 9100" },

	{ DEVICE_ID_RADEON_Qh,	rt_r200,	"Radeon 8500 Qh" },
	{ DEVICE_ID_RADEON_Qi,	rt_r200,	"Radeon 8500 Qi" },
	{ DEVICE_ID_RADEON_Qj,	rt_r200,	"Radeon 8500 Qj" },
	{ DEVICE_ID_RADEON_Qk,	rt_r200,	"Radeon 8500 Qk" },
	{ DEVICE_ID_RADEON_BB,	rt_r200,	"ALL-IN-Wonder Radeon 8500 DV" },

	// RV250 (cut-down R200 with integrated TV-Out)
	{ DEVICE_ID_RADEON_Id,  rt_rv250,	"Radeon 9000 Id" },
	{ DEVICE_ID_RADEON_Ie,  rt_rv250,	"Radeon 9000 Ie" },
	{ DEVICE_ID_RADEON_If,  rt_rv250,	"Radeon 9000" },
	{ DEVICE_ID_RADEON_Ig,  rt_rv250,	"Radeon 9000 Ig" },

	// M9 (based on rv250)
	{ DEVICE_ID_RADEON_Ld,	rt_m9,		"Radeon Mobility 9000 Ld" },
	{ DEVICE_ID_RADEON_Le,	rt_m9,		"Radeon Mobility 9000 Le" },
	{ DEVICE_ID_RADEON_Lf,	rt_m9,		"Radeon Mobility 9000 Lf" },
	{ DEVICE_ID_RADEON_Lg,	rt_m9,		"Radeon Mobility 9000 Lg" },
	
	// RV280 (rv250 with higher frequency)
	// this entry violates naming scheme, so it's probably wrong
	{ DEVICE_ID_RADEON_Zprea, rt_rv280,	"Radeon 9200" },
	{ DEVICE_ID_RADEON_Za,	rt_rv280,	"Radeon 9200" },
	{ DEVICE_ID_RADEON_Zd,	rt_rv280,	"Radeon 9200 SE" },
	
	// M9+ (based on rv280)
	{ DEVICE_ID_RADEON_5c61,rt_m9plus,	"Radeon Mobility 9200" },

	// R300
	{ DEVICE_ID_RADEON_ND,	rt_r300,	"Radeon 9700 ND" },
	{ DEVICE_ID_RADEON_NE,	rt_r300,	"Radeon 9700 NE" },
	{ DEVICE_ID_RADEON_NF,	rt_r300,	"Radeon 9600 XT" },
	{ DEVICE_ID_RADEON_NG,	rt_r300,	"Radeon 9700 NG" },

	{ DEVICE_ID_RADEON_AD,	rt_r300,	"Radeon 9700 AD" },
	{ DEVICE_ID_RADEON_AE,	rt_r300,	"Radeon 9700 AE" },
	{ DEVICE_ID_RADEON_AF,	rt_r300,	"Radeon 9700 AF" },
	{ DEVICE_ID_RADEON_AG,	rt_r300,	"Radeon 9700 AG" },

	// RV350
	{ DEVICE_ID_RADEON_AP, 	rt_rv350,	"Radeon 9600 AP" },
	{ DEVICE_ID_RADEON_AQ, 	rt_rv350,	"Radeon 9600 AQ" },
	{ DEVICE_ID_RADEON_AS, 	rt_rv350,	"Radeon 9600 AS" },
	
	// M10 (based on rv350)
	{ DEVICE_ID_RADEON_NP,	rt_m10,		"Radeon Mobility 9600 NP" },
	// not sure about that: ROM signature is "RADEON" which means r100
	{ DEVICE_ID_RADEON_NT,	rt_m10,		"Radeon Mobility FireGL T2" },

	// RV360 (probably minor revision of rv350)
	{ DEVICE_ID_RADEON_AR, 	rt_rv360,	"Radeon 9600 AR" },

	// R350
	{ DEVICE_ID_RADEON_AH,	rt_r350,	"Radeon 9800 AH" },
	{ DEVICE_ID_RADEON_NH,	rt_r350,	"Radeon 9800 Pro NH" },
	{ DEVICE_ID_RADEON_NI,	rt_r350,	"Radeon 9800 NI" },

	// R360 (probably minor revision of r350)
	{ DEVICE_ID_RADEON_NJ, 	rt_r360,	"Radeon 9800 XT" },

	// rs100 (aka IGP 320)
	{ DEVICE_ID_IGP320M, 	rt_rs100,	"Radeon IGP 320M" },
	
	// rs200 (aka IGP 340)
	{ DEVICE_ID_RADEON_C7,	rt_rs200,	"IGP330M/340M/350M (U2) 4337" },
	{ DEVICE_ID_RADEON_A7,	rt_rs200,	"IGP 340" },
	
	{ 0, 0, NULL }
};


// list of supported vendors (there aren't many ;)
static struct {
	uint16	vendor_id;
	RadeonDevice *devices;
} SupportedVendors[] = {
	{ VENDOR_ID_ATI, radeon_device_list },
	{ 0x0000, NULL }
};

// check, whether there is *any* supported card plugged in
bool Radeon_CardDetect( void )
{
	long		pci_index = 0;
	pci_info	pcii;
	bool		found_one = FALSE;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci_bus) != B_OK)
		return B_ERROR;

	while ((*pci_bus->get_nth_pci_info)(pci_index, &pcii) == B_NO_ERROR) {
		int vendor = 0;
		
		while (SupportedVendors[vendor].vendor_id) {
			if (SupportedVendors[vendor].vendor_id == pcii.vendor_id) {
				RadeonDevice *devices = SupportedVendors[vendor].devices;
				
				while (devices->device_id) {
					if (devices->device_id == pcii.device_id ) {
						rom_info ri;
						
						if( Radeon_MapBIOS( &pcii, &ri ) == B_OK ) {
							Radeon_UnmapBIOS( &ri );

							SHOW_INFO( 0, "found supported device pci index %ld, device 0x%04x/0x%04x", 
								pci_index, pcii.vendor_id, pcii.device_id );
							found_one = TRUE;
							goto done;
						}
					}
					devices++;
				}
			}
			vendor++;
		}

		pci_index++;
	}
	SHOW_INFO0( 0, "no supported devices found" );

done:
	put_module(B_PCI_MODULE_NAME);
	
	return (found_one ? B_OK : B_ERROR);
}

// !extend this array whenever a new ASIC is a added!
static struct {
	const char 		*name;			// name of ASIC
	tv_chip_type	tv_chip;		// TV-Out chip (if any)
	bool 			has_crtc2;		// has second CRTC
	bool			is_mobility;	// mobility chip
	bool			has_vip;		// has VIP/I2C
	bool			new_pll;		// reference divider of PPLL moved to other location
	bool			is_igp;			// integrated graphics
} asic_properties[] =
{
	{ "r100",	tc_external_rt1,	false,	false,	true,	false,	false },	// only original Radeons have one crtc only
	{ "ve",		tc_internal_rt1, 	true,	false,	true,	false,	false },
	{ "m6",		tc_internal_rt1, 	true,	true,	false,	false,	false },
	{ "rs100",	tc_internal_rt1,	true,	true,	false,	false,	true },
	{ "rv200",	tc_internal_rt2, 	true,	false,	true,	false,	false },
	{ "m7",		tc_internal_rt1, 	true,	true,	false,	false,	false },
	{ "rs200",	tc_internal_rt1, 	true,	true,	false,	false,	true },
	{ "r200",	tc_external_rt1, 	true,	false,	true,	false,	false },	// r200 has external TV-Out encoder
	{ "rv250",	tc_internal_rt2, 	true,	false,	true,	false,	false },
	{ "m9",		tc_internal_rt2, 	true,	true,	false,	false,	false },
	{ "rv280",	tc_internal_rt2, 	true,	false,	true,	false,	false },
	{ "m9plus",	tc_internal_rt2, 	true,	true,	false,	false,	false },
	{ "r300",	tc_internal_rt2, 	true,	false,	true,	true,	false },
	{ "r300_4p",tc_internal_rt2, 	true,	false,	true,	true,	false },
	{ "rv350",	tc_internal_rt2, 	true,	false,	true,	true,	false },
	{ "m10",	tc_internal_rt2, 	true,	true,	false,	true,	false },
	{ "rv360",	tc_internal_rt2, 	true,	false,	true,	true,	false },
	{ "r350",	tc_internal_rt2, 	true,	false,	true,	true,	false },
	{ "r360",	tc_internal_rt2, 	true,	false,	true,	true,	false }
};


// get next supported device
static bool probeDevice( device_info *di )
{
	int vendor;
	
	/* if we match a supported vendor */
	for( vendor = 0; SupportedVendors[vendor].vendor_id; ++vendor ) {
		RadeonDevice *device;

		if( SupportedVendors[vendor].vendor_id != di->pcii.vendor_id ) 
			continue;
			
		for( device = SupportedVendors[vendor].devices; device->device_id; ++device ) {
			// avoid double-detection
			if (device->device_id != di->pcii.device_id ) 
				continue;
			
			di->num_crtc = asic_properties[device->asic].has_crtc2 ? 2 : 1;
			di->tv_chip = asic_properties[device->asic].tv_chip;
			di->asic = device->asic;
			di->is_mobility = asic_properties[device->asic].is_mobility;
			di->has_vip = asic_properties[device->asic].has_vip;
			di->new_pll = asic_properties[device->asic].new_pll;
			di->is_igp = asic_properties[device->asic].is_igp;
			
			if( Radeon_MapBIOS( &di->pcii, &di->rom ) != B_OK )
				// give up checking this device - no BIOS, no fun
				return false;
				
			if( Radeon_ReadBIOSData( di ) != B_OK ) {
				Radeon_UnmapBIOS( &di->rom );
				return false;
			}

			// we don't need BIOS any more
			Radeon_UnmapBIOS( &di->rom );
			
			SHOW_INFO( 0, "found %s; ASIC: %s", device->name, asic_properties[device->asic].name );
			
			sprintf(di->name, "graphics/%04X_%04X_%02X%02X%02X",
				di->pcii.vendor_id, di->pcii.device_id,
				di->pcii.bus, di->pcii.device, di->pcii.function);
			SHOW_FLOW( 3, "making /dev/%s", di->name );

			// we always publish it as a video grabber; we should check for Rage
			// Theater, but the corresponding code (vip.c) needs a fully initialized
			// driver, and this is too much hazzly, so we leave it to the media add-on
			// to verify that the card really supports video-in
			sprintf(di->video_name, "video/radeon/%04X_%04X_%02X%02X%02X",
				di->pcii.vendor_id, di->pcii.device_id,
				di->pcii.bus, di->pcii.device, di->pcii.function);

			di->is_open = 0;
			di->shared_area = -1;
			di->si = NULL;
	
			return true;
		}
	}
	
	return false;
}


// gather list of supported devices
// (currently, we rely on proper BIOS detection, which
//  only works for primary graphics adapter, so multiple
//  devices won't really work)
void Radeon_ProbeDevices( void ) 
{
	uint32 pci_index = 0;
	uint32 count = 0;
	device_info *di = devices->di;

	while( count < MAX_DEVICES ) {
		memset( di, 0, sizeof( *di ));
		
		if( (*pci_bus->get_nth_pci_info)(pci_index, &(di->pcii)) != B_NO_ERROR)
			break;
			
		if( probeDevice( di )) {
			devices->device_names[2*count] = di->name;
			devices->device_names[2*count+1] = di->video_name;
			di++;
			count++;
		}

		pci_index++;
	}
	
	devices->count = count;
	devices->device_names[2*count] = NULL;
	
	SHOW_INFO( 0, "%ld supported devices", count );
}
