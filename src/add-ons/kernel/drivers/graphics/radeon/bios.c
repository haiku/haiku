/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	BIOS detection and retrieval of vital data
	
	Most of this data should be gathered directly,
	especially monitor detection should be done on
	demand so not all monitors need to be connected
	during boot
*/

#include "radeon_driver.h"
#include "mmio.h"
#include "bios_regs.h"
#include "config_regs.h"
#include "memcntrl_regs.h"
#include "buscntrl_regs.h"
#include "fp_regs.h"
#include "crtc_regs.h"
#include "ddc_regs.h"
#include "radeon_bios.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))

#define RADEON_BIOS8(v) 	 (di->rom.rom_ptr[v])
#define RADEON_BIOS16(v) 	((di->rom.rom_ptr[v]) | \
						  	 (di->rom.rom_ptr[(v) + 1] << 8))
#define RADEON_BIOS32(v) 	((di->rom.rom_ptr[v]) | \
							 (di->rom.rom_ptr[(v) + 1] << 8) \
							 (di->rom.rom_ptr[(v) + 2] << 16) \
							 (di->rom.rom_ptr[(v) + 3] << 24))

static const char ati_rom_sig[] = "761295520";

// find address of ROM;
// this code is really nasty as maintaining the radeon signatures
// is almost impossible (the signatures provided by ATI are always out-dated);
// further, if there is more then one card built into the computer, we
// may detect the wrong BIOS! 
// we have two possible solutions:
// 1. use the PCI location as stored in BIOS
// 2. verify the IO-base address as stored in BIOS
// I have no clue how these values are _written_ into the BIOS, and
// unfortunately, every BIOS does the detection in a different way,
// so I'm not sure which is the _right_ way of doing it
static char *Radeon_FindRom( rom_info *ri )
{       
	uint32 segstart;
	uint8 *rom_base;
	char *rom;
	int i;
	
	for( segstart = 0x000c0000; segstart < 0x000f0000; segstart += 0x00001000 ) {
		bool found = false;
		
		// find ROM		
		rom_base = ri->bios_ptr + segstart - 0xc0000;
		
		if( rom_base[0] != 0x55 || rom_base[1] != 0xaa )
			continue;
	
		// find signature of ATI                              
		rom = rom_base;

		found = false;
				
		for( i = 0; i < 128 - strlen( ati_rom_sig ); i++ ) {
			if( ati_rom_sig[0] == rom_base[i] ) {
				if( strncmp(ati_rom_sig, rom_base + i, strlen( ati_rom_sig )) == 0 ) {
					found = true;
					break;
				}
			}
		}
		
		if( !found )
			continue;

		// EK don't bother looking for signiture now, due to lack of consistancy.
				
		SHOW_INFO( 2, "found ROM @0x%lx", segstart );
		return rom_base;
	}
	
	SHOW_INFO0( 2, "no ROM found" );
	return NULL;
}


