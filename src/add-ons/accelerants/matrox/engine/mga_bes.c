/* G200-G550 Back End Scaler functions V0.13 beta1 */
/* Written by Rudolf Cornelissen 05/08-2002 */

#define MODULE_BIT 0x00000200

#include "mga_std.h"

status_t gx00_configure_bes(const overlay_buffer *ob, const overlay_window *ow, int offset)
{
	/* yuy2 (4:2:2) colorspace calculations */
	/* Note: Some calculations will have to be modified for other colorspaces if they are incorporated. */

	/* Note:
	 * in BeOS R5.0.3 (maybe DANO works different):
	 * 'ow->offset_xxx' is always 0, so not used;
	 * 'ow->width' and 'ow->height' are the output window size: does not change
	 * if window is clipping;
	 * 'ow->h_start' and 'ow->v_start' are the left-top position of the output
	 * window. These values can be negative: this means the window is clipping
	 * at the left or the top of the display, respectively. */
	 
	/* misc used variables */
	uint32 temp32;
	uint16 temp1, temp2;
	/* interval representation, used for scaling calculations */
	uint16 intrep;
	/* inverse scaling factor, used for source positioning */
	uint32 ifactor;
	/* used for vertical weight starting value */
	uint32 weight;

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


	/*************************************
	 *** sync to BES (Back End Scaler) ***
	 *************************************/

	/* Make sure reprogramming the BES completes before the next retrace occurs, to prevent
	 * register-update glitches (double buffer feature).
	 * Programming the BES needs about 50 lines with a 1600 x 1200 x 90Hz screen with
	 * logging mostly disabled on a P3-500. */

	LOG(3,("Overlay: entering at Vcount %d\n", CR1R(VCOUNT)));
	while (CR1R(VCOUNT) > (si->dm.timing.v_total - 100));
	LOG(3,("Overlay: starting at Vcount %d\n", CR1R(VCOUNT)));

	/****************************************
	 *** setup all edges of output window ***
	 ****************************************/

	/* setup left and right edges of output window */
	temp32 = 0;
	/* left edge coordinate of output window, must be inside desktop */
	/* clipping on the left side */
	if (ow->h_start < 0)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the right side */
		if (ow->h_start >= (si->dm.virtual_width - 1))
		{
			/* width < 2 is not allowed */
			temp1 = (si->dm.virtual_width - 2) & 0x7ff;
		} 
		else
		/* no clipping here */
		{
			temp1 = (uint16)ow->h_start & 0x7ff;
		}
	} 
	temp32 |= temp1 << 16;
	/* right edge coordinate of output window, must be inside desktop */
	/* width < 2 is not allowed */
	if (ow->width < 2) 
	{
		temp2 = (temp1 + 1) & 0x7ff;
	}
	else 
	{
		/* clipping on the right side */
		if ((ow->h_start + ow->width - 1) > (si->dm.virtual_width - 1))
		{
			temp2 = (si->dm.virtual_width - 1) & 0x7ff;
		}
		else
		{
			/* clipping on the left side */
			if ((ow->h_start + ow->width - 1) < 1)
			{
				/* width < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(ow->h_start + ow->width - 1)) & 0x7ff;
			}
		}
	}
	temp32 |= temp2 << 0;
	BESW(HCOORD, temp32);
	LOG(4,("Overlay: left-edge output %d, right-edge output %d\n",temp1, temp2));

	/* setup top and bottom edges of output window */
	temp32 = 0;
	/* top edge coordinate of output window, must be inside desktop */
	/* clipping on the top side */
	if (ow->v_start < 0)
	{
		temp1 = 0;
	}
	else
	{
		/* clipping on the bottom side */
		if (ow->v_start >= (si->dm.virtual_height - 1))
		{
			/* height < 2 is not allowed */
			temp1 = (si->dm.virtual_height - 2) & 0x7ff;
		} 
		else
		/* no clipping here */
		{
			temp1 = (uint16)ow->v_start & 0x7ff;
		}
	} 
	temp32 |= temp1 << 16;
	/* bottom edge coordinate of output window, must be inside desktop */
	/* height < 2 is not allowed */
	if (ow->height < 2) 
	{
		temp2 = (temp1 + 1) & 0x7ff;
	}
	else 
	{
		/* clipping on the bottom side */
		if ((ow->v_start + ow->height - 1) > (si->dm.virtual_height - 1))
		{
			temp2 = (si->dm.virtual_height - 1) & 0x7ff;
		}
		else
		{
			/* clipping on the top side */
			if ((ow->v_start + ow->height - 1) < 1)
			{
				/* height < 2 is not allowed */
				temp2 = 1;
			}
			else
			/* no clipping here */
			{
				temp2 = ((uint16)(ow->v_start + ow->height - 1)) & 0x7ff;
			}
		}
	}
	temp32 |= temp2 << 0;
	BESW(VCOORD, temp32);
	LOG(4,("Overlay: top-edge output %d, bottom-edge output %d\n",temp1, temp2));


	/*********************************************
	 *** setup horizontal scaling and clipping ***
	 *********************************************/

	LOG(4,("Overlay: input picture width = %d, height = %d\n",
			(ob->width - si->overlay.myBufInfo[offset].slopspace), ob->height));
	LOG(4,("Overlay: output picture width = %d, height = %d\n", ow->width, ow->height));

	/* do horizontal scaling... */
	/* determine interval representation value */
	if (ow->flags & B_OVERLAY_HORIZONTAL_FILTERING)
	{
		/* horizontal filtering is ON */
		if (((ob->width - si->overlay.myBufInfo[offset].slopspace) == ow->width) | (ow->width < 2))
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
		if ((ow->width < (ob->width - si->overlay.myBufInfo[offset].slopspace)) & (ow->width >= 2))
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

	/* calculate inverse horizontal scaling factor */
	/* (using standard scaling formula: neglecting round-off var as extra error is very small..) */
	ifactor = ((ob->width - si->overlay.myBufInfo[offset].slopspace - intrep) << 16) / 
			 (ow->width - intrep); 
	LOG(4,("Overlay: horizontal scaling factor is %f\n", (float)65536 / ifactor));

	/* compensate for accelerated 2x zoom (slowdown BES if pixelclock is too high) */
	temp32 = ifactor * acczoom;
	LOG(4,("Overlay: horizontal speed compensated factor is %f\n", (float)65536 / temp32));

	/* check scaling factor (and modify if needed) to be within scaling limits */
	if ((((ob->width - si->overlay.myBufInfo[offset].slopspace) << 16) / 16384) > temp32)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		temp32 = (((ob->width - si->overlay.myBufInfo[offset].slopspace) << 16) / 16384);
		LOG(4,("Overlay: horizontal scaling factor too large, clamping at %f\n", (float)65536 / temp32));
	}
	if (temp32 >= (32 << 16))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		temp32 = 0x1ffffc;
		LOG(4,("Overlay: horizontal scaling factor too small, clamping at %f\n", (float)65536 / temp32));
	}
	/* AND below is required by hardware */
	temp32 &= 0x001ffffc;
	BESW(HISCAL, temp32);


	/* do horizontal clipping... */
	/* Setup horizontal source start: first (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap.
	 * The input bitmaps slopspace is automatically excluded from the calculations this way! */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */

	temp32 = 0;
	/* check for destination horizontal clipping at left side */
	if (ow->h_start < 0)
	{
		/* check if entire destination picture is clipping left:
		 * (2 pixels will be clamped onscreen at least) */
		if ((ow->h_start + ow->width - 1) < 1)
		{
			/* increase 'first contributing pixel' with 'fixed value': (total dest. width - 2) */
			temp32 += (ow->width - 2);
		}
		else
		{
			/* increase 'first contributing pixel' with actual number of dest. clipping pixels */
			temp32 += (0 - ow->h_start);
		}
		LOG(4,("Overlay: clipping left...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		temp32 *= ifactor;
	}
	/* AND below required by hardware */
	temp32 &= 0x03fffffc;
	BESW(HSRCST, temp32);
	LOG(4,("Overlay: first hor. (sub)pixel of input bitmap contributing %f\n", temp32 / (float)65536));


	/* Setup horizontal source end: last (sub)pixel contributing to output picture */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * After this is done, include the scaling factor so you get a value based on the input bitmap. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source end pos.! */

	temp32 = 0;
	/* check for destination horizontal clipping at right side */
	if ((ow->h_start + ow->width - 1) > (si->dm.virtual_width - 1))
	{
		/* check if entire destination picture is clipping right:
		 * (2 pixels will be clamped onscreen at least) */
		if (ow->h_start > (si->dm.virtual_width - 2))
		{
			/* increase 'number of clipping pixels' with 'fixed value': (total dest. width - 2) */
			temp32 += (ow->width - 2);
		}
		else
		{
			/* increase 'number of clipping pixels' with actual number of dest. clipping pixels */
			temp32 += ((ow->h_start + ow->width - 1) - (si->dm.virtual_width - 1));
		}
		LOG(4,("Overlay: clipping right...\n"));

		/* The calculated value is based on scaling = 1x. So we now compensate for scaling.
		 * Note that this also already takes care of aligning the value to the BES register! */
		temp32 *= ifactor;

		/* now subtract this value from the last used pixel in inputbuffer, aligned to BES */
		temp32 = (((ob->width - 1) - si->overlay.myBufInfo[offset].slopspace) << 16) - temp32;
	}
	else
	{
		/* set last contributing pixel to last used pixel in inputbuffer, aligned to BES */
		temp32 = ((ob->width - 1) - si->overlay.myBufInfo[offset].slopspace) << 16;
	}
	/* AND below required by hardware */
	temp32 &= 0x03fffffc;
	BESW(HSRCEND, temp32);
	LOG(4,("Overlay: last horizontal (sub)pixel of input bitmap contributing %f\n", temp32 / (float)65536));


	/* setup horizontal source last position excluding slopspace */
	temp32 = ((ob->width - 1) - si->overlay.myBufInfo[offset].slopspace) << 16; 
	/* AND below required by hardware */
	temp32 &= 0x03ff0000;
	BESW(HSRCLST, temp32);


	/*******************************************
	 *** setup vertical scaling and clipping ***
	 *******************************************/

	/* do vertical scaling... */
	/* determine interval representation value */
	if (ow->flags & B_OVERLAY_VERTICAL_FILTERING)
	{
		/* vertical filtering is ON */
		if ((ob->height == ow->height) | (ow->height < 2))
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
		if ((ow->height < ob->height) & (ow->height >= 2))
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

	/* calculate inverse vertical scaling factor */
	/* (using standard scaling formula: neglecting round-off var as extra error is very small..) */
	ifactor = ((ob->height - intrep) << 16) / (ow->height - intrep); 
	LOG(4,("Overlay: vertical scaling factor is %f\n", (float)65536 / ifactor));

	/* preserve ifactor for source positioning calculations later on */
	temp32 = ifactor;

	/* check scaling factor (and modify if needed) to be within scaling limits */
	if (((ob->height << 16) / 16384) > temp32)
	{
		/* (non-inverse) factor too large, set factor to max. valid value */
		temp32 = ((ob->height << 16) / 16384);
		LOG(4,("Overlay: vertical scaling factor too large, clamping at %f\n", (float)65536 / temp32));
	}
	if (temp32 >= (32 << 16))
	{
		/* (non-inverse) factor too small, set factor to min. valid value */
		temp32 = 0x1ffffc;
		LOG(4,("Overlay: vertical scaling factor too small, clamping at %f\n", (float)65536 / temp32));
	}
	/* AND below is required by hardware */
	temp32 &= 0x001ffffc;
	BESW(VISCAL, temp32);


	/* do vertical clipping... */
	/* Setup vertical source start: first (sub)pixel contributing to output picture.
	 * Note: this exists of two parts:
	 * 1. setup fractional part (sign is always 'positive');
	 * 2. setup relative base_adress, taking clipping on top into account. 
	 * Both parts are done intertwined below. */
	/* Note:
	 * The method is to calculate, based on 1:1 scaling, based on the output window.
	 * 'After' this is done, include the scaling factor so you get a value based on the input bitmap. */
	/* Note also:
	 * Even if the scaling factor is clamping we instruct the BES to use the correct source start pos.! */

	/* calculate relative base_adress and 'vertical weight fractional part' */
	weight = 0;
	temp32 = (uint32)((vuint32 *)ob->buffer);
	temp32 -= (uint32)((vuint32 *)si->framebuffer);
	LOG(4,("Overlay: topleft corner of input bitmap (cardRAM offset) $%08x\n",temp32));
	/* calculate origin adress */
	/* check for destination vertical clipping at top side */
	if (ow->v_start < 0)
	{
		/* check if entire destination picture is clipping at top:
		 * (2 pixels will be clamped onscreen at least) */
		if ((ow->v_start + ow->height - 1) < 1)
		{
			/* increase source buffer origin with 'fixed value':
			 * (integer part of ('total height - 2' of dest. picture in pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			temp32 += ((((ow->height - 2) * ifactor) >> 16) * ob->bytes_per_row);
			weight = (ow->height - 2) * ifactor;
		}
		else
		{
			/* increase source buffer origin with:
			 * (integer part of (number of destination picture clipping pixels * inverse scaling factor)) *
			 * bytes per row source picture */
			temp32 += ((((0 - ow->v_start) * ifactor) >> 16) * ob->bytes_per_row);
			weight = (0 - ow->v_start) * ifactor;
		}
		LOG(4,("Overlay: clipping at top: buffer origin is (cardRAM offset) $%08x\n",temp32));
	}
	LOG(4,("Overlay: first vert. (sub)pixel of input bitmap contributing %f\n", weight / (float)65536));

	/* Note:
	 * Because later G200 and all > G200 overlay units will ignore b0-3 of the calculated adress,
	 * we do not use the above way for horizontal source positioning.
	 * (Early G200 cards ignore b0-2.)
	 * If we did, 8 source-image pixel jumps (in 4:2:2 colorspace) will occur if the picture
	 * is shifted horizontally during left clipping on later G200 and all > G200 cards, while 
	 * early G200 cards will have 4 source-image pixel jumps occuring. */

	/* AND below is required by G200-G550 hardware. All cards can have max. 32Mb RAM on board
	 * (incl. later G200 cards!). Compatible setting used (between early G200 and the rest),
	 * this has no downside consequences here. */
	temp32 &= 0x01fffff0;
	/* buffer A topleft corner of field 1 (origin)(field 1 contains our full frames) */
	BESW(A1ORG, temp32);

	/* field 1 weight: AND below required by hardware, also make sure 'sign' is always 'positive' */
	temp32 = weight & 0x0000fffc;
	BESW(V1WGHT, temp32);


	/* setup field 1 (is our complete frame) vertical source contributing height - 1.
	 * Note:
	 * This value is bottom-unclipped! (as it should be according to the MGA specs...) */
	temp32 = (ob->height - 1) - (weight >> 16);
	/* AND below required by hardware */
	temp32 &= 0x000003ff;
	BESW(V1SRCLST, temp32);
	LOG(4,("Overlay: input bitmap bottom-unclipped contributing height (integer part) %d\n", temp32 + 1));


	/**************************
	 *** setup color keying ***
	 **************************/

	LOG(4,("Overlay: key_red %d, key_green %d, key_blue %d, key_alpha %d\n",
		ow->red.value, ow->green.value, ow->blue.value, ow->alpha.value));
	LOG(4,("Overlay: mask_red %d, mask_green %d, mask_blue %d, mask_alpha %d\n",
		ow->red.mask, ow->green.mask, ow->blue.mask, ow->alpha.mask));

	/* setup colorkeying */
	DXIW(COLKEY, (ow->alpha.value & ow->alpha.mask));

	DXIW(COLKEY0RED, (ow->red.value & ow->red.mask));
	DXIW(COLKEY0GREEN, (ow->green.value & ow->green.mask));
	DXIW(COLKEY0BLUE, (ow->blue.value & ow->blue.mask));

	DXIW(COLMSK, ow->alpha.mask);

	DXIW(COLMSK0RED, ow->red.mask);
	DXIW(COLMSK0GREEN, ow->green.mask);
	DXIW(COLMSK0BLUE, ow->blue.mask);

	/* enable colorkeying */
	DXIW(KEYOPMODE,0x01);


	/*************************
	 *** setup misc. stuff ***
	 *************************/

	/* setup brightness and contrast to be 'neutral' (this is not implemented on G200) */
	BESW(LUMACTL, 0x00000080);

	/* setup source pitch including slopspace (in pixels) */
	temp32 = ob->width;
	/* AND below required by hardware */
	temp32 &= 0x00000fff;
	BESW(PITCH, temp32);


	/*************************
	 *** setup BES control ***
	 *************************/

	/* BES global control: setup functions */
	temp32 = 0;

	/* slowdown BES if nessesary */
	if (acczoom == 1)
	{
		/* run at full speed and resolution */
		temp32 |= 0 << 0;
		/* disable filtering for half speed interpolation */
		temp32 |= 0 << 1;
	}
	else
	{
		/* run at half speed and resolution */
		temp32 |= 1 << 0;
		/* enable filtering for half speed interpolation */
		temp32 |= 1 << 1;
	}

	/* 4:2:0 specific setup: not needed here */
	temp32 |= 0 << 3;
	/* BES testregister: keep zero */	
	temp32 |= 0 << 4;
	/* the following bits marked (> G200) *must* be zero on G200: */
	/* 4:2:0 specific setup: not needed here (> G200) */
	temp32 |= 0 << 5;
	/* select yuy2 byte-order to B_YCbCr422 (> G200) */
	temp32 |= 0 << 6;
	/* BES internal contrast and brighness controls are not used, disabled (> G200) */
	temp32 |= 0 << 7;
	/* RGB specific setup: not needed here, so disabled (> G200) */
	temp32 |= 0 << 8;
	temp32 |= 0 << 9;
	/* 4:2:0 specific setup: not needed here (> G200) */
	temp32 |= 0 << 10;
	/* Tell BES when to copy the new register values to the actual active registers.
	 * bits 16-27 (12 bits) are the CRTC vert. count value at which copying takes
	 * place.
	 * (This is the double buffering feature: programming must be completed *before*
	 *  the CRTC vert count value set here!) */
	/* CRTC vert count for copying = $000, so during retrace, line 0. */
	temp32 |= 0x000 << 16;
	BESW(GLOBCTL, temp32);  

	/* BES control: enable scaler and setup functions */
	/* pre-reset all bits */
	temp32 = 0;
	/* enable BES */
	temp32 |= 1 << 0;
	/* we start displaying at an even startline (zero) in 'field 1' (no hardware de-interlacing is used) */
	temp32 |= 0 << 6;
	/* we don't use field 2, so its startline is not important */
	temp32 |= 0 << 7;

	LOG(4,("Overlay: ow->flags is $%08x\n",ow->flags));
	/* enable horizontal filtering on scaling if asked for */
	if (ow->flags & B_OVERLAY_HORIZONTAL_FILTERING)
	{
		temp32 |= 1 << 10;
		LOG(4,("Overlay: using horizontal filtering\n"));
	}
	else
	{
		temp32 |= 0 << 10;
	}
	/* enable vertical filtering on scaling if asked for */
	if (ow->flags & B_OVERLAY_VERTICAL_FILTERING)
	{
		temp32 |= 1 << 11;
		LOG(4,("Overlay: using vertical filtering\n"));
	}
	else
	{
		temp32 |= 0 << 11;
	}

	/* use actual calculated weight for horizontal interpolation if scaling */
	temp32 |= 0 << 12;
	/* use horizontal chroma interpolation upsampling on BES input picture */
	temp32 |= 1 << 16;
	/* select 4:2:2 BES input format */
	temp32 |= 0 << 17;
	/* dithering is not used */
	temp32 |= 0 << 18;
	/* horizontal mirroring is not used */
	temp32 |= 0 << 19;
	/* BES output should be in color */
	temp32 |= 0 << 20;
	/* BES output blanking is disabled: we want a picture, no 'black box'! */
	temp32 |= 0 << 21;
	/* we do software field select (field select is not used) */	
	temp32 |= 0 << 24;
	/* we always display field 1 in buffer A, this contains our full frames */
	/* select field 1 */
	temp32 |= 0 << 25;
	/* select buffer A */
	temp32 |= 0 << 26;
	BESW(CTL, temp32);  

	LOG(3,("Overlay: completed at Vcount %d\n", CR1R(VCOUNT)));

	return B_OK;
}

status_t gx00_release_bes()
{
	/* setup BES control: disable scaler */
	BESW(CTL, 0x00000000);  

	return B_OK;
}
