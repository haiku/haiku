/* second CTRC functionality

   Authors:
   Mark Watson 6/2000,
   Rudolf Cornelissen 12/2002 - 12/2003
*/

#define MODULE_BIT 0x00020000

#include "mga_std.h"

/*set a mode line - inputs are in pixels/scanlines*/
status_t g400_crtc2_set_timing(display_mode target)
{
	uint32 temp;

	LOG(4,("CRTC2: setting timing\n"));

	if ((!(target.flags & TV_BITS)) || (si->ps.card_type <= G400MAX))
	{
		/* G450/G550 monitor mode, and all modes on older cards */

		/* check horizontal timing parameters are to nearest 8 pixels */
		if ((target.timing.h_display & 0x07) | (target.timing.h_sync_start & 0x07) |
			(target.timing.h_sync_end & 0x07) | (target.timing.h_total & 0x07))
		{
			LOG(8,("CRTC2: Horizontal timings are not multiples of 8 pixels\n"));
			return B_ERROR;
		}

		/* make sure NTSC clock killer circuitry is disabled */
		CR2W(DATACTL, (CR2R(DATACTL) & ~0x00000010));

		/* make sure CRTC2 is set to progressive scan for monitor mode */
		CR2W(CTL, (CR2R(CTL) & ~0x02001000));

		/* program the second CRTC */
		CR2W(HPARAM, ((((target.timing.h_display - 8) & 0x0fff) << 16) | 
										((target.timing.h_total - 8) & 0x0fff)));
		CR2W(HSYNC, ((((target.timing.h_sync_end - 8) & 0x0fff) << 16) |
										((target.timing.h_sync_start - 8) & 0x0fff)));
		CR2W(VPARAM, ((((target.timing.v_display - 1) & 0x0fff) << 16) |
										((target.timing.v_total - 1) & 0x0fff)));
		CR2W(VSYNC, ((((target.timing.v_sync_end - 1) & 0x0fff) << 16) |
										((target.timing.v_sync_start - 1) & 0x0fff)));
		//Mark: (wrong AFAIK, warning: SETMODE MAVEN-CRTC delay is now tuned to new setup!!)
		//CR2W(PRELOAD, (((target.timing.v_sync_start & 0x0fff) << 16) |
		//								(target.timing.h_sync_start & 0x0fff)));
		CR2W(PRELOAD, ((((target.timing.v_sync_start - 1) & 0x0fff) << 16) |
										((target.timing.h_sync_start - 8) & 0x0fff)));

		temp = (0xfff << 16);
		if (!(target.timing.flags & B_POSITIVE_HSYNC)) temp |= (0x01 << 8);
		if (!(target.timing.flags & B_POSITIVE_VSYNC)) temp |= (0x01 << 9);
		CR2W(MISC, temp);

		/* On <= G400MAX dualhead cards we need to send a copy to the MAVEN;
		 * unless TVout is active */
		if ((si->ps.secondary_head) && (!(target.flags & TV_BITS)))
													gx00_maven_set_timing(target);
	}
	else
	{
		/* G450/G550 TVout mode */
		display_mode tv_mode = target;
		uint8 frame;
		unsigned int vcount, prev_vcount;

		LOG(4,("CRTC2: setting up G450/G550 TVout mode\n"));

		/* check horizontal timing parameters are to nearest 8 pixels */
		if ((tv_mode.timing.h_display & 0x07) | (tv_mode.timing.h_sync_start & 0x07) |
			(tv_mode.timing.h_sync_end & 0x07))
		{
			LOG(8,("CRTC2: Horizontal timings are not multiples of 8 pixels\n"));
			return B_ERROR;
		}

		/* disable NTSC clock killer circuitry */
		CR2W(DATACTL, (CR2R(DATACTL) & ~0x00000010));

		if (tv_mode.timing.h_total & 0x07)
		{
			/* we rely on this for both PAL and NTSC modes if h_total is 'illegal' */
			LOG(4,("CRTC2: enabling clock killer circuitry\n"));
			CR2W(DATACTL, (CR2R(DATACTL) | 0x00000010));
		}

		/* make sure h_total is valid for TVout mode */
		tv_mode.timing.h_total &= ~0x07;

		/* modify tv_mode for interlaced use */
		tv_mode.timing.v_display >>= 1;
		tv_mode.timing.v_sync_start >>= 1;
		tv_mode.timing.v_sync_end >>= 1;
		tv_mode.timing.v_total >>= 1;

		/*program the second CRTC*/
		CR2W(HPARAM, ((((tv_mode.timing.h_display - 8) & 0x0fff) << 16) | 
										((tv_mode.timing.h_total - 8) & 0x0fff)));
		CR2W(HSYNC, ((((tv_mode.timing.h_sync_end - 8) & 0x0fff) << 16) |
										((tv_mode.timing.h_sync_start - 8) & 0x0fff)));
		CR2W(VPARAM, ((((tv_mode.timing.v_display - 1) & 0x0fff) << 16) |
										((tv_mode.timing.v_total - 1) & 0x0fff)));
		CR2W(VSYNC, ((((tv_mode.timing.v_sync_end - 1) & 0x0fff) << 16) |
										((tv_mode.timing.v_sync_start - 1) & 0x0fff)));
		//Mark: (wrong AFAIK, warning: SETMODE MAVEN-CRTC delay is now tuned to new setup!!)
		//CR2W(PRELOAD, (((tv_mode.timing.v_sync_start & 0x0fff) << 16) |
		//								(tv_mode.timing.h_sync_start & 0x0fff)));
		CR2W(PRELOAD, ((((tv_mode.timing.v_sync_start - 1) & 0x0fff) << 16) |
										((tv_mode.timing.h_sync_start - 8) & 0x0fff)));

		/* set CRTC2 to interlaced mode:
		 * First enable progressive scan mode while making sure
		 * CRTC2 is setup for TVout mode use... */
		CR2W(CTL, ((CR2R(CTL) & ~0x02000000) | 0x00001000));
		/* now synchronize to the start of a frame... */
		prev_vcount = 0;
		for (frame = 0; frame < 2; frame++)
		{
			for (;;)
			{
				vcount = (CR2R(VCOUNT) & 0x00000fff);
				if (vcount >= prev_vcount)
					prev_vcount = vcount;
				else
					break;
			}
		}
		/* and start interlaced mode now! */
		CR2W(CTL, (CR2R(CTL) | 0x02000000));

		temp = (0xfff << 16);
		if (!(tv_mode.timing.flags & B_POSITIVE_HSYNC)) temp |= (0x01 << 8);
		if (!(tv_mode.timing.flags & B_POSITIVE_VSYNC)) temp |= (0x01 << 9);
		CR2W(MISC, temp);
	}

	return B_OK;
}

