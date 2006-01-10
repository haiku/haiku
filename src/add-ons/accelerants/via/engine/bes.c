/* VIA Unichrome Back End Scaler functions */
/* Written by Rudolf Cornelissen 05/2002-1/2006 */

#define MODULE_BIT 0x00000200

#include "std.h"

typedef struct move_overlay_info move_overlay_info;

struct move_overlay_info
{
	uint32 hcoordv;		/* left and right edges of video output window */
	uint32 vcoordv;		/* top and bottom edges of video output window */
	uint32 hsrcstv;		/* horizontal source start in source buffer (clipping) */
	uint32 hsrcendv;	/* horizontal source end in source buffer (clipping) */
	uint32 v1srcstv;	/* vertical source start in source buffer (clipping) */
	uint32 a1orgv;		/* alternate source clipping via startadress of source buffer */
};

static void eng_bes_calc_move_overlay(move_overlay_info *moi);
static void eng_bes_program_move_overlay(move_overlay_info moi);

/* returns true if the current displaymode leaves enough bandwidth for overlay
 * support, false if not. */
bool eng_bes_chk_bandwidth()
{
	float refresh, bandwidth;
	uint8 depth;

	switch(si->dm.space)
	{
	case B_CMAP8:        depth =  8; break;
	case B_RGB15_LITTLE: depth = 16; break;
	case B_RGB16_LITTLE: depth = 16; break;
	case B_RGB32_LITTLE: depth = 32; break;
	default:
		LOG(8,("Overlay: Invalid colour depth 0x%08x\n", si->dm.space));
		return false;
	}

	refresh =
		(si->dm.timing.pixel_clock * 1000) /
		(si->dm.timing.h_total * si->dm.timing.v_total);
	bandwidth =
		si->dm.timing.h_display * si->dm.timing.v_display * refresh * depth;
	LOG(8,("Overlay: Current mode's refreshrate is %.2fHz, bandwidth is %.0f\n",
		refresh, bandwidth));

	switch (((CRTCR(MEMCLK)) & 0x70) >> 4)
	{
	case 0: /* SDR  66 */
	case 1: /* SDR 100 */
	case 2: /* SDR 133 */
		/* memory is too slow, sorry. */
		return false;
		break;
	case 3: /* DDR 100 */
		/* DDR100's basic limit... */
		if (bandwidth > 921600000.0) return false;
		/* ... but we have constraints at higher than 800x600 */
		if (si->dm.timing.h_display > 800)
		{
			if (depth != 8) return false;
			if (si->dm.timing.v_display > 768) return false;
			if (refresh > 60.2) return false;
		}
		break;
	case 4: /* DDR 133 */
		if (bandwidth > 4045440000.0) return false;
		break;
	default: /* not (yet?) used */
		return false;
		break;
	}

	return true;
}

/* move the overlay output window in virtualscreens */
/* Note:
 * si->dm.h_display_start and si->dm.v_display_start determine where the new
 * output window is located! */
void eng_bes_move_overlay()
{
	move_overlay_info moi;

	/* abort if overlay is not active */
	if (!si->overlay.active) return;

	eng_bes_calc_move_overlay(&moi);
	eng_bes_program_move_overlay(moi);
}

