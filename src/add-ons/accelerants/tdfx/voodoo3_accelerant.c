/*
   - PPC Port: Andreas Drewke (andreas_dr@gmx.de)
   - Voodoo3Driver 0.02 (c) by Carwyn Jones (2002)
   
*/
#include <Accelerant.h>
#include "voodoo3_io.h"
#include "voodoo3_accelerant.h"
#include "GlobalData.h"
#include <ByteOrder.h>

static Voodoo3Accel accel = { 0, 0, 0, 0, 0};
#define abs(x) (x<0?-x:x)


static __inline__ void write_crt_registers(uint8 reg, uint8 val)
{
	outw(accel.io_base + CRTC_INDEX - 0x300, (val << 8) | reg);
}

static __inline__ void write_seq_registers(uint8 reg, uint8 val)
{
	outw(accel.io_base + SEQ_INDEX - 0x300, (val << 8) | reg);
}

static __inline__ void write_graph_registers(uint8 reg, uint8 val)
{
	outw(accel.io_base + GRA_INDEX - 0x300, (val << 8) | reg);
}

static __inline__ void write_attr_registers(uint8 reg, uint8 val)
{
	uint8 index;
    inb (accel.io_base + IS1_R - 0x300);
    index = inb(accel.io_base + ATT_IW - 0x300);
    outb(accel.io_base + ATT_IW - 0x300, reg);
    outb(accel.io_base + ATT_IW - 0x300, val);
    outb(accel.io_base + ATT_IW - 0x300, index);
}

static inline void vga_outb(unsigned long reg,  unsigned char val)
{ 
	outb(accel.io_base + reg - 0x300, val); 
}

static inline void voodoo3_write32(unsigned int reg, unsigned long val)
{
	PCI_MEM_WR_32(accel.regs_base + reg, val);
}

static inline uint32 voodoo3_read32(unsigned int reg) {
	return PCI_MEM_RD_32(accel.regs_base + reg);
}

static inline void voodoo3_make_room(unsigned int len) {
	while((voodoo3_read32(STATUS) & 0x1f) < len);
}

static void voodoo3_initialize()
{
	unsigned long vgainit0 = 0;
	unsigned long vidcfg = 0;
	unsigned long cpp = 16;
	vgainit0 = 
    VGAINIT0_8BIT_DAC     |
    VGAINIT0_EXT_ENABLE   |
    VGAINIT0_WAKEUP_3C3   |
    VGAINIT0_ALT_READBACK |
    VGAINIT0_EXTSHIFTOUT;
	
	vidcfg = 
    VIDCFG_VIDPROC_ENABLE |
    VIDCFG_DESK_ENABLE    |
    ((cpp - 1) << VIDCFG_PIXFMT_SHIFT) |
    (cpp != 1 ? VIDCFG_CLUT_BYPASS : 0);

    voodoo3_make_room(2);
    voodoo3_write32(VGAINIT0, vgainit0);
    voodoo3_write32(VIDPROCCFG, vidcfg);
	voodoo3_wait_idle();
}

int voodoo3_init(uint8 *registers_base, uint32 io_base)
{
	if(!registers_base) return 0;
	accel.regs_base = registers_base;
	accel.io_base = io_base;
	voodoo3_initialize();
	return 1;
}

uint32 voodoo3_bits_per_pixel()
{
	return accel.bpp;
}

void voodoo3_set_monitor_defaults()
{
	MonitorRegs m; 
	MonitorRegs *mode;
	int i;

	memset(&m, 0, sizeof(MonitorRegs));
	
	mode = &m;
    mode->crtc_regs[0] = 159;
    mode->crtc_regs[1] = 127;
    mode->crtc_regs[2] = 127;
    mode->crtc_regs[3] = 131;
    mode->crtc_regs[4] = 129;
    
    mode->crtc_regs[5] = 141;
    mode->crtc_regs[6] = 30;
    mode->crtc_regs[7] = 245;
    mode->crtc_regs[8] = 0;
    mode->crtc_regs[9] = 96;
    
    mode->crtc_regs[10] = 0;
    mode->crtc_regs[11] = 0;
    mode->crtc_regs[12] = 0;
    mode->crtc_regs[13] = 0;
    mode->crtc_regs[14] = 0;
    mode->crtc_regs[15] = 0;
    
    mode->crtc_regs[16] = 1;
    mode->crtc_regs[17] = 36;
    mode->crtc_regs[18] = 255;
    mode->crtc_regs[19] = 127;
    mode->crtc_regs[20] = 0;
    mode->crtc_regs[21] = 255;
    
    mode->crtc_regs[22] = 31;
    mode->crtc_regs[23] = 128;
    mode->crtc_regs[24] = 255;
    
	vga_outb(MISC_W, 0x0f);
    
	for (i = 0; i < 0x19; i ++)
		write_crt_registers(i, mode->crtc_regs[i]);

}

