/* NeoMagic Back End Scaler functions */
/* Written by Rudolf Cornelissen 05/2002-08/2004 */

#define MODULE_BIT 0x00000200

#include "nm_std.h"

//fixme: implement: (used for virtual screens!)
//void move_overlay(uint16 hdisp_start, uint16 vdisp_start);

status_t nm_configure_bes
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
 
	/* bes setup data */
	nm_bes_data bi;
	/* misc used variables */
	uint16 temp1, temp2;
	/* BES output coordinate system for virtual workspaces */
	uint16 crtc_hstart, crtc_vstart, crtc_hend, crtc_vend;
	/* inverse scaling factor, used for source positioning */
	uint32 ifactor;
	/* copy of overlay view which has checked valid values */
	overlay_view my_ov;


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

	/* the BES does not respect virtual_workspaces, but adheres to CRTC
	 * constraints only */
	crtc_hstart = si->dm.h_display_start;
	/* horizontal end is the first position beyond the displayed range on the CRTC */
	crtc_hend = crtc_hstart + si->dm.timing.h_display;
	crtc_vstart = si->dm.v_display_start;
	/* vertical end is the first position beyond the displayed range on the CRTC */
	crtc_vend = crtc_vstart + si->dm.timing.v_display;


	/****************************************
	 *** setup all edges of output window ***
	 ****************************************/

	/* setup left and right edges of output window */
	bi.hcoordv = 0;
	/* left edge coordinate of output window, must be inside desktop */
	/* clipping on the left side */
	if (ow->h_start < crtc_hstart)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the right side */
		if (ow->h_start >= (crtc_hend - 1))
		{
			/* width < 2 is not allowed */
			temp1 = (crtc_hend - crtc_hstart - 2);
		} 
		else
		/* no clipping here */
		{
			temp1 = (ow->h_start - crtc_hstart);
		}
	} 
	bi.hcoordv |= temp1 << 16;
	/* right edge coordinate of output window, must be inside desktop */
	/* width < 2 is not allowed */
	if (ow->width < 2) 
	{
		temp2 = (temp1 + 1);
	}
	else 
	{
		/* clipping on the right side */
		if ((ow->h_start + ow->width - 1) > (crtc_hend - 1))
		{
			temp2 = (crtc_hend - crtc_hstart - 1);
		}
		else
		{
			/* clipping on the left side */
			if ((ow->h_start + ow->width - 1) < (crtc_hstart + 1))
			{
				/* width < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(ow->h_start + ow->width - crtc_hstart - 1));
			}
		}
	}
	bi.hcoordv |= temp2 << 0;
	LOG(4,("Overlay: CRTC left-edge output %d, right-edge output %d\n",temp1, temp2));

	/* setup top and bottom edges of output window */
	bi.vcoordv = 0;
	/* top edge coordinate of output window, must be inside desktop */
	/* clipping on the top side */
	if (ow->v_start < crtc_vstart)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the bottom side */
		if (ow->v_start >= (crtc_vend - 1))
		{
			/* height < 2 is not allowed */
			temp1 = (crtc_vend - crtc_vstart - 2);
		} 
		else
		/* no clipping here */
		{
			temp1 = (ow->v_start - crtc_vstart);
		}
	} 
	bi.vcoordv |= temp1 << 16;
	/* bottom edge coordinate of output window, must be inside desktop */
	/* height < 2 is not allowed */
	if (ow->height < 2) 
	{
		temp2 = (temp1 + 1);
	}
	else 
	{
		/* clipping on the bottom side */
		if ((ow->v_start + ow->height - 1) > (crtc_vend - 1))
		{
			temp2 = (crtc_vend - crtc_vstart - 1);
		}
		else
		{
			/* clipping on the top side */
			if ((ow->v_start + ow->height - 1) < (crtc_vstart + 1))
			{
				/* height < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(ow->v_start + ow->height - crtc_vstart - 1));
			}
		}
	}
	bi.vcoordv |= temp2 << 0;
	LOG(4,("Overlay: CRTC top-edge output %d, bottom-edge output %d\n",temp1, temp2));


	/*********************************************
	 *** setup horizontal scaling and clipping ***
	 *********************************************/

	LOG(6,("Overlay: total input picture width = %d, height = %d\n",
			(ob->width - si->overlay.myBufInfo[offset].slopspace), ob->height));
	LOG(6,("Overlay: output picture width = %d, height = %d\n", ow->width, ow->height));

	/* calculate inverse horizontal scaling factor, taking zoom into account */
	ifactor = ((((uint32)my_ov.width) << 12) / ow->width); 

	/* correct factor to prevent most-right visible 'line' from distorting */
	ifactor -= 1;
	bi.hiscalv = ifactor;
	LOG(4,("Overlay: horizontal scaling factor is %f\n", (float)4096 / ifactor));

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* the upscaling limit is 8.0 (see official Neomagic specsheets) */
	if (bi.hiscalv < 0x00000200)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		bi.hiscalv = 0x00000200;
		LOG(4,("Overlay: horizontal scaling factor too large, clamping at %f\n", (float)4096 / bi.hiscalv));
	}
	/* horizontal downscaling cannot be done by NM BES hardware */
	if (bi.hiscalv > (1 << 12))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		bi.hiscalv = 0x1000;
		LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)4096 / bi.hiscalv));
	}


	/* do horizontal clipping... */
	/* Setup horizontal source start: first (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap.
	 * Then add the left starting position of the bitmap's view (zoom function) to get the final value needed.
	 * Note: The input bitmaps slopspace is automatically excluded from the calculations this way! */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */
	bi.hsrcstv = 0;
	/* check for destination horizontal clipping at left side */
	if (ow->h_start < crtc_hstart)
	{
		/* check if entire destination picture is clipping left:
		 * (2 pixels will be clamped onscreen at least) */
		if ((ow->h_start + ow->width - 1) < (crtc_hstart + 1))
		{
			/* increase 'first contributing pixel' with 'fixed value': (total dest. width - 2) */
			bi.hsrcstv += (ow->width - 2);
		}
		else
		{
			/* increase 'first contributing pixel' with actual number of dest. clipping pixels */
			bi.hsrcstv += (crtc_hstart - ow->h_start);
		}
		LOG(4,("Overlay: clipping left...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		bi.hsrcstv *= (ifactor << 4);
	}
	/* take zoom into account */
	bi.hsrcstv += ((uint32)my_ov.h_start) << 16;
	LOG(4,("Overlay: first hor. (sub)pixel of input bitmap contributing %f\n", bi.hsrcstv / (float)65536));


	/* Setup horizontal source end: last (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap.
	 * Then add the right ending position of the bitmap's view (zoom function) to get the final value needed. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source end pos.! */

	bi.hsrcendv = 0;
	/* check for destination horizontal clipping at right side */
	if ((ow->h_start + ow->width - 1) > (crtc_hend - 1))
	{
		/* check if entire destination picture is clipping right:
		 * (2 pixels will be clamped onscreen at least) */
		if (ow->h_start > (crtc_hend - 2))
		{
			/* increase 'number of clipping pixels' with 'fixed value': (total dest. width - 2) */
			bi.hsrcendv += (ow->width - 2);
		}
		else
		{
			/* increase 'number of clipping pixels' with actual number of dest. clipping pixels */
			bi.hsrcendv += ((ow->h_start + ow->width - 1) - (crtc_hend - 1));
		}
		LOG(4,("Overlay: clipping right...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		bi.hsrcendv *= (ifactor << 4);
		/* now subtract this value from the last used pixel in (zoomed) inputbuffer, aligned to BES */
		bi.hsrcendv = (((uint32)((my_ov.h_start + my_ov.width) - 1)) << 16) - bi.hsrcendv;
	}
	else
	{
		/* set last contributing pixel to last used pixel in (zoomed) inputbuffer, aligned to BES */
		bi.hsrcendv = (((uint32)((my_ov.h_start + my_ov.width) - 1)) << 16);
	}
	/* AND below required by hardware */
	bi.hsrcendv &= 0x03ffffff;
	LOG(4,("Overlay: last horizontal (sub)pixel of input bitmap contributing %f\n", bi.hsrcendv / (float)65536));


	/*******************************************
	 *** setup vertical scaling and clipping ***
	 *******************************************/

	/* calculate inverse vertical scaling factor, taking zoom into account */
	ifactor = ((((uint32)my_ov.height) << 12) / ow->height); 

	/* correct factor to prevent lowest visible line from distorting */
	ifactor -= 1;
	LOG(4,("Overlay: vertical scaling factor is %f\n", (float)4096 / ifactor));

	/* preserve ifactor for source positioning calculations later on */
	bi.viscalv = ifactor;

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* the upscaling limit is 8.0 (see official Neomagic specsheets) */
	if (bi.viscalv < 0x00000200)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		bi.viscalv = 0x00000200;
		LOG(4,("Overlay: vertical scaling factor too large, clamping at %f\n", (float)4096 / bi.viscalv));
	}
	/* vertical downscaling cannot be done by NM BES hardware */
	if (bi.viscalv > (1 << 12))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		bi.viscalv = 0x1000;
		LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)4096 / bi.viscalv));
	}


	/* do vertical clipping... */
	/* Setup vertical source start: first (sub)pixel contributing to output picture.
	 * Note: this exists of two parts:
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
	bi.weight = 0;
	bi.a1orgv = (uint32)((vuint32 *)ob->buffer);
	bi.a1orgv -= (uint32)((vuint32 *)si->framebuffer);
	/* calculate origin adress */
	LOG(4,("Overlay: topleft corner of input bitmap (cardRAM offset) $%08x\n",bi.a1orgv));
	/* check for destination vertical clipping at top side */
	if (ow->v_start < crtc_vstart)
	{
		/* check if entire destination picture is clipping at top:
		 * (2 pixels will be clamped onscreen at least) */
		if ((ow->v_start + ow->height - 1) < (crtc_vstart + 1))
		{
			/* increase source buffer origin with 'fixed value':
			 * (integer part of ('total height - 2' of dest. picture in pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			bi.weight = (ow->height - 2) * (ifactor << 4);
			bi.a1orgv += ((bi.weight >> 16) * ob->bytes_per_row);
		}
		else
		{
			/* increase source buffer origin with:
			 * (integer part of (number of destination picture clipping pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			bi.weight = (crtc_vstart - ow->v_start) * (ifactor << 4);
			bi.a1orgv += ((bi.weight >> 16) * ob->bytes_per_row);
		}
		LOG(4,("Overlay: clipping at top...\n"));
	}
	/* take zoom into account */
	bi.a1orgv += (my_ov.v_start * ob->bytes_per_row);
	bi.weight += (((uint32)my_ov.v_start) << 16);

	/* now include 'pixel precise' left clipping...
	 * (subpixel precision is not supported by NeoMagic cards) */
	bi.a1orgv += ((bi.hsrcstv >> 16) * 2);
	/* we need to step in 4-byte (2 pixel) granularity due to the nature of yuy2 */
	bi.a1orgv &= ~0x03;
	LOG(4,("Overlay: 'contributing part of buffer' origin is (cardRAM offset) $%08x\n",bi.a1orgv));
	LOG(4,("Overlay: first vert. (sub)pixel of input bitmap contributing %f\n", bi.weight / (float)65536));


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
	bi.globctlv = 0;

	/* enable BES */
	bi.globctlv |= 1 << 0;
	/* enable colorkeying */
	bi.globctlv |= 1 << 1;
	/* b3 = 1: distorts right-half of overlay output. Keeping it zero. */
	/* colorspace is YV12, I420 or YUY2 (no RV15 or RV16) */
	bi.globctlv |= 0 << 5;

	/* enable auto-alternating hardware buffers if alternating buffers is enabled (NM2160) */
	bi.globctlv |= 1 << 8;
	/* disable capture */
	bi.globctlv |= 1 << 13;
	/* capture: display one buffer (no alternating buffers) */
	bi.globctlv |= 0 << 14;
	/* capture: display frame (no field) */
	bi.globctlv |= 0 << 15;

	/* BTW: horizontal and vertical filtering are always turned on in NM hardware. */


	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Make sure reprogramming the BES completes before the next retrace occurs,
	 * to prevent register-update glitches (double buffer feature). */

	//fixme...

//	LOG(3,("Overlay: starting register programming beyond Vcount %d\n", CR1R(VCOUNT)));
	/* Even at 1600x1200x90Hz, a single line still takes about 9uS to complete:
	 * this resolution will generate about 180Mhz pixelclock while we can do
	 * upto 360Mhz. So snooze about 4uS to prevent bus-congestion... 
	 * Appr. 200 lines time will provide enough room even on a 100Mhz CPU if it's
	 * screen is set to the highest refreshrate/resolution possible. */
//	while (CR1R(VCOUNT) > (si->dm.timing.v_total - 200)) snooze(4);


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	if (si->ps.card_type >= NM2097)
	{
		/* helper: some cards use pixels to define buffer pitch, others use bytes */
		uint16 buf_pitch = ob->width;

		/* PCI card */
		LOG(4,("Overlay: accelerant is programming BES\n"));
		/* unlock card overlay sequencer registers (b5 = 1) */
		PCIGRPHW(GENLOCK, (PCIGRPHR(GENLOCK) | 0x20));
		/* destination rectangle #1 (output window position and size) */
		PCIGRPHW(HD1COORD1L, ((bi.hcoordv >> 16) & 0xff));
		PCIGRPHW(HD1COORD2L, (bi.hcoordv & 0xff));
		PCIGRPHW(HD1COORD21H, (((bi.hcoordv >> 4) & 0xf0) | ((bi.hcoordv >> 24) & 0x0f)));
		PCIGRPHW(VD1COORD1L, ((bi.vcoordv >> 16) & 0xff));
		PCIGRPHW(VD1COORD2L, (bi.vcoordv & 0xff));
		PCIGRPHW(VD1COORD21H, (((bi.vcoordv >> 4) & 0xf0) | ((bi.vcoordv >> 24) & 0x0f)));
		/* scaling */
		PCIGRPHW(XSCALEL, (bi.hiscalv & 0xff));
		PCIGRPHW(XSCALEH, ((bi.hiscalv >> 8) & 0xff));
		PCIGRPHW(YSCALEL, (bi.viscalv & 0xff));
		PCIGRPHW(YSCALEH, ((bi.viscalv >> 8) & 0xff));
		/* inputbuffer #1 origin */
		/* (we don't program buffer #2 as it's unused.) */
		if (si->ps.card_type < NM2200)
		{
			bi.a1orgv >>= 1;
			/* horizontal source end does not use subpixelprecision: granularity is 8 pixels */
			/* notes:
			 * - correctly programming horizontal source end minimizes used bandwidth;
			 * - adding 9 below is in fact:
			 *   - adding 1 to round-up to the nearest whole source-end value
			       (making SURE we NEVER are a (tiny) bit too low);
			     - adding 1 to convert 'last used position' to 'number of used pixels';
			     - adding 7 to round-up to the nearest higher (or equal) valid register
			       value (needed because of it's 8-pixel granularity). */
			PCIGRPHW(0xbc, ((((bi.hsrcendv >> 16) + 9) >> 3) - 1));
		}
		else
		{
			/* NM2200 and later cards use bytes to define buffer pitch */
			buf_pitch <<= 1;
			/* horizontal source end does not use subpixelprecision: granularity is 16 pixels */
			/* notes:
			 * - programming this register just a tiny bit too low messes up vertical
			 *   scaling badly (also distortion stripes and flickering are reported)!
			 * - not programming this register correctly will mess-up the picture when
			 *   it's partly clipping on the right side of the screen...
			 * - make absolutely sure the engine can fetch the last pixel needed from
			 *   the sourcebitmap even if only to generate a tiny subpixel from it!
			 *   (see remarks for < NM2200 cards regarding programming this register) */
			PCIGRPHW(0xbc, ((((bi.hsrcendv >> 16) + 17) >> 4) - 1));
		}
		PCIGRPHW(BUF1ORGL, (bi.a1orgv & 0xff));
		PCIGRPHW(BUF1ORGM, ((bi.a1orgv >> 8) & 0xff));
		PCIGRPHW(BUF1ORGH, ((bi.a1orgv >> 16) & 0xff));
		/* ??? */
		PCIGRPHW(0xbd, 0x02);
		PCIGRPHW(0xbe, 0x00);
		/* b2 = 0: don't use horizontal mirroring (NM2160) */
		/* other bits do ??? */
		PCIGRPHW(0xbf, 0x02);

		/* destination rectangle #2 (output window position and size) */
/*
	{
		uint16 left = 0;
		uint16 right = 128;
		uint16 top = 0;
		uint16 bottom = 128;

		PCISEQW(HD2COORD1L, (left & 0xff));
		PCISEQW(HD2COORD2L, (right & 0xff));
		PCISEQW(HD2COORD21H, (((right >> 4) & 0xf0) | ((left >> 8) & 0x0f)));
		PCISEQW(VD2COORD1L, (top & 0xff));
		PCISEQW(VD2COORD2L, (bottom & 0xff));
		PCISEQW(VD2COORD21H, (((bottom >> 4) & 0xf0) | ((top >> 8) & 0x0f)));
	}
*/
		/* ??? */
	    PCISEQW(0x1c, 0xfb);
    	PCISEQW(0x1d, 0x00);
		PCISEQW(0x1e, 0xe2);
    	PCISEQW(0x1f, 0x02);
 		/* b1 = 0: disable alternating hardware buffers (NM2160) */
		/* other bits do ??? */
 		PCISEQW(0x09, 0x11);
		/* we don't use PCMCIA Zoomed Video port capturing, set 1:1 scale just in case */
		/* (b6-4 = Y downscale = 100%, b2-0 = X downscale = 100%;
		 *  downscaling selectable in 12.5% steps on increasing setting by 1) */
		PCISEQW(ZVCAP_DSCAL, 0x00);
		/* global BES control */
		PCIGRPHW(BESCTRL1, (bi.globctlv & 0xff));
		PCISEQW(BESCTRL2, ((bi.globctlv >> 8) & 0xff));


		/**************************
		 *** setup color keying ***
		 **************************/

		PCIGRPHW(COLKEY_R, (ow->red.value & ow->red.mask));
		PCIGRPHW(COLKEY_G, (ow->green.value & ow->green.mask));
		PCIGRPHW(COLKEY_B, (ow->blue.value & ow->blue.mask));


		/*************************
		 *** setup misc. stuff ***
		 *************************/

		/* setup brightness to be 'neutral' (two's complement number) */
		PCIGRPHW(BRIGHTNESS, 0x00);

		/* setup inputbuffer #1 pitch including slopspace */
		/* (we don't program the pitch for inputbuffer #2 as it's unused.) */
		PCIGRPHW(BUF1PITCHL, (buf_pitch & 0xff));
		PCIGRPHW(BUF1PITCHH, ((buf_pitch >> 8) & 0xff));
	}
	else
	{
		/* ISA card. Speed required, so:
		 * program entire sequence in kerneldriver in one context switch! */
		LOG(4,("Overlay: kerneldriver programs BES\n"));

		/* complete BES info struct... */
		bi.card_type = si->ps.card_type;
		bi.colkey_r = (ow->red.value & ow->red.mask);
		bi.colkey_g = (ow->green.value & ow->green.mask);
		bi.colkey_b = (ow->blue.value & ow->blue.mask);
		bi.ob_width = ob->width;
		/* ... and call kerneldriver to program the BES */
		bi.magic = NM_PRIVATE_DATA_MAGIC;
		ioctl(fd, NM_PGM_BES, &bi, sizeof(bi));
	}

	/* on a 500Mhz P3 CPU just logging a line costs 400uS (18-19 vcounts at 1024x768x60Hz)!
	 * programming the registers above actually costs 180uS here */
//	LOG(3,("Overlay: completed at Vcount %d\n", CR1R(VCOUNT)));

	return B_OK;
}

status_t nm_release_bes()
{
	/* setup BES control: disable scaler */
	if (si->ps.card_type >= NM2097)
	{
		/* PCI card */
		PCIGRPHW(BESCTRL1, 0x02);
		PCISEQW(BESCTRL2, 0xa0);
	}
	else
	{
		/* ISA card */
		ISAGRPHW(BESCTRL1, 0x02);
		ISASEQW(BESCTRL2, 0xa0);
	}

	return B_OK;
}
