/*
 * Copyright (c) 2002-2004, Thomas Kurschel
 * Distributed under the terms of the MIT license.
 */
#ifndef _RADEON_DRIVER_H
#define _RADEON_DRIVER_H


#include "radeon_interface.h"
#include "memory_manager.h"

#include <KernelExport.h>
#include <GraphicsDefs.h>
#include "AGP.h"

// logging helpers
extern int debug_level_flow;
extern int debug_level_info;
extern int debug_level_error;

/*#define DEBUG_WAIT_ON_MSG 1000000
#define DEBUG_WAIT_ON_ERROR 1000000*/

#define DEBUG_MSG_PREFIX "Radeon - "
#include "debug_ext.h"

#include "log_coll.h"


#define MAX_DEVICES	8

// size of PCI GART;
// the only user is the command processor, which needs 1 MB,
// so make sure that GART is large enough
#define PCI_GART_SIZE 1024*1024

// size of CP ring buffer in dwords
#define CP_RING_SIZE 2048


// GART info (either PCI or AGP)
typedef struct {
	// data accessed via GART
	struct {
		area_id area;			// area of data
		size_t size;			// size of buffer
		void *ptr;				// CPU pointer to data
		area_id unaligned_area;	// unaligned address (see PCI_GART.c)
	} buffer;

	// GATT info (address translation table)
	struct {
		area_id area;			// area containing GATT
		uint32 *ptr;			// CPU pointer to GATT
		uint32 phys;			// physical address of GATT
	} GATT;
} GART_info;

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
	int width;
} ram_info;

// ROM information
typedef struct {
	area_id		bios_area;		// only mapped during detection
	
	uint32		phys_address;	// physical address of BIOS
	uint32		size;			// size in bytes
	
	// the following is only useful if ROM is mapped during detection,
	// but the difference if the offset of hw info
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

	radeon_type	asic;
	uint8 		num_crtc;
	tv_chip_type tv_chip;
	bool		is_mobility;
	bool		new_pll;
	bool		has_vip;
	bool		is_igp;
	bool		has_no_i2c;
	bool		acc_dma;

	fp_info		fp_info;
	disp_entity	routing;
	
	general_pll_info pll;
	tmds_pll_info 	 tmds_pll[4];
	ram_info	ram;	
	char		ram_type[32];	// human-readable name of ram type
	uint32		local_mem_size;

	rom_info	rom;
	bool 		is_atombios;	// legacy ROM, or "AtomBios"
	
	GART_info	pci_gart;		// PCI GART
	GART_info	agp_gart;		// AGP GART (unsupported)
	memory_type_e	nonlocal_map;	// default type of non-local memory;
	
	mem_info	*memmgr[mt_last+1];	// memory managers;
								// if there is no AGP; the entries for non_local
								// and PCI are the same
	
	// VBI data
	uint32      interrupt_count;
	uint32		vbi_count[2];
	// VBI emulation
	int32		shutdown_virtual_irq;	// true, to shutdown virtual interrupts
	timer_info  ti_a;           /* a pool of two timer managment buffers */
    timer_info  ti_b;
    timer_info  *current_timer; /* the timer buffer that's currently in use */
    
    // DMA GUI engine
    sem_id		dma_sem;
    uint32		dma_desc_max_num;
    uint32		dma_desc_handle;
    uint32		dma_desc_offset;
    
    // capture engine
    spinlock	cap_spinlock;	// synchronization for following capture data
   	sem_id		cap_sem;		// semaphore released on capture interrupt
	uint32		cap_int_status;	// content of CAP_INT_STATUS during lost capture irq
	uint32		cap_counter;	// counter of capture interrupts
	bigtime_t	cap_timestamp;	// timestamp of last capture interrupt
    
	uint32		dac2_cntl;		// original dac2_cntl register content

	radeon_settings	settings;	// overrides read from radeon.settings

	pci_info	pcii;
	agp_info	agpi;
	char		name[MAX_RADEON_DEVICE_NAME_LENGTH];
	char		video_name[MAX_RADEON_DEVICE_NAME_LENGTH];
} device_info;


// device list and some global data
typedef struct {
	uint32		count;
	benaphore	kernel;
	// every device is exported as a graphics and a video card
	char		*device_names[2*MAX_DEVICES+1];
	device_info	di[MAX_DEVICES];
} radeon_devices;


extern pci_module_info *pci_bus;
extern agp_gart_module_info *sAGP;
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
	

// PCI_GART.c
status_t Radeon_InitPCIGART( device_info *di );
void Radeon_CleanupPCIGART( device_info *di );


// irq.c
status_t Radeon_SetupIRQ( device_info *di, char *buffer );
void Radeon_CleanupIRQ( device_info *di );


// agp.c
void Radeon_Set_AGP( device_info *di, bool enable_agp );


// mem_controller.c
void Radeon_InitMemController( device_info *di );


// CP_setup.c
void Radeon_WaitForIdle( device_info *di, bool acquire_lock, bool keep_lock );
void Radeon_WaitForFifo( device_info *di, int entries );
void Radeon_ResetEngine( device_info *di );
status_t Radeon_InitCP( device_info *di );
void Radeon_UninitCP( device_info *di );
void Radeon_SetDynamicClock( device_info *di, int mode );


// vip.c
bool Radeon_VIPRead( device_info *di, uint channel, uint address, uint32 *data, bool lock );
bool Radeon_VIPWrite( device_info *di, uint8 channel, uint address, uint32 data, bool lock );
bool Radeon_VIPFifoRead(device_info *di, uint8 channel, uint32 address, uint32 count, uint8 *buffer, bool lock);
bool Radeon_VIPFifoWrite(device_info *di, uint8 channel, uint32 address, uint32 count, uint8 *buffer, bool lock);
int Radeon_FindVIPDevice( device_info *di, uint32 device_id );
void Radeon_VIPReset( device_info *di, bool lock );


// dma.c
status_t Radeon_InitDMA( device_info *di );
status_t Radeon_DMACopy( 
	device_info *di, uint32 src, char *target, size_t size, 
	bool lock_mem, bool contiguous );
	
#endif
