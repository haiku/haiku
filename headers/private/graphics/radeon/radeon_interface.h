/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon accelerant
		
	Interface between kernel driver and accelerant
*/


#ifndef _RADEON_INTERFACE_H
#define _RADEON_INTERFACE_H

#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>
#include "video_overlay.h"
#include "benaphore.h"


// magic code for ioctls
#define RADEON_PRIVATE_DATA_MAGIC	'TKRA'

#define MAX_RADEON_DEVICE_NAME_LENGTH MAXPATHLEN

// list ioctls
enum {
	RADEON_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,

	RADEON_DEVICE_NAME,
	RADEON_GET_LOG_SIZE,
	RADEON_GET_LOG_DATA,
	RADEON_ALLOC_LOCAL_MEM,
	RADEON_FREE_LOCAL_MEM,
	
	RADEON_SET_I2C_SIGNALS,
	RADEON_GET_I2C_SIGNALS
};


// list of multi-monitor modes
typedef enum {
	mm_none,	// use one display only
	mm_combine,	// combine displays to larger workspace
	mm_clone,	// clone workspace, all displays show the 
				// same but have independant timing
	mm_mirror	// mirror ports (as used by Laptop) - not implemented yet
} multi_mode_e;


// type of monitor connected
typedef enum {
	dt_none,
	dt_crt_1,		// CRT on primary port (i.e. via DVI2CRT adapter)
	dt_crt_2,		// CRT on secondary port (i.e. standard CRT connector)
	dt_lvds,		// laptop flap panel
	dt_dvi_1,		// DVI on primary port (i.e. standard DVI connector)
	
	// the following connectors/devices are not supported
	dt_dvi_2,		// DVI on secondary port (only provided by few models)
	dt_ctv,			// composite TV
	dt_stv			// S-Video out
} display_type_e;


// type of ASIC
typedef enum {
	rt_r100,		// original Radeon
	rt_ve,			// original VE version
	rt_m6,			// original mobile Radeon
	rt_rv200,		// Radeon 7500
	rt_m7,			// mobile Radeon 7500
	rt_r200,		// Radeon 8500/9100
	rt_rv250,		// Radeon 9000
	rt_rv280,		// Radeon 9200
	rt_m9,			// mobile Radeon 9000
	rt_r300,		// Radeon 9700
	rt_r300_4p,		// Radeon 9500
	rt_rv350,		// Radeon 9600
	rt_rv360,		// Radeon 9600
	rt_r350,		// Radeon 9800
	rt_r360			// Radeon 9800
} radeon_type;


// info about cursor
typedef struct {
	uint8*	data;		// pointer to framebuffer containing cursor image
	uint16	hot_x;
	uint16	hot_y;
	uint16	x;
	uint16	y;
	uint16	width;
	uint16	height;
	uint32 	mem_handle;	// memory handle
	uint32	fb_offset;	// offset in frame buffer
	bool	is_visible;	// official flag whether cursor is visible
} cursor_info;


// list of register content (used for mode changes)
typedef struct {
	// CRTC regs
	uint32		crtc_h_total_disp;
	uint32		crtc_h_sync_strt_wid;
	uint32		crtc_v_total_disp;
	uint32		crtc_v_sync_strt_wid;
	uint32		crtc_pitch;
	uint32		crtc_gen_cntl;
	uint32		crtc_ext_cntl;
	uint32		crtc_offset_cntl;
	
	// Flat panel regs
	// many of them aren't touched by us, so they aren't stored here
    uint32		fp_gen_cntl;
    uint32		fp_horz_stretch;
    uint32		fp_panel_cntl;
    uint32		fp_vert_stretch;
    uint32		lvds_gen_cntl;

	// DAC regs
	uint32		dac_cntl;
	//uint32		dac2_cntl;
	
	// PLL regs
	uint32 		ppll_div_3;
	uint32		ppll_ref_div;
	uint32		htotal_cntl;
	
	uint32		dot_clock_freq;	// in 10 kHz
    uint32		pll_output_freq;// in 10 kHz
    int			feedback_div;
    int			post_div;
    
   	// Common regs
	uint32		surface_cntl;
	uint32		disp_output_cntl;
} port_regs;


// port as seen by accelerant
typedef struct {
   	bool		is_crtc2;
   	int			physical_port;	// idx of physical port
        
	uint32		rel_x, rel_y;	// relative position in multi-monitor mode
	bool		cursor_on_screen;	// cursor is visible on this port
	
	display_mode mode;			// display mode of this port
} virtual_port;


// physical port
typedef struct {
	display_type_e disp_type;
	sem_id		vblank;			// vertical blank interrupt semaphore
} physical_port;


// info about flat panel connected to port
typedef struct {
	uint panel_pwr_delay;
	uint panel_xres, panel_yres;
	
	uint h_blank, h_over_plus, h_sync_width;
	uint v_blank, v_over_plus, v_sync_width;
	uint dot_clock;				// in kHz (this is BeOS like, ATI uses 10 kHz steps!)
	
	display_type_e disp_type;
	
	uint64 h_ratio;				// current stretch ratio, needed for overlays
	uint64 v_ratio;				// (mode_res/native_res; 16.16)
} fp_info;


// info about PLL on graphics card
typedef struct {
	uint32 max_pll_freq;
	uint32 min_pll_freq;
	uint32 xclk;
	uint32 ref_div;
	uint32 ref_freq;
} pll_info;


typedef struct overlay_buffer_node {
	struct overlay_buffer_node *next, *prev;
	uint32 mem_handle;
	uint32 mem_offset;
	uint ati_space;				// colour space according to ATI
	uint test_reg;				// content of test reg
	overlay_buffer buffer;
} overlay_buffer_node;