void voodoo3_wait_idle()
{
	int i = 0;

	voodoo3_make_room(1);
	voodoo3_write32(COMMAND_3D, COMMAND_3D_NOP);

	while(1) {
		i = (voodoo3_read32(STATUS) & STATUS_BUSY) ? 0 : i + 1;
		if(i == 3) break;
	}
}

#define REFFREQ 14318.18

static int
voodoo3_calc_pll(int freq, int *f_out, int isBanshee) {
  int m, n, k, best_m, best_n, best_k, f_cur, best_error;
  int minm, maxm;

  best_error=freq;
  best_n=best_m=best_k=0;
  if (isBanshee) {
    minm=24;
    maxm=24;
  } else {
    minm=1;
    maxm=57; /* This used to be 64, alas it seems the last 8 (funny that ?)
              * values cause jittering at lower resolutions. I've not done
              * any calculations to what the adjustment affects clock ranges,
              * but I can still run at 1600x1200@75Hz */
  }
  for (n=1; n<256; n++) {
    f_cur=REFFREQ*(n+2);
    if (f_cur<freq) {
      f_cur=f_cur/3;
      if (freq-f_cur<best_error) {
	best_error=freq-f_cur;
	best_n=n;
	best_m=1;
	best_k=0;
	continue;
      }
    }
    for (m=minm; m<maxm; m++) {
      for (k=0; k<4; k++) {
	f_cur=REFFREQ*(n+2)/(m+2)/(1<<k);
	if (abs(f_cur-freq)<best_error) {
	  best_error=abs(f_cur-freq);
	  best_n=n;
	  best_m=m;
	  best_k=k;
	}
      }
    }
  }
  n=best_n;
  m=best_m;
  k=best_k;
  *f_out=REFFREQ*(n+2)/(m+2)/(1<<k);
  return (n<<8)|(m<<2)|k;
}

