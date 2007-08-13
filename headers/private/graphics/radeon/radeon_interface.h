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
#include "ddc.h"

// magic code for ioctls
// changed from TKRA to TKR1 for RADEON_WAITFORFIFO ioctl
// changed from TKRA to TKR2 for VIP FIFO ioctls
#define RADEON_PRIVATE_DATA_MAGIC	'TKR2'

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
	RADEON_WAITFORFIFO,
	RADEON_RESETENGINE,
	RADEON_VIPREAD,
	RADEON_VIPWRITE,
	RADEON_VIPFIFOREAD,
	RADEON_VIPFIFOWRITE,
	RADEON_FINDVIPDEVICE,
	RADEON_VIPRESET,

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

typedef struct {
	uint32	loginfo;
	uint32	logflow;
	uint32	logerror;
	bool	switchhead;
	bool	force_lcd;
	bool	dynamic_clocks; // power saving / management for mobility chips
	bool	force_pci;
	bool	unhide_fastwrites;
	bool	force_acc_dma;	// one or the other
	bool	force_acc_mmio; // one or the other
	bool	acc_writeback;
} radeon_settings;

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

typedef enum
{
    ddc_none_detected,
    ddc_monid,
    ddc_dvi,
    ddc_vga,
    ddc_crt2
} radeon_ddc_type;

typedef enum
{
    mt_unknown = -1,
    mt_none    = 0,
    mt_crt     = 1,
    mt_lcd     = 2,
    mt_dfp     = 3,
    mt_ctv     = 4,
    mt_stv     = 5
} radeon_monitor_type;

typedef enum
{
    connector_none,
    connector_proprietary,
    connector_crt,
    connector_dvi_i,
    connector_dvi_d,
    connector_ctv,
    connector_stv,
    connector_unsupported
} radeon_connector_type;

typedef enum
{
    connector_none_atom,
    connector_vga_atom,
    connector_dvi_i_atom,
    connector_dvi_d_atom,
    connector_dvi_a_atom,
    connector_stv_atom,
    connector_ctv_atom,
    connector_lvds_atom,
    connector_digital_atom,
    connector_unsupported_atom
} radeon_connector_type_atom;

typedef enum
{
    dac_unknown = -1,
    dac_primary = 0,
    dac_tvdac   = 1
} radeon_dac_type;

typedef enum
{
    tmds_unknown = -1,
    tmds_int     = 0,
    tmds_ext     = 1
} radeon_tmds_type;

typedef struct
{
    radeon_ddc_type ddc_type;
    radeon_dac_type dac_type;
    radeon_tmds_type tmds_type;
    radeon_connector_type connector_type;
    radeon_monitor_type mon_type;
    edid1_info edid;
    bool edid_valid;
} radeon_connector;

typedef struct
{
    bool has_secondary;

    /*
     * The next two are used to make sure CRTC2 is restored before CRTC_EXT,
     * otherwise it could lead to blank screens.
     */
    bool is_secondary_restored;
    bool restore_primary;

    int mon_type1;
    int mon_type2;
        
    bool reversed_DAC;	/* TVDAC used as primary dac */
    bool reversed_TMDS;	/* DDC_DVI is used for external TMDS */
    
    radeon_connector port_info[2];
} disp_entity, *ptr_disp_entity;


// type of ASIC
typedef enum {
	rt_r100,		// original Radeon
	rt_rv100,		// original VE version
	rt_rs100,		// IGP 320M
	rt_rv200,		// Radeon 7500
	rt_rs200,		// IGP 330M/340M/350M
	rt_r200,		// Radeon 8500/9100
	rt_rv250,		// Radeon 9000
	rt_rs300,		// IGP rs300
	rt_rv280,		// Radeon 9200	
	// from here on, r300 and up must be located as ATI modified the 
	// PLL design and the PLL code only tests for >= rt_r300
	rt_r300,		// Radeon 9700
	rt_r350,		// Radeon 9800
	rt_rv350,		// Radeon 9600
	rt_rv380,		// X600
	rt_r420			// X800
} radeon_type;

