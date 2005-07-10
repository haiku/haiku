/* Nvidia TNT and GeForce Back End Scaler functions */
/* Written by Rudolf Cornelissen 05/2002-9/2004 */

#define MODULE_BIT 0x00000200

#include "std.h"

typedef struct move_overlay_info move_overlay_info;

struct move_overlay_info
{
	uint32 hcoordv;		/* left and right edges of video output window */
	uint32 vcoordv;		/* top and bottom edges of video output window */
	uint32 hsrcstv;		/* horizontal source start in source buffer (clipping) */
	uint32 v1srcstv;	/* vertical source start in source buffer (clipping) */
	uint32 a1orgv;		/* alternate source clipping via startadress of source buffer */
};

static void eng_bes_calc_move_overlay(move_overlay_info *moi);
static void eng_bes_program_move_overlay(move_overlay_info moi);

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
			/* on pre-NV10 we need to do clipping in the source
			 * bitmap because no seperate clipping registers exist... */
			if (si->ps.card_arch < NV10A)
				moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		else
		{
			/* increase 'first contributing pixel' with:
			 * number of destination picture clipping pixels * inverse scaling factor */
			moi->v1srcstv = (crtc_vstart - si->overlay.ow.v_start) * si->overlay.v_ifactor;
			/* on pre-NV10 we need to do clipping in the source
			 * bitmap because no seperate clipping registers exist... */
			if (si->ps.card_arch < NV10A)
				moi->a1orgv += ((moi->v1srcstv >> 16) * si->overlay.ob.bytes_per_row);
		}
		LOG(4,("Overlay: clipping at top...\n"));
	}
	/* take zoom into account */
	moi->v1srcstv += (((uint32)si->overlay.my_ov.v_start) << 16);
	if (si->ps.card_arch < NV10A)
	{
		moi->a1orgv += (si->overlay.my_ov.v_start * si->overlay.ob.bytes_per_row);
		LOG(4,("Overlay: 'contributing part of buffer' origin is (cardRAM offset) $%08x\n", moi->a1orgv));
	}
	LOG(4,("Overlay: first vert. (sub)pixel of input bitmap contributing %f\n", moi->v1srcstv / (float)65536));

	/* AND below is probably required by hardware. */
	/* Buffer A topleft corner of field 1 (origin)(field 1 contains our full frames) */
	moi->a1orgv &= 0xfffffff0;
}

static void eng_bes_program_move_overlay(move_overlay_info moi)
{
	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Done in card hardware:
	 * double buffered registers + trigger if programming complete feature. */


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	if (si->ps.card_arch < NV10A)
	{
		/* unknown, but needed (otherwise high-res distortions and only half the frames */
		BESW(NV04_OE_STATE, 0x00000000);
		/* select buffer 0 as active (b16) */
		BESW(NV04_SU_STATE, 0x00000000);
		/* unknown (no effect?) */
		BESW(NV04_RM_STATE, 0x00000000);
		/* setup clipped(!) buffer startadress in RAM */
		/* RIVA128 - TNT bes doesn't have clipping registers, so no subpixelprecise clipping
		 * either. We do pixelprecise vertical and 'two pixel' precise horizontal clipping here. */
		/* (program both buffers to prevent sync distortions) */
		/* first include 'pixel precise' left clipping... (top clipping was already included) */
		moi.a1orgv += ((moi.hsrcstv >> 16) * 2);
		/* we need to step in 4-byte (2 pixel) granularity due to the nature of yuy2 */
		BESW(NV04_0BUFADR, (moi.a1orgv & ~0x03));
		BESW(NV04_1BUFADR, (moi.a1orgv & ~0x03));
		/* setup output window position */
		BESW(NV04_DSTREF, ((moi.vcoordv & 0xffff0000) | ((moi.hcoordv & 0xffff0000) >> 16)));
		/* setup output window size */
		BESW(NV04_DSTSIZE, (
			(((moi.vcoordv & 0x0000ffff) - ((moi.vcoordv & 0xffff0000) >> 16) + 1) << 16) |
			((moi.hcoordv & 0x0000ffff) - ((moi.hcoordv & 0xffff0000) >> 16) + 1)
			));
		/* select buffer 1 as active (b16) */
		BESW(NV04_SU_STATE, 0x00010000);
	}
	else
	{
		/* >= NV10A */
	
		/* setup buffer origin: GeForce uses subpixel precise clipping on left and top! (12.4 values) */
		BESW(NV10_0SRCREF, ((moi.v1srcstv << 4) & 0xffff0000) | ((moi.hsrcstv >> 12) & 0x0000ffff));
		/* setup output window position */
		BESW(NV10_0DSTREF, ((moi.vcoordv & 0xffff0000) | ((moi.hcoordv & 0xffff0000) >> 16)));
		/* setup output window size */
		BESW(NV10_0DSTSIZE, (
			(((moi.vcoordv & 0x0000ffff) - ((moi.vcoordv & 0xffff0000) >> 16) + 1) << 16) |
			((moi.hcoordv & 0x0000ffff) - ((moi.hcoordv & 0xffff0000) >> 16) + 1)
			));
		/* We only use buffer buffer 0: select it. (0x01 = buffer 0, 0x10 = buffer 1) */
		/* This also triggers activation of programmed values (double buffered registers feature) */
		BESW(NV10_BUFSEL, 0x00000001);
	}
}

