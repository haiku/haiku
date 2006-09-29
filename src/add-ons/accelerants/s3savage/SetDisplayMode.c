/*
    Copyright 1999, Be Incorporated.   All Rights Reserved.
    This file may be used under the terms of the Be Sample Code License.
*/
/*
 * Copyright 1999  Erdi Chen
 */

#include "GlobalData.h"
#include "generic.h"
#include "s3drv.h"
#include "s3accel.h"
#include "s3mmio.h"

#include <stdio.h>
#include <sys/ioctl.h>

/*
    Enable/Disable interrupts.  Just a wrapper around the
    ioctl() to the kernel driver.
    */
static void interrupt_enable(bool flag)
{
    status_t result;
    savage_set_bool_state sbs;

    /* set the magic number so the driver knows we're for real */
    sbs.magic = SAVAGE_PRIVATE_DATA_MAGIC;
    sbs.do_it = flag;
    /* contact driver and get a pointer to the registers and shared data */
    result = ioctl(fd, SAVAGE_RUN_INTERRUPTS, &sbs, sizeof(sbs));
}

/*
    Calculates the number of bits for a given color_space.
    Usefull for mode setup routines, etc.
    */
static uint32 calcBitsPerPixel(uint32 cs)
{
    uint32  bpp = 0;

    switch (cs)
    {
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
    The code to actually configure the display.
    Do all of the error checking in PROPOSE_DISPLAY_MODE(),
    and just assume that the values I get here are acceptable.
    */
static void do_set_display_mode(display_mode *dm)
{
    uint  bpp;
    SVGAMode s3mode;
    PixelTiming timing;
    status_t result;

    dpf ("do_set_display_mode begin\n");
    /* disable interrupts using the kernel driver */
    interrupt_enable(false);

    bpp = calcBitsPerPixel (dm->space);

    timing.dot_clock_khz = dm->timing.pixel_clock;
    timing.x.disp = dm->timing.h_display;
    timing.x.sync_start = dm->timing.h_sync_start;
    timing.x.sync_end = dm->timing.h_sync_end;
    timing.x.total = dm->timing.h_total;
    timing.x.polarity = (dm->timing.flags & B_POSITIVE_HSYNC) ? 1 : 0;
    timing.y.disp = dm->timing.v_display;
    timing.y.sync_start = dm->timing.v_sync_start;
    timing.y.sync_end = dm->timing.v_sync_end;
    timing.y.total = dm->timing.v_total;
    timing.y.polarity = (dm->timing.flags & B_POSITIVE_VSYNC) ? 1 : 0;
    dpf ("TIMING = { %d,  %d, %d, %d, %d, %d,  %d, %d, %d, %d, %d }\n",
         timing.dot_clock_khz,
         timing.x.disp, timing.x.sync_start, timing.x.sync_end,
         timing.x.total, timing.x.polarity,
         timing.y.disp, timing.y.sync_start, timing.y.sync_end,
         timing.y.total, timing.y.polarity);

    s3drv_unlock_regs ();
    memset (&s3mode, 0, sizeof (s3mode));
    result = s3drv_get_mode (&timing, bpp, (byte_t*)&s3mode, sizeof (s3mode));
    if (result > 0) dpf ("s3drv_get_mode succeeded\n");
    else  dpf ("s3drv_get_mode failed\n");
    dpf ("depth = %d, width = %d, height = %d\n",
         s3mode.pixel_size, s3mode.pixel_width, s3mode.pixel_height);
    result = s3drv_restore_mode ((byte_t*)&s3mode, sizeof (s3mode), true);
    if (result > 0) dpf ("s3drv_set_mode succeeded\n");
    else  dpf ("s3drv_set_mode failed\n");

    /* enable interrupts using the kernel driver */
    interrupt_enable(true);

    dpf ("si->start_addr = %d\n", si->start_addr);
    s3drv_set_display_start (si->start_addr);
    s3drv_set_logical_width (dm->virtual_width);
    si->fbc.bytes_per_row = (dm->virtual_width * bpp + 7) / 8;
    s3accel_init (s3drv_get_context ());
    dpf ("s3accel_init called\n");

    //s3drv_lock_regs ();
    dpf ("do_set_display_mode done\n");
}

/*
    The exported mode setting routine.  First validate the mode,
    then call our private routine to hammer the registers.
    */
status_t SET_DISPLAY_MODE (display_mode *mode_to_set)
{
    display_mode bounds, target;

    /* ask for the specific mode */
    target = bounds = *mode_to_set;
    if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
    	return B_ERROR;
    do_set_display_mode(&target);
    si->dm = target;
    return B_OK;
}

/*
    Set which pixel of the virtual frame buffer will show up in the
    top left corner of the display device.  Used for page-flipping
    games and virtual desktops.
    */
status_t MOVE_DISPLAY (uint16 h_display_start, uint16 v_display_start)
{
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

    /* actually set the registers */
    s3drv_adjust_viewport (h_display_start, v_display_start);

    return B_OK;
}

/*
    Set the indexed color palette.
    */
void SET_INDEXED_COLORS (
    uint count, uint8 first, uint8 *color_data, uint32 flags)
{
    /*
    Some cards use the indexed color regisers in the DAC for gamma correction.
    If this is true with your device (and it probably is), you need to protect
    against setting these registers when not in an indexed mode.
    */
    if (si->dm.space != B_CMAP8) return;

    /*
    There isn't any need to keep a copy of the data being stored,
    as the app_server will set the colors each time it switches to
    an 8bpp mode, or takes ownership of an 8bpp mode after a GameKit
    app has used it.
    */
    s3drv_unlock_regs ();
    write_sr ((byte_t*)regs + 0x8000, 0x1B, 0x10);
    while (count--)
    {
        s3drv_set_palette (
            first++, color_data[0], color_data[1], color_data[2]);
        color_data += 3;
    }
    s3drv_lock_regs ();
}


/* masks for DPMS control bits */
enum
{
    H_SYNC_OFF = 0x01,
    V_SYNC_OFF = 0x02,
    DISPLAY_OFF = 0x04,
    BITSMASK = H_SYNC_OFF | V_SYNC_OFF | DISPLAY_OFF
};

/*
    Put the display into one of the Display Power Management modes.
    */
status_t SET_DPMS_MODE(uint32 dpms_flags)
{
    uint32 LTemp;

    /*
    The register containing the horizontal and vertical sync control bits
    usually contains other control bits, so do a read-modify-write.
    */
    /* FIXME - read the bits */
    LTemp = *regs;
    /* this mask is device dependant */
    LTemp &= ~BITSMASK;	/* clear all disable bits (including display disable) */

    /* now pick one of the DPMS configurations */
    switch(dpms_flags)
    {
    	case B_DPMS_ON:	/* H: on, V: on */
    		/* do nothing, bits already clear */
    		/* usually, but it may be different for your device */
    		break;
    	case B_DPMS_STAND_BY: /* H: off, V: on, display off */
    		LTemp |= H_SYNC_OFF | DISPLAY_OFF;
    		break;
    	case B_DPMS_SUSPEND: /* H: on, V: off, display off */
    		LTemp |= V_SYNC_OFF | DISPLAY_OFF;
    		break;
    	case B_DPMS_OFF: /* H: off, V: off, display off */
    		LTemp |= H_SYNC_OFF | V_SYNC_OFF | DISPLAY_OFF;
    		break;
    	default:
    		return B_ERROR;
    }
    /* FIXME - write the bits */
    *regs = LTemp;

    /*
    NOTE: if you're driving a device with a backlight (like a digital
    LCD), you may want to turn off the backlight here when in a DPMS
    power saving mode.
    
    if (dpms_flags == B_DPMS_ON)
    	// turn on the back light 
    	;
    else
    	// turn off the back light
    	;
    */

    return B_OK;
}

/*
    Report device DPMS capabilities.  Most newer cards can do it all.
    I've only seen one older card that can do a subset (ON and OFF).
    Very early cards may not be able to do this at all.
    */
uint32 DPMS_CAPABILITIES (void)
{
    return B_DPMS_ON | B_DPMS_STAND_BY  | B_DPMS_SUSPEND | B_DPMS_OFF;
}


/*
    Return the current DPMS mode.
*/
uint32 DPMS_MODE (void)
{
    uint32 LTemp;
    uint32 mode = B_DPMS_ON;

    /* read the control bits from the device */
    /* FIXME - read the bits */
    LTemp = *regs;
    
    /* what mode is set? */
    switch (LTemp & (H_SYNC_OFF | V_SYNC_OFF))
    {
    	case 0:	/* H: on, V: on */
    		mode = B_DPMS_ON;
    		break;
    	case H_SYNC_OFF: /* H: off, V: on */
    		mode = B_DPMS_STAND_BY;
    		break;
    	case V_SYNC_OFF: /* H: on, V: off */
    		mode = B_DPMS_SUSPEND;
    		break;
    	case (H_SYNC_OFF | V_SYNC_OFF): /* H: off, V: off */
    		mode = B_DPMS_OFF;
    		break;
    }
    return mode;
}