static void eng_bes_calc_move_overlay(move_overlay_info *moi)
{
	/* misc used variables */
	uint16 temp1, temp2;
	/* visible screen window in virtual workspaces */
	uint16 crtc_hstart, crtc_vstart, crtc_hend, crtc_vend;

	/* do 'overlay follow head' in dualhead modes on dualhead cards */
	if (si->ps.secondary_head)
	{
		switch (si->dm.flags & DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			if ((si->overlay.ow.h_start + (si->overlay.ow.width / 2)) <
					(si->dm.h_display_start + si->dm.timing.h_display))
				eng_bes_to_crtc(si->crtc_switch_mode);
			else
				eng_bes_to_crtc(!si->crtc_switch_mode);
			break;
		default:
				eng_bes_to_crtc(si->crtc_switch_mode);
			break;
		}
	}

	/* the BES does not respect virtual_workspaces, but adheres to CRTC
	 * constraints only */
	crtc_hstart = si->dm.h_display_start;
	/* make dualhead stretch and switch mode work while we're at it.. */
	if (si->overlay.crtc)
	{
		crtc_hstart += si->dm.timing.h_display;
	}

	/* horizontal end is the first position beyond the displayed range on the CRTC */
	crtc_hend = crtc_hstart + si->dm.timing.h_display;
	crtc_vstart = si->dm.v_display_start;
	/* vertical end is the first position beyond the displayed range on the CRTC */
	crtc_vend = crtc_vstart + si->dm.timing.v_display;


	/****************************************
	 *** setup all edges of output window ***
	 ****************************************/

	/* setup left and right edges of output window */
	moi->hcoordv = 0;
	/* left edge coordinate of output window, must be inside desktop */
	/* clipping on the left side */
	if (si->overlay.ow.h_start < crtc_hstart)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the right side */
		if (si->overlay.ow.h_start >= (crtc_hend - 1))
		{
			/* width < 2 is not allowed */
			temp1 = (crtc_hend - crtc_hstart - 2) & 0x7ff;
		} 
		else
		/* no clipping here */
		{
			temp1 = (si->overlay.ow.h_start - crtc_hstart) & 0x7ff;
		}
	} 
	moi->hcoordv |= temp1 << 16;
	/* right edge coordinate of output window, must be inside desktop */
	/* width < 2 is not allowed */
	if (si->overlay.ow.width < 2) 
	{
		temp2 = (temp1 + 1) & 0x7ff;
	}
	else 
	{
		/* clipping on the right side */
		if ((si->overlay.ow.h_start + si->overlay.ow.width - 1) > (crtc_hend - 1))
		{
			temp2 = (crtc_hend - crtc_hstart - 1) & 0x7ff;
		}
		else
		{
			/* clipping on the left side */
			if ((si->overlay.ow.h_start + si->overlay.ow.width - 1) < (crtc_hstart + 1))
			{
				/* width < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(si->overlay.ow.h_start + si->overlay.ow.width - crtc_hstart - 1)) & 0x7ff;
			}
		}
	}
	moi->hcoordv |= temp2 << 0;
	LOG(4,("Overlay: CRTC left-edge output %d, right-edge output %d\n",temp1, temp2));

	/* setup top and bottom edges of output window */
	moi->vcoordv = 0;
	/* top edge coordinate of output window, must be inside desktop */
	/* clipping on the top side */
	if (si->overlay.ow.v_start < crtc_vstart)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the bottom side */
		if (si->overlay.ow.v_start >= (crtc_vend - 1))
		{
			/* height < 2 is not allowed */
			temp1 = (crtc_vend - crtc_vstart - 2) & 0x7ff;
		} 
		else
		/* no clipping here */
		{
			temp1 = (si->overlay.ow.v_start - crtc_vstart) & 0x7ff;
		}
	} 
	moi->vcoordv |= temp1 << 16;
	/* bottom edge coordinate of output window, must be inside desktop */
	/* height < 2 is not allowed */
	if (si->overlay.ow.height < 2) 
	{
		temp2 = (temp1 + 1) & 0x7ff;
	}
	else 
	{
		/* clipping on the bottom side */
		if ((si->overlay.ow.v_start + si->overlay.ow.height - 1) > (crtc_vend - 1))
		{
			temp2 = (crtc_vend - crtc_vstart - 1) & 0x7ff;
		}
		else
		{
			/* clipping on the top side */
			if ((si->overlay.ow.v_start + si->overlay.ow.height - 1) < (crtc_vstart + 1))
			{
				/* height < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(si->overlay.ow.v_start + si->overlay.ow.height - crtc_vstart - 1)) & 0x7ff;
			}
		}
	}
	moi->vcoordv |= temp2 << 0;
	LOG(4,("Overlay: CRTC top-edge output %d, bottom-edge output %d\n",temp1, temp2));


	/*********************************
	 *** setup horizontal clipping ***
	 *********************************/

	/* Setup horizontal source start: first (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap.
	 * Then add the left starting position of the bitmap's view (zoom function) to get the final value needed.
	 * Note: The input bitmaps slopspace is automatically excluded from the calculations this way! */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */
	moi->hsrcstv = 0;
	/* check for destination horizontal clipping at left side */
	if (si->overlay.ow.h_start < crtc_hstart)
	{
		/* check if entire destination picture is clipping left:
		 * (2 pixels will be clamped onscreen at least) */
		if ((si->overlay.ow.h_start + si->overlay.ow.width - 1) < (crtc_hstart + 1))
		{
			/* increase 'first contributing pixel' with 'fixed value': (total dest. width - 2) */
			moi->hsrcstv += (si->overlay.ow.width - 2);
		}
		else
		{
			/* increase 'first contributing pixel' with actual number of dest. clipping pixels */
			moi->hsrcstv += (crtc_hstart - si->overlay.ow.h_start);
		}
		LOG(4,("Overlay: clipping left...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		moi->hsrcstv *= si->overlay.h_ifactor;
	}
	/* take zoom into account */
	moi->hsrcstv += ((uint32)si->overlay.my_ov.h_start) << 16;
	/* AND below required by hardware */
	moi->hsrcstv &= 0x03fffffc;
	LOG(4,("Overlay: first hor. (sub)pixel of input bitmap contributing %f\n", moi->hsrcstv / (float)65536));

	/* Setup horizontal source end: last (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap.
	 * Then add the right ending position of the bitmap's view (zoom function) to get the final value needed. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source end pos.! */

	moi->hsrcendv = 0;
	/* check for destination horizontal clipping at right side */
	if ((si->overlay.ow.h_start + si->overlay.ow.width - 1) > (crtc_hend - 1))
	{
		/* check if entire destination picture is clipping right:
		 * (2 pixels will be clamped onscreen at least) */
		if (si->overlay.ow.h_start > (crtc_hend - 2))
		{
			/* increase 'number of clipping pixels' with 'fixed value': (total dest. width - 2) */
			moi->hsrcendv += (si->overlay.ow.width - 2);
		}
		else
		{
			/* increase 'number of clipping pixels' with actual number of dest. clipping pixels */
			moi->hsrcendv += ((si->overlay.ow.h_start + si->overlay.ow.width - 1) - (crtc_hend - 1));
		}
		LOG(4,("Overlay: clipping right...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		moi->hsrcendv *= si->overlay.h_ifactor;
		/* now subtract this value from the last used pixel in (zoomed) inputbuffer, aligned to BES */
		moi->hsrcendv = (((uint32)((si->overlay.my_ov.h_start + si->overlay.my_ov.width) - 1)) << 16) - moi->hsrcendv;
	}
	else
	{
		/* set last contributing pixel to last used pixel in (zoomed) inputbuffer, aligned to BES */
		moi->hsrcendv = (((uint32)((si->overlay.my_ov.h_start + si->overlay.my_ov.width) - 1)) << 16);
	}
	/* AND below required by hardware */
	moi->hsrcendv &= 0x03ffffff;
	LOG(4,("Overlay: last horizontal (sub)pixel of input bitmap contributing %f\n", moi->hsrcendv / (float)65536));


	/*******************************
	 *** setup vertical clipping ***
	 *******************************/

	/* calculate inputbitmap origin adress */
	moi->a1orgv = (uint32)((vuint32 *)si->overlay.ob.buffer);
	moi->a1orgv -= (uint32)((vuint32 *)si->framebuffer);
	LOG(4,("Overlay: topleft corner of input bitmap (cardRAM offset) $%08x\n", moi->a1orgv));

	/* Setup vertical source start: first (sub)pixel contributing to output picture. */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * 'After' this is done, include the scaling factor so you get a value based on the input bitmap. 
	 * Then add the top starting position of the bitmap's view (zoom function) to get the final value needed. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */

	moi->v1srcstv = 0;
	/* check for destination vertical clipping at top side */
	if (si->overlay.ow.v_start < crtc_vstart)
	{
		/* check if entire destination picture is clipping at top:
		 * (2 pixels will be clamped onscreen at least) */
		if ((si->overlay.ow.v_start + si->overlay.ow.height - 1) < (crtc_vstart + 1))
		{
			/* increase 'number of clipping pixels' with 'fixed value':
			 * 'total height - 2' of dest. picture in pixels * inverse scaling factor */
			moi->v1srcstv = (si->overlay.ow.height - 2) * si->overlay.v_ifactor;
			/* we need to do clipping in the source bitmap because no seperate clipping
			 * registers exist... */
			moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		else
		{
			/* increase 'first contributing pixel' with:
			 * number of destination picture clipping pixels * inverse scaling factor */
			moi->v1srcstv = (crtc_vstart - si->overlay.ow.v_start) * si->overlay.v_ifactor;
			/* we need to do clipping in the source bitmap because no seperate clipping
			 * registers exist... */
			moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		LOG(4,("Overlay: clipping at top...\n"));
	}
	/* take zoom into account */
	moi->v1srcstv += (((uint32)si->overlay.my_ov.v_start) << 16);
	moi->a1orgv += (si->overlay.my_ov.v_start * si->overlay.ob.bytes_per_row);
	LOG(4,("Overlay: 'contributing part of buffer' origin is (cardRAM offset) $%08x\n", moi->a1orgv));
	LOG(4,("Overlay: first vert. (sub)pixel of input bitmap contributing %f\n", moi->v1srcstv / (float)65536));

	/* AND below is probably required by hardware. */
	/* Buffer A topleft corner of field 1 (origin)(field 1 contains our full frames) */
	moi->a1orgv &= 0x07fffff0;
}

static void eng_bes_program_move_overlay(move_overlay_info moi)
{
	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Done in card hardware:
	 * double buffered registers + trigger during 'BES-'VBI feature. */


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	/* setup clipped(!) buffer startadress in RAM */
	/* VIA bes doesn't have clipping registers, so no subpixelprecise clipping
	 * either. We do pixelprecise vertical and 'two pixel' precise horizontal clipping here. */
	/* first include 'pixel precise' left clipping... (top clipping was already included) */
	moi.a1orgv += ((moi.hsrcstv >> 16) * 2);
	/* we need to step in 4-byte (2 pixel) granularity due to the nature of yuy2 */
	BESW(VID1Y_ADDR0, (moi.a1orgv & 0x07fffffc));

	/* horizontal source end does not use subpixelprecision: granularity is 8 pixels */
	/* notes:
	 * - make absolutely sure the engine can fetch the last pixel needed from
	 *   the sourcebitmap even if only to generate a tiny subpixel from it!
	 * - the engine uses byte format instead of pixel format;
	 * - the engine uses 16 bytes, so 8 pixels granularity. */
	BESW(VID1_FETCH, (((((moi.hsrcendv >> 16) + 1 + 0x0007) & ~0x0007) * 2) << (20 - 4)));

	/* setup output window position */
	BESW(VID1_HVSTART, ((moi.hcoordv & 0xffff0000) | ((moi.vcoordv & 0xffff0000) >> 16)));

	/* setup output window size */
	BESW(VID1_SIZE, (((moi.hcoordv & 0x0000ffff) << 16) | (moi.vcoordv & 0x0000ffff)));

	/* enable colorkeying (b0 = 1), disable chromakeying (b1 = 0), Vid1 on top of Vid3 (b20 = 0),
	 * all registers are loaded during the next 'BES-'VBI (b28 = 1), Vid1 cmds fire (b31 = 1) */
	BESW(COMPOSE, 0x90000001);
}

status_t eng_bes_to_crtc(bool crtc)
{
	if (si->ps.secondary_head)
	{
		if (crtc)
		{
			LOG(4,("Overlay: switching overlay to CRTC2\n"));
			/* switch overlay engine to CRTC2 */
//			ENG_REG32(RG32_FUNCSEL) &= ~0x00001000;
//			ENG_REG32(RG32_2FUNCSEL) |= 0x00001000;
			si->overlay.crtc = !si->crtc_switch_mode;
		}
		else
		{
			LOG(4,("Overlay: switching overlay to CRTC1\n"));
			/* switch overlay engine to CRTC1 */
//			ENG_REG32(RG32_2FUNCSEL) &= ~0x00001000;
//			ENG_REG32(RG32_FUNCSEL) |= 0x00001000;
			si->overlay.crtc = si->crtc_switch_mode;
		}
		return B_OK;
	}
	else
	{
		return B_ERROR;
	}
}

status_t eng_bes_init()
{
	if (si->ps.chip_rev < 0x10)
	{
		/* select colorspace setup for B_YCbCr422 */
		BESW(VID1_COLSPAC1, 0x140020f2);
		BESW(VID1_COLSPAC2, 0x0a0a2c00);
		/* fifo depth is $20 (b0-5), threshold $10 (b8-13), prethreshold $1d (b24-29) */
		BESW(VID1_FIFO, 0x1d00101f);
	}
	else
	{
		/* select colorspace setup for B_YCbCr422 */
		BESW(VID1_COLSPAC1, 0x13000ded);
		BESW(VID1_COLSPAC2, 0x13171000);
		/* fifo depth is $40 (b0-5), threshold $38 (b8-13), prethreshold $38 (b24-29) */
		BESW(VID1_FIFO, 0x3800383f);
	}

		/* disable overlay ints (b0 = buffer 0, b4 = buffer 1) */
//		BESW(NV04_INTE, 0x00000000);
		/* shut off GeForce4MX MPEG2 decoder */
//		BESW(DEC_GENCTRL, 0x00000000);
		/* setup BES memory-range mask */
//		BESW(NV10_0MEMMASK, (si->ps.memory_size - 1));
		/* setup brightness, contrast and saturation to be 'neutral' */
//		BESW(NV10_0BRICON, ((0x1000 << 16) | 0x1000));
//		BESW(NV10_0SAT, ((0x0000 << 16) | 0x1000));

	return B_OK;
}

status_t eng_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov, int offset)
{
	/* yuy2 (4:2:2) colorspace calculations */

	/* Note:
	 * in BeOS R5.0.3 and DANO:
	 * 'ow->offset_xxx' is always 0, so not used;
	 * 'ow->width' and 'ow->height' are the output window size: does not change
	 * if window is clipping;
	 * 'ow->h_start' and 'ow->v_start' are the left-top position of the output
	 * window. These values can be negative: this means the window is clipping
	 * at the left or the top of the display, respectively. */

	/* 'ov' is the view in the source bitmap, so which part of the bitmap is actually
	 * displayed on screen. This is used for the 'hardware zoom' function. */
 
	/* output window position and clipping info for source buffer */
	move_overlay_info moi;
	/* calculated BES register values */
	uint32 	hiscalv, viscalv;
	/* interval representation, used for scaling calculations */
	uint16 intrep;
	/* inverse scaling factor, used for source positioning */
	uint32 ifactor;
	/* copy of overlay view which has checked valid values */
	overlay_view my_ov;
	/* true if scaling needed */
	bool scale_x, scale_y;
	/* for computing scaling register value */
	uint32 scaleval;
	/* for computing 'pre-scaling' on downscaling */
	uint32 minictrl;

	/**************************************************************************************
	 *** copy, check and limit if needed the user-specified view into the intput bitmap ***
	 **************************************************************************************/
	my_ov = *ov;
	/* check for valid 'coordinates' */
	if (my_ov.width == 0) my_ov.width++;
	if (my_ov.height == 0) my_ov.height++;
	if (my_ov.h_start > ((ob->width - si->overlay.myBufInfo[offset].slopspace) - 1))
		my_ov.h_start = ((ob->width - si->overlay.myBufInfo[offset].slopspace) - 1);
	if (((my_ov.h_start + my_ov.width) - 1) > ((ob->width - si->overlay.myBufInfo[offset].slopspace) - 1))
		my_ov.width = ((((ob->width - si->overlay.myBufInfo[offset].slopspace) - 1) - my_ov.h_start) + 1);
	if (my_ov.v_start > (ob->height - 1))
		my_ov.v_start = (ob->height - 1);
	if (((my_ov.v_start + my_ov.height) - 1) > (ob->height - 1))
		my_ov.height = (((ob->height - 1) - my_ov.v_start) + 1);

	LOG(4,("Overlay: inputbuffer view (zoom) left %d, top %d, width %d, height %d\n",
		my_ov.h_start, my_ov.v_start, my_ov.width, my_ov.height));

	/* save for eng_bes_calc_move_overlay() */
	si->overlay.ow = *ow;
	si->overlay.ob = *ob;
	si->overlay.my_ov = my_ov;


	/********************************
	 *** setup horizontal scaling ***
	 ********************************/
	LOG(4,("Overlay: total input picture width = %d, height = %d\n",
			(ob->width - si->overlay.myBufInfo[offset].slopspace), ob->height));
	LOG(4,("Overlay: output picture width = %d, height = %d\n", ow->width, ow->height));

	/* preset X and Y prescaling to be 1x */
	minictrl = 0x00000000;
	/* determine interval representation value, taking zoom into account */
	if (ow->flags & B_OVERLAY_HORIZONTAL_FILTERING)
	{
		/* horizontal filtering is ON */
		if ((my_ov.width == ow->width) | (ow->width < 2))
		{
			/* no horizontal scaling used, OR destination width < 2 */
			intrep = 0;
		}
		else
		{
			intrep = 1;
		}
	}
	else
	{
		/* horizontal filtering is OFF */
		if ((ow->width < my_ov.width) & (ow->width >= 2))
		{
			/* horizontal downscaling used AND destination width >= 2 */
			intrep = 1;
		}
		else
		{
			intrep = 0;
		}
	}
	LOG(4,("Overlay: horizontal interval representation value is %d\n",intrep));

	/* calculate inverse horizontal scaling factor, taking zoom into account */
	/* standard scaling formula: */
	ifactor = (((uint32)(my_ov.width - intrep)) << 16) / (ow->width - intrep); 

	/* correct factor to prevent most-right visible 'line' from distorting */
	ifactor -= (1 << 5);
	hiscalv = ifactor;
	/* save for eng_bes_calc_move_overlay() */
	si->overlay.h_ifactor = ifactor;
	LOG(4,("Overlay: horizontal scaling factor is %f\n", (float)65536 / ifactor));

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* all cards have a upscaling limit of 8.0 (see official nVidia specsheets) */
	//fixme: checkout...
	if (hiscalv < 0x00002000)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		hiscalv = 0x00002000;
		LOG(4,("Overlay: horizontal scaling factor too large, clamping at %f\n", (float)65536 / hiscalv));
	}
	/* VIA has a 'downscaling' limit of 1.0, but seperate prescaling to 1/16th can be done.
	 * (X-scaler has 11bit register with 0.11 format value, with special 1.0 scaling factor setting;
	 *  prescaler has fixed 1x, 1/2x, 1/4x, 1/8x and 1/16x settings.) */
	if (hiscalv > 0x00100000)
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		hiscalv = 0x00100000;
		LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)2048 / (hiscalv >> 5)));
	}

	/* setup pre-downscaling if 'requested' */
	if ((hiscalv > 0x00010000) && (hiscalv <= 0x00020000))
	{
		/* instruct BES to horizontal prescale 0.5x */
		minictrl |= 0x01000000;
		/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
		hiscalv >>= 1;
	}
	else 
		if ((hiscalv > 0x00020000) && (hiscalv <= 0x00040000))
		{
			/* instruct BES to horizontal prescale 0.25x */
			minictrl |= 0x03000000;
			/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
			hiscalv >>= 2;
		}
		else
			if ((hiscalv > 0x00040000) && (hiscalv <= 0x00080000))
			{
				/* instruct BES to horizontal prescale 0.125x */
				minictrl |= 0x05000000;
				/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
				hiscalv >>= 3;
			}
			else
				if ((hiscalv > 0x00080000) && (hiscalv <= 0x00100000))
				{
					/* instruct BES to horizontal prescale 0.125x */
					minictrl |= 0x07000000;
					/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
					hiscalv >>= 4;
				}

	/* only instruct normal scaler to scale if it must do so */
	scale_x = true;
	if (hiscalv == 0x00010000) scale_x = false;

	/* AND below is required by hardware */
	hiscalv &= 0x0000ffe0;


	/******************************
	 *** setup vertical scaling ***
	 ******************************/

	/* determine interval representation value, taking zoom into account */
	if (ow->flags & B_OVERLAY_VERTICAL_FILTERING)
	{
		/* vertical filtering is ON */
		if ((my_ov.height == ow->height) | (ow->height < 2))
		{
			/* no vertical scaling used, OR destination height < 2 */
			intrep = 0;
		}
		else
		{
			intrep = 1;
		}
	}
	else
	{
		/* vertical filtering is OFF */
		if ((ow->height < my_ov.height) & (ow->height >= 2))
		{
			/* vertical downscaling used AND destination height >= 2 */
			intrep = 1;
		}
		else
		{
			intrep = 0;
		}
	}
	LOG(4,("Overlay: vertical interval representation value is %d\n",intrep));

	/* calculate inverse vertical scaling factor, taking zoom into account */
	/* standard scaling formula: */
	ifactor = (((uint32)(my_ov.height - intrep)) << 16) / (ow->height - intrep); 

	/* correct factor to prevent lowest visible line from distorting */
	ifactor -= (1 << 6);
	LOG(4,("Overlay: vertical scaling factor is %f\n", (float)65536 / ifactor));

	/* preserve ifactor for source positioning calculations later on */
	viscalv = ifactor;
	/* save for eng_bes_calc_move_overlay() */
	si->overlay.v_ifactor = ifactor;

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* all cards have a upscaling limit of 8.0 (see official nVidia specsheets) */
	//fixme: checkout...
	if (viscalv < 0x00002000)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		viscalv = 0x00002000;
		LOG(4,("Overlay: vertical scaling factor too large, clamping at %f\n", (float)65536 / viscalv));
	}
	/* VIA has a 'downscaling' limit of 1.0, but seperate prescaling to 1/16th can be done.
	 * (Y-scaler has 10bit register with 0.10 format value, with special 1.0 scaling factor setting;
	 *  prescaler has fixed 1x, 1/2x, 1/4x, 1/8x and 1/16x settings.) */
	if (viscalv > 0x00100000)
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		viscalv = 0x00100000;
		LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)1024 / (viscalv >> 6)));
	}

	/* setup pre-downscaling if 'requested' */
	if ((viscalv > 0x00010000) && (viscalv <= 0x00020000))
	{
		/* instruct BES to horizontal prescale 0.5x */
		minictrl |= 0x00010000;
		/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
		viscalv >>= 1;
	}
	else 
		if ((viscalv > 0x00020000) && (viscalv <= 0x00040000))
		{
			/* instruct BES to horizontal prescale 0.25x */
			minictrl |= 0x00030000;
			/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
			viscalv >>= 2;
		}
		else
			if ((viscalv > 0x00040000) && (viscalv <= 0x00080000))
			{
				/* instruct BES to horizontal prescale 0.125x */
				minictrl |= 0x00050000;
				/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
				viscalv >>= 3;
			}
			else
				if ((viscalv > 0x00080000) && (viscalv <= 0x00100000))
				{
					/* instruct BES to horizontal prescale 0.125x */
					minictrl |= 0x00070000;
					/* correct normal scalingfactor so total scaling is 0.5 <= factor < 1.0x */
					viscalv >>= 4;
				}

	/* only instruct normal scaler to scale if it must do so */
	scale_y = true;
	if (viscalv == 0x00010000) scale_y = false;

	/* AND below is required by hardware */
	viscalv &= 0x0000ffc0;


	/********************************************************************************
	 *** setup all edges of output window, setup horizontal and vertical clipping ***
	 ********************************************************************************/
	eng_bes_calc_move_overlay(&moi);


	/*****************************
	 *** log color keying info ***
	 *****************************/

	LOG(4,("Overlay: key_red %d, key_green %d, key_blue %d, key_alpha %d\n",
		ow->red.value, ow->green.value, ow->blue.value, ow->alpha.value));
	LOG(4,("Overlay: mask_red %d, mask_green %d, mask_blue %d, mask_alpha %d\n",
		ow->red.mask, ow->green.mask, ow->blue.mask, ow->alpha.mask));


	/*****************
	 *** log flags ***
	 *****************/

	LOG(4,("Overlay: ow->flags is $%08x\n",ow->flags));
	/* BTW: horizontal and vertical filtering are fixed and turned on for GeForce overlay. */


	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Done in card hardware:
	 * double buffered registers + trigger during 'BES-'VBI feature. */


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	/* setup clipped(!) buffer startadress in RAM */
	/* VIA bes doesn't have clipping registers, so no subpixelprecise clipping
	 * either. We do pixelprecise vertical and 'two pixel' precise horizontal clipping here. */
	/* first include 'pixel precise' left clipping... (top clipping was already included) */
	moi.a1orgv += ((moi.hsrcstv >> 16) * 2);
	/* we need to step in 4-byte (2 pixel) granularity due to the nature of yuy2 */
	BESW(VID1Y_ADDR0, (moi.a1orgv & 0x07fffffc));

	/* horizontal source end does not use subpixelprecision: granularity is 8 pixels */
	/* notes:
	 * - make absolutely sure the engine can fetch the last pixel needed from
	 *   the sourcebitmap even if only to generate a tiny subpixel from it!
	 * - the engine uses byte format instead of pixel format;
	 * - the engine uses 16 bytes, so 8 pixels granularity. */
	BESW(VID1_FETCH, (((((moi.hsrcendv >> 16) + 1 + 0x0007) & ~0x0007) * 2) << (20 - 4)));

	/* enable horizontal filtering if asked for */
	if (ow->flags & B_OVERLAY_HORIZONTAL_FILTERING)
	{
		minictrl |= (1 << 1);
		LOG(4,("Overlay: using horizontal interpolation on scaling\n"));
	}
	/* enable vertical filtering if asked for */
	if (ow->flags & B_OVERLAY_VERTICAL_FILTERING)
	{
		/* vertical interpolation b0, interpolation on Y, Cb and Cr all (b2) */
		minictrl |= ((1 << 2) | (1 << 0));
		LOG(4,("Overlay: using vertical interpolation on scaling\n"));
	}
	/* and program horizontal and vertical 'prescaling' for downscaling */
	BESW(VID1_MINI_CTL, minictrl);

	/* setup buffersize */
	BESW(V1_SOURCE_WH, ((ob->height << 16) | (ob->width)));

	/* setup buffer source pitch including slopspace (in bytes) */
	BESW(VID1_STRIDE, (ob->width * 2));

	/* setup output window position */
	BESW(VID1_HVSTART, ((moi.hcoordv & 0xffff0000) | ((moi.vcoordv & 0xffff0000) >> 16)));

	/* setup output window size */
	BESW(VID1_SIZE, (((moi.hcoordv & 0x0000ffff) << 16) | (moi.vcoordv & 0x0000ffff)));

	/* setup horizontal and vertical scaling:
	 * setup horizontal scaling enable (b31), setup vertical scaling enable (b15).
	 * Note:
	 * Vertical scaling has a different resolution than horizontal scaling(!).  */
	scaleval = 0x00000000;
	if (scale_x) scaleval |= 0x80000000;
	if (scale_y) scaleval |= 0x00008000;
	BESW(VID1_ZOOM, (scaleval | ((hiscalv << 16) >> 5) | (viscalv >> 6)));

	if (si->ps.chip_rev < 0x10)
	{
		/* enable BES (b0), format yuv422 (b2-4 = %000), set colorspace sign (b7 = 1),
		 * input is frame (not field) picture (b9 = 0), expire = $5 (b16-19),
		 * select field (not frame)(!) base (b24 = 0) */
		BESW(VID1_CTL, 0x00050081);
	}
	else
	{
		/* enable BES (b0), format yuv422 (b2-4 = %000), set colorspace sign (b7 = 1),
		 * input is frame (not field) picture (b9 = 0), expire = $f (b16-19),
		 * select field (not frame)(!) base (b24 = 0) */
		BESW(VID1_CTL, 0x000f0081);
	}


	/**************************
	 *** setup color keying ***
	 **************************/

	/* setup colorkeying */
	switch(si->dm.space)
	{
	case B_CMAP8:
		{
			/* do color palette index lookup for current colorkey */
			/* note:
			 * since apparantly some hardware works with color indexes instead of colors,
			 * it might be a good idea(!!) to include the colorindex in the system's
			 * overlay_window struct. */
			static uint8 *r,*g,*b;
			static uint32 idx;
			r = si->color_data;
			g = r + 256;
			b = g + 256;
			/* if index 1 doesn't help us, we assume 0 will (got to program something anyway) */
			//fixme, note, tweakalert:
			//I'm counting down for a reason:
			//BeOS assigns the color white (0x00ffffff) to two indexes in the palette:
			//index 0x3f and 0xff. In the framebuffer index 0xff is used (apparantly).
			//The hardware compares framebuffer to given key, so the BES must receive 0xff.
			for (idx = 255; idx > 0; idx--)
			{
				if ((r[idx] == ow->red.value) &&
					(g[idx] == ow->green.value) &&
					(b[idx] == ow->blue.value))
						break;
			}
			LOG(4,("Overlay: colorkey's palette index is $%02x\n", idx));
			/* program color palette index into BES engine */
			BESW(COLKEY, idx);
		}
		break;
	case B_RGB15_LITTLE:
		BESW(COLKEY, (
			((ow->blue.value & ow->blue.mask) << 0)   |
			((ow->green.value & ow->green.mask) << 5) |
			((ow->red.value & ow->red.mask) << 10)
			/* alpha keying is not supported here */
			));
		break;
	case B_RGB16_LITTLE:
		BESW(COLKEY, (
			((ow->blue.value & ow->blue.mask) << 0)   |
			((ow->green.value & ow->green.mask) << 5) |
			((ow->red.value & ow->red.mask) << 11)
			/* this space has no alpha bits */
			));
		break;
	case B_RGB32_LITTLE:
	default:
		BESW(COLKEY, (
			((ow->blue.value & ow->blue.mask) << 0)   |
			((ow->green.value & ow->green.mask) << 8) |
			((ow->red.value & ow->red.mask) << 16)
			/* alpha keying is not supported here */
			));
		break;
	}

	/* disable chromakeying (b1 = 0), Vid1 on top of Vid3 (b20 = 0),
	 * all registers are loaded during the next 'BES-'VBI (b28 = 1), Vid1 cmds fire (b31 = 1) */
	if (ow->flags & B_OVERLAY_COLOR_KEY)
	{
		/* enable colorkeying (b0 = 1) */
		BESW(COMPOSE, 0x90000001);
	}
	else
	{
		/* disable colorkeying (b0 = 0) */
		BESW(COMPOSE, 0x90000000);
	}

	/* note that overlay is in use (for eng_bes_move_overlay()) */
	si->overlay.active = true;

	return B_OK;
}

status_t eng_release_bes()
{
	/* setup BES control: disable scaler (b0 = 0) */
	BESW(VID1_CTL, 0x00000000);

	/* make sure the 'disable' command really gets executed: (no 'VBI' anymore if BES disabled) */
	/* all registers are loaded immediately (b29 = 1), Vid1 cmds fire (b31 = 1) */
	BESW(COMPOSE, 0xa0000000);

	/* note that overlay is not in use (for eng_bes_move_overlay()) */
	si->overlay.active = false;

	return B_OK;
}
