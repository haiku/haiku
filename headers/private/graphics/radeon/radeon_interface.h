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
	RADEON_ALLOC_MEM,
	RADEON_FREE_MEM,
	
	RADEON_WAITFORIDLE,
	RADEON_RESETENGINE,
	RADEON_VIPREAD,
	RADEON_VIPWRITE,
	RADEON_FINDVIPDEVICE,

	RADEON_WAIT_FOR_CAP_IRQ,	
	RADEON_DMACOPY,
};


// number of indirect buffers
// see CP.c for this magic number
#define NUM_INDIRECT_BUFFERS 253
// size of an indirect buffer in dwords; 
// as hardware wants buffers to be 4k aligned and as we store all
// buffers in one chunk, the size per buffer in bytes must be a multiple of 4k
#define INDIRECT_BUFFER_SIZE (4096/4)


// type of memory
typedef enum {
	mt_local,		// local graphics memory
	mt_PCI,			// PCI memory (read: fully cachable)
	mt_AGP,			// AGP memory (read: not cachable; currently not supported)
	mt_nonlocal,	// non-local graphics memory (alias to one of the 
					// previously defined types, see si->nonlocal_type)
	
	mt_last = mt_nonlocal
} memory_type_e;

// list of multi-monitor modes
typedef enum {
	mm_none,	// use one display only
	mm_combine,	// combine displays to larger workspace
	mm_clone,	// clone workspace, all displays show the 
				// same but have independant timing
	mm_mirror	// mirror heads (as used by Laptop) - not implemented yet
} multi_mode_e;


// displays devices;
// this must be a bit field as multiple devices may be connected to one CRTC
typedef enum {
	dd_none = 0,
	dd_tv_crt = 1,	// CRT on TV-DAC, i.e. on DVI port
	dd_crt = 2,		// CRT on CRT-DAC, i.e. VGA port
	dd_lvds = 4,	// laptop flap panel
	dd_dvi = 8,		// DVI on primary port (i.e. standard DVI connector)
	dd_ctv = 16,	// composite TV on TV-DAC
	dd_stv = 32,	// S-Video out on TV-DAC
	
	// the following connectors/devices are not supported
	dd_dvi_ext = 64	// external DVI (only provided by few models)
} display_device_e;


// type of ASIC
typedef enum {
	rt_r100,		// original Radeon
	rt_ve,			// original VE version
	rt_m6,			// original mobile Radeon
	rt_rs100,		// IGP 320M
	rt_rv200,		// Radeon 7500
	rt_m7,			// mobile Radeon 7500
	rt_rs200,		// IGP 330M/340M/350M
	rt_r200,		// Radeon 8500/9100
	rt_rv250,		// Radeon 9000
	rt_rv280,		// Radeon 9200
	rt_m9,			// mobile Radeon 9000
	
	// from here on, r300 and up must be located as ATI modified the PLL
	// with r300 and the code only tests for >= rt_r300
	rt_r300,		// Radeon 9700
	rt_r300_4p,		// Radeon 9500
	rt_rv350,		// Radeon 9600
	rt_rv360,		// Radeon 9600
	rt_r350,		// Radeon 9800
	rt_r360			// Radeon 9800
} radeon_type;


// TV standard
typedef enum {
	ts_ntsc,
	ts_pal,
	ts_palm,
	ts_palcn,
	ts_scart_pal,
	ts_pal60
} tv_standard;


// type of TV-Chip
typedef enum {
	tc_none,
	tc_external_rt1,	// external Rage Theatre
	tc_internal_rt1,	// internal version 1
	tc_internal_rt2,	// internal version 2
} tv_chip_type;


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


// head as seen by accelerant
typedef struct {
	int			physical_head;	// idx of physical head
	uint32		rel_x, rel_y;	// relative position in multi-monitor mode
} virtual_head;