status_t g400_crtc2_depth(int mode)
{
	/* validate bit depth and set mode */
	/* also clears TVout mode (b12) */
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

status_t g400_crtc2_dpms(bool display, bool h, bool v)
{
	LOG(4,("CRTC2: setting DPMS: "));

	if (si->ps.card_type <= G400MAX)
	{
		if (display && h && v)
		{
			/* enable CRTC2 and don't touch the rest */
			CR2W(CTL, ((CR2R(CTL) & 0xFFF0177E) | 0x01));
			LOG(4,("display on, hsync enabled, vsync enabled\n"));
		}
		else
		{
			/* disable CRTC2 and don't touch the rest */
			CR2W(CTL, (CR2R(CTL) & 0xFFF0177E));
			LOG(4,("display off, hsync disabled, vsync disabled\n"));
		}

		/* On <= G400MAX dualhead cards we always need to send a 'copy' to the MAVEN */
		if (si->ps.secondary_head) gx00_maven_dpms(display, h, v);
	}
	else /* G450/G550 */
	{
		/* set HD15 and DVI-A sync pol. to be fixed 'straight through' from the CRTCs,
		 * and preset enabled sync signals on both connectors.
		 * (polarities and primary DPMS are done via other registers.) */
		uint8 temp = 0x00;

		if (display)
		{
			/* enable CRTC2 and don't touch the rest */
			CR2W(CTL, ((CR2R(CTL) & 0xFFF0177E) | 0x01));
			LOG(4,("display on, "));
		}
		else
		{
			/* disable CRTC2 and don't touch the rest */
			CR2W(CTL, (CR2R(CTL) & 0xFFF0177E));
			LOG(4,("display off, "));
		}

		if (si->crossed_conns)
		{
			if (h)
			{
				/* enable DVI-A hsync */
				temp &= ~0x01;
				LOG(4,("hsync enabled, "));
			}
			else
			{
				/* disable DVI-A hsync */
				temp |= 0x01;
				LOG(4,("hsync disabled, "));
			}
			if (v)
			{
				/* enable DVI-A vsync */
				temp &= ~0x02;
				LOG(4,("vsync enabled\n"));
			}
			else
			{
				/* disable DVI-A vsync */
				temp |= 0x02;
				LOG(4,("vsync disabled\n"));
			}
		}
		else
		{
			if (h)
			{
				/* enable HD15 hsync */
				temp &= ~0x10;
				LOG(4,("hsync enabled, "));
			}
			else
			{
				/* disable HD15 hsync */
				temp |= 0x10;
				LOG(4,("hsync disabled, "));
			}
			if (v)
			{
				/* enable HD15 vsync */
				temp &= ~0x20;
				LOG(4,("vsync enabled\n"));
			}
			else
			{
				/* disable HD15 vsync */
				temp |= 0x20;
				LOG(4,("vsync disabled\n"));
			}
		}
		/* program new syncs */
		DXIW(SYNCCTRL, temp);
	}

	return B_OK;
}

status_t g400_crtc2_dpms_fetch(bool *display, bool *h, bool *v)
{
	*display = (CR2R(CTL) & 0x00000001);

	if ((si->ps.card_type <= G400MAX))
	{
		/* no full DPMS support: display controls all signals */
		*h = *v = *display;
	}
	else
	{
		/* G450 and G550 have full DPMS support on CRTC2 */
		if (si->crossed_conns)
		{
			*h = !(DXIR(SYNCCTRL) & 0x01);
			*v = !(DXIR(SYNCCTRL) & 0x02);
		}
		else
		{
			*h = !(DXIR(SYNCCTRL) & 0x10);
			*v = !(DXIR(SYNCCTRL) & 0x20);
		}
	}

	LOG(4,("CTRC2: fetched DPMS state: "));
	if (*display) LOG(4,("display on, "));
	else LOG(4,("display off, "));
	if (*h) LOG(4,("hsync enabled, "));
	else LOG(4,("hsync disabled, "));
	if (*v) LOG(4,("vsync enabled\n"));
	else LOG(4,("vsync disabled\n"));

	return B_OK;
}

status_t g400_crtc2_set_display_pitch() 
{
	uint32 offset;

	LOG(4,("CRTC2: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs */
	offset = si->fbc.bytes_per_row;
	if (si->interlaced_tv_mode)
	{
		LOG(4,("CRTC2: setting interlaced mode\n"));
		/* double the CRTC2 linelength so fields are displayed instead of frames */
		offset *= 2;
	}
	else
		LOG(4,("CRTC2: setting progressive scan mode\n"));

	LOG(2,("CRTC2: offset set to %d bytes\n", offset));

	/* program the head */
	CR2W(OFFSET,offset);
	return B_OK;
}

status_t g400_crtc2_set_display_start(uint32 startadd,uint8 bpp) 
{
	LOG(4,("CRTC2: setting card RAM to be displayed for %d bits per pixel\n", bpp));

	LOG(2,("CRTC2: startadd: $%x\n",startadd));
	LOG(2,("CRTC2: frameRAM: $%x\n",si->framebuffer));
	LOG(2,("CRTC2: framebuffer: $%x\n",si->fbc.frame_buffer));

	if (si->interlaced_tv_mode)
	{
		LOG(4,("CRTC2: setting up fields for interlaced mode\n"));
		/* program the head for interlaced use */
		//fixme: seperate both heads: we need a secondary si->fbc!
		/* setup field 0 startadress in buffer to read picture's odd lines */
		CR2W(STARTADD0,	(startadd + si->fbc.bytes_per_row));
		/* setup field 1 startadress in buffer to read picture's even lines */
		CR2W(STARTADD1, startadd);
	}
	else
	{
		LOG(4,("CRTC2: setting up frames for progressive scan mode\n"));
		/* program the head for non-interlaced use */
		CR2W(STARTADD0, startadd);
	}

	return B_OK;
}