// PLL info is stored in ROM, probably it's too easy to replace it
// and thus they produce cards with different timings
static void Radeon_GetPLLInfo( device_info *di )
{
	uint8 *bios_header;
	uint8 *tmp;
	PLL_BLOCK pll, *pll_info;
	
	bios_header = di->rom.rom_ptr + *(uint16 *)(di->rom.rom_ptr + 0x48);
	pll_info = (PLL_BLOCK *)(di->rom.rom_ptr + *(uint16 *)(bios_header + 0x30));

	// determine type of ROM

	tmp = bios_header + 4;

    if ((	*tmp 	== 'A' 
   		&& *(tmp+1) == 'T' 
   		&& *(tmp+2) == 'O' 
   		&& *(tmp+3) == 'M'
   		) 
   		|| 
   		(	*tmp	== 'M' 
   		&& *(tmp+1) == 'O' 
   		&& *(tmp+2) == 'T' 
   		&& *(tmp+3) == 'A'
   		))
	{
		int bios_header, master_data_start, pll_start;
		di->is_atombios = true;
	
		bios_header 	  	 = RADEON_BIOS16(0x48);
		master_data_start 	 = RADEON_BIOS16(bios_header + 32);
		pll_start 		  	 = RADEON_BIOS16(master_data_start + 12);
		
		di->pll.ref_div 	 = 0; 
		di->pll.max_pll_freq = RADEON_BIOS16(pll_start + 32);
		di->pll.xclk 		 = RADEON_BIOS16(pll_start + 72);
		di->pll.min_pll_freq = RADEON_BIOS16(pll_start + 78);
		di->pll.ref_freq 	 = RADEON_BIOS16(pll_start + 82);

		SHOW_INFO( 2, "TESTING ref_clk=%ld, ref_div=%ld, xclk=%ld, min_freq=%ld, max_freq=%ld from ATOM Bios",
		di->pll.ref_freq, di->pll.ref_div, di->pll.xclk, 
		di->pll.min_pll_freq, di->pll.max_pll_freq );
			
		// Unused by beos driver so it appears...
		// info->sclk = RADEON_BIOS32(pll_info_block + 8) / 100.0;
		// info->mclk = RADEON_BIOS32(pll_info_block + 12) / 100.0;
		// if (info->sclk == 0) info->sclk = 200;
		// if (info->mclk == 0) info->mclk = 200;
			
	}
    else
	{
		di->is_atombios = false;
			
		memcpy( &pll, pll_info, sizeof( pll ));
	
		di->pll.xclk 		 = (uint32)pll.XCLK;
		di->pll.ref_freq 	 = (uint32)pll.PCLK_ref_freq;
		di->pll.ref_div 	 = (uint32)pll.PCLK_ref_divider;
		di->pll.min_pll_freq = pll.PCLK_min_freq;
		di->pll.max_pll_freq = pll.PCLK_max_freq;
		
		SHOW_INFO( 2, "ref_clk=%ld, ref_div=%ld, xclk=%ld, min_freq=%ld, max_freq=%ld from Legacy BIOS",
		di->pll.ref_freq, di->pll.ref_div, di->pll.xclk, 
		di->pll.min_pll_freq, di->pll.max_pll_freq );
		
	}
	
}

/*
const char *Mon2Str[] = {
	"N/C",
	"CRT",
	"CRT",
	"Laptop flatpanel",
	"DVI (flatpanel)",
	"secondary DVI (flatpanel) - unsupported",
	"Composite TV",
	"S-Video out"
};*/

/*
// ask BIOS what kind of monitor is connected to each port
static void Radeon_GetMonType( device_info *di )
{
	unsigned int tmp;
	
	SHOW_FLOW0( 3, "" );
	
	di->disp_type[0] = di->disp_type[1] = dt_none;

	if (di->has_crtc2) {
		tmp = INREG( di->regs, RADEON_BIOS_4_SCRATCH );
		
		// ordering of "if"s is important as multiple
		// devices can be concurrently connected to one port
		// (like both a CRT and a TV)
		
		// primary port
		// having flat-panel support is most important
		if (tmp & 0x08)
			di->disp_type[0] = dt_dvi;
		else if (tmp & 0x4)
			di->disp_type[0] = dt_lvds;
		else if (tmp & 0x200)
			di->disp_type[0] = dt_tv_crt;
		else if (tmp & 0x10)
			di->disp_type[0] = dt_ctv;
		else if (tmp & 0x20)
			di->disp_type[0] = dt_stv;

		// secondary port
		// having TV-Out support is more important then CRT support
		// (CRT gets signal anyway)
		if (tmp & 0x1000)
			di->disp_type[1] = dt_ctv;
		else if (tmp & 0x2000)
			di->disp_type[1] = dt_stv;
		else if (tmp & 0x2)
			di->disp_type[1] = dt_crt;
		else if (tmp & 0x800)
			di->disp_type[1] = dt_dvi_ext;
		else if (tmp & 0x400)
			// this is unlikely - I only know about one LVDS unit
			di->disp_type[1] = dt_lvds;
	} else {
		// regular Radeon
		// TBD: no TV-Out detection
		di->disp_type[0] = dt_none;

		tmp = INREG( di->regs, RADEON_FP_GEN_CNTL);

		if( tmp & RADEON_FP_EN_TMDS )
			di->disp_type[0] = dt_dvi;
		else
			di->disp_type[0] = dt_crt;
	}
	
	SHOW_INFO( 1, "BIOS reports %s on primary and %s on secondary port", 
		Mon2Str[di->disp_type[0]], Mon2Str[di->disp_type[1]]);
		
	// remove unsupported devices
	if( di->disp_type[0] >= dt_dvi_ext )
		di->disp_type[0] = dt_none;
	if( di->disp_type[1] >= dt_dvi_ext )
		di->disp_type[1] = dt_none;

	// HACK: overlays can only be shown on first CRTC;
	// if there's nothing on first port, connect
	// second port to first CRTC (proper signal routing
	// is hopefully done by BIOS)
	if( di->has_crtc2 ) {
		if( di->disp_type[0] == dt_none && di->disp_type[1] == dt_crt ) {
			di->disp_type[0] = dt_crt;
			di->disp_type[1] = dt_none;
		}
	}
	
	SHOW_INFO( 1, "Effective routing: %s on primary and %s on secondary port", 
		Mon2Str[di->disp_type[0]], Mon2Str[di->disp_type[1]]);
}
*/

