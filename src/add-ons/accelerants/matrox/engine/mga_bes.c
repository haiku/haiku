/* G200-G550 Back End Scaler functions */
/* Written by Rudolf Cornelissen 05/2002-11/2009 */

#define MODULE_BIT 0x00000200

#include "mga_std.h"

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

static void gx00_bes_calc_move_overlay(move_overlay_info *moi);
static void gx00_bes_program_move_overlay(move_overlay_info moi);

/* move the overlay output window in virtualscreens */
/* Note:
 * si->dm.h_display_start and si->dm.v_display_start determine where the new
 * output window is located! */
void gx00_bes_move_overlay()
{
	move_overlay_info moi;

	/* abort if overlay is not active */
	if (!si->overlay.active) return;

	gx00_bes_calc_move_overlay(&moi);
	gx00_bes_program_move_overlay(moi);
}

static void gx00_bes_calc_move_overlay(move_overlay_info *moi)
{
	/* misc used variables */
	uint16 temp1, temp2;
	/* visible screen window in virtual workspaces */
	uint16 crtc_hstart, crtc_vstart, crtc_hend, crtc_vend;

	/* the BES does not respect virtual_workspaces, but adheres to CRTC
	 * constraints only */
	crtc_hstart = si->dm.h_display_start;
	/* make dualhead switch mode with TVout enabled work while we're at it.. */
	if (si->switched_crtcs)
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
	moi->hsrcstv &= 0x07fffffc;
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
	/* AND below required by hardware (confirmed G200 can do upto 1024 pixels, G450 and G550 can do above.) */
	moi->hsrcendv &= 0x07fffffc;
	LOG(4,("Overlay: last horizontal (sub)pixel of input bitmap contributing %f\n", moi->hsrcendv / (float)65536));


	/*******************************
	 *** setup vertical clipping ***
	 *******************************/

	/* Setup vertical source start: first (sub)pixel contributing to output picture. */
	/* Note: this exists of two parts:
	 * 1. setup fractional part (sign is always 'positive');
	 * 2. setup relative base_adress, taking clipping on top (and zoom) into account. 
	 * Both parts are done intertwined below. */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * 'After' this is done, include the scaling factor so you get a value based on the input bitmap. 
	 * Then add the top starting position of the bitmap's view (zoom function) to get the final value needed. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */

	/* calculate relative base_adress and 'vertical weight fractional part' */
	moi->v1srcstv = 0;
	/* calculate origin adress */
	moi->a1orgv = (uint32)((vuint32 *)si->overlay.ob.buffer);
	moi->a1orgv -= (uint32)((vuint32 *)si->framebuffer);
	LOG(4,("Overlay: topleft corner of input bitmap (cardRAM offset) $%08x\n", moi->a1orgv));
	/* check for destination vertical clipping at top side */
	if (si->overlay.ow.v_start < crtc_vstart)
	{
		/* check if entire destination picture is clipping at top:
		 * (2 pixels will be clamped onscreen at least) */
		if ((si->overlay.ow.v_start + si->overlay.ow.height - 1) < (crtc_vstart + 1))
		{
			/* increase source buffer origin with 'fixed value':
			 * (integer part of ('total height - 2' of dest. picture in pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			moi->v1srcstv = (si->overlay.ow.height - 2) * si->overlay.v_ifactor;
			moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		else
		{
			/* increase source buffer origin with:
			 * (integer part of (number of destination picture clipping pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			moi->v1srcstv = (crtc_vstart - si->overlay.ow.v_start) * si->overlay.v_ifactor;
			moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		LOG(4,("Overlay: clipping at top...\n"));
	}
	/* take zoom into account */
	moi->v1srcstv += (((uint32)si->overlay.my_ov.v_start) << 16);
	moi->a1orgv += (si->overlay.my_ov.v_start * si->overlay.ob.bytes_per_row);
	LOG(4,("Overlay: 'contributing part of buffer' origin is (cardRAM offset) $%08x\n", moi->a1orgv));
	LOG(4,("Overlay: first vert. (sub)pixel of input bitmap contributing %f\n", moi->v1srcstv / (float)65536));

	/* Note:
	 * Because all > G200 overlay units will ignore b0-3 of the calculated adress,
	 * we do not use the above way for horizontal source positioning.
	 * (G200 cards ignore b0-2.)
	 * If we did, 8 source-image pixel jumps (in 4:2:2 colorspace) will occur if the picture
	 * is shifted horizontally during left clipping on all > G200 cards, while G200 cards
	 * will have 4 source-image pixel jumps occuring. */

	/* AND below is required by G200-G550 hardware. > G200 cards can have max. 32Mb RAM on board
	 * (16Mb on G200 cards). Compatible setting used (between G200 and the rest), this has no
	 * downside consequences here. */
	/* Buffer A topleft corner of field 1 (origin)(field 1 contains our full frames) */
	moi->a1orgv &= 0x01fffff0;

	/* field 1 weight: AND below required by hardware, also make sure 'sign' is always 'positive' */
	moi->v1srcstv &= 0x0000fffc;
}

static void gx00_bes_program_move_overlay(move_overlay_info moi)
{
	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Make sure reprogramming the BES completes before the next retrace occurs,
	 * to prevent register-update glitches (double buffer feature). */

	LOG(3,("Overlay: starting register programming beyond Vcount %d\n", CR1R(VCOUNT)));
	/* Even at 1600x1200x90Hz, a single line still takes about 9uS to complete:
	 * this resolution will generate about 180Mhz pixelclock while we can do
	 * upto 360Mhz. So snooze about 4uS to prevent bus-congestion... 
	 * Appr. 200 lines time will provide enough room even on a 100Mhz CPU if it's
	 * screen is set to the highest refreshrate/resolution possible. */
	while ((uint16)CR1R(VCOUNT) > (si->dm.timing.v_total - 200)) snooze(4);


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	BESW(HCOORD, moi.hcoordv);
	BESW(VCOORD, moi.vcoordv);
	BESW(HSRCST, moi.hsrcstv);
	BESW(HSRCEND, moi.hsrcendv);
	BESW(A1ORG, moi.a1orgv);
	BESW(V1WGHT, moi.v1srcstv);

	/* on a 500Mhz P3 CPU just logging a line costs 400uS (18-19 vcounts at 1024x768x60Hz)!
	 * programming the registers above actually costs 180uS here */
	LOG(3,("Overlay: completed at Vcount %d\n", CR1R(VCOUNT)));
}

status_t gx00_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov, int offset)
{
	/* yuy2 (4:2:2) colorspace calculations */
	/* Note: Some calculations will have to be modified for other colorspaces if they are incorporated. */

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
	uint32 	hiscalv, hsrclstv, viscalv, v1srclstv, globctlv, ctlv;
	/* interval representation, used for scaling calculations */
	uint16 intrep;
	/* inverse scaling factor, used for source positioning */
	uint32 ifactor;
	/* copy of overlay view which has checked valid values */
	overlay_view my_ov;

	/* Slowdown the G200-G550 BES if the pixelclock is too high for it to cope.
	 * This will in fact half the horizontal resolution of the BES with high
	 * pixelclocks (by setting a BES hardware 'zoom' = 2x).
	 * If you want optimal output quality better make sure you set the refreshrate/resolution
	 * of your monitor not too high ... */
	uint16 acczoom = 1;
	LOG(4,("Overlay: pixelclock is %dkHz, ", si->dm.timing.pixel_clock));
	if (si->dm.timing.pixel_clock > BESMAXSPEED)
	{
		/* BES running at half speed and resolution */
		/* This is how it works (BES slowing down):
		 * - Activate BES internal horizontal hardware scaling = 4x (in GLOBCTL below),
		 * - This also sets up BES only getting half the amount of pixels per line from
		 *   the input picture buffer (in effect half-ing the BES pixelclock input speed).
		 * Now in order to get the picture back to original size, we need to also double
		 * the inverse horizontal scaling factor here (x4 /2 /2 = 1x again).
		 * Note that every other pixel is now doubled or interpolated, according to another
		 * GLOBCTL bit. */
		acczoom = 2;
		LOG(4,("slowing down BES!\n"));
	}
	else
	{
		/* BES running at full speed and resolution */
		LOG(4,("BES is running at full speed\n"));
	}


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

	LOG(6,("Overlay: inputbuffer view (zoom) left %d, top %d, width %d, height %d\n",
		my_ov.h_start, my_ov.v_start, my_ov.width, my_ov.height));

	/* save for nv_bes_calc_move_overlay() */
	si->overlay.ow = *ow;
	si->overlay.ob = *ob;
	si->overlay.my_ov = my_ov;


	/********************************
	 *** setup horizontal scaling ***
	 ********************************/

	LOG(6,("Overlay: total input picture width = %d, height = %d\n",
			(ob->width - si->overlay.myBufInfo[offset].slopspace), ob->height));
	LOG(6,("Overlay: output picture width = %d, height = %d\n", ow->width, ow->height));

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
	ifactor -= (1 << 2);
	LOG(4,("Overlay: horizontal scaling factor is %f\n", (float)65536 / ifactor));

	/* compensate for accelerated 2x zoom (slowdown BES if pixelclock is too high) */
	hiscalv = ifactor * acczoom;
	/* save for gx00_bes_calc_move_overlay() */
	si->overlay.h_ifactor = ifactor;
	LOG(4,("Overlay: horizontal speed compensated factor is %f\n", (float)65536 / hiscalv));

	/* check scaling factor (and modify if needed) to be within scaling limits */
	if (((((uint32)my_ov.width) << 16) / 16384) > hiscalv)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		hiscalv = ((((uint32)my_ov.width) << 16) / 16384);
		LOG(4,("Overlay: horizontal scaling factor too large, clamping at %f\n", (float)65536 / hiscalv));
	}
	if (hiscalv >= (32 << 16))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		hiscalv = 0x1ffffc;
		LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)65536 / hiscalv));
	}
	/* AND below is required by hardware */
	hiscalv &= 0x001ffffc;


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
	ifactor -= (1 << 2);
	LOG(4,("Overlay: vertical scaling factor is %f\n", (float)65536 / ifactor));

	/* preserve ifactor for source positioning calculations later on */
	viscalv = ifactor;
	/* save for gx00_bes_calc_move_overlay() */
	si->overlay.v_ifactor = ifactor;

	/* check scaling factor (and modify if needed) to be within scaling limits */
	if (((((uint32)my_ov.height) << 16) / 16384) > viscalv)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		viscalv = ((((uint32)my_ov.height) << 16) / 16384);
		LOG(4,("Overlay: vertical scaling factor too large, clamping at %f\n", (float)65536 / viscalv));
	}
	if (viscalv >= (32 << 16))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		viscalv = 0x1ffffc;
		LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)65536 / viscalv));
	}
	/* AND below is required by hardware */
	viscalv &= 0x001ffffc;


	/********************************************************************************
	 *** setup all edges of output window, setup horizontal and vertical clipping ***
	 ********************************************************************************/
	gx00_bes_calc_move_overlay(&moi);


	/***************************************
	 *** setup misc. source bitmap stuff ***
	 ***************************************/

	/* setup horizontal source last position excluding slopspace: 
	 * this is the last pixel that will be used for calculating interpolated pixels */
	hsrclstv = ((ob->width - 1) - si->overlay.myBufInfo[offset].slopspace) << 16; 
	/* AND below required by hardware */
	hsrclstv &= 0x07ff0000;

	/* setup field 1 (is our complete frame) vertical source last position.
	 * this is the last pixel that will be used for calculating interpolated pixels */
	v1srclstv = (ob->height - 1);
	/* AND below required by hardware */
	v1srclstv &= 0x000007ff;


	/*****************************
	 *** log color keying info ***
	 *****************************/

	LOG(6,("Overlay: key_red %d, key_green %d, key_blue %d, key_alpha %d\n",
		ow->red.value, ow->green.value, ow->blue.value, ow->alpha.value));
	LOG(6,("Overlay: mask_red %d, mask_green %d, mask_blue %d, mask_alpha %d\n",
		ow->red.mask, ow->green.mask, ow->blue.mask, ow->alpha.mask));


	/*************************
	 *** setup BES control ***
	 *************************/

	/* BES global control: setup functions */
	globctlv = 0;

	/* slowdown BES if nessesary */
	if (acczoom == 1)
	{
		/* run at full speed and resolution */
		globctlv |= 0 << 0;
		/* disable filtering for half speed interpolation */
		globctlv |= 0 << 1;
	}
	else
	{
		/* run at half speed and resolution */
		globctlv |= 1 << 0;
		/* enable filtering for half speed interpolation */
		globctlv |= 1 << 1;
	}

	/* 4:2:0 specific setup: not needed here */
	globctlv |= 0 << 3;
	/* BES testregister: keep zero */	
	globctlv |= 0 << 4;
	/* the following bits marked (> G200) *must* be zero on G200: */
	/* 4:2:0 specific setup: not needed here (> G200) */
	globctlv |= 0 << 5;
	/* select yuy2 byte-order to B_YCbCr422 (> G200) */
	globctlv |= 0 << 6;
	/* BES internal contrast and brighness controls are not used, disabled (> G200) */
	globctlv |= 0 << 7;
	/* RGB specific setup: not needed here, so disabled (> G200) */
	globctlv |= 0 << 8;
	globctlv |= 0 << 9;
	/* 4:2:0 specific setup: not needed here (> G200) */
	globctlv |= 0 << 10;
	/* Tell BES when to copy the new register values to the actual active registers.
	 * bits 16-27 (12 bits) are the CRTC vert. count value at which copying takes
	 * place.
	 * (This is the double buffering feature: programming must be completed *before*
	 *  the CRTC vert count value set here!) */
	/* CRTC vert count for copying = $000, so during retrace, line 0. */
	globctlv |= 0x000 << 16;

	/* BES control: enable scaler and setup functions */
	/* pre-reset all bits */
	ctlv = 0;
	/* enable BES */
	ctlv |= 1 << 0;
	/* we start displaying at an even startline (zero) in 'field 1' (no hardware de-interlacing is used) */
	ctlv |= 0 << 6;
	/* we don't use field 2, so its startline is not important */
	ctlv |= 0 << 7;

	LOG(6,("Overlay: ow->flags is $%08x\n",ow->flags));
	/* enable horizontal filtering on scaling if asked for: if we *are* actually scaling */
	if ((ow->flags & B_OVERLAY_HORIZONTAL_FILTERING) && (hiscalv != (0x01 << 16)))
	{
		ctlv |= 1 << 10;
		LOG(6,("Overlay: using horizontal interpolation on scaling\n"));
	}
	else
	{
		ctlv |= 0 << 10;
		LOG(6,("Overlay: using horizontal dropping or replication on scaling\n"));
	}
	/* enable vertical filtering on scaling if asked for: if we are *upscaling* only */
	if ((ow->flags & B_OVERLAY_VERTICAL_FILTERING) && (viscalv < (0x01 << 16)) && (ob->width <= 1024))	{
		ctlv |= 1 << 11;
		LOG(6,("Overlay: using vertical interpolation on scaling\n"));
	} else {
		ctlv |= 0 << 11;
		LOG(6,("Overlay: using vertical dropping or replication on scaling\n"));
	}

	/* use actual calculated weight for horizontal interpolation */
	ctlv |= 0 << 12;
	/* use horizontal chroma interpolation upsampling on BES input picture */
	ctlv |= 1 << 16;
	/* select 4:2:2 BES input format */
	ctlv |= 0 << 17;
	/* dithering is enabled */
	ctlv |= 1 << 18;
	/* horizontal mirroring is not used */
	ctlv |= 0 << 19;
	/* BES output should be in color */
	ctlv |= 0 << 20;
	/* BES output blanking is disabled: we want a picture, no 'black box'! */
	ctlv |= 0 << 21;
	/* we do software field select (field select is not used) */	
	ctlv |= 0 << 24;
	/* we always display field 1 in buffer A, this contains our full frames */
	/* select field 1 */
	ctlv |= 0 << 25;
	/* select buffer A */
	ctlv |= 0 << 26;


	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Make sure reprogramming the BES completes before the next retrace occurs,
	 * to prevent register-update glitches (double buffer feature). */

	LOG(3,("Overlay: starting register programming beyond Vcount %d\n", CR1R(VCOUNT)));
	/* Even at 1600x1200x90Hz, a single line still takes about 9uS to complete:
	 * this resolution will generate about 180Mhz pixelclock while we can do
	 * upto 360Mhz. So snooze about 4uS to prevent bus-congestion... 
	 * Appr. 200 lines time will provide enough room even on a 100Mhz CPU if it's
	 * screen is set to the highest refreshrate/resolution possible. */
	while ((uint16)CR1R(VCOUNT) > (si->dm.timing.v_total - 200)) snooze(4);


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	BESW(HCOORD, moi.hcoordv);
	BESW(VCOORD, moi.vcoordv);
	BESW(HISCAL, hiscalv);
	BESW(HSRCST, moi.hsrcstv);
	BESW(HSRCEND, moi.hsrcendv);
	BESW(HSRCLST, hsrclstv);
	BESW(VISCAL, viscalv);
	BESW(A1ORG, moi.a1orgv);
	BESW(V1WGHT, moi.v1srcstv);
	BESW(V1SRCLST, v1srclstv);
	BESW(GLOBCTL, globctlv);
	BESW(CTL, ctlv);


	/**************************
	 *** setup color keying ***
	 **************************/

	/* setup colorkeying */
	DXIW(COLKEY, (ow->alpha.value & ow->alpha.mask));

	DXIW(COLKEY0RED, (ow->red.value & ow->red.mask));
	DXIW(COLKEY0GREEN, (ow->green.value & ow->green.mask));
	DXIW(COLKEY0BLUE, (ow->blue.value & ow->blue.mask));

	DXIW(COLMSK, ow->alpha.mask);

	DXIW(COLMSK0RED, ow->red.mask);
	DXIW(COLMSK0GREEN, ow->green.mask);
	DXIW(COLMSK0BLUE, ow->blue.mask);

	/* setup colorkeying */
	if (ow->flags & B_OVERLAY_COLOR_KEY)
		DXIW(KEYOPMODE,0x01);
	else
		DXIW(KEYOPMODE,0x00);


	/*************************
	 *** setup misc. stuff ***
	 *************************/

	/* setup brightness and contrast to be 'neutral' (this is not implemented on G200) */
	BESW(LUMACTL, 0x00000080);

	/* setup source pitch including slopspace (in pixels); AND is required by hardware */
	BESW(PITCH, (ob->width & 0x00000fff));

	/* on a 500Mhz P3 CPU just logging a line costs 400uS (18-19 vcounts at 1024x768x60Hz)!
	 * programming the registers above actually costs 180uS here */
	LOG(3,("Overlay: completed at Vcount %d\n", CR1R(VCOUNT)));

	/* note that overlay is in use (for gx00_bes_move_overlay()) */
	si->overlay.active = true;

	return B_OK;
}

status_t gx00_release_bes()
{
	/* setup BES control: disable scaler */
	BESW(CTL, 0x00000000);  

	/* note that overlay is not in use (for gx00_bes_move_overlay()) */
	si->overlay.active = false;

	return B_OK;
}