#define IS_RV100_VARIANT ( \
		(ai->si->asic == rt_rv100)  ||  \
        (ai->si->asic == rt_rv200)  ||  \
        (ai->si->asic == rt_rs100)  ||  \
        (ai->si->asic == rt_rs200)  ||  \
        (ai->si->asic == rt_rv250)  ||  \
        (ai->si->asic == rt_rv280)  ||  \
        (ai->si->asic == rt_rs300))

#define IS_DI_R300_VARIANT ( \
		(di->asic == rt_r300)  ||  \
        (di->asic == rt_r350)  || \
        (di->asic == rt_rv350) || \
        (di->asic == rt_rv380) || \
        (di->asic == rt_r420))
        
#define IS_R300_VARIANT ( \
		(ai->si->asic == rt_r300)  ||  \
        (ai->si->asic == rt_r350)  || \
        (ai->si->asic == rt_rv350) || \
        (ai->si->asic == rt_rv380) || \
        (ai->si->asic == rt_r420))
        
// TV standard
typedef enum {
	ts_off,
	ts_ntsc,
	ts_pal_bdghi,
	ts_pal_m,
	ts_pal_nc,
	ts_scart_pal,
	ts_pal_60,
	ts_max = ts_pal_60
} tv_standard_e;


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


// info about flat panel connected to LVDS or DVI port
typedef struct {
	uint panel_pwr_delay;
	uint panel_xres, panel_yres;
	
	uint h_blank, h_over_plus, h_sync_width;
	uint v_blank, v_over_plus, v_sync_width;
	uint dot_clock;				// in kHz (this is BeOS like, ATI uses 10 kHz steps!)

	bool is_fp2;				// true, if second flat panel
//	display_type_e disp_type;
	uint16 ref_div;
	uint8 post_div;
	uint8 feedback_div;
	bool fixed_dividers;
	
	uint64 h_ratio;				// current stretch ratio, needed for overlays
	uint64 v_ratio;				// (mode_res/native_res; 32.32)
} fp_info;


// crtc info
typedef struct {
	//bool		is_crtc2;			// true, if crtc2
	int8		flatpanel_port;		// linked flat panel port (-1 if none)
	bool		cursor_on_screen;	// cursor is visible on this head
	int			crtc_idx;			// index of CRTC
	display_device_e active_displays; // currently driven displays
	display_device_e chosen_displays; // displays to be driven after next mode switch
	sem_id		vblank;				// vertical blank interrupt semaphore
	uint32		rel_x, rel_y;	// relative position in multi-monitor mode
	display_mode mode;				// display mode of this head
} crtc_info;


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

// info for ext tmds pll
typedef struct {
	uint32 freq;
	uint32 value;
} tmds_pll_info;

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
	int				crtc_idx;	// crtc where the overlay is shown on
	uint32			rel_offset;	// offset of overlay source due to clipping
} overlay_info;