void voodoo3_setup_monitor(display_mode *dm)
{
	MonitorRegs mod; MonitorRegs *mode;
	int i;
	uint32 horizontal_display_end, horizontal_sync_start,
		horizontal_sync_end, horizontal_total, horizontal_blanking_start,
		horizontal_blanking_end;
	
	uint32 vertical_display_enable_end, vertical_sync_start,
		vertical_sync_end, vertical_total, vertical_blanking_start,
		vertical_blanking_end;
		
	uint32 wd; // CRTC offset

	memset(&mod, 0, sizeof(mode));
	
	mode = &mod;
	
    wd = (dm->timing.h_display >> 3) - 1;
	horizontal_display_end  = (dm->timing.h_display >> 3) - 1;
	horizontal_sync_start  = (dm->timing.h_sync_start >> 3) - 1;
	horizontal_sync_end  = (dm->timing.h_sync_end >> 3) - 1;
	horizontal_total  = (dm->timing.h_total   >> 3) - 1;
	horizontal_blanking_start = horizontal_display_end;
	horizontal_blanking_end = horizontal_total;

	vertical_display_enable_end  = dm->timing.v_display - 1;
	vertical_sync_start  = dm->timing.v_sync_start;// - 1;
	vertical_sync_end  = dm->timing.v_sync_end;// - 1;
	vertical_total  = dm->timing.v_total   - 2;
	vertical_blanking_start = vertical_display_enable_end;
	vertical_blanking_end = vertical_total;
	
	mode->misc = 
    0x0f |
    (dm->timing.h_display < 400 ? 0xa0 :
     dm->timing.h_display < 480 ? 0x60 :
     dm->timing.h_display < 768 ? 0xe0 : 0x20);
     
    mode->seq_regs[0] = 3;
    mode->seq_regs[1] = 1;
    mode->seq_regs[2] = 8;
    mode->seq_regs[3] = 0;
    mode->seq_regs[4] = 6;
    
    // crtc regs start
    mode->crtc_regs[0] = horizontal_total - 4;
    mode->crtc_regs[1] = horizontal_display_end;
    mode->crtc_regs[2] = horizontal_blanking_start;
    mode->crtc_regs[3] = 0x80 | (horizontal_blanking_end & 0x1f);
    mode->crtc_regs[4] = horizontal_sync_start;
    
    mode->crtc_regs[5] = ((horizontal_blanking_end & 0x20) << 2) | (horizontal_sync_end & 0x1f);
    mode->crtc_regs[6] = vertical_total;
    mode->crtc_regs[7] = ((vertical_sync_start & 0x200) >> 2) |
    ((vertical_display_enable_end & 0x200) >> 3) |
    ((vertical_total & 0x200) >> 4) |
    0x10 |
    ((vertical_blanking_start & 0x100) >> 5) |
    ((vertical_sync_start  & 0x100) >> 6) |
    ((vertical_display_enable_end  & 0x100) >> 7) |
    ((vertical_total  & 0x100) >> 8);
    
    mode->crtc_regs[8] = 0;
    mode->crtc_regs[9] = 0x40 |
    ((vertical_blanking_start & 0x200) >> 4);
    
    mode->crtc_regs[10] = 0;
    mode->crtc_regs[11] = 0;
    mode->crtc_regs[12] = 0;
    mode->crtc_regs[13] = 0;
    mode->crtc_regs[14] = 0;
    mode->crtc_regs[15] = 0;
    
    mode->crtc_regs[16] = vertical_sync_start;
    mode->crtc_regs[17] = (vertical_sync_end & 0x0f) | 0x20;
    mode->crtc_regs[18] = vertical_display_enable_end;
    mode->crtc_regs[19] = wd; // CRTC offset
    mode->crtc_regs[20] = 0;
    mode->crtc_regs[21] = vertical_blanking_start;
    
    mode->crtc_regs[22] = vertical_blanking_end + 1;
    mode->crtc_regs[23] = 128;
    mode->crtc_regs[24] = 255;
    
    // attr regs start
    mode->attr_regs[0] = 0;
    mode->attr_regs[1] = 0;
    mode->attr_regs[2] = 0;
    mode->attr_regs[3] = 0;
    mode->attr_regs[4] = 0;
    mode->attr_regs[5] = 0;
    
    mode->attr_regs[6] = 0;
    mode->attr_regs[7] = 0;
    mode->attr_regs[8] = 0;
    mode->attr_regs[9] = 0;
    mode->attr_regs[10] = 0;
    mode->attr_regs[11] = 0;
    
    mode->attr_regs[12] = 0;
    mode->attr_regs[13] = 0;
    mode->attr_regs[14] = 0;
    mode->attr_regs[15] = 0;
    mode->attr_regs[16] = 1;
    mode->attr_regs[17] = 0;
    mode->attr_regs[18] = 15;
    mode->attr_regs[19] = 0;
    // attr regs end
    
    // graph regs start
    mode->graph_regs[0] = 159;
    mode->graph_regs[1] = 127;
    mode->graph_regs[2] = 127;
    mode->graph_regs[3] = 131;
    mode->graph_regs[4] = 130;
    mode->graph_regs[5] = 142;
    mode->graph_regs[6] = 30;
    mode->graph_regs[7] = 245;
    mode->graph_regs[8] = 0;

	vga_outb(MISC_W, mode->misc | 0x01);
    
	for(i = 0; i < 5; i++)
		write_seq_registers(i, mode->seq_regs[i]);
    for (i = 0; i < 0x19; i ++)
        write_crt_registers(i, mode->crtc_regs[i]);
    for (i = 0; i < 0x14; i ++)
        write_attr_registers(i, mode->attr_regs[i]);
    for (i = 0; i < 0x09; i ++)
        write_graph_registers(i, mode->graph_regs[i]);
}

