/* Author:
   Rudolf Cornelissen 4/2003-4/2004
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
	DUMP_CFG (DEVID,     0);
	DUMP_CFG (DEVCTRL,   0);
	DUMP_CFG (CLASS,     0);
	DUMP_CFG (HEADER,    0);
	DUMP_CFG (NMBASE2,  0);
	DUMP_CFG (NMBASE1,  0);
	DUMP_CFG (NMBASE3,  0);
	DUMP_CFG (SUBSYSIDR, 0);
	DUMP_CFG (ROMBASE,   0);
	DUMP_CFG (CAP_PTR,   0);
	DUMP_CFG (INTCTRL,   0);
	DUMP_CFG (OPTION,    0);
	DUMP_CFG (NM_INDEX, 0);
	DUMP_CFG (NM_DATA,  0);
	DUMP_CFG (SUBSYSIDW, 0);
	DUMP_CFG (OPTION2,   G100);
	DUMP_CFG (OPTION3,   0);
	DUMP_CFG (OPTION4,   0);
	DUMP_CFG (PM_IDENT,  G100);
	DUMP_CFG (PM_CSR,    G100);
	DUMP_CFG (AGP_IDENT, 0);
	DUMP_CFG (AGP_STS,   0);
	DUMP_CFG (AGP_CMD,   0);
#undef DUMP_CFG
}
	
status_t nm_general_powerup()
{
	status_t status;

	LOG(1,("POWERUP: Neomagic (open)BeOS Accelerant 0.06-9 running.\n"));

	/* detect card type and power it up */
	switch(CFGR(DEVID))
	{
	case 0x000110c8: //NM2070 ISA
		si->ps.card_type = NM2070;
		LOG(4,("POWERUP: Detected MagicGraph 128 (NM2070)\n"));
		break;
	case 0x000210c8: //NM2090 ISA
		si->ps.card_type = NM2090;
		LOG(4,("POWERUP: Detected MagicGraph 128V (NM2090)\n"));
		break;
	case 0x000310c8: //NM2093 ISA
		si->ps.card_type = NM2093;
		LOG(4,("POWERUP: Detected MagicGraph 128ZV (NM2093)\n"));
		break;
	case 0x008310c8: //NM2097 PCI
		si->ps.card_type = NM2097;
		LOG(4,("POWERUP: Detected MagicGraph 128ZV+ (NM2097)\n"));
		break;
	case 0x000410c8: //NM2160 PCI
		si->ps.card_type = NM2160;
		LOG(4,("POWERUP: Detected MagicGraph 128XD (NM2160)\n"));
		break;
	case 0x000510c8: //NM2200
		si->ps.card_type = NM2200;
		LOG(4,("POWERUP: Detected MagicMedia 256AV (NM2200)\n"));
		break;
	case 0x002510c8: //NM2230
		si->ps.card_type = NM2230;
		LOG(4,("POWERUP: Detected MagicMedia 256AV+ (NM2230)\n"));
		break;
	case 0x000610c8: //NM2360
		si->ps.card_type = NM2360;
		LOG(4,("POWERUP: Detected MagicMedia 256ZX (NM2360)\n"));
		break;
	case 0x001610c8: //NM2380
		si->ps.card_type = NM2380;
		LOG(4,("POWERUP: Detected MagicMedia 256XL+ (NM2380)\n"));
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

	/* G100 SGRAM and SDRAM use external pix and dac refs, do *not* activate internals!
	 * (this would create electrical shortcuts,
	 * resulting in extra chip heat and distortions visible on screen */
	/* set voltage reference - using DAC reference block partly */
//	DXIW(VREFCTRL,0x03);
	/* wait for 100ms for voltage reference to stabilize */
	delay(100000);
	/* power up the SYSPLL */
//	CFGW(OPTION,CFGR(OPTION)|0x20);
	/* power up the PIXPLL */
//	DXIW(PIXCLKCTRL,0x08);

	/* disable pixelclock oscillations before switching on CLUT */
//	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) | 0x04));
	/* disable 15bit mode CLUT-overlay function */
//	DXIW(GENCTRL, DXIR(GENCTRL & 0xfd));
	/* CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC */
//	DXIW(MISCCTRL,0x1b);
//	snooze(250);
	/* re-enable pixelclock oscillations */
//	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	/*make sure card is in powergraphics mode*/
//	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
//	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
//	g100_dac_set_sys_pll();

	/* 'official' RAM initialisation */
//	LOG(2,("INIT: RAM init\n"));
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
//	ACCW(PLNWT,0x00000000);
//	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
//	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* set memory configuration including:
	 * - no split framebuffer.
	 * - Mark says b14 (G200) should be done also though not defined for G100 in spec,
	 * - b3 v3_mem_type was included by Mark for memconfig setup: but looks like not defined */
//	CFGW(OPTION,(CFGR(OPTION)&0xFFFF8FFF) | ((si->ps.v3_mem_type & 0x04) << 10));
	/* set memory buffer type:
	 * - Mark says: if((v3_mem_type & 0x03) == 0x03) then do not or-in bits in option2;
	 *   but looks like v3_mem_type b1 is not defined,
	 * - Mark also says: place v3_mem_type b1 in option2 bit13 (if not 0x03) but b13 = reserved. */
//	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|((si->ps.v3_mem_type & 0x01) << 12));
	/* set RAM read tap delay */
//	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFFFF0) | ((si->ps.v3_mem_type & 0xf0) >> 4));
	/* wait 200uS minimum */
//	snooze(250);

	/* reset memory (MACCESS is a write only register!) */
//	ACCW(MACCESS, 0x00000000);
	/* select JEDEC reset method */
//	ACCW(MACCESS, 0x00004000);
	/* perform actual RAM reset */
//	ACCW(MACCESS, 0x0000c000);
//	snooze(250);
	/* start memory refresh */
//	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));
	/* set memory control waitstate again AFTER the RAM reset */
//	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* end 'official' RAM initialisation. */

	/* Bus parameters: enable retries, use advanced read */
//	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));

	/*enable writing to crtc registers*/
//	VGAW_I(CRTC,0x11,0);

	/* turn on display */
	nm_crtc_dpms(true, true, true);

	return B_OK;
}

