/*
	Copyright (c) 2002, Thomas Kurschel
	

	Part of Radeon kernel driver
		
	Common header file
*/

#ifndef _RADEON_DRIVER_H
#define _RADEON_DRIVER_H

#include <radeon_interface.h>
#include "memmgr.h"

#include <KernelExport.h>
#include <GraphicsDefs.h>


// logging helpers
extern int debug_level_flow;
extern int debug_level_info;
extern int debug_level_error;

/*#define DEBUG_WAIT_ON_MSG 1000000
#define DEBUG_WAIT_ON_ERROR 1000000*/

#define DEBUG_MSG_PREFIX "Radeon - "
#include <debug_ext.h>

#ifdef ENABLE_LOGGING
#include "log_coll.h"
#endif


#define MAX_DEVICES	8


// DMA buffer
typedef struct DMA_buffer {
	area_id buffer_area;
	size_t size;
	void *ptr;
	
	area_id GART_area;
	uint32 *GART_ptr;
	uint32 GART_phys;
	area_id unaligned_area;
} DMA_buffer;

// info about graphics RAM
typedef struct {
	int ml;
	int MB;
	int Trcd;
	int Trp;
	int Twr;
	int CL;
	int Tr2w;
	int loop_latency;
	int Rloop;
} ram_info;

typedef struct {
	area_id		bios_area;
	
	uint8		*bios_ptr;		// begin of entire BIOS
	uint8		*rom_ptr;		// begin of ROM containing hw info
} rom_info;


// timer for VBI emulation
typedef struct {
	timer		te;				/* timer entry for add_timer() */
	struct device_info	*di;	/* pointer to the owning device */
	bigtime_t	when_target;	/* when we're supposed to wake up */
} timer_info;


// info about one device
typedef struct device_info {
	uint32		is_open;
	area_id		shared_area;
	shared_info	*si;
	
	area_id		virtual_card_area;
	virtual_card *vc;
	vuint8		*regs;

	bool 		has_crtc2;
	radeon_type	asic;
	display_type_e disp_type[2];
	fp_info		fp_info;
	
	pll_info	pll;
	ram_info	ram;	
	char		ram_type[32];	// human-readable name of ram type
	uint32		local_mem_size;

	rom_info	rom;		
	
	DMA_buffer	DMABuffer;
	
	mem_info	*local_memmgr;
	
	// VBI data
	uint32      interrupt_count;
	uint32		vbi_count[2];
	// VBI emulation
	int32		shutdown_virtual_irq;	// true, to shutdown virtual interrupts
	timer_info  ti_a;           /* a pool of two timer managment buffers */
    timer_info  ti_b;
    timer_info  *current_timer; /* the timer buffer that's currently in use */
    
	uint32		dac2_cntl;		// original dac2_cntl register content

	pci_info	pcii;
	char		name[MAX_RADEON_DEVICE_NAME_LENGTH];	
} device_info;


// device list and some global data
typedef struct {
	uint32		count;
	benaphore	kernel;
	char		*device_names[MAX_DEVICES+1];
	device_info	di[MAX_DEVICES];
} radeon_devices;


extern pci_module_info *pci_bus;
extern radeon_devices *devices;


// detect.c
bool Radeon_CardDetect( void );
void Radeon_ProbeDevices( void );


// init.c
status_t Radeon_FirstOpen( device_info *di );
void Radeon_LastClose( device_info *di );
status_t Radeon_MapDevice( device_info *di, bool mmio_only );
void Radeon_UnmapDevice(device_info *di);


// bios.c
status_t Radeon_MapBIOS( pci_info *pcii, rom_info *ri );
void Radeon_UnmapBIOS( rom_info *ri );
status_t Radeon_ReadBIOSData( device_info *di );
	

// dma.c
void Radeon_CleanupDMA( device_info *di );
status_t Radeon_InitDMA( device_info *di );


// irq.c
status_t Radeon_SetupIRQ( device_info *di, char *buffer );
void Radeon_CleanupIRQ( device_info *di );


// agp.c
void Radeon_Fix_AGP();

#endif