void voodoo_set_desktop_regs(uint32 bits_per_pixel, display_mode *dm)
{
	uint32 miscinit0=0;
	int vidpll, fout;
	int vidproc = VIDPROCDEFAULT;
	uint32 bpp = (bits_per_pixel + 7) / 8;
	uint32 bytes_per_row = (dm->virtual_width * bits_per_pixel + 7) / 8;

	accel.bpp = bits_per_pixel;
	accel.width = dm->virtual_width;
	accel.height = dm->virtual_height;	
	voodoo3_setup_monitor(dm);
	
	vidproc &= ~(0x1c0000); // clear bits 18 to 20, bpp in vidproccfg
	vidproc |= ((bpp - 1) << VIDCFG_PIXFMT_SHIFT); // enable bits 18 to 20 to the required bpp
	vidpll = voodoo3_calc_pll(dm->timing.pixel_clock, &fout, 0);
	
	switch(bits_per_pixel) { // bit 10 of vidproccfg, is enabled or disabled as needed for the Desktop
		case 8:
			vidproc &= ~(1 << 10); // bit 10 off for palettized modes only, off means palette is used
			break;
		case 16:
			#if __POWERPC__
				miscinit0 = 0xc0000000;
			#endif
			vidproc |= (1 << 10); // bypass palette for 16bit modes
			break;
		case 32:
			#if __POWERPC__
				miscinit0 = 0x40000000;
			#endif			
			vidproc |= (1 << 10); // Same for 32bit modes
			break;
	}
	
	voodoo3_wait_idle();
	
	voodoo3_write32(MISCINIT1, voodoo3_read32(MISCINIT1) | 0x01);

	voodoo3_make_room(4);
	voodoo3_write32(VGAINIT0, 4928);
	voodoo3_write32(DACMODE, 0);
	voodoo3_write32(VIDDESKSTRIDE, bytes_per_row);
	voodoo3_write32(PLLCTRL0, vidpll);
	
	voodoo3_make_room(5);
	voodoo3_write32(VIDSCREENSIZE, dm->virtual_width | (dm->virtual_height << 12));
	voodoo3_write32(VIDDESKSTART,  1024);
	voodoo3_write32(VIDPROCCFG, vidproc);
	voodoo3_write32(VGAINIT1, 0);
	voodoo3_write32(MISCINIT0, miscinit0);
	
	voodoo3_make_room(8);
	voodoo3_write32(SRCBASE, 1024);
	voodoo3_write32(DSTBASE, 1024);
	voodoo3_write32(COMMANDEXTRA_2D, 0);
  	voodoo3_write32(CLIP0MIN,        0);
  	voodoo3_write32(CLIP0MAX,        0x0fff0fff);
  	voodoo3_write32(CLIP1MIN,        0);
  	voodoo3_write32(CLIP1MAX,        0x0fff0fff);
	voodoo3_write32(SRCXY, 0);
	voodoo3_wait_idle();
}

#if __POWERPC__
static uint32 ppcSwap(uint32 val, uint32 bpp) {
  switch (bpp) {
	  default:
	    return val;
	  case 16:
	    return ((val & 0x00ff00ff) << 8) |
		       ((val & 0xff00ff00) >> 8);
	  case 24:
	  case 32:
		return ((((val) & 0x000000ff) << 24) | 
    	        (((val) & 0x0000ff00) << 8) |  
        	    (((val) & 0x00ff0000) >> 8) |  
            	(((val) & 0xff000000) >> 24));
  }
}
#endif


void voodoo3_set_cursor(uint8 *data, uint16 width, uint8 height, uint8 *andMask, uint8 *xorMask)
{
	int i;
	uint32 fbcurptr=voodoo3_read32(HWCURPATADDR);
	uint8 *cursor = (uint8 *)data + fbcurptr;
	uint16 *xordata = (uint16 *)xorMask;
	uint16 *anddata = (uint16 *)andMask;
	uint64 *cursor64 = (uint64 *)cursor;
#if __POWERPC__
	uint32 *cursor32 = (uint32 *)data + fbcurptr;
#endif
	for(i = 0; i < 16; i++) {
		*cursor64++ = ((~0 << 16) | (*anddata++));
		*cursor64++ = ((0 << 16) | (*xordata++));
	}
	
	for(i = 128; i < 176; i++) {
		*cursor64++ = ~0;
		*cursor64++ = 0;
	}
//	for PPC Byteswapping
#if __POWERPC__
    for (i=0; i<256; i++) {
      uint32 val = cursor32[i];
      cursor32[i] = ppcSwap(val,accel.bpp);
    }
#endif

}

