/* second CTRC functionality

   Authors:
   Mark Watson 6/2000,
   Rudolf Cornelissen 12/2002
*/

#define MODULE_BIT 0x00020000

#include "mga_std.h"

/*set a mode line - inputs are in pixels/scanlines*/
status_t g400_crtc2_set_timing(
	uint32 hdisp_e,uint32 hsync_s,uint32 hsync_e,uint32 htotal,
	uint32 vdisp_e,uint32 vsync_s,uint32 vsync_e,uint32 vtotal,
	uint8 hsync_pos,uint8 vsync_pos
	)
{
	LOG(4,("CRTC2: setting timing\n"));

	/*check horizontal timing parameters are to nearest 8 pixels*/
	if ((hdisp_e&7)|(hsync_s&7)|(hsync_e&7)|(htotal&7))
	{
		LOG(8,("CRTC2:Horizontal timings are not multiples of 8 pixels\n"));
		return B_ERROR;
	}

	/*program the second CRTC*/
	CR2W(HPARAM, ((((hdisp_e - 8) & 0x0fff) << 16) | ((htotal - 8) & 0x0fff)));
	CR2W(HSYNC, ((((hsync_e - 8) & 0x0fff) << 16) | ((hsync_s - 8) & 0x0fff)));
	CR2W(VPARAM, ((((vdisp_e - 1) & 0x0fff) << 16) | ((vtotal - 1) & 0x0fff)));
	CR2W(VSYNC, ((((vsync_e - 1) & 0x0fff) << 16) | ((vsync_s - 1) & 0x0fff)));
	//Mark: (wrong AFAIK, warning: SETMODE MAVEN-CRTC delay is now tuned to new setup!!)
	//CR2W(PRELOAD, (((vsync_s & 0x0fff) << 16) | (hsync_s & 0x0fff)));
	CR2W(PRELOAD, ((((vsync_s - 1) & 0x0fff) << 16) | ((hsync_s - 8) & 0x0fff)));
	CR2W(MISC, ((0xfff << 16) | (((!hsync_pos) & 0x01) << 8) | (((!vsync_pos) & 0x01) << 9)));

	return B_OK;
}

status_t g400_crtc2_depth(int mode)
{
	/*validate bit depth and set mode*/
	switch(mode)
	{
	case BPP16:case BPP32DIR:
		CR2W(CTL,(CR2R(CTL)&0xFF10077F)|(mode<<21));
		break;
	case BPP8:case BPP15:case BPP24:case BPP32:default:
		LOG(8,("CRTC2:Invalid bit depth\n"));
		return B_ERROR;
		break;
	}

	return B_OK;
}

status_t g400_crtc2_dpms(uint8 display,uint8 h,uint8 v)
{
	//fixme: CTL b0=1 is CRTC2 enabled, 0 is disabled. This code is dangerous...
	CR2W(CTL,(CR2R(CTL)&0xFFF0077E)|(display&h&v)); /*enable second CRTC if required*/

	/*ignore h,v because they are not supported*/
	return B_OK;
}

status_t g400_crtc2_dpms_fetch(uint8 * display,uint8 * h,uint8 * v)
{
	*display=CR2R(CTL)&1;

	*h=*v=1; /*h/vsync always enabled on second CRTC, does not support other*/

	return B_OK;
}

status_t g400_crtc2_set_display_pitch(uint32 pitch,uint8 bpp) 
{
	uint32 offset;

	LOG(4,("CRTC2: setting card pitch 0x%08x bpp %d\n", pitch, bpp));

	/*figure out offset value hardware needs*/
	offset = pitch*(bpp>>3);

	LOG(2,("CRTC2: offset: %x\n",offset));

	/*program the card!*/
	CR2W(OFFSET,offset);
	return B_OK;
}

status_t g400_crtc2_set_display_start(uint32 startadd,uint8 bpp) 
{
	LOG(4,("CRTC2: setting card RAM to be displayed bpp %d\n", bpp));

	LOG(2,("CRTC2: startadd: %x\n",startadd));
	LOG(2,("CRTC2: frameRAM: %x\n",si->framebuffer));
	LOG(2,("CRTC2: framebuffer: %x\n",si->fbc.frame_buffer));

	/*program the card!*/
	CR2W(STARTADD0,startadd);
	return B_OK;
}