// get flat panel info (does only make sense for Laptops
// with integrated display, but looking for it doesn't hurt,
// who knows which strange kind of combination is out there?)
static bool Radeon_GetBIOSDFPInfo( device_info *di )
{
	uint16 bios_header;
	uint16 fpi_offset;
	FPI_BLOCK fpi;
	char panel_name[30];
	int i;
	
	uint16 tmp;
	
	bios_header = RADEON_BIOS16( 0x48 );

	if (di->is_atombios)
	{
		int master_data_start;
		master_data_start = RADEON_BIOS16( bios_header + 32 );

		tmp = RADEON_BIOS16( master_data_start + 16 );
		if( tmp )
		{
	
		    di->fp_info.panel_xres		= RADEON_BIOS16( tmp +  6 );
		    di->fp_info.panel_yres		= RADEON_BIOS16( tmp + 10 );
		    di->fp_info.dot_clock		= RADEON_BIOS16( tmp +  4 ) * 10;
		    di->fp_info.h_blank			= RADEON_BIOS16( tmp +  8 );
		    di->fp_info.h_over_plus		= RADEON_BIOS16( tmp + 14 );
		    di->fp_info.h_sync_width	= RADEON_BIOS16( tmp + 16 );
		    di->fp_info.v_blank      	= RADEON_BIOS16( tmp + 12 );
			di->fp_info.v_over_plus		= RADEON_BIOS16( tmp + 18 );
		    di->fp_info.h_sync_width	= RADEON_BIOS16( tmp + 20 );
		    di->fp_info.panel_pwr_delay	= RADEON_BIOS16( tmp + 40 );
	
		    SHOW_INFO( 2, "Panel Info from ATOMBIOS:\n" 
					"XRes: %d, YRes: %d, DotClock: %d\n" 
					"HBlank: %d, HOverPlus: %d, HSyncWidth: %d\n" 
					"VBlank: %d, VOverPlus: %d, VSyncWidth: %d\n" 
					"PanelPowerDelay: %d\n",
					di->fp_info.panel_xres,	di->fp_info.panel_yres,	di->fp_info.dot_clock,
					di->fp_info.h_blank, di->fp_info.h_over_plus, di->fp_info.h_sync_width,
					di->fp_info.v_blank, di->fp_info.v_over_plus, di->fp_info.h_sync_width,
					di->fp_info.panel_pwr_delay	);
			       
		} 
		else 
		{
		    di->fp_info.panel_pwr_delay = 200;
			SHOW_ERROR0( 2, "No Panel Info Table found in BIOS" );
			return false;
		}
	} // is_atombios
	else
	{
	
		fpi_offset = RADEON_BIOS16(bios_header + 0x40);
		
		if( !fpi_offset ) {
			di->fp_info.panel_pwr_delay = 200;
			SHOW_ERROR0( 2, "No Panel Info Table found in BIOS" );
			return false;
		} 
			
		memcpy( &fpi, di->rom.rom_ptr + fpi_offset, sizeof( fpi ));
		
		memcpy( panel_name, &fpi.name, sizeof( fpi.name ) );
		panel_name[sizeof( fpi.name )] = 0;
		
		SHOW_INFO( 2, "Panel ID string: %s", panel_name );
				
		di->fp_info.panel_xres = fpi.panel_xres;
		di->fp_info.panel_yres = fpi.panel_yres;
		
		SHOW_INFO( 2, "Panel Size from BIOS: %dx%d", 
			di->fp_info.panel_xres, di->fp_info.panel_yres);
			
		di->fp_info.panel_pwr_delay = fpi.panel_pwr_delay;	
		if( di->fp_info.panel_pwr_delay > 2000 || di->fp_info.panel_pwr_delay < 0 )
			di->fp_info.panel_pwr_delay = 2000;
			
		di->fp_info.ref_div = fpi.ref_div;
		di->fp_info.post_div = fpi.post_div;
		di->fp_info.feedback_div = fpi.feedback_div;
	
		di->fp_info.fixed_dividers =
			di->fp_info.ref_div != 0 && di->fp_info.feedback_div > 3;
		
	
		// there might be multiple supported resolutions stored;
		// we are looking for native resolution
		for( i = 0; i < 20; ++i ) {
			uint16 fpi_timing_ofs;
			FPI_TIMING_BLOCK fpi_timing;
			
			fpi_timing_ofs = fpi.fpi_timing_ofs[i];
			
			if( fpi_timing_ofs == 0 )
				break;
				
			memcpy( &fpi_timing, di->rom.rom_ptr + fpi_timing_ofs, sizeof( fpi_timing ));
	
			if( fpi_timing.panel_xres != di->fp_info.panel_xres ||
				fpi_timing.panel_yres != di->fp_info.panel_yres )
				continue;
			
			di->fp_info.h_blank			= (fpi_timing.h_total - fpi_timing.h_display) * 8;
			// TBD: seems like upper four bits of hsync_start contain garbage
			di->fp_info.h_over_plus 	= ((fpi_timing.h_sync_start & 0xfff) - fpi_timing.h_display - 1) * 8;
			di->fp_info.h_sync_width 	= fpi_timing.h_sync_width * 8;
			di->fp_info.v_blank			= fpi_timing.v_total - fpi_timing.v_display;
			di->fp_info.v_over_plus 	= (fpi_timing.v_sync & 0x7ff) - fpi_timing.v_display;
			di->fp_info.v_sync_width 	= (fpi_timing.v_sync & 0xf800) >> 11;          
			di->fp_info.dot_clock 		= fpi_timing.dot_clock * 10;
			return true;
		}
	} // not is_atombios
	
	SHOW_ERROR0( 2, "Radeon: couldn't get Panel Timing from BIOS" );
	return false;
}


