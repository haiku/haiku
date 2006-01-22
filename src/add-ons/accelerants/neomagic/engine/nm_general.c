/* Author:
   Rudolf Cornelissen 4/2003-1/2006
*/

#define MODULE_BIT 0x00008000

#include "nm_std.h"

static status_t test_ram(void);
static status_t nmxxxx_general_powerup (void);
static status_t nm_general_bios_to_powergraphics(void);

static void nm_dump_configuration_space (void)
{
#define DUMP_CFG(reg, type) if (si->ps.card_type >= type) do { \
	uint32 value = CFGR(reg); \
	MSG(("configuration_space 0x%02x %20s 0x%08x\n", \
		NMCFG_##reg, #reg, value)); \
} while (0)
	DUMP_CFG (DEVID,	0);
	DUMP_CFG (DEVCTRL,	0);
	DUMP_CFG (CLASS,	0);
	DUMP_CFG (HEADER,	0);
	DUMP_CFG (BASE1FB,	0);
	DUMP_CFG (BASE2REG1,0);
	DUMP_CFG (BASE3REG2,0);
	DUMP_CFG (BASE4,	0);
	DUMP_CFG (BASE5,	0);
	DUMP_CFG (BASE6,	0);
	DUMP_CFG (BASE7,	0);
	DUMP_CFG (SUBSYSID1,0);
	DUMP_CFG (ROMBASE,	0);
	DUMP_CFG (CAPPTR,	0);
	DUMP_CFG (CFG_1,	0);
	DUMP_CFG (INTERRUPT,0);
	DUMP_CFG (CFG_3,	0);
	DUMP_CFG (CFG_4,	0);
	DUMP_CFG (CFG_5,	0);
	DUMP_CFG (CFG_6,	0);
	DUMP_CFG (CFG_7,	0);
	DUMP_CFG (CFG_8,	0);
	DUMP_CFG (CFG_9,	0);
	DUMP_CFG (CFG_10,	0);
	DUMP_CFG (CFG_11,	0);
	DUMP_CFG (CFG_12,	0);
	DUMP_CFG (CFG_13,	0);
	DUMP_CFG (CFG_14,	0);
	DUMP_CFG (CFG_15,	0);
	DUMP_CFG (CFG_16,	0);
	DUMP_CFG (CFG_17,	0);
	DUMP_CFG (CFG_18,	0);
	DUMP_CFG (CFG_19,	0);
	DUMP_CFG (CFG_20,	0);
	DUMP_CFG (CFG_21,	0);
	DUMP_CFG (CFG_22,	0);
	DUMP_CFG (CFG_23,	0);
	DUMP_CFG (CFG_24,	0);
	DUMP_CFG (CFG_25,	0);
	DUMP_CFG (CFG_26,	0);
	DUMP_CFG (CFG_27,	0);
	DUMP_CFG (CFG_28,	0);
	DUMP_CFG (CFG_29,	0);
	DUMP_CFG (CFG_30,	0);
	DUMP_CFG (CFG_31,	0);
	DUMP_CFG (CFG_32,	0);
	DUMP_CFG (CFG_33,	0);
	DUMP_CFG (CFG_34,	0);
	DUMP_CFG (CFG_35,	0);
	DUMP_CFG (CFG_36,	0);
	DUMP_CFG (CFG_37,	0);
	DUMP_CFG (CFG_38,	0);
	DUMP_CFG (CFG_39,	0);
	DUMP_CFG (CFG_40,	0);
	DUMP_CFG (CFG_41,	0);
	DUMP_CFG (CFG_42,	0);
	DUMP_CFG (CFG_43,	0);
	DUMP_CFG (CFG_44,	0);
	DUMP_CFG (CFG_45,	0);
	DUMP_CFG (CFG_46,	0);
	DUMP_CFG (CFG_47,	0);
	DUMP_CFG (CFG_48,	0);
	DUMP_CFG (CFG_49,	0);
	DUMP_CFG (CFG_50,	0);
#undef DUMP_CFG
}
	
status_t nm_general_powerup()
{
	status_t status;

	LOG(1,("POWERUP: Haiku Neomagic Accelerant 0.14 running.\n"));

	/* log VBLANK INT usability status */
	if (si->ps.int_assigned)
		LOG(4,("POWERUP: Usable INT assigned to HW; Vblank semaphore enabled\n"));
	else
		LOG(4,("POWERUP: No (usable) INT assigned to HW; Vblank semaphore disabled\n"));

	/* WARNING:
	 * _adi.name_ and _adi.chipset_ can contain 31 readable characters max.!!! */

	/* detect card type and power it up */
	switch(CFGR(DEVID))
	{
	case 0x000110c8: //NM2070 ISA
		si->ps.card_type = NM2070;
		sprintf(si->adi.name, "Neomagic MagicGraph 128");
		sprintf(si->adi.chipset, "NM2070 (ISA)");
		break;
	case 0x000210c8: //NM2090 ISA
		si->ps.card_type = NM2090;
		sprintf(si->adi.name, "Neomagic MagicGraph 128V");
		sprintf(si->adi.chipset, "NM2090 (ISA)");
		break;
	case 0x000310c8: //NM2093 ISA
		si->ps.card_type = NM2093;
		sprintf(si->adi.name, "Neomagic MagicGraph 128ZV");
		sprintf(si->adi.chipset, "NM2093 (ISA)");
		break;
	case 0x008310c8: //NM2097 PCI
		si->ps.card_type = NM2097;
		sprintf(si->adi.name, "Neomagic MagicGraph 128ZV+");
		sprintf(si->adi.chipset, "NM2097 (PCI)");
		break;
	case 0x000410c8: //NM2160 PCI
		si->ps.card_type = NM2160;
		sprintf(si->adi.name, "Neomagic MagicGraph 128XD");
		sprintf(si->adi.chipset, "NM2160 (PCI)");
		break;
	case 0x000510c8: //NM2200
		si->ps.card_type = NM2200;
		sprintf(si->adi.name, "Neomagic MagicMedia 256AV");
		sprintf(si->adi.chipset, "NM2200");
		break;
	case 0x002510c8: //NM2230
		si->ps.card_type = NM2230;
		sprintf(si->adi.name, "Neomagic MagicMedia 256AV+");
		sprintf(si->adi.chipset, "NM2230");
		break;
	case 0x000610c8: //NM2360
		si->ps.card_type = NM2360;
		sprintf(si->adi.name, "Neomagic MagicMedia 256ZX");
		sprintf(si->adi.chipset, "NM2360");
		break;
	case 0x001610c8: //NM2380
		si->ps.card_type = NM2380;
		sprintf(si->adi.name, "Neomagic MagicMedia 256XL+");
		sprintf(si->adi.chipset, "NM2380");
		break;
	default:
		LOG(8,("POWERUP: Failed to detect valid card 0x%08x\n",CFGR(DEVID)));
		return B_ERROR;
	}

	/* power up the card */
	status = nmxxxx_general_powerup();

	/* override memory detection if requested by user */
	if (si->settings.memory != 0)
		si->ps.memory_size = si->settings.memory;

	return status;
}

static status_t test_ram(void)
{
	uint32 value, offset;
	status_t result = B_OK;

	/* make sure we don't corrupt the hardware cursor by using fbc.frame_buffer. */
	if (si->fbc.frame_buffer == NULL)
	{
		LOG(8,("INIT: test_ram detected NULL pointer.\n"));
		return B_ERROR;
	}

	for (offset = 0, value = 0x55aa55aa; offset < 256; offset++)
	{
		/* write testpattern to cardRAM */
		((uint32 *)si->fbc.frame_buffer)[offset] = value;
		/* toggle testpattern */
		value = 0xffffffff - value;
	}

	for (offset = 0, value = 0x55aa55aa; offset < 256; offset++)
	{
		/* readback and verify testpattern from cardRAM */
		if (((uint32 *)si->fbc.frame_buffer)[offset] != value) result = B_ERROR;
		/* toggle testpattern */
		value = 0xffffffff - value;
	}
	return result;
}

/* NOTE:
 * This routine *has* to be done *after* SetDispplayMode has been executed,
 * or test results will not be representative!
 * (CAS latency is dependant on nm setup on some (DRAM) boards) */
status_t nm_set_cas_latency()
{
	/* check current RAM access to see if we need to change anything */
	if (test_ram() == B_OK)
	{
		LOG(4,("INIT: RAM access OK.\n"));
		return B_OK;
	}

	/* check if we read PINS at starttime so we have valid registersettings at our disposal */
	LOG(4,("INIT: RAM access errors; not fixable: missing coldstart specs.\n"));
	return B_ERROR;
}

void nm_unlock()
{
	/* unlock cards GRAPHICS registers (any other value than 0x26 should lock it again) */
    ISAGRPHW(GRPHXLOCK, 0x26);
}

static status_t nmxxxx_general_powerup()
{
	uint8 temp;
//	status_t result;
	
	LOG(4, ("INIT: powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) nm_dump_configuration_space();

	/* set ISA registermapping to VGA colormode */
	temp = (ISARB(MISCR) | 0x01);
	/* we need to wait a bit or the card will mess-up it's register values.. */
	snooze(10);
	ISAWB(MISCW, temp);

	/* unlock cards GRAPHICS registers (any other value than 0x26 should lock it again) */
    ISAGRPHW(GRPHXLOCK, 0x26);

	/* unlock cards CRTC registers */
//don't touch: most cards get into trouble on this!
//	ISAGRPHW(GENLOCK, 0x00);

	/* initialize the shared_info struct */
	set_specs();
	/* log the struct settings */
	dump_specs();

	/* activate PCI access: b7 = framebuffer, b6 = registers */
	ISAGRPHW(IFACECTRL2, 0xc0);

	/* disable VGA-mode cursor (just to be sure) */
	ISACRTCW(VGACURCTRL, 0x00);

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	/*if (si->settings.usebios || (result != B_OK)) */return nm_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	LOG(2,("INIT: powerup\n"));

	/* turn off both displays and the hardcursor (also disables transfers) */
	nm_crtc_dpms(false, false, false);
	nm_crtc_cursor_hide();

	/* setup sequencer clocking mode */
	ISASEQW(CLKMODE, 0x21);

	//fixme: setup coldstart capability...

	/* turn on display */
	nm_crtc_dpms(true, true, true);

	return B_OK;
}

status_t nm_general_output_select()
{
	/* log currently selected output */
	switch (nm_general_output_read() & 0x03)
	{
	case 0x01:
		LOG(2, ("INIT: external CRT only mode active\n"));
		break;
	case 0x02:
		LOG(2, ("INIT: internal LCD only mode active\n"));
		break;
	case 0x03:
		LOG(2, ("INIT: simultaneous LCD/CRT mode active\n"));
		break;
	}
	return B_OK;
}

uint8 nm_general_output_read()
{
	uint8 size_outputs;

	/* read panelsize and currently active outputs */
	size_outputs = ISAGRPHR(PANELCTRL1);

	/* update noted currently active outputs if prudent:
	 * - tracks active output devices, even if the keyboard shortcut is used because
	 *   using that key will take the selected output devices out of DPMS sleep mode
	 *   if programmed via these bits;
	 * - when the shortcut key is not used, and DPMS was in a power-saving mode while
	 *   programmed via these bits, an output device will (still) be shut-off. */
	if (si->ps.card_type < NM2200)
	{
		/* both output devices do DPMS via these bits.
		 * if one of the bits is on we have a valid setting if no power-saving mode
		 * is active, or the keyboard shortcut key for selecting output devices has
		 * been used during power-saving mode. */
		if (size_outputs & 0x03) si->ps.outputs = (size_outputs & 0x03);
	}
	else
	{
		/* check if any power-saving mode active */
		if 	(!(ISASEQR(CLKMODE) & 0x20))
		{
			/* screen is on:
			 * we can 'safely' copy both output devices settings... */
			if (size_outputs & 0x03)
			{
				si->ps.outputs = (size_outputs & 0x03);
			}
			else
			{
				/* ... unless no output is active, as this is illegal then */
				si->ps.outputs = 0x02;

				LOG(4, ("INIT: illegal outputmode detected, assuming internal mode!\n"));
			}
		}
		else
		{
			/* screen is off:
			 * only the internal panel does DPMS via these bits, so the external setting
			 * can always be copied */
			si->ps.outputs &= 0xfe;
			si->ps.outputs |= (size_outputs & 0x01);
			/* if the panel is on, we can note that (the shortcut key has been used) */
			if (size_outputs & 0x02) si->ps.outputs |= 0x02;
		}
	}

	return ((size_outputs & 0xfc) | (si->ps.outputs & 0x03));
}

/* basic change of card state from VGA to powergraphics -> should work from BIOS init state*/
static 
status_t nm_general_bios_to_powergraphics()
{
	uint8 temp;

	LOG(2, ("INIT: Skipping card coldstart!\n"));

	/* turn off display */
	nm_crtc_dpms(false, false, false);

	/* set card to 'enhanced' mode: (only VGA standard registers used for NeoMagic cards) */
	/* (keep) card enabled, set plain normal memory usage, no old VGA 'tricks' ... */
	ISACRTCW(MODECTL, 0xc3);
	/* ... plain sequential memory use, more than 64Kb RAM installed,
	 * switch to graphics mode ... */
	ISASEQW(MEMMODE, 0x0e);
	/* ... disable bitplane tweaking ... */
	/* note:
	 * NeoMagic cards have custom b4-b7 use in this register! */
	ISAGRPHW(ENSETRESET, 0x80);
	/* ... no logical function tweaking with display data, no data rotation ... */
	ISAGRPHW(DATAROTATE, 0x00);
	/* ... reset read map select to plane 0 ... */
	ISAGRPHW(READMAPSEL, 0x00);
	/* ... set standard mode ... */
	ISAGRPHW(MODE, 0x00);
	/* ... ISA framebuffer mapping is 64Kb window, switch to graphics mode (again),
	 * select standard adressing ... */
	ISAGRPHW(MISC, 0x05);
	/* ... disable bit masking ... */
	ISAGRPHW(BITMASK, 0xff);
	/* ... attributes are in color, switch to graphics mode (again) ... */
	ISAATBW(MODECTL, 0x01);
	/* ... set overscan color to black ... */
	ISAATBW(OSCANCOLOR, 0x00);
	/* ... enable all color planes ... */
	ISAATBW(COLPLANE_EN, 0x0f);
	/* ... reset horizontal pixelpanning ... */
	ISAATBW(HORPIXPAN, 0x00);
	/* ...  reset colorpalette groupselect bits ... */
	ISAATBW(COLSEL, 0x00);
	/* ... do unknown standard VGA register ... */
	/* note:
	 * NeoMagic cards have custom b6 use in this register! */
	ISAATBW(0x16, 0x01);
	/* ... and enable all four byteplanes. */
	ISASEQW(MAPMASK, 0x0f);

	/* setup sequencer clocking mode */
	ISASEQW(CLKMODE, 0x21);

	/* enable memory above 256Kb: set b4 (disables adress wraparound at 256Kb boundary) */
	ISAGRPHW(FBSTADDE, 0x10);

	/* this register has influence on the CRTC framebuffer offset, so reset! */
	//fixme: investigate further...
	ISAGRPHW(0x15, 0x00);

	/* enable fast PCI write bursts (b4-5) */
	temp = ((ISAGRPHR(IFACECTRL1) & 0x0f) | 0x30);
	/* we need to wait a bit or the card will mess-up it's register values.. */
	snooze(10);
	ISAGRPHW(IFACECTRL1, temp);

	/* this speeds up RAM writes according to XFree driver */
//	fixme: don't touch until more is known: part of RAM or CORE PLL???
//	ISAGRPHW(SPEED, 0xc0);

	/* turn on display */
	nm_crtc_dpms(true, true, true);

	return B_OK;
}

/* Check if mode virtual_size adheres to the cards _maximum_ contraints, and modify
 * virtual_size to the nearest valid maximum for the mode on the card if not so.
 * Then: check if virtual_width adheres to the cards _multiple_ constraints, and
 * create mode slopspace if not so.
 * We use acc multiple constraints here if we expect we can use acceleration, because
 * acc constraints are worse than CRTC constraints.
 *
 * Mode slopspace is reflected in fbc->bytes_per_row BTW. */
status_t nm_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row, bool *acc_mode)
{
	/* Note:
	 * This routine assumes that the CRTC memory pitch granularity is 'smaller than',
	 * or 'equals' the acceleration engine memory pitch granularity! */

	uint32 video_pitch = 0;
	uint32 crtc_mask;
	uint8 depth = 8;

	/* determine pixel multiple based on CRTC memory pitch constraints.
	 * (Note: Don't mix this up with CRTC timing contraints! Those are
	 *        multiples of 8 for horizontal, 1 for vertical timing.)
	 *
	 * CRTC pitch constraints are 'officially' the same for all Neomagic cards */
	switch (si->ps.card_type)
	{
	case NM2070:
		switch (target->space)
		{
			case B_CMAP8: crtc_mask = 0x07; depth =  8; break;
			/* Note for NM2070 only:
			 * The following depths have bandwidth trouble (pixel noise) with the
			 * 'official' crtc_masks (used as defaults below). Masks of 0x1f are
			 * needed (confirmed 15 and 16 bit spaces) to minimize this. */
			//fixme: doublecheck for NM2090 and NM2093: correct code if needed!
			case B_RGB15: crtc_mask = 0x1f; depth = 16; break;
			case B_RGB16: crtc_mask = 0x1f; depth = 16; break;
			case B_RGB24: crtc_mask = 0x1f; depth = 24; break; 
			default:
				LOG(8,("INIT: unsupported colorspace: 0x%08x\n", target->space));
				return B_ERROR;
		}
		break;
	default:
		switch (target->space)
		{
			case B_CMAP8: crtc_mask = 0x07; depth =  8; break;
			case B_RGB15: crtc_mask = 0x03; depth = 16; break;
			case B_RGB16: crtc_mask = 0x03; depth = 16; break;
			case B_RGB24: crtc_mask = 0x07; depth = 24; break; 
			default:
				LOG(8,("INIT: unsupported colorspace: 0x%08x\n", target->space));
				return B_ERROR;
		}
		break;
	}

	/* check if we can setup this mode with acceleration: */
	*acc_mode = true;

	/* pre-NM2200 cards don't support accelerated 24bit modes */
	if ((si->ps.card_type < NM2200) && (target->space == B_RGB24)) *acc_mode = false;

	/* virtual_width */
	if (si->ps.card_type == NM2070)
	{
		/* confirmed NM2070 */
		if (target->virtual_width > 1024) *acc_mode = false;
	}
	else
	{
		/* confirmed for all cards */
		if (target->virtual_width > 1600) *acc_mode = false;
	}

	/* virtual_height (confirmed NM2070, NM2097, NM2160 and NM2200 */
	if (target->virtual_height > 1024) *acc_mode = false;

	/* now check virtual_size based on CRTC constraints and modify if needed */
//fixme: checkout cardspecs here!! (NM2160 can do 8192 _bytes_ at least (in theory))
	{
		/* virtual_width */
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > 16368)
				target->virtual_width = 16368;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > 8184)
				target->virtual_width = 8184;
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > 5456)
				target->virtual_width = 5456;
			break;
		}

		/* virtual_height: The only constraint here is the cards memory size which is
		 * checked later on in ProposeMode: virtual_height is adjusted then if needed.
		 * 'Limiting here' to the variable size that's at least available (uint16). */
		if (target->virtual_height > 65535) target->virtual_height = 65535;
	}

	/* OK, now we know that virtual_width is valid, and it's needing no slopspace if
	 * it was confined above, so we can finally calculate safely if we need slopspace
	 * for this mode... */
	if (*acc_mode)
	{
		if (si->ps.card_type == NM2070)
		{
			uint32 acc_mask = 0;

			/* determine pixel multiple based on 2D engine constraints */
			switch (target->space)
			{
				case B_CMAP8: acc_mask = 0x07; break;
				/* Note:
				 * The following depths have actual acc_masks of 0x03. 0x1f is used
				 * because on lower acc_masks more bandwidth trouble arises.
				 * (pixel noise) */
				//fixme: doublecheck for NM2090 and NM2093: correct code if needed!
				case B_RGB15: acc_mask = 0x1f; break;
				case B_RGB16: acc_mask = 0x1f; break;
			}
			video_pitch = ((target->virtual_width + acc_mask) & ~acc_mask);
		}
		else
		{
			/* Note:
			 * We prefer unaccelerated modes above accelerated ones here if not enough
			 * RAM exists and the mode can be closer matched to the requested one if
			 * unaccelerated. We do this because of the large amount of slopspace
			 * sometimes needed here to keep a mode accelerated. */

			uint32 mem_avail, bytes_X_height;

			/* calculate amount of available memory */
			mem_avail = (si->ps.memory_size * 1024);
			if (si->settings.hardcursor) mem_avail -= si->ps.curmem_size;
			/* helper */
			bytes_X_height = (depth >> 3) * target->virtual_height;

			/* Accelerated modes work with a table, there are very few fixed settings.. */
			if (target->virtual_width == 640) video_pitch = 640;
			else
				if (target->virtual_width <= 800)
				{
					if (((800 * bytes_X_height) > mem_avail) &&
						(target->virtual_width < (800 - crtc_mask)))
						*acc_mode = false;
					else
						video_pitch = 800;
				}
				else
					if (target->virtual_width <= 1024)
					{
						if (((1024 * bytes_X_height) > mem_avail) &&
							(target->virtual_width < (1024 - crtc_mask)))
							*acc_mode = false;
						else
							video_pitch = 1024;
					}
					else
						if (target->virtual_width <= 1152)
						{
							if (((1152 * bytes_X_height) > mem_avail) &&
								(target->virtual_width < (1152 - crtc_mask)))
								*acc_mode = false;
							else
								video_pitch = 1152;
						}
						else
							if (target->virtual_width <= 1280)
							{
								if (((1280 * bytes_X_height) > mem_avail) &&
									(target->virtual_width < (1280 - crtc_mask)))
									*acc_mode = false;
								else
									video_pitch = 1280;
							}
							else
								if (target->virtual_width <= 1600)
								{
									if (((1600 * bytes_X_height) > mem_avail) &&
										(target->virtual_width < (1600 - crtc_mask)))
										*acc_mode = false;
									else
										video_pitch = 1600;
								}
		}
	}
	if (!*acc_mode)
		video_pitch = ((target->virtual_width + crtc_mask) & ~crtc_mask);

	LOG(2,("INIT: memory pitch will be set to %d pixels for colorspace 0x%08x\n",
														video_pitch, target->space)); 
	if (target->virtual_width != video_pitch)
		LOG(2,("INIT: effective mode slopspace is %d pixels\n", 
											(video_pitch - target->virtual_width)));

	/* now calculate bytes_per_row for this mode */
	*bytes_per_row = video_pitch * (depth >> 3);

	return B_OK;
}
