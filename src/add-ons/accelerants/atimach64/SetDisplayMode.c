#include <GraphicsDefs.h>

#include "GlobalData.h"
#include "generic.h"
#include "Mach64.h"
#include <sys/ioctl.h>


/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag) {
	status_t result;
	atimach64_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = ATIMACH64_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, ATIMACH64_RUN_INTERRUPTS, &sbs, sizeof(sbs));
}

/*
	Calculates the number of bits for a given color_space.
	Usefull for mode setup routines, etc.
*/
uint32 calcBitsPerPixel(uint32 cs) {
	uint32	bpp = 0;

	switch (cs) {
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			bpp = 32; break;
		case B_RGB24_BIG:
		case B_RGB24_LITTLE:
			bpp = 24; break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			bpp = 16; break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			bpp = 15; break;
		case B_CMAP8:
			bpp = 8; break;
	}
	return bpp;
}

/*
	The code to actually configure the display.  Unfortunately, there's not much
	that can be provided in the way of sample code.  If you're lucky, you're writing
	a driver for a device that has all (or at least most) of the bits for a particular
	configuration value in the same register, rather than spread out over the standard
	VGA registers + a zillion expansion bits.  In any case, I've found that the way
	to simplify this routine is to do all of the error checking in PROPOSE_DISPLAY_MODE(),
	and just assume that the values I get here are acceptable.
*/
static void do_set_display_mode(display_mode *dm) {

   mach64CRTCRegRec Regs;
  

   CalcCRTCRegs(&Regs, dm);

	/* disable interrupts using the kernel driver */
   interrupt_enable(false);

	SetCRTCRegs(&Regs, dm);

	/* Initialize RAMDAC */
	InitRAMDAC(calcBitsPerPixel(dm->space), dm->flags);

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

}