typedef struct {
	overlay_token	ot;
	overlay_buffer  ob;
	overlay_window	ow;
	overlay_view	ov;
	uint16			h_display_start;
	uint16			v_display_start;
	
	overlay_buffer_node *on;
	int8			port;		// physical port where the overlay is shown on
	uint32			rel_offset;	// offset of overlay source due to clipping
} overlay_info;

// each accelerator gets one "virtual card", i.e. you
// can have independant accelerators for each port
// (this is an ongoing project ;)
typedef struct {
	uint32		id;				// identifier used to know which card the 2D accelerator
								// is prepared for (we use area_id of this structure)
	virtual_port	ports[2];
	uint8		num_ports;
	
	int8		independant_ports;	// number of ports to be programmed independantly
	int8		different_ports;	// number of ports showing different parts of framebuffer
	bool		scroll;			// scrolling in virtual area enabled

	uint32		datatype;		// Radeon code for pixel format
	uint		bpp;			// bytes per pixel
	uint32		pitch;			// byte offset between two lines

	uint32		eff_width, eff_height;	// size of visible area (including both monitors)
	uint32		fb_mem_handle;	// memory handle
	uint32		fb_offset;		// offset of frame buffer in graphics mem

	cursor_info	cursor;

	multi_mode_e	wanted_multi_mode;	// multi monitor mode as requested by user

	bool		swapDisplays;	// true to swap monitors

	frame_buffer_config fbc;	// data for direct frame buffer access

	display_mode mode;			// offical mode with multi-monitor bits set	
	overlay_buffer_node	*overlay_buffers;	// list of allocated overlay buffers
	
	//int8		whished_overlay_port;	// port where users whishes the overlay to be
	bool		uses_overlay;	// true if this virtual card owns overlay
} virtual_card;

typedef struct {
	vint32	inuse;				// one, if someone allocated overlay port
								// (this doesn't necessarily mean that an overlay is shown)
	uint32	token;				// arbitrarily chosen token to identify overlay owner
								// (increased by 1 whenever there is a new owner)
	uint32	auto_flip_reg;		// content of auto_flip_reg
} overlay_mgr_info;

// data published by kernel and shared by all accelerant/virtual cards
typedef struct {
	// filled out by kernel
	uint16		vendor_id;	// PCI vendor id
	uint16		device_id;	// PCI device id
	uint8		revision;	// PCI device revision
	
	bool 		has_crtc2;	// has second CRTC
	radeon_type	asic;
		
	pll_info	pll;
	
	area_id		regs_area;	// area of memory mapped registers
	area_id		fb_area;	// area of frame buffer
	uint8		*framebuffer;	// pointer to frame buffer (visible by all apps!)
	void		*framebuffer_pci;	// physical address of frame buffer

	physical_port ports[2];
	fp_info		fp_port;

	uint32	local_mem_size;		// size of graphics memory
	
	uint32	AGP_vm_start;		// logical address (for graphics card) of AGP memory
	uint32	AGP_vm_size;		// size of AGP address range
	
	uint32	nonlocal_vm_start;	// logical address (for graphics card) of nonlocal memory
								// always use this one, as it is valid in PCI mode too
	uint32	*nonlocal_mem;		// logical address (for CPU) of nonlocal memory
	uint32	nonlocal_mem_size;	// size of nonlocal memory

		
	// set by accelerator
	struct {
		uint64		count;		// count of submitted CP commands
		uint64		last_idle;	// count when engine was idle last time
		uint64		written;	// last count passed to CP
		benaphore	lock;		// engine lock
	} engine;
	
	struct {					// DMA ring buffer of CP
		uint32 start_offset;	// offset in DMA buffer
		uint32 tail, tail_mask;	// next write position in dwords; mask for wrap-arounds
		uint32 size;			// size in dwords
		uint32 head_offset;		// offset for automatically updates head in DMA buffer

		uint32 *start;			// pointer to ring buffer
		vuint32 *head;			// pointer to automatically updates read position
	} ring;
	
	vuint32 *scratch_ptr;		// pointer to scratch registers (in DMA buffer)
		
	area_id	mode_list_area;		// area containing display mode list
	uint	mode_count;
	
	uint32	active_vc;	// currently selected virtual card
	
	uint32	dac_cntl2;			// content of dac_cntl2 register

	overlay_info	pending_overlay;
	overlay_info	active_overlay;	
	overlay_mgr_info overlay_mgr;
	
	// data needed for VBI emulation 
	// (currently not fully implemented - if the user disabled graphics card
	//  IRQ in the BIOS, it's his fault)
	int		refresh_period;		// duration of one frame in ms
	int		blank_period;		// vertical blank period of a frame in ms
	int		enable_virtual_irq;	// true, to enable virtual interrupts
	
	struct log_info_t *log;		// fast logger data
} shared_info;


// retrieve the area_id of the kernel/accelerant shared info
typedef struct {
	uint32	magic;				// magic number
	area_id	shared_info_area;
	area_id	virtual_card_area;
} radeon_get_private_data;

// get devie name (used to clone accelerant)
typedef struct {
	uint32	magic;				// magic number
	char	*name;				// pointer to buffer containing name (in)
} radeon_device_name;

// alloc local memory
typedef struct {
	uint32	magic;
	uint32	size;
	uint32	fb_offset;
	uint32	handle;
} radeon_alloc_local_mem;

// free local memory
typedef struct {
	uint32 	magic;
	uint32	handle;
} radeon_free_local_mem;

// get/set i2c signals
typedef struct {
	uint32 magic;
	int port;
	int value;
} radeon_getset_i2c;

#endif