// info about flat panel connected to LVDS or DVI port
typedef struct {
	uint panel_pwr_delay;
	uint panel_xres, panel_yres;
	
	uint h_blank, h_over_plus, h_sync_width;
	uint v_blank, v_over_plus, v_sync_width;
	uint dot_clock;				// in kHz (this is BeOS like, ATI uses 10 kHz steps!)

	bool is_fp2;				// true, if second flat panel
//	display_type_e disp_type;
	
	uint64 h_ratio;				// current stretch ratio, needed for overlays
	uint64 v_ratio;				// (mode_res/native_res; 32.32)
} fp_info;


// physical head
typedef struct {
	bool		is_crtc2;			// true, if connected to crtc2
	display_device_e active_displays; // currently driven displays
	display_device_e chosen_displays;	// displays to be driven by next mode switch
	sem_id		vblank;				// vertical blank interrupt semaphore
	bool		cursor_on_screen;	// cursor is visible on this head
	display_mode mode;				// display mode of this head
	int8		flatpanel_port;		// linked flat panel port (-1 if none)
} physical_head;


// info about PLLs on graphics card as retrieved from BIOS
// all values are in 10kHz
typedef struct {
	uint32 max_pll_freq;		// maximum PLL output frequency
	uint32 min_pll_freq;		// minimum PLL output frequency
	uint32 xclk;				// core frequency
	uint32 ref_div;				// default reference divider
	uint32 ref_freq;			// PLL reference frequency
} general_pll_info;


// mapping of pll divider code to actual divider value
typedef struct {
	uint8 divider;				// divider
	uint8 code;					// code as used in register
} pll_divider_map;


// info about a PLL
// all values are in 10 kHz
typedef struct {
	pll_divider_map *post_divs;	// list of possible post dividers
	pll_divider_map *extra_post_divs; // list of possible extra post dividers
	uint32 ref_freq;			// reference frequency
	uint32 vco_min, vco_max;	// VCO frequency range
	uint32 min_ref_div, max_ref_div; // reference divider range
	uint32 pll_in_min, pll_in_max; // PLL input frequency range
	uint32 extra_feedback_div;	// hardwired divider before feedback divider
	uint32 min_feedback_div, max_feedback_div; // feedback divider range
	uint32 best_vco;			// preferred VCO frequency (0 for don't care)
} pll_info;


// PLL divider values
typedef struct {
	uint32 post_code;			// code for post divider
	uint32 post;				// value of post divider
	uint32 extra_post_code;		// code for extra post divider
	uint32 extra_post;			// value of extra post divider
	uint32 ref;					// reference divider
	uint32 feedback;			// feedback divider
	uint32 freq;				// resulting frequency
} pll_dividers;


// TV-Out parameters
typedef struct {
	uint16 y_accum_init;
	uint16 uv_accum_init;
	uint16 uv_inc;
	uint16 h_inc;
	uint32 tv_clocks_to_active;
	
	uint16 f_restart;
	uint16 v_restart;
	uint16 h_restart;
	bool mode888;
	
	uint16 y_saw_tooth_slope;
	uint16 y_saw_tooth_amp;
	uint16 y_rise_accum_init;
	uint16 y_fall_accum_init;
	bool y_coeff_enable;
	uint8 y_coeff_value;
	
	pll_dividers	tv_dividers;
	pll_dividers	crt_dividers;
} tv_params;