// each accelerant gets one "virtual card", i.e. you
// can have independant accelerants for each head
// (this is an ongoing project ;)
typedef struct {
	uint32		id;				// identifier used to know which card the 2D accelerator
								// is prepared for (we use area_id of this structure)
	bool		assigned_crtc[2];	// mask of heads assigned to virtual card
	bool		used_crtc[2];	// mask of heads assigned to virtual card
	
	display_device_e controlled_displays; // displays devices controlled byvc
	display_device_e connected_displays; // bit-field of connected displays
	
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

	bool		swap_displays;	// true to swap monitors
	bool		use_laptop_panel; // true to always use laptop panel
	tv_standard_e tv_standard;	// standard to use for TV Out
	bool		enforce_mode_change; // set to make sure next display mode change 
								// is executed even if display mode seems to be
								// still the same

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
	bool	acc_dma;			// prevent use of dma engine
	
	// set by accelerant
	struct {
		uint64		count;		// count of submitted CP commands
		uint64		last_idle;	// count when engine was idle last time
		uint64		written;	// last count passed to CP
		benaphore	lock;		// engine lock
	} engine;	

	uint16		vendor_id;		// PCI vendor id
	uint16		device_id;		// PCI device id
	uint8		revision;		// PCI device revision
	
	radeon_type	asic;			// ASIC version
	bool		is_mobility;		// mobility version
	bool		is_igp;			// might need to know if it's an integrated chip
	bool		is_atombios;

	tv_chip_type 	tv_chip;		// type of TV-Out encoder
	bool		new_pll;		// r300 style PLL
	bool		has_no_i2c; 		// I2C is broken
	uint16		panel_pwr_delay;	// delay for LCD backlight to stabilise
	uint8		theatre_channel;	// VIP channel of Rage Theatre (if applicable)
		
	general_pll_info	pll;
	tmds_pll_info		tmds_pll[4];

	area_id		regs_area;		// area of memory mapped registers
	area_id		ROM_area;		// area of ROM
	void		*framebuffer_pci;	// physical address of frame buffer (aka local memory)
							// this is a hack needed by BeOS

	crtc_info	crtc[2];		// info about each crtc
	uint8		num_crtc;		// number of physical heads
	
	fp_info		flatpanels[2];	// info about connected flat panels (if any)
	disp_entity	routing;		// info if display connector routings eg DVI-I <- EXT TMDS <- DAC2 <- CRTC2

	memory_type_info	memory[mt_last];	// info about memory types
	memory_type_e	nonlocal_type;	// default type of non-local memory

	uint8	*local_mem;			// address of local memory; 
								// this is a hack requested by BeOS
												
	area_id	mode_list_area;		// area containing display mode list
	uint	mode_count;
	
	uint32	active_vc;			// currently selected virtual card in terms of 2D acceleration
		
	uint32	dac_cntl2;			// content of dac_cntl2 register
	uint32	tmds_pll_cntl;			// undocumented here be dragons
	uint32	tmds_transmitter_cntl;		// undocumented here be dragons

	overlay_info	pending_overlay;	// overlay to be shown
	overlay_info	active_overlay;		// overlay shown
	overlay_mgr_info overlay_mgr;		// status of overlay
	
	// data needed for VBI emulation 
	// (currently not fully implemented - if the user disabled graphics card
	//  IRQ in the BIOS, it's his fault)
	int		refresh_period;		// duration of one frame in ms
	int		blank_period;		// vertical blank period of a frame in ms
	int		enable_virtual_irq;	// true, to enable virtual interrupts
	
	radeon_settings	settings;	// settings from radeon.settings file
	
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

// wait for idle
typedef struct {
	uint32 			magic;
	int				entries;	// keep lock after engine is idle
} radeon_wait_for_fifo;

// read VIP register
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32 			data;		// read data
	bool			lock;		// true, if CP lock must be acquired
} radeon_vip_read;

// write VIP register
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32 			data;		// data to write
	bool			lock;		// true, if CP lock must be acquired
} radeon_vip_write;

// read VIP fifo
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32			count;		// size of buffer
	uint8 			*data;		// read data
	bool			lock;		// true, if CP lock must be acquired
} radeon_vip_fifo_read;

// write VIP fifo
typedef struct {
	uint32 			magic;
	uint 			channel;	// channel, i.e. device
	uint 			address;	// address
	uint32			count;		// size of buffer
	uint8 			*data;		// data to write
	bool			lock;		// true, if CP lock must be acquired
} radeon_vip_fifo_write;

// find channel of device with given ID
typedef struct {
	uint32 			magic;
	uint32 			device_id;	// id of device
	uint 			channel;	// channel of device (-1 if not found)
} radeon_find_vip_device;

// reset / init VIP
typedef struct {
	uint32 			magic;
	bool			lock;
} radeon_vip_reset;


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