status_t nm_general_output_select()
{
	/* log currently selected output */
	switch (nm_general_output_read())
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
	uint8 output;

	output = (ISAGRPHR(PANELCTRL1) & 0x03);

	if (output == 0)
	{
		/* using 'failsafe' mode: the flatpanel needs all the protection it can get... */
		LOG(4, ("INIT: illegal outputmode detected, reporting internal mode!\n"));
		output = 2;
	}

	return output;
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
	 * CRTC pitch constraints are the same for all Neomagic cards */
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

	/* check if we can setup this mode with acceleration:
	 * constraints are same for all cards */
	*acc_mode = true;

	switch (si->ps.card_type)
	{
	case NM2097:
	case NM2160:
		if (target->space != B_RGB24) *acc_mode = true;
		else *acc_mode = false;
		break;
	default:
		*acc_mode = false;
		break;
	}

	/* virtual_width */
	if (target->virtual_width > 1600) *acc_mode = false;

	/* virtual_height */
	if (si->ps.card_type < NM2200)
	{
		/* confirmed NM2097 and NM2160 */
		if (target->virtual_height > 1024) *acc_mode = false;
	}
	else
	{
		/* fixme: needs confirmation, assuming this height will still work.. */
		if (target->virtual_height > 2048) *acc_mode = false;
	}

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
	/* note:
	 * we prefer unaccelerated modes above accelerated ones if not enough RAM exists
	 * and the mode can be closer matched to the requested one if unaccelerated. */
	if (*acc_mode)
	{
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