// TV-timing
typedef struct {
	uint32 freq;				// TV sub carrier frequency x12
    uint16 h_total;
    uint16 h_sync_len;
    uint16 h_genclk_delay;
    uint16 h_setup_delay;
    uint16 h_active_delay;
    uint16 h_active_len;
    uint16 v_total;
    uint16 v_active_lines;
    uint16 v_field_total;
    uint16 v_fields;
    uint16 f_total;
    uint16 frame_size_adjust;
    uint32 scale;
} tv_timing;


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
	
	// RMX registers
	uint32		fp_horz_stretch;
    uint32		fp_vert_stretch;
    
    // Flat panel regs
	uint32		fp_gen_cntl;
	uint32		fp_panel_cntl;
    uint32		lvds_gen_cntl;
	uint32		fp_h_sync_strt_wid;
	uint32		fp_v_sync_strt_wid;
	uint32		fp2_gen_cntl;
    
    uint32		fp2_h_sync_strt_wid;
	uint32		fp2_v_sync_strt_wid;

	// DAC regs
	uint32		dac_cntl2;
	uint32		dac_cntl;
	uint32		disp_hw_debug;
	
	// PLL regs
	uint32 		ppll_div_3;
	uint32		ppll_ref_div;
	uint32		htotal_cntl;
	
	// pure information
	uint32		dot_clock_freq;	// in 10 kHz
    uint32		pll_output_freq;// in 10 kHz
    int			feedback_div;
    int			post_div;
    
   	// Common regs
	uint32		surface_cntl;
	uint32		disp_output_cntl;
	
	// TV-Out registers
	uint32		tv_ftotal;
	uint32		tv_vscaler_cntl1;
	uint32		tv_y_saw_tooth_cntl;
	uint32		tv_y_fall_cntl;
	uint32		tv_y_rise_cntl;
	uint32		tv_vscaler_cntl2;
	uint32 		tv_hrestart;
	uint32 		tv_vrestart;
	uint32 		tv_frestart;
	uint32		tv_tv_pll_cntl;
	uint32 		tv_crt_pll_cntl;
	uint32		tv_clock_sel_cntl;
	uint32 		tv_clkout_cntl;
	uint32		tv_htotal;
	uint32 		tv_hsize;
	uint32		tv_hdisp;
	uint32		tv_hstart;
	uint32		tv_vtotal;
	uint32		tv_vdisp;
	uint32		tv_sync_size;
	uint32		tv_timing_cntl;
	uint32		tv_dac_cntl;		// affects CRT connected to TV-DAC
	uint32		tv_modulator_cntl1;
	uint32		tv_modulator_cntl2;
	uint32		tv_data_delay_a;
	uint32		tv_data_delay_b;
	uint32		tv_frame_lock_cntl;
	uint32		tv_pll_cntl1;
	uint32		tv_rgb_cntl;
	uint32		tv_pre_dac_mux_cntl;
	uint32		tv_master_cntl;
	uint32		tv_uv_adr;
	uint32		tv_pll_fine_cntl;
} port_regs;


// one overlay buffer
typedef struct overlay_buffer_node {
	struct overlay_buffer_node *next, *prev;
	uint32 mem_handle;
	uint32 mem_offset;
	uint ati_space;				// colour space according to ATI
	uint test_reg;				// content of test reg
	overlay_buffer buffer;
} overlay_buffer_node;


// info about active overlay
typedef struct {
	overlay_token	ot;
	overlay_buffer  ob;
	overlay_window	ow;
	overlay_view	ov;
	uint16			h_display_start;
	uint16			v_display_start;
	
	overlay_buffer_node *on;	// current buffer
	overlay_buffer_node *prev_on; // previous buffer (for temporal deinterlace, currently unused)
	int8			head;		// physical head where the overlay is shown on
	uint32			rel_offset;	// offset of overlay source due to clipping
} overlay_info;