// try to reverse engineer DFP specification from 
// timing currently set up in graphics cards registers
// (effectively, we hope that BIOS has set it up correctly
//  and noone has messed registers up yet; let's pray)
static void Radeon_RevEnvDFPSize( device_info *di )
{
	vuint8 *regs = di->regs;
		
	di->fp_info.panel_yres = 
		((INREG( regs, RADEON_FP_VERT_STRETCH ) & RADEON_VERT_PANEL_SIZE) 
		>> RADEON_VERT_PANEL_SIZE_SHIFT) + 1;

	di->fp_info.panel_xres = 
		(((INREG( regs, RADEON_FP_HORZ_STRETCH ) & RADEON_HORZ_PANEL_SIZE) 
		>> RADEON_HORZ_PANEL_SIZE_SHIFT) + 1) * 8;
	
	SHOW_INFO( 2, "detected panel size from registers: %dx%d", 
		di->fp_info.panel_xres, di->fp_info.panel_yres);
}


// once more for getting precise timing
static void Radeon_RevEnvDFPTiming( device_info *di )
{
	vuint8 *regs = di->regs;
	uint32 r;
	uint16 a, b;

	
	r = INREG( regs, RADEON_FP_CRTC_H_TOTAL_DISP );
	// the magic "4" was found by trial and error and probably stems from fudge (see crtc.c)
	a = (r & RADEON_FP_CRTC_H_TOTAL_MASK)/* + 4*/;
	b = (r & RADEON_FP_CRTC_H_DISP_MASK) >> RADEON_FP_CRTC_H_DISP_SHIFT;
	di->fp_info.h_blank = (a - b) * 8;
	
	SHOW_FLOW( 2, "h_total=%d, h_disp=%d", a * 8, b * 8 );
	
	r = INREG( regs, RADEON_FP_H_SYNC_STRT_WID );
	di->fp_info.h_over_plus = 
		((r & RADEON_FP_H_SYNC_STRT_CHAR_MASK)
			  	>> RADEON_FP_H_SYNC_STRT_CHAR_SHIFT) - b/* - 1*/;
	di->fp_info.h_over_plus *= 8;
	di->fp_info.h_sync_width =    
		((r & RADEON_FP_H_SYNC_WID_MASK)
				>> RADEON_FP_H_SYNC_WID_SHIFT);
	// TBD: this seems to be wrong
	// (my BIOS tells 112, this calculation leads to 24!)
	di->fp_info.h_sync_width *= 8;
	
	r = INREG( regs, RADEON_FP_CRTC_V_TOTAL_DISP );
	a = (r & RADEON_FP_CRTC_V_TOTAL_MASK)/* + 1*/;
	b = (r & RADEON_FP_CRTC_V_DISP_MASK) >> RADEON_FP_CRTC_V_DISP_SHIFT;
	di->fp_info.v_blank = a - b;
	
	SHOW_FLOW( 2, "v_total=%d, v_disp=%d", a, b );
	
	r = INREG( regs, RADEON_FP_V_SYNC_STRT_WID );
	di->fp_info.v_over_plus = (r & RADEON_FP_V_SYNC_STRT_MASK) - b;
	di->fp_info.v_sync_width = ((r & RADEON_FP_V_SYNC_WID_MASK)
		>> RADEON_FP_V_SYNC_WID_SHIFT)/* + 1*/;
	
	// standard CRTC
	r = INREG( regs, RADEON_CRTC_H_TOTAL_DISP );
	a = (r & RADEON_CRTC_H_TOTAL);
	b = (r & RADEON_CRTC_H_DISP) >> RADEON_CRTC_H_DISP_SHIFT;
	di->fp_info.h_blank = (a - b) * 8;
	
	SHOW_FLOW( 2, "h_total=%d, h_disp=%d", a * 8, b * 8 );
	
	r = INREG( regs, RADEON_CRTC_H_SYNC_STRT_WID );
	di->fp_info.h_over_plus = 
		((r & RADEON_CRTC_H_SYNC_STRT_CHAR)
			  	>> RADEON_CRTC_H_SYNC_STRT_CHAR_SHIFT) - b;
	di->fp_info.h_over_plus *= 8;
	di->fp_info.h_sync_width =    
		((r & RADEON_CRTC_H_SYNC_WID)
				>> RADEON_CRTC_H_SYNC_WID_SHIFT);
	di->fp_info.h_sync_width *= 8;
	
	r = INREG( regs, RADEON_CRTC_V_TOTAL_DISP );
	a = (r & RADEON_CRTC_V_TOTAL);
	b = (r & RADEON_CRTC_V_DISP) >> RADEON_CRTC_V_DISP_SHIFT;
	di->fp_info.v_blank = a - b;
	
	SHOW_FLOW( 2, "v_total=%d, v_disp=%d", a, b );
	
	r = INREG( regs, RADEON_CRTC_V_SYNC_STRT_WID );
	di->fp_info.v_over_plus = (r & RADEON_CRTC_V_SYNC_STRT) - b;
	di->fp_info.v_sync_width = ((r & RADEON_CRTC_V_SYNC_WID)
		>> RADEON_CRTC_V_SYNC_WID_SHIFT);
}