void voodoo3_init_cursor_address()
{

	voodoo3_make_room(1);
	voodoo3_write32(HWCURPATADDR, 0);
	voodoo3_wait_idle();

}

void voodoo3_set_cursor_colors(int fg, int bg)
{


	voodoo3_make_room(2);
	voodoo3_write32(HWCURC0, bg & 0xffffff);
	voodoo3_write32(HWCURC1, fg  & 0xffffff);
	voodoo3_wait_idle();

}

void voodoo3_show_cursor()
{

	voodoo3_make_room(1);
	voodoo3_write32(VIDPROCCFG, voodoo3_read32(VIDPROCCFG) | (1 << 27));
	voodoo3_wait_idle();

}

void voodoo3_hide_cursor()
{

	voodoo3_make_room(1);
	voodoo3_write32(VIDPROCCFG, voodoo3_read32(VIDPROCCFG) & ~(1 << 27));
	voodoo3_wait_idle();

}

void voodoo3_move_cursor(int x, int y)
{
	#if __INTEL__
	#define xoffset 63
	#define yoffset 63	
    #else
	#define xoffset	15
	#define yoffset 63	
	#endif
	x += xoffset;
	y += yoffset;
	voodoo3_write32(HWCURLOC, x | (y << 16));
}

void voodoo3_screen_to_screen_blit(list_packet_blit *list, uint32 bytes_per_row, uint32 bits_per_pixel)
{
	uint32 fmt = bytes_per_row | ((bits_per_pixel + ((bits_per_pixel == 8) ? 0 : 8)) << 13);
	uint32 blitcmd = COMMAND_2D_S2S_BITBLT | (ROP_COPY << 24);

	if (list->src_x <= list->dest_x) {
        blitcmd |= BIT(14);
		list->src_x += (list->width - 1); list->dest_x += (list->width - 1); 
	}
	if (list->src_y <= list->dest_y) {
		blitcmd |= BIT(15);
		list->src_y += (list->height - 1); list->dest_y += (list->height - 1);
	}
	voodoo3_make_room(6);

	voodoo3_write32(SRCFORMAT, fmt);
	voodoo3_write32(DSTFORMAT, fmt);
	voodoo3_write32(COMMAND_2D, blitcmd); 
	voodoo3_write32(DSTSIZE,   list->width | (list->height << 16));
	voodoo3_write32(DSTXY,     list->dest_x | (list->dest_y << 16));
	voodoo3_write32(LAUNCH_2D, list->src_x | (list->src_y << 16)); 
	voodoo3_wait_idle();

}