// each accelerant gets one "virtual card", i.e. you
// can have independant accelerants for each head
// (this is an ongoing project ;)
typedef struct {
	uint32		id;				// identifier used to know which card the 2D accelerator
								// is prepared for (we use area_id of this structure)
	virtual_head heads[2];		// heads assigned to virtual card
	uint8		num_heads;		// number of heads assigned to virtual card
	
	int8		independant_heads;	// number of heads to be programmed independantly
	int8		different_heads;	// number of heads showing different parts of framebuffer
	bool		scroll;			// scrolling in virtual area enabled

	uint32		datatype;		// Radeon code for pixel format
	uint		bpp;			// bytes per pixel
	uint32		pitch;			// byte offset between two lines

	uint32		eff_width, eff_height;	// size of visible area (including both monitors)
	uint32		fb_mem_handle;	// memory handle
	uint32		fb_offset;		// offset of frame buffer in graphics mem

	cursor_info	cursor;

	multi_mode_e	wanted_multi_mode;	// multi monitor mode as requested by user

	bool		swap_displays;	// true to swap monitors

	frame_buffer_config fbc;	// data for direct frame buffer access

	display_mode mode;			// offical mode with multi-monitor bits set	
	overlay_buffer_node	*overlay_buffers;	// list of allocated overlay buffers
	
	//int8		whished_overlay_head;	// head where users whishes the overlay to be
	bool		uses_overlay;	// true if this virtual card owns overlay
	
	int			state_buffer_idx;
	int			state_buffer_size;
} virtual_card;


// status of overlay
typedef struct {
	vint32	inuse;				// one, if someone allocated overlay head
								// (this doesn't necessarily mean that an overlay is shown)
	uint32	token;				// arbitrarily chosen token to identify overlay owner
								// (increased by 1 whenever there is a new owner)
	uint32	auto_flip_reg;		// content of auto_flip_reg
} overlay_mgr_info;


// info about indirect CP buffer
typedef struct {
	int		next;				// next used/free buffer (-1 for EOL)
	int32	send_tag;			// tag assigned when buffer was submitted
} indirect_buffer;


// info about command processor (CP) state
typedef struct {
	benaphore	lock;			// lock to be acquired when talking to CP or
								// when accesing this structure

	// ring buffer (in non-local memory)
	struct {
		uint32	mem_offset;			// offset in non-local memory
		uint32	vm_base;			// base of ring buffer as seen by graphics card
		uint32	tail, tail_mask;	// next write position in dwords; mask for wrap-arounds
		uint32	size;				// size in dwords
		//uint32	head_offset;	// offset for automatically updates head in DMA buffer

		//uint32			start_offset;
		memory_type_e	mem_type;
		//uint32	*start;			// pointer to ring buffer
		//vuint32	*head;			// pointer to automatically updated read position
		
		uint32	space;			// known space in ring buffer
		uint32 	mem_handle;		// handle of memory of indirect buffers
	} ring;
	
	// feedback registers (in PCI memory)
	struct {
		//vuint32 *ptr;			// pointer to scratch registers
		uint32			scratch_mem_offset;	// offset of scratch registers in feedback memory
		uint32			head_mem_offset;	// offset of head register in feedback memory
		uint32			scratch_vm_start;	// virtual address of scratch as seen by GC
		uint32			head_vm_address;	// virtual address of head as seen by GC
		memory_type_e	mem_type;		// memory type of feedback memory
		uint32 			mem_handle;		// handle of feedback memory
	} feedback;
	

	// indirect buffers (in non-local memory)
	// for indeces: -1 means "none"
	struct {
		int			free_list;		// index of first empty buffer
		int			oldest,			// oldest submitted buffer
					newest;			// newest submitted buffer
		int			active_state;	// index of active state buffer
		uint64		cur_tag;		// tag of last submitted indirect buffer

		memory_type_e mem_type;		
		uint32		mem_offset;		// offset of indirect buffers in non-local memory		
		uint32		vm_start;		// start of indirect buffers as seen by graphics card
		
		indirect_buffer	buffers[NUM_INDIRECT_BUFFERS];	// info about buffers
		uint32 		mem_handle;		// handle of memory of indirect buffers
	} buffers;
} CP_info;


// info about different graphics-related memory
// (see memory_type_e)
typedef struct {
	area_id		area;				// area to memory
	uint32		size;				// usable size in bytes
	uint32		virtual_addr_start;	// virtual address (for graphics card!)
	uint32		virtual_size;		// reserved virtual address space in bytes
} memory_type_info;


