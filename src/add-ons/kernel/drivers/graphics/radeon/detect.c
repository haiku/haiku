/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon kernel driver
		
	Graphics card detection
*/


#include "radeon_driver.h"

#include <stdio.h>


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

// RV280
#define DEVICE_ID_RADEON_Ya_	0x5960
#define DEVICE_ID_RADEON_Ya	0x5961
#define DEVICE_ID_RADEON_Yd	0x5964

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
#define DEVICE_ID_RADEON_NO		0x4e50
#define DEVICE_ID_RADEON_NS		0x4e54

// rv360
#define DEVICE_ID_RADEON_AR		0x4152

// r350
#define DEVICE_ID_RADEON_AH		0x4148
#define DEVICE_ID_RADEON_NH		0x4e48
#define DEVICE_ID_RADEON_NI		0x4e49

// r360
#define DEVICE_ID_RADEON_NJ		0x4e4a

#define DEVICE_ID_IGP320M		0x4336

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
	
	// RV200 (dual CRT)
	{ DEVICE_ID_RADEON_QW,	rt_rv200,	"Radeon 7500 / ALL-IN-WONDER Radeon 7500" },
	{ DEVICE_ID_RADEON_QX,	rt_rv200,	"Radeon 7500 QX" },
	
	// R200 mobility (based on RV200)
	{ DEVICE_ID_RADEON_LW,	rt_m7,		"Radeon Mobility 7500" },
	{ DEVICE_ID_RADEON_LX,	rt_m7,		"Radeon Mobility 7500 GL" },
	
	// R200
	{ DEVICE_ID_RADEON_QH,	rt_r200,	"Radeon 8500 QH" },
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

	// RV250 (cut-down R200)
	{ DEVICE_ID_RADEON_Id,  rt_rv250,	"Radeon 9000 Id" },
	{ DEVICE_ID_RADEON_Ie,  rt_rv250,	"Radeon 9000 Ie" },
	{ DEVICE_ID_RADEON_If,  rt_rv250,	"Radeon 9000" },
	{ DEVICE_ID_RADEON_Ig,  rt_rv250,	"Radeon 9000 Ig" },

	// M9 (based on rv250)
	{ DEVICE_ID_RADEON_Ld,	rt_m9,		"Radeon Mobility 9000 Ld" },
	{ DEVICE_ID_RADEON_Le,	rt_m9,		"Radeon Mobility 9000 Le" },
	{ DEVICE_ID_RADEON_Lf,	rt_m9,		"Radeon Mobility 9000 Lf" },
	{ DEVICE_ID_RADEON_Lg,	rt_m9,		"Radeon Mobility 9000 Lg" },
	
	// RV280
	// the naming scheme can't properly handle this id
	{ DEVICE_ID_RADEON_Ya_, rt_rv250,	"Radeon 9200" },
	{ DEVICE_ID_RADEON_Ya,	rt_rv250,	"Radeon 9200" },
	{ DEVICE_ID_RADEON_Yd,	rt_rv250,	"Radeon 9200 SE" },

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
	{ DEVICE_ID_RADEON_NO, 	rt_rv350,	"Radeon Mobility 9600 Pro Turbo" },
	{ DEVICE_ID_RADEON_NS, 	rt_rv350,	"Mobility FireGL T2" },
	
	// RV360 (probably minor revision of rv350)
	{ DEVICE_ID_RADEON_AR, 	rt_rv360,	"Radeon 9600 AR" },

	// R350
	{ DEVICE_ID_RADEON_AH,	rt_r350,	"Radeon 9800 AH" },
	{ DEVICE_ID_RADEON_NH,	rt_r350,	"Radeon 9800 Pro NH" },
	{ DEVICE_ID_RADEON_NI,	rt_r350,	"Radeon 9800 NI" },

	// R360 (probably minor revision of r350)
	{ DEVICE_ID_RADEON_NJ, 	rt_r360,	"Radeon 9800 XT" },

	{ DEVICE_ID_IGP320M, 	rt_ve,		"Radeon IGP 320M" },
	
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
static bool has_crtc2[] =
{
	false,	// only original Radeons have one crtc only
	false,
	true,
	true,	
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true,
	true
};

static const char *asic_name[] = 
{
	"r100",
	"ve",
	"m6",
	"rv200",
	"m7",
	"r200",
	"rv250",
	"rv280",
	"m9",
	"r300",
	"r300_4p",
	"rv350",
	"rv360",
	"r350",
	"r360",
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
			
			di->has_crtc2 = has_crtc2[device->asic];
			di->asic = device->asic;
			
			if( Radeon_MapBIOS( &di->pcii, &di->rom ) != B_OK )
				// give up checking this device - no BIOS, no fun
				return false;
				
			if( Radeon_ReadBIOSData( di ) != B_OK ) {
				Radeon_UnmapBIOS( &di->rom );
				return false;
			}

			// we don't need BIOS any more
			Radeon_UnmapBIOS( &di->rom );
			
			SHOW_INFO( 0, "found %s; ASIC: %s", device->name, asic_name[device->asic] );
			
			sprintf(di->name, "graphics/%04X_%04X_%02X%02X%02X",
				di->pcii.vendor_id, di->pcii.device_id,
				di->pcii.bus, di->pcii.device, di->pcii.function);
			SHOW_FLOW( 3, "making /dev/%s", di->name );
	
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
			devices->device_names[count] = di->name;
			di++;
			count++;
		}

		pci_index++;
	}
	
	devices->count = count;
	devices->device_names[count] = NULL;
	
	SHOW_INFO( 0, "%ld supported devices", count );
}