status_t eng_bes_to_crtc(bool crtc)
{
	if (si->ps.secondary_head)
	{
		if (crtc)
		{
			LOG(4,("Overlay: switching overlay to CRTC2\n"));
			/* switch overlay engine to CRTC2 */
			ENG_RG32(RG32_FUNCSEL) &= ~0x00001000;
			ENG_RG32(RG32_2FUNCSEL) |= 0x00001000;
			si->overlay.crtc = !si->crtc_switch_mode;
		}
		else
		{
			LOG(4,("Overlay: switching overlay to CRTC1\n"));
			/* switch overlay engine to CRTC1 */
			ENG_RG32(RG32_2FUNCSEL) &= ~0x00001000;
			ENG_RG32(RG32_FUNCSEL) |= 0x00001000;
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
	if (si->ps.card_arch < NV10A)
	{
		/* disable overlay ints (b0 = buffer 0, b4 = buffer 1) */
		BESW(NV04_INTE, 0x00000000);

		/* setup saturation to be 'neutral' */
		BESW(NV04_SAT, 0x00000000);
		/* setup RGB brightness to be 'neutral' */
		BESW(NV04_RED_AMP, 0x00000069);
		BESW(NV04_GRN_AMP, 0x0000003e);
		BESW(NV04_BLU_AMP, 0x00000089);

		/* setup fifo for fetching data */
		BESW(NV04_FIFOBURL, 0x00000003);
		BESW(NV04_FIFOTHRS, 0x00000038);

		/* unknown, but needed (registers only have b0 implemented) */
		/* (program both buffers to prevent sync distortions) */
		BESW(NV04_0OFFSET, 0x00000000);
		BESW(NV04_1OFFSET, 0x00000000);
	}
	else
	{
		/* >= NV10A */

		/* disable overlay ints (b0 = buffer 0, b4 = buffer 1) */
		BESW(NV10_INTE, 0x00000000);
		/* shut off GeForce4MX MPEG2 decoder */
		BESW(DEC_GENCTRL, 0x00000000);
		/* setup BES memory-range mask */
		BESW(NV10_0MEMMASK, (si->ps.memory_size - 1));
		/* unknown, but needed */
		BESW(NV10_0OFFSET, 0x00000000);

		/* setup brightness, contrast and saturation to be 'neutral' */
		BESW(NV10_0BRICON, ((0x1000 << 16) | 0x1000));
		BESW(NV10_0SAT, ((0x0000 << 16) | 0x1000));
	}

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
	hiscalv = ifactor;
	/* save for eng_bes_calc_move_overlay() */
	si->overlay.h_ifactor = ifactor;
	LOG(4,("Overlay: horizontal scaling factor is %f\n", (float)65536 / ifactor));

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* all cards have a upscaling limit of 8.0 (see official nVidia specsheets) */
	if (hiscalv < 0x00002000)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		hiscalv = 0x00002000;
		LOG(4,("Overlay: horizontal scaling factor too large, clamping at %f\n", (float)65536 / hiscalv));
	}
	switch (si->ps.card_arch)
	{
	case NV04A:
		/* Riva128-TNT2 series have a 'downscaling' limit of 1.000489
		 * (16bit register with 0.11 format value) */
		if (hiscalv > 0x0000ffff)
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			hiscalv = 0x0000ffff;
			LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)2048 / (hiscalv >> 5)));
		}
		break;
	case NV30A:
	case NV40A:
		/* GeForceFX series and up have a downscaling limit of 0.5 (except NV31!) */
		if ((hiscalv > (2 << 16)) && (si->ps.card_type != NV31))
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			hiscalv = (2 << 16);
			LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)65536 / hiscalv));
		}
		/* NV31 (confirmed GeForceFX 5600) has NV20A scaling limits!
		 * So let it fall through... */
		if (si->ps.card_type != NV31) break;
	default:
		/* the rest has a downscaling limit of 0.125 */
		if (hiscalv > (8 << 16))
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			hiscalv = (8 << 16);
			LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)65536 / hiscalv));
		}
		break;
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
	/* save for eng_bes_calc_move_overlay() */
	si->overlay.v_ifactor = ifactor;

	/* check scaling factor (and modify if needed) to be within scaling limits */
	/* all cards have a upscaling limit of 8.0 (see official nVidia specsheets) */
	if (viscalv < 0x00002000)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		viscalv = 0x00002000;
		LOG(4,("Overlay: vertical scaling factor too large, clamping at %f\n", (float)65536 / viscalv));
	}
	switch (si->ps.card_arch)
	{
	case NV04A:
		/* Riva128-TNT2 series have a 'downscaling' limit of 1.000489
		 * (16bit register with 0.11 format value) */
		if (viscalv > 0x0000ffff)
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			viscalv = 0x0000ffff;
			LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)2048 / (viscalv >> 5)));
		}
		break;
	case NV30A:
	case NV40A:
		/* GeForceFX series and up have a downscaling limit of 0.5 (except NV31!) */
		if ((viscalv > (2 << 16)) && (si->ps.card_type != NV31))
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			viscalv = (2 << 16);
			LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)65536 / viscalv));
		}
		/* NV31 (confirmed GeForceFX 5600) has NV20A scaling limits!
		 * So let it fall through... */
		if (si->ps.card_type != NV31) break;
	default:
		/* the rest has a downscaling limit of 0.125 */
		if (viscalv > (8 << 16))
		{
			/* (non-inverse) factor too small, set factor to min. valid value */
			viscalv = (8 << 16);
			LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)65536 / viscalv));
		}
		break;
	}
	/* AND below is required by hardware */
	viscalv &= 0x001ffffc;


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
	 * double buffered registers + trigger if programming complete feature. */


	/**************************************
	 *** actually program the registers ***
	 **************************************/

	if (si->ps.card_arch < NV10A)
	{
		/* unknown, but needed (otherwise high-res distortions and only half the frames */
		BESW(NV04_OE_STATE, 0x00000000);
		/* select buffer 0 as active (b16) */
		BESW(NV04_SU_STATE, 0x00000000);
		/* unknown (no effect?) */
		BESW(NV04_RM_STATE, 0x00000000);
		/* setup clipped(!) buffer startadress in RAM */
		/* RIVA128 - TNT bes doesn't have clipping registers, so no subpixelprecise clipping
		 * either. We do pixelprecise vertical and 'two pixel' precise horizontal clipping here. */
		/* (program both buffers to prevent sync distortions) */
		/* first include 'pixel precise' left clipping... (top clipping was already included) */
		moi.a1orgv += ((moi.hsrcstv >> 16) * 2);
		/* we need to step in 4-byte (2 pixel) granularity due to the nature of yuy2 */
		BESW(NV04_0BUFADR, (moi.a1orgv & ~0x03));
		BESW(NV04_1BUFADR, (moi.a1orgv & ~0x03));
		/* setup buffer source pitch including slopspace (in bytes).
		 * Note:
		 * source pitch granularity = 16 pixels on the RIVA128 - TNT (so pre-NV10) bes */
		/* (program both buffers to prevent sync distortions) */
		BESW(NV04_0SRCPTCH, (ob->width * 2));
		BESW(NV04_1SRCPTCH, (ob->width * 2));
		/* setup output window position */
		BESW(NV04_DSTREF, ((moi.vcoordv & 0xffff0000) | ((moi.hcoordv & 0xffff0000) >> 16)));
		/* setup output window size */
		BESW(NV04_DSTSIZE, (
			(((moi.vcoordv & 0x0000ffff) - ((moi.vcoordv & 0xffff0000) >> 16) + 1) << 16) |
			((moi.hcoordv & 0x0000ffff) - ((moi.hcoordv & 0xffff0000) >> 16) + 1)
			));
		/* setup horizontal and vertical scaling */
		BESW(NV04_ISCALVH, (((viscalv << 16) >> 5) | (hiscalv >> 5)));
		/* enable vertical filtering (b0) */
		BESW(NV04_CTRL_V, 0x00000001);
		/* enable horizontal filtering (no effect?) */
		BESW(NV04_CTRL_H, 0x00000111);

		/* enable BES (b0), enable colorkeying (b4), format yuy2 (b8: 0 = ccir) */
		BESW(NV04_GENCTRL, 0x00000111);
		/* select buffer 1 as active (b16) */
		BESW(NV04_SU_STATE, 0x00010000);

		/**************************
		 *** setup color keying ***
		 **************************/

		/* setup colorkeying */
		switch(si->dm.space)
		{
		case B_RGB15_LITTLE:
			BESW(NV04_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 5) |
				((ow->red.value & ow->red.mask) << 10)    |
				((ow->alpha.value & ow->alpha.mask) << 15)
				));
			break;
		case B_RGB16_LITTLE:
			BESW(NV04_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 5) |
				((ow->red.value & ow->red.mask) << 11)
				/* this space has no alpha bits */
				));
			break;
		case B_CMAP8:
		case B_RGB32_LITTLE:
		default:
			BESW(NV04_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 8) |
				((ow->red.value & ow->red.mask) << 16)    |
				((ow->alpha.value & ow->alpha.mask) << 24)
				));
			break;
		}
	}
	else
	{
		/* >= NV10A */
	
		/* setup buffer origin: GeForce uses subpixel precise clipping on left and top! (12.4 values) */
		BESW(NV10_0SRCREF, ((moi.v1srcstv << 4) & 0xffff0000) | ((moi.hsrcstv >> 12) & 0x0000ffff));
		/* setup buffersize */
		//fixme if needed: width must be even officially...
		BESW(NV10_0SRCSIZE, ((ob->height << 16) | ob->width));
		/* setup source pitch including slopspace (in bytes),
		 * b16: select YUY2 (0 = YV12), b20: use colorkey, b24: no iturbt_709 (do iturbt_601) */
		/* Note:
		 * source pitch granularity = 32 pixels on GeForce cards!! */
		BESW(NV10_0SRCPTCH, (((ob->width * 2) & 0x0000ffff) | (1 << 16) | (1 << 20) | (0 << 24)));
		/* setup output window position */
		BESW(NV10_0DSTREF, ((moi.vcoordv & 0xffff0000) | ((moi.hcoordv & 0xffff0000) >> 16)));
		/* setup output window size */
		BESW(NV10_0DSTSIZE, (
			(((moi.vcoordv & 0x0000ffff) - ((moi.vcoordv & 0xffff0000) >> 16) + 1) << 16) |
			((moi.hcoordv & 0x0000ffff) - ((moi.hcoordv & 0xffff0000) >> 16) + 1)
			));
		/* setup horizontal scaling */
		BESW(NV10_0ISCALH, (hiscalv << 4));
		/* setup vertical scaling */
		BESW(NV10_0ISCALV, (viscalv << 4));
		/* setup (unclipped!) buffer startadress in RAM */
		BESW(NV10_0BUFADR, moi.a1orgv);
		/* enable BES (b0 = 0) */
		BESW(NV10_GENCTRL, 0x00000000);
		/* We only use buffer buffer 0: select it. (0x01 = buffer 0, 0x10 = buffer 1) */
		/* This also triggers activation of programmed values (double buffered registers feature) */
		BESW(NV10_BUFSEL, 0x00000001);

		/**************************
		 *** setup color keying ***
		 **************************/

		/* setup colorkeying */
		switch(si->dm.space)
		{
		case B_RGB15_LITTLE:
			BESW(NV10_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 5) |
				((ow->red.value & ow->red.mask) << 10)    |
				((ow->alpha.value & ow->alpha.mask) << 15)
				));
			break;
		case B_RGB16_LITTLE:
			BESW(NV10_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 5) |
				((ow->red.value & ow->red.mask) << 11)
				/* this space has no alpha bits */
				));
			break;
		case B_CMAP8:
		case B_RGB32_LITTLE:
		default:
			BESW(NV10_COLKEY, (
				((ow->blue.value & ow->blue.mask) << 0)   |
				((ow->green.value & ow->green.mask) << 8) |
				((ow->red.value & ow->red.mask) << 16)    |
				((ow->alpha.value & ow->alpha.mask) << 24)
				));
			break;
		}
	}

	/* note that overlay is in use (for eng_bes_move_overlay()) */
	si->overlay.active = true;

	return B_OK;
}

status_t eng_release_bes()
{
	if (si->ps.card_arch < NV10A)
	{
		/* setup BES control: disable scaler (b0 = 0) */
		BESW(NV04_GENCTRL, 0x00000000);
	}
	else
	{
		/* setup BES control: disable scaler (b0 = 1) */
		BESW(NV10_GENCTRL, 0x00000001);  
	}

	/* note that overlay is not in use (for eng_bes_move_overlay()) */
	si->overlay.active = false;

	return B_OK;
}