// data published by kernel and shared by all accelerant/virtual cards
typedef struct {
	// filled out by kernel
	CP_info	cp;				// info concerning command processor
	
	// set by accelerant
	struct {
		uint64		count;		// count of submitted CP commands
		uint64		last_idle;	// count when engine was idle last time
		uint64		written;	// last count passed to CP
		benaphore	lock;		// engine lock
	} engine;	

	uint16		vendor_id;	// PCI vendor id
	uint16		device_id;	// PCI device id
	uint8		revision;	// PCI device revision
	
	//bool 		has_crtc2;	// has second CRTC
	radeon_type	asic;		// ASIC version
	bool		is_mobility; // mobility version
	tv_chip_type tv_chip;	// type of TV-Out encoder
	
	uint8		theatre_channel;	// VIP channel of Rage Theatre (if applicable)
		
	general_pll_info	pll;
	
	area_id		regs_area;	// area of memory mapped registers
	area_id		ROM_area;	// area of ROM
	//area_id		fb_area;	// area of frame buffer
	void		*framebuffer_pci;	// physical address of frame buffer (aka local memory)
								// this is a hack needed by BeOS

	physical_head heads[2];		// physical heads
	uint8		num_heads;		// number of physical heads
	
	display_device_e connected_displays; // bit-field of connected displays

	fp_info		flatpanels[2];	// info about connected flat panels (if any)

	memory_type_info	memory[mt_last];	// info about memory types
	memory_type_e	nonlocal_type;	// default type of non-local memory

	uint8	*local_mem;			// address of local memory; 
								// this is a hack requested by BeOS
												
	area_id	mode_list_area;		// area containing display mode list
	uint	mode_count;
	
	uint32	active_vc;			// currently selected virtual card in terms of 2D acceleration
		
	uint32	dac_cntl2;			// content of dac_cntl2 register

	overlay_info	pending_overlay;	// overlay to be shown
	overlay_info	active_overlay;		// overlay shown
	overlay_mgr_info overlay_mgr;		// status of overlay
	
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

// alloc (non-)local memory
typedef struct {
	uint32			magic;
	memory_type_e	memory_type;// type of memory
	uint32			size;		// size in bytes
	uint32			offset;		// offset in memory
	uint32			handle;		// handle (needed to free memory)
	bool			global;		// set this to true if memory should persist even
								// if client gets terminated
} radeon_alloc_mem;

// free (non-)local memory
typedef struct {
	uint32 			magic;
	memory_type_e	memory_type;// type of memory
	uint32			handle;		// memory handle 
	bool			global;		// must be same as on alloc_local_mem
} radeon_free_mem;

// wait for idle
typedef struct {
	uint32 			magic;
	bool			keep_lock;	// keep lock after engine is idle
} radeon_wait_for_idle;

// read VIP register
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32 			data;		// read data
} radeon_vip_read;

// write VIP register
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32 			data;		// data to write
} radeon_vip_write;

// find channel of device with given ID
typedef struct {
	uint32 			magic;
	uint32 			device_id;	// id of device
	uint 			channel;	// channel of device (-1 if not found)
} radeon_find_vip_device;

// wait for capture interrupt and get status about
typedef struct {
	uint32 			magic;
	bigtime_t		timeout;	// timeout to wait for irq
	bigtime_t		timestamp;	// timestamp when last capturing was finished
	uint32			int_status;	// content of RADEON_CAP_INT_STATUS
	uint32			counter;	// number of capture interrupts so far
} radeon_wait_for_cap_irq;

// copy data from frame buffer to some memory location
typedef struct {
	uint32 			magic;
	uint32			src;		// offset of source data in frame buffer
	void			*target;	// target buffer
	size_t			size;		// number of bytes to copy
	bool			lock_mem;	// true, if target needs to be locked
	bool			contiguous;	// true, if target is physically contiguous
} radeon_dma_copy;

// parameter for ioctl without further arguments
typedef struct {
	uint32			magic;
} radeon_no_arg;

#endif