/*
// get everything in terms of monitors connected to the card
static void Radeon_GetBIOSMon( device_info *di )
{
	Radeon_GetMonType( di );
	    
    // reset all Flat Panel Info; 
    // it gets filled out step by step, and this way we know what's still missing
    memset( &di->fp_info, 0, sizeof( di->fp_info ));
    
    // we assume that the only fp port is combined with standard port 0
	di->fp_info.disp_type = di->disp_type[0];

	if( di->is_mobility ) {
		// there is a flat panel - get info about it
		Radeon_GetBIOSDFPInfo( di );
		
		// if BIOS doesn't know, ask the registers
		if( di->fp_info.panel_xres == 0 || di->fp_info.panel_yres == 0)
			Radeon_RevEnvDFPSize( di );
			
		if( di->fp_info.h_blank == 0 || di->fp_info.v_blank == 0)
			Radeon_RevEnvDFPTiming( di );
			
		SHOW_INFO( 2, "h_disp=%d, h_blank=%d, h_over_plus=%d, h_sync_width=%d", 
			di->fp_info.panel_xres, di->fp_info.h_blank, di->fp_info.h_over_plus, di->fp_info.h_sync_width );
		SHOW_INFO( 2, "v_disp=%d, v_blank=%d, v_over_plus=%d, v_sync_width=%d", 
			di->fp_info.panel_yres, di->fp_info.v_blank, di->fp_info.v_over_plus, di->fp_info.v_sync_width );			
		SHOW_INFO( 2, "pixel_clock=%d", di->fp_info.dot_clock );
	}
}
*/