/*
	The exported mode setting routine.  First validate the mode, then call our
	private routine to hammer the registers.
*/
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) {
	display_mode bounds, target;

	/* ask for the specific mode */
	target = bounds = *mode_to_set;
	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;
   
	do_set_display_mode(&target);

	/* update shared info */
   target.h_display_start = 0;
   target.v_display_start = 0;   
	si->dm = target;

	/* calculate bytes per row ans set it */
	si->fbc.bytes_per_row = ((calcBitsPerPixel(target.space) + 1) / 8)
	  *target.virtual_width;

	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.  Used for page-flipping
	games and virtual desktops.
*/
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start) {
  int byte_offset;
	/*
	Many devices have limitations on the granularity of the horizontal offset.
	Make any checks for this here.  A future revision of the driver API will
	add a hook to return the granularity for a given display mode.
	*/
	/* most cards can handle multiples of 8 */
	if (h_display_start & 0x07)
		return B_ERROR;
	/* do not run past end of display */
	if ((si->dm.timing.h_display + h_display_start) > si->dm.virtual_width)
		return B_ERROR;
	if ((si->dm.timing.v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

   /* Calculate offset remebering to include the cursor area */
   byte_offset = (si->fbc.frame_buffer - si->framebuffer) >> 3;
   byte_offset += ((h_display_start + v_display_start*si->dm.virtual_width) *
                  (calcBitsPerPixel(si->dm.space) / 8)) >> 3;

	/* actually set the registers */   
   if (~(si->dm.flags & B_HARDWARE_CURSOR))
     SHOW_CURSOR(false);

   outw(CRTC_OFF_PITCH, (inw(CRTC_OFF_PITCH) & 0xfff00000) | byte_offset);

	return B_OK;
}

/*
	Set the indexed color palette.
*/
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {

	/*
	Some cards use the indexed color regisers in the DAC for gamma correction.
	If this is true with your device (and it probably is), you need to protect
	against setting these registers when not in an indexed mode.
	*/
	if (si->dm.space != B_CMAP8) return;

	/*
	There isn't any need to keep a copy of the data being stored, as the app_server
	will set the colors each time it switches to an 8bpp mode, or takes ownership
	of an 8bpp mode after a GameKit app has used it.
	*/

	/*
	You're on your own from here.  Chances are your code will look something like
	what follows:
	*/
	/*
	Due to the nature of the SAMPLE card, we need to do bytewise writes
	to the dac regs.  Stupid, but there you go.  So, we do it like we're
	supposed to.
	*/
	
   WaitQueue(1);
   
	outb(DAC_REGS, first);

	while (count--) {
		outb(DAC_REGS+1, *color_data++);
		outb(DAC_REGS+1, *color_data++);
		outb(DAC_REGS+1, *color_data++);
	}
}


/* masks for DPMS control bits */
enum {
	H_SYNC_OFF = 0x01,
	V_SYNC_OFF = 0x02,
	DISPLAY_OFF = 0x04,
	BITSMASK = H_SYNC_OFF | V_SYNC_OFF | DISPLAY_OFF
};

/*
	Put the display into one of the Display Power Management modes.
*/
status_t SET_DPMS_MODE(uint32 dpms_flags) {
	uint32 LTemp;

	/*
	The register containing the horizontal and vertical sync control bits
	usually contains other control bits, so do a read-modify-write.
	*/
   /* read bits */
	LTemp = inw(CRTC_GEN_CNTL);
	/* mask them */
	LTemp &= ~(CRTC_HSYNC_DIS | CRTC_VSYNC_DIS);	/* clear all disable bits (including display disable) */

	/* now pick one of the DPMS configurations */
	switch(dpms_flags) {
		case B_DPMS_ON:	/* H: on, V: on */
			/* do nothing, bits already clear */
			/* usually, but it may be different for your device */
			break;
		case B_DPMS_STAND_BY: /* H: off, V: on, display off */
			LTemp |= CRTC_HSYNC_DIS;
			break;
		case B_DPMS_SUSPEND: /* H: on, V: off, display off */
			LTemp |= CRTC_VSYNC_DIS;
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			LTemp |= CRTC_HSYNC_DIS | CRTC_VSYNC_DIS;
			break;
		default:
			return B_ERROR;
	}
	/* write the bits */
   outw(CRTC_GEN_CNTL, LTemp);

   snooze(10000);

	/*
	NOTE: if you're driving a device with a backlight (like a digital LCD),
	you may want to turn off the backlight here when in a DPMS power saving mode.
	*/
	if (dpms_flags == B_DPMS_ON)
		/* turn on the back light */
		;
	else
		/* turn off the back light */
		;

	return B_OK;
}

/*
	Report device DPMS capabilities.  Most newer cards can do it all.
	I've only seen one older card that can do a subset (ON and OFF).
	Very early cards may not be able to do this at all.
*/
uint32 DPMS_CAPABILITIES(void) {
	return 	B_DPMS_ON | B_DPMS_STAND_BY  | B_DPMS_SUSPEND | B_DPMS_OFF;
}


/*
	Return the current DPMS mode.
*/
uint32 DPMS_MODE(void) {
	uint32 LTemp;
	uint32 mode = B_DPMS_ON;

	/* read the control bits from the device */
	/* read the bits */
   LTemp = inw(CRTC_GEN_CNTL);
	
	/* what mode is set? */
	switch (LTemp & (CRTC_HSYNC_DIS | CRTC_VSYNC_DIS)) {
		case 0:	/* H: on, V: on */
			mode = B_DPMS_ON;
			break;
		case CRTC_HSYNC_DIS: /* H: off, V: on */
			mode = B_DPMS_STAND_BY;
			break;
		case CRTC_VSYNC_DIS: /* H: on, V: off */
			mode = B_DPMS_SUSPEND;
			break;
		case (CRTC_HSYNC_DIS | CRTC_VSYNC_DIS): /* H: off, V: off */
			mode = B_DPMS_OFF;
			break;
	}
	return mode;
}