void voodoo3_fill_rect(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel)
{
	uint32 fmt;
	fmt = bytes_per_row | ((bits_per_pixel + ((bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(5);
	voodoo3_write32(DSTFORMAT, fmt);
	voodoo3_write32(COLORFORE, color);
	voodoo3_write32(COMMAND_2D, COMMAND_2D_FILLRECT | (ROP_COPY << 24));
	voodoo3_write32(DSTSIZE,    list->w | (list->h << 16));
	voodoo3_write32(LAUNCH_2D,  list->x | (list->y << 16));
	voodoo3_wait_idle();

}

void voodoo3_invert_rect(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel)
{
	uint32 fmt;
	fmt = bytes_per_row | ((bits_per_pixel + ((bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(5);
	voodoo3_write32(DSTFORMAT, fmt);
	voodoo3_write32(COLORFORE, color);
	voodoo3_write32(COMMAND_2D, COMMAND_2D_FILLRECT | (ROP_INVERT << 24));
	voodoo3_write32(DSTSIZE,    list->w | (list->h << 16));
	voodoo3_write32(LAUNCH_2D,  list->x | (list->y << 16));
	voodoo3_wait_idle();

}

void voodoo3_fill_span(list_packet *list, uint32 color, uint32 bytes_per_row, uint32 bits_per_pixel)
{
	uint32 fmt;
	fmt = bytes_per_row | ((bits_per_pixel + ((bits_per_pixel == 8) ? 0 : 8)) << 13);

	voodoo3_make_room(5);
	voodoo3_write32(DSTFORMAT, fmt);
	voodoo3_write32(COLORFORE, color);
	voodoo3_write32(COMMAND_2D, COMMAND_2D_FILLRECT | (ROP_COPY << 24));
	voodoo3_write32(DSTSIZE,    list->w | (list->h << 16));
	voodoo3_write32(LAUNCH_2D,  list->x | (list->y << 16));
	voodoo3_wait_idle();

}

void voodoo3_set_palette(int index, uint32 color)
{

	voodoo3_make_room(2);
	voodoo3_write32(DACADDR, index);
	voodoo3_write32(DACDATA, color);

}

uint32 voodoo3_get_memory_size() {
	uint32 draminit0 = 0;
	uint32 draminit1 = 0;
	uint32 miscinit1 = 0;
	uint32 lfbsize   = 0;
	int32 sgram_p     = 0;
	draminit0 = voodoo3_read32(DRAMINIT0);  
	draminit1 = voodoo3_read32(DRAMINIT1);
	sgram_p = (draminit1 & DRAMINIT1_MEM_SDRAM) ? 0 : 1;
  	lfbsize = sgram_p ?
		(((draminit0 & DRAMINIT0_SGRAM_NUM)  ? 2 : 1) * 
		((draminit0 & DRAMINIT0_SGRAM_TYPE) ? 8 : 4) * 1024 * 1024) :
		16 * 1024 * 1024;

	/* disable block writes for SDRAM (why?) */
	miscinit1 = voodoo3_read32(MISCINIT1);
	miscinit1 |= sgram_p ? 0 : MISCINIT1_2DBLOCK_DIS;
	miscinit1 |= MISCINIT1_CLUT_INV;
	voodoo3_make_room(1);
	voodoo3_write32(MISCINIT1, miscinit1);
	return lfbsize;
}

void voodoo3_display_overlay(const overlay_window *ow, const overlay_buffer *ob, const overlay_view* ov) {

	uint8 red, green, blue;
	
	int32 offset;
    int32 pitch;
    int32 dudx, dvdy;
    uint32 left=0;
    uint32 srcwidth;
    uint32 vidcfg, stride;
	uint32 x1, y1, x2, y2;	
	uint32 offset_left, offset_top;

	if (ow==NULL||ob==NULL||ov==NULL) return;

	offset=(uint32)ob->buffer_dma; // Start OFFSET as PCI Space
    pitch=((ob->width<<1) + 3) & ~3;  // offset (of Desktop && Overlay)
   

	offset_left=ow->h_start<0?-ow->h_start:0;
	offset_top=ow->v_start<0?-ow->v_start:0;

    dudx = (ob->width << 20) / (ow->width);
    dvdy = (ob->height << 20) / (ow->height);

    left = ((offset_left*dudx<<16) & 0x0001ffff) << 3; 

	x1=ow->h_start + offset_left;
	y1=ow->v_start + offset_top;	
	x2=x1 + ow->width;
	if (x2>=accel.width) x2=accel.width; 
	y2=y1 + ow->height;
	if (y2>=accel.height) y2=accel.height; 	
	
	offset+=((offset_left*dudx>>20) & ~1) << 1;
	srcwidth=(ob->width<<20); 

    vidcfg=voodoo3_read32(VIDPROCCFG);
	stride=voodoo3_read32(VIDDESKTOPOVERLAYSTRIDE);
    
    vidcfg &= ~VIDPROCCFGMASK;   
	vidcfg |= VIDCFG_CHROMA_KEY | VIDCFG_OVL_ENABLE | VIDCFG_OVL_NOT_VIDEO_IN | 
		VIDCFG_OVL_CLUT_BYPASS;

    if(ob->width != ow->width)
    	vidcfg |= VIDCFG_OVL_HSCALE;
    if(ob->height != ow->height)
    	vidcfg |= VIDCFG_OVL_VSCALE;

	switch( ob->space) {
			case B_YCbCr422:
#if __POWERPC__
				switch(accel.bpp) {
					case 8:
						vidcfg |= VIDCFG_OVL_FMT_YUYV422 << VIDCFG_OVL_FMT_SHIFT;
						break;
					case 16:
						vidcfg |= VIDCFG_OVL_FMT_UYVY422 << VIDCFG_OVL_FMT_SHIFT;
						break;
					default:
						ddpf(( "YCbCr overlays aren't supported in 32 bit mode" ));
						return;
				}
#else
				vidcfg |= VIDCFG_OVL_FMT_YUYV422 << VIDCFG_OVL_FMT_SHIFT;
#endif
				break;
#if __POWERPC__
			case B_RGB16_BIG:
#else
			case B_RGB16:
#endif
				vidcfg |= VIDCFG_OVL_FMT_RGB565 << VIDCFG_OVL_FMT_SHIFT;
				break;
			default: return;

	}

    /* can't do bilinear filtering when in 2X mode */
  	if( (vidcfg & VIDCFG_2X) == 0 )
		vidcfg |= VIDCFG_OVL_FILTER_BILIN << VIDCFG_OVL_FILTER_SHIFT;

	voodoo3_make_room( 12 );
	
    voodoo3_write32(VIDPROCCFG, vidcfg);

	if ((ow->flags & B_OVERLAY_COLOR_KEY)==B_OVERLAY_COLOR_KEY) {
		uint32 color=0;
		red=ow->red.value & ow->red.mask;
		green=ow->green.value & ow->green.mask;
		blue=ow->blue.value & ow->blue.mask;		
		switch (voodoo3_bits_per_pixel()) {
			case(8):
			case(32):
				color=	(red << 16)+(green << 8)+(blue << 0);
				break;
			case(16):
				color=	(red << 11)+(green << 5)+(blue << 0);
				break;
		}
	    voodoo3_write32(VIDCHROMAMIN, color);
    	voodoo3_write32(VIDCHROMAMAX, color);
    }

	/* Setting up position of Overlaywindow on Screen */
    voodoo3_write32(VIDOVERLAYSTARTCOORDS, x1 | (y1 << 12));
	voodoo3_write32(VIDOVERLAYENDSCREENCOORDS, (x2 - 1) | ((y2 - 1) << 12));
	/* Setting up scale and position of overlay in Graphics Memory */
	voodoo3_write32(VIDOVERLAYDUDX, dudx);
	voodoo3_write32(VIDOVERLAYDUDXOFFSETSRCWIDTH, left | srcwidth);
	voodoo3_write32(VIDOVERLAYDVDY, dvdy);
	voodoo3_write32(VIDOVERLAYDVDYOFFSET, (y1 & 0x0000ffff) << 3);

    stride &= 0x0000ffff;
    stride |= pitch << 16;

    voodoo3_write32(VIDDESKTOPOVERLAYSTRIDE, stride);
	voodoo3_write32(SST_3D_LEFTOVERLAYBUF, offset);
	voodoo3_write32(VIDINADDR0, offset);
}

void voodoo3_reset_overlay() {

    uint32 vidcfg, stride;

    vidcfg=voodoo3_read32(VIDPROCCFG);
	stride=voodoo3_read32(VIDDESKTOPOVERLAYSTRIDE);


    /* reset the video */
	vidcfg &= ~VIDPROCCFGMASK;
    voodoo3_write32(VIDPROCCFG, vidcfg);
    voodoo3_write32(RGBMAXDELTA, 0x0080808);
    voodoo3_write32(VIDCHROMAMIN, 0);
    voodoo3_write32(VIDCHROMAMAX, 0);

}

// To stop the overlay
void voodoo3_stop_overlay() {

    uint32 vidcfg;

    vidcfg=voodoo3_read32(VIDPROCCFG);

    /* reset the video */
	vidcfg &= ~VIDPROCCFGMASK;
    voodoo3_write32(VIDPROCCFG, vidcfg);

}