// get info about Laptop flat panel
static void Radeon_GetFPData( device_info *di )
{
    // reset all Flat Panel Info; 
    // it gets filled out step by step, and this way we know what's still missing
    memset( &di->fp_info, 0, sizeof( di->fp_info ));

	// we only use BIOS for Laptop flat panels
	if( !di->is_mobility )
		return;
		    
	// ask BIOS about flat panel spec
	Radeon_GetBIOSDFPInfo( di );
	
	// if BIOS doesn't know, ask the registers
	if( di->fp_info.panel_xres == 0 || di->fp_info.panel_yres == 0)
		Radeon_RevEnvDFPSize( di );
		
	if( di->fp_info.h_blank == 0 || di->fp_info.v_blank == 0)
		Radeon_RevEnvDFPTiming( di );
		
	SHOW_INFO( 2, "h_disp=%d, h_blank=%d, h_over_plus=%d, h_sync_width=%d", 
		di->fp_info.panel_xres, di->fp_info.h_blank, di->fp_info.h_over_plus, di->fp_info.h_sync_width );
	SHOW_INFO( 2, "v_disp=%d, v_blank=%d, v_over_plus=%d, v_sync_width=%d", 
		di->fp_info.panel_yres, di->fp_info.v_blank, di->fp_info.v_over_plus, di->fp_info.v_sync_width );			
	SHOW_INFO( 2, "pixel_clock=%d", di->fp_info.dot_clock );
}

// detect amount of graphics memory
static void Radeon_DetectRAM( device_info *di )
{
	vuint8 *regs = di->regs;

	if( !di->is_igp	)
		di->local_mem_size = INREG( regs, RADEON_CONFIG_MEMSIZE ) & RADEON_CONFIG_MEMSIZE_MASK;
	else {
		uint32 tom;
		
		tom = INREG( regs, RADEON_NB_TOM );
		di->local_mem_size = ((tom >> 16) + 1 - (tom & 0xffff)) << 16;
	}

	// some production boards of m6 will return 0 if it's 8 MB
	if( di->local_mem_size == 0 )
		di->local_mem_size = 8 * 1024 *1024;

	switch( INREG( regs, RADEON_MEM_SDRAM_MODE_REG ) & RADEON_MEM_CFG_TYPE_MASK ) {
		case RADEON_MEM_CFG_SDR:
			// SDR SGRAM (2:1)
			strcpy(di->ram_type, "SDR SGRAM");
			di->ram.ml = 4;
			di->ram.MB = 4;
			di->ram.Trcd = 1;
			di->ram.Trp = 2;
			di->ram.Twr = 1;
			di->ram.CL = 2;
			di->ram.loop_latency = 16;
			di->ram.Rloop = 16;
			di->ram.Tr2w = 0;			
			break;
			
		case RADEON_MEM_CFG_DDR:
			// DDR SGRAM
			strcpy(di->ram_type, "DDR SGRAM");
			di->ram.ml = 4;
			di->ram.MB = 4;
			di->ram.Trcd = 3;
			di->ram.Trp = 3;
			di->ram.Twr = 2;
			di->ram.CL = 3;
			di->ram.Tr2w = 1;
			di->ram.loop_latency = 16;
			di->ram.Rloop = 16;
			break;
			
		// only one bit, so there's no default
	}
	
	SHOW_INFO( 1, "%ld MB %s found", di->local_mem_size / 1024 / 1024, 
		di->ram_type );
		
/*	if( di->local_mem_size > 64 * 1024 * 1024 ) {
		di->local_mem_size = 64 * 1024 * 1024;
		
		SHOW_INFO0( 1, "restricted to 64 MB" );
	}*/
}


// map and verify card's BIOS to see whether this really is a Radeon
// (as we need BIOS for further info we have to make sure we use the right one)
status_t Radeon_MapBIOS( pci_info *pcii, rom_info *ri )
{
	char buffer[100];
	
	sprintf(buffer, "%04X_%04X_%02X%02X%02X bios",
		pcii->vendor_id, pcii->device_id,
		pcii->bus, pcii->device, pcii->function);

	// we only scan BIOS at legacy location in first MB;
	// using the PCI location would improve detection, especially
	// if multiple graphics cards are installed
	// BUT: BeOS uses the first graphics card it finds (sorted by
	// device name), thus you couldn't choose in BIOS which card 
	// to use; checking the legacy location ensures that the card is
	// only detected if it's the primary card
	ri->phys_address = 0xc0000;
	ri->size = 0x40000;
	
	ri->bios_area = map_physical_memory( buffer, (void *)ri->phys_address, 
		ri->size, B_ANY_KERNEL_ADDRESS, B_READ_AREA, (void **)&ri->bios_ptr );
	if( ri->bios_area < 0 )
		return ri->bios_area;
	
	ri->rom_ptr = Radeon_FindRom( ri );
	
	// on success, adjust physical address to found ROM
	if( ri->rom_ptr != NULL )
		ri->phys_address += ri->rom_ptr - ri->bios_ptr;
	
	return ri->rom_ptr != NULL ? B_OK : B_ERROR;
}


// unmap card's BIOS
void Radeon_UnmapBIOS( rom_info *ri )
{
	delete_area( ri->bios_area );
	
	ri->bios_ptr = ri->rom_ptr = NULL;
}


// get everything valuable from BIOS (BIOS must be mapped)
status_t Radeon_ReadBIOSData( device_info *di )
{
	shared_info dummy_si;
	status_t result = B_OK;
	
	// give Radeon_MapDevice something to play with
	di->si = &dummy_si;
	
	// don't map frame buffer - we don't know its proper size yet!
	result = Radeon_MapDevice( di, true );
	if( result < 0 )
		goto err1;

	Radeon_GetPLLInfo( di );
	Radeon_GetFPData( di );
	Radeon_DetectRAM( di );
	
	Radeon_UnmapDevice( di );
	
err1:
	di->si = NULL;
	
	return result;
}
