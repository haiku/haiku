/* Authors:
   Mark Watson 12/1999,
   Apsed,
   Rudolf Cornelissen 10/2002-10/2009
*/

#define MODULE_BIT 0x00008000

#include "mga_std.h"

static status_t test_ram(void);
static status_t mil_general_powerup (void);
static status_t g100_general_powerup (void);
static status_t g200_general_powerup (void);
static status_t g400_general_powerup (void);
static status_t g450_general_powerup (void);
static status_t gx00_general_bios_to_powergraphics(void);

static void mga_dump_configuration_space (void)
{
#define DUMP_CFG(reg, type) if (si->ps.card_type >= type) do { \
	uint32 value = CFGR(reg); \
	MSG(("configuration_space 0x%02x %20s 0x%08x\n", \
		MGACFG_##reg, #reg, value)); \
} while (0)
	DUMP_CFG (DEVID,     0);
	DUMP_CFG (DEVCTRL,   0);
	DUMP_CFG (CLASS,     0);
	DUMP_CFG (HEADER,    0);
	DUMP_CFG (MGABASE2,  0);
	DUMP_CFG (MGABASE1,  0);
	DUMP_CFG (MGABASE3,  MYST);
	DUMP_CFG (SUBSYSIDR, MYST);
	DUMP_CFG (ROMBASE,   0);
	DUMP_CFG (CAP_PTR,   MIL2);
	DUMP_CFG (INTCTRL,   0);
	DUMP_CFG (OPTION,    0);
	DUMP_CFG (MGA_INDEX, 0);
	DUMP_CFG (MGA_DATA,  0);
	DUMP_CFG (SUBSYSIDW, MYST);
	DUMP_CFG (OPTION2,   G100);
	DUMP_CFG (OPTION3,   G400);
	DUMP_CFG (OPTION4,   G400);
	DUMP_CFG (PM_IDENT,  G100);
	DUMP_CFG (PM_CSR,    G100);
	DUMP_CFG (AGP_IDENT, MIL2);
	DUMP_CFG (AGP_STS,   MIL2);
	DUMP_CFG (AGP_CMD,   MIL2);
#undef DUMP_CFG
}

status_t gx00_general_powerup()
{
	status_t status;
	uint8 card_class;

	LOG(1,("POWERUP: Haiku Matrox Accelerant 0.33 running.\n"));

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
	case 0x051a102b: //MGA-1064 Mystique PCI
		sprintf(si->adi.name, "Matrox Mystique PCI");
		sprintf(si->adi.chipset, "MGA-1064");
		LOG(8,("POWERUP: Unimplemented Matrox device %08x\n",CFGR(DEVID)));
		return B_ERROR;
	case 0x0519102b: //MGA-2064 Millenium PCI
		si->ps.card_type = MIL1;
		sprintf(si->adi.name, "Matrox Millennium I");
		sprintf(si->adi.chipset, "MGA-2064");
		status = mil_general_powerup();
		break;
	case 0x051b102b:case 0x051f102b: //MGA-2164 Millenium II PCI/AGP
		si->ps.card_type = MIL2;
		sprintf(si->adi.name, "Matrox Millennium II");
		sprintf(si->adi.chipset, "MGA-2164");
		status = mil_general_powerup();
		break;
	case 0x1000102b:case 0x1001102b: //G100
		si->ps.card_type = G100;
		sprintf(si->adi.name, "Matrox MGA G100");
		sprintf(si->adi.chipset, "G100");
		status = g100_general_powerup();
		break;
	case 0x0520102b:case 0x0521102b: //G200
		si->ps.card_type = G200;
		sprintf(si->adi.name, "Matrox MGA G200");
		sprintf(si->adi.chipset, "G200");
		status = g200_general_powerup();
		break;
	case 0x0525102b: //G400, G400MAX or G450
		/* get classinfo to distinguish different types */
		card_class = CFGR(CLASS) & 0xff;
		if (card_class & 0x80)
		{
			/* G450 */
			si->ps.card_type = G450;
			sprintf(si->adi.name, "Matrox MGA G450");
			sprintf(si->adi.chipset, "G450 revision %x", (card_class & 0x7f));
			LOG(4, ("50 revision %x\n", card_class & 0x7f));
			status = g450_general_powerup();
		}
		else
		{
			/* standard G400, G400MAX */
			/* the only difference is the max RAMDAC speed, accounted for via pins. */
			si->ps.card_type = G400;
			sprintf(si->adi.name, "Matrox MGA G400");
			sprintf(si->adi.chipset, "G400 revision %x", (card_class & 0x7f));
			status = g400_general_powerup();
		}
		break;
	case 0x2527102b://G550 patch from Jean-Michel Batto
		si->ps.card_type = G450;
		sprintf(si->adi.name, "Matrox MGA G550");
		sprintf(si->adi.chipset, "G550");
		status = g450_general_powerup();
		break;	
	default:
		LOG(8,("POWERUP: Failed to detect valid card 0x%08x\n",CFGR(DEVID)));
		return B_ERROR;
	}

	/* override memory detection if requested by user */
	if (si->settings.memory != 0)
		si->ps.memory_size = si->settings.memory;

	return status;
}

static status_t test_ram()
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
		((vuint32 *)si->fbc.frame_buffer)[offset] = value;
		/* toggle testpattern */
		value = 0xffffffff - value;
	}

	for (offset = 0, value = 0x55aa55aa; offset < 256; offset++)
	{
		/* readback and verify testpattern from cardRAM */
		if (((vuint32 *)si->fbc.frame_buffer)[offset] != value) result = B_ERROR;
		/* toggle testpattern */
		value = 0xffffffff - value;
	}
	return result;
}

/* NOTE:
 * This routine *has* to be done *after* SetDispplayMode has been executed,
 * or test results will not be representative!
 * (CAS latency is dependant on MGA setup on some (DRAM) boards) */
status_t mga_set_cas_latency()
{
	status_t result = B_ERROR;
	uint8 latency = 0;

	/* check current RAM access to see if we need to change anything */
	if (test_ram() == B_OK)
	{
		LOG(4,("INIT: RAM access OK.\n"));
		return B_OK;
	}

	/* check if we read PINS at starttime so we have valid registersettings at our disposal */
	if (si->ps.pins_status != B_OK)
	{
		LOG(4,("INIT: RAM access errors; not fixable: PINS was not read from cardBIOS.\n"));
		return B_ERROR;
	}

	/* OK. We might have a problem, try to fix it now.. */
	LOG(4,("INIT: RAM access errors; tuning CAS latency if prudent...\n"));

	switch(si->ps.card_type)
	{
	case G100:
			if (!si->ps.sdram)
			{
				LOG(4,("INIT: G100 SGRAM CAS tuning not permitted, aborting.\n"));
				return B_OK;
			}
			/* SDRAM card */
			for (latency = 4; latency >= 2; latency-- )
			{
				/* MCTLWTST is a write-only register! */
				ACCW(MCTLWTST, ((si->ps.mctlwtst_reg & 0xfffffffc) | (latency - 2)));
				result = test_ram();
				if (result == B_OK) break;
			}
			break;
	case G200:
			/* fixme: implement this */
			LOG(4,("INIT: G200 RAM CAS tuning not implemented, aborting.\n"));
			return B_OK;
			break;
	case G400:
	case G400MAX:
			/* fixme: implement this if needed */
			LOG(4,("INIT: G400/G400MAX RAM CAS tuning not implemented, aborting.\n"));
			return B_OK;
			break;
	case G450:
	case G550:
			/* fixme: implement this if needed */
			LOG(4,("INIT: G450/G550 RAM CAS tuning not implemented, aborting.\n"));
			return B_OK;
			break;
	default:
			/* fixme: Millenium2 and others if needed */
			LOG(4,("INIT: RAM CAS tuning not implemented for this card, aborting.\n"));
			return B_OK;
			break;
	}
	if (result == B_OK)
		LOG(4,("INIT: RAM access OK. CAS latency set to %d cycles.\n", latency));
	else
		LOG(4,("INIT: RAM access not fixable. CAS latency set to %d cycles.\n", latency));

	return result;
}

static 
status_t mil_general_powerup()
{
	status_t result;

	LOG(4, ("INIT: Millenium I/II powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

//remove this:
	fake_pins();
	LOG(2, ("INIT: Using faked PINS for now:\n"));
	dump_pins();
//end remove this.

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
//restore this line:
//	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	//set to powergraphics etc.
	LOG(2, ("INIT: Skipping card coldstart!\n"));
	mil2_dac_init();

//ok:
	/* disable overscan, select 0 IRE, select straight-through sync signals from CRTC */
	DXIW (GENCTRL, (DXIR (GENCTRL) & 0x0c));
	/* fixme: checkout if we need this sync inverting stuff: already done via CRTC!?!	
		| (vsync_pos?  0x00:0x02) 
		| (hsync_pos?  0x00:0x01)); */

	/* 8-bit DAC, enable DAC */
	DXIW(MISCCTRL, 0x0c);
//

	VGAW_I(SEQ,1,0x00);
	/*enable screen*/

	return B_OK;
}

static 
status_t g100_general_powerup()
{
	status_t result;
	
	LOG(4, ("INIT: G100 powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	LOG(2,("INIT: PLL/LUT/DAC powerup\n"));
	/* turn off both displays and the hardcursor (also disables transfers) */
	gx00_crtc_dpms(false, false, false);
	/* on singlehead cards with TVout program the MAVEN as well */
	if (si->ps.tvout) gx00_maven_dpms(false, false, false);
	gx00_crtc_cursor_hide();
	/* G100 SGRAM and SDRAM use external pix and dac refs, do *not* activate internals!
	 * (this would create electrical shortcuts,
	 * resulting in extra chip heat and distortions visible on screen */
	/* set voltage reference - using DAC reference block partly */
	DXIW(VREFCTRL,0x03);
	/* wait for 100ms for voltage reference to stabilize */
	delay(100000);
	/* power up the SYSPLL */
	CFGW(OPTION,CFGR(OPTION)|0x20);
	/* power up the PIXPLL */
	DXIW(PIXCLKCTRL,0x08);

	/* disable pixelclock oscillations before switching on CLUT */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) | 0x04));
	/* disable 15bit mode CLUT-overlay function */
	DXIW(GENCTRL, DXIR(GENCTRL & 0xfd));
	/* CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC */
	DXIW(MISCCTRL,0x1b);
	snooze(250);
	/* re-enable pixelclock oscillations */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	/* setup i2c bus */
	i2c_init();

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g100_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* set memory configuration including:
	 * - no split framebuffer.
	 * - Mark says b14 (G200) should be done also though not defined for G100 in spec,
	 * - b3 v3_mem_type was included by Mark for memconfig setup: but looks like not defined */
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF8FFF) | ((si->ps.v3_mem_type & 0x04) << 10));
	/* set memory buffer type:
	 * - Mark says: if((v3_mem_type & 0x03) == 0x03) then do not or-in bits in option2;
	 *   but looks like v3_mem_type b1 is not defined,
	 * - Mark also says: place v3_mem_type b1 in option2 bit13 (if not 0x03) but b13 = reserved. */
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|((si->ps.v3_mem_type & 0x01) << 12));
	/* set RAM read tap delay */
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFFFF0) | ((si->ps.v3_mem_type & 0xf0) >> 4));
	/* wait 200uS minimum */
	snooze(250);

	/* reset memory (MACCESS is a write only register!) */
	ACCW(MACCESS, 0x00000000);
	/* select JEDEC reset method */
	ACCW(MACCESS, 0x00004000);
	/* perform actual RAM reset */
	ACCW(MACCESS, 0x0000c000);
	snooze(250);
	/* start memory refresh */
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));
	/* set memory control waitstate again AFTER the RAM reset */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* end 'official' RAM initialisation. */

	/* Bus parameters: enable retries, use advanced read */
	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);

	return B_OK;
}

static 
status_t g200_general_powerup()
{
	status_t result;
	
	LOG(4, ("INIT: G200 powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	LOG(2,("INIT: PLL/LUT/DAC powerup\n"));
	/* turn off both displays and the hardcursor (also disables transfers) */
	gx00_crtc_dpms(false, false, false);
	/* on singlehead cards with TVout program the MAVEN as well */
	if (si->ps.tvout) gx00_maven_dpms(false, false, false);
	gx00_crtc_cursor_hide();
	/* G200 SGRAM and SDRAM use external pix and dac refs, do *not* activate internals!
	 * (this would create electrical shortcuts,
	 * resulting in extra chip heat and distortions visible on screen */
	/* set voltage reference - using DAC reference block partly */
	DXIW(VREFCTRL,0x03);
	/* wait for 100ms for voltage reference to stabilize */
	delay(100000);
	/* power up the SYSPLL */
	CFGW(OPTION,CFGR(OPTION)|0x20);
	/* power up the PIXPLL */
	DXIW(PIXCLKCTRL,0x08);

	/* disable pixelclock oscillations before switching on CLUT */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) | 0x04));
	/* disable 15bit mode CLUT-overlay function */
	DXIW(GENCTRL, DXIR(GENCTRL & 0xfd));
	/* CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC */
	DXIW(MISCCTRL,0x1b);
	snooze(250);
	/* re-enable pixelclock oscillations */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	/* setup i2c bus */
	i2c_init();

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g200_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* disable hardware plane write mask if SDRAM card */
	if (si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) & 0xffffbfff));
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* set memory configuration including:
	 * - SDRAM / SGRAM special functions select. */
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF) | ((si->ps.v3_mem_type & 0x07) << 10));
	if (!si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) | (0x01 << 14)));
	/* set memory buffer type */
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|((si->ps.v3_option2_reg & 0x03) << 12));
	/* set mode register opcode and streamer flow control */	
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0x0000FFFF)|(si->ps.memrdbk_reg & 0xffff0000));
	/* set RAM read tap delays */
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.memrdbk_reg & 0x0000ffff));
	/* wait 200uS minimum */
	snooze(250);

	/* reset memory (MACCESS is a write only register!) */
	ACCW(MACCESS, 0x00000000);
	/* perform actual RAM reset */
	ACCW(MACCESS, 0x00008000);
	snooze(250);
	/* start memory refresh */
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));
	/* set memory control waitstate again AFTER the RAM reset */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* end 'official' RAM initialisation. */

	/* Bus parameters: enable retries, use advanced read */
	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);

	return B_OK;
}

static 
status_t g400_general_powerup()
{
	status_t result;
	
	LOG(4, ("INIT: G400/G400MAX powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/* reset MAVEN so we know the sync polarity is at reset situation (Hpos, Vpos) */
	if (si->ps.tvout)
	{
		ACCW(RST, 0x00000002);
		snooze(1000);
		ACCW(RST, 0x00000000);
	}

	/*power up the PLLs,LUT,DAC*/
	LOG(4,("INIT: PLL/LUT/DAC powerup\n"));
	/* turn off both displays and the hardcursor (also disables transfers) */
	gx00_crtc_dpms(false, false, false);
	g400_crtc2_dpms(false, false, false);
	gx00_crtc_cursor_hide();

	/* set voltage reference - not using DAC reference block */
	DXIW(VREFCTRL,0x00);
	/* wait for 100ms for voltage reference to stabilize */
	delay(100000);
	/* power up the SYSPLL */
	CFGW(OPTION,CFGR(OPTION)|0x20);
	/* power up the PIXPLL */
	DXIW(PIXCLKCTRL,0x08);

	/* disable pixelclock oscillations before switching on CLUT */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) | 0x04));
	/* disable 15bit mode CLUT-overlay function */
	DXIW(GENCTRL, DXIR(GENCTRL & 0xfd));
	/* CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC */
	DXIW(MISCCTRL,0x9b);
	snooze(250);
	/* re-enable pixelclock oscillations */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	DXIW(MAFCDEL,0x02);                 /*makes CRTC2 stable! Matrox specify 8, but use 4 - grrrr!*/
	DXIW(PANELMODE,0x00);               /*eclipse panellink*/

	/* setup i2c bus */
	i2c_init();

	/* make sure card is in powergraphics mode */
	VGAW_I(CRTCEXT,3,0x80);      

	/* set the system clocks to powergraphics speed */
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g400_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* disable hardware plane write mask if SDRAM card */
	if (si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) & 0xffffbfff));
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
	ACCW(MCTLWTST, si->ps.mctlwtst_reg);
	/* set memory configuration including:
	 * - SDRAM / SGRAM special functions select. */
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF) | (si->ps.option_reg & 0x00001c00));
	if (!si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) | (0x01 << 14)));
	/* set mode register opcode and streamer flow control */	
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0x0000FFFF)|(si->ps.memrdbk_reg & 0xffff0000));
	/* set RAM read tap delays */
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.memrdbk_reg & 0x0000ffff));
	/* wait 200uS minimum */
	snooze(250);

	/* reset memory (MACCESS is a write only register!) */
	ACCW(MACCESS, 0x00000000);
	/* perform actual RAM reset */
	ACCW(MACCESS, 0x00008000);
	snooze(250);
	/* start memory refresh */
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));
	/* set memory control waitstate again AFTER the RAM reset */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* end 'official' RAM initialisation. */

	/* 'advance read' busparameter and 'memory priority' enable/disable setup */
	CFGW(OPTION, ((CFGR(OPTION) & 0xefbfffff) | (si->ps.option_reg & 0x10400000)));

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);
	if (si->ps.secondary_head)
	{
		MAVW(LOCK,0x01);
		CR2W(DATACTL,0x00000000);
	}

	return B_OK;
}

static 
status_t g450_general_powerup()
{
	status_t result;
	uint32 pwr_cas[] = {0, 1, 5, 6, 7, 5, 2, 3};

	/* used for convenience: MACCESS is a write only register! */
	uint32 maccess = 0x00000000;

	LOG(4, ("INIT: G450/G550 powerup\n"));
	LOG(4, ("INIT: Detected %s (%s)\n", si->adi.name, si->adi.chipset));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* check output connector setup */
	if (si->ps.primary_dvi && si->ps.secondary_head &&
		si->ps.tvout && (i2c_sec_tv_adapter() != B_OK))
	{
		/* signal CRTC2 DPMS which connector to program or readout */
		si->crossed_conns = true;
	}

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/* power up the PLLs,LUT,DAC */
	LOG(4,("INIT: PLL/LUT/DAC powerup\n"));
	/* disable outputs */
	DXIW(OUTPUTCONN,0x00); 
	/* turn off both displays and the hardcursor (also disables transfers) */
	gx00_crtc_dpms(false, false, false);
	g400_crtc2_dpms(false, false, false);
	gx00_crtc_cursor_hide();

	/* power up everything except DVI electronics (for now) */
	DXIW(PWRCTRL,0x1b); 
	/* set voltage reference - not using DAC reference block */
	DXIW(VREFCTRL,0x00);
	/* wait for 100ms for voltage reference to stabilize */
	delay(100000);
	/* power up the SYSPLL */
	CFGW(OPTION,CFGR(OPTION)|0x20);
	/* power up the PIXPLL */
	DXIW(PIXCLKCTRL,0x08);

	/* disable pixelclock oscillations before switching on CLUT */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) | 0x04));
	/* disable 15bit mode CLUT-overlay function */
	DXIW(GENCTRL, DXIR(GENCTRL & 0xfd));
	/* CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC */
	DXIW(MISCCTRL,0x9b);
	snooze(250);

	/* re-enable pixelclock oscillations */
	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	//fixme:
	DXIW(MAFCDEL,0x02);                 /*makes CRTC2 stable! Matrox specify 8, but use 4 - grrrr!*/
	DXIW(PANELMODE,0x00);               /*eclipse panellink*/

	/* setup i2c bus */
	i2c_init();

	/* make sure card is in powergraphics mode */
	VGAW_I(CRTCEXT,3,0x80);      

	/* set the system clocks to powergraphics speed */
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g450_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* stop memory refresh, and setup b9, memconfig, b13, sgram planemask function, b21 fields,
	 * and don't touch the rest */
	CFGW(OPTION, ((CFGR(OPTION) & 0xf8400164) | (si->ps.option_reg & 0x00207e00)));
	/* setup b10-b15 unknown field */
	CFGW(OPTION2, ((CFGR(OPTION2) & 0xffff0200) | (si->ps.option2_reg & 0x0000fc00)));

	/* program memory control waitstates */
	ACCW(MCTLWTST, si->ps.mctlwtst_reg);
	/* program option4 b0-3 and b29-30 fields, reset the rest: stop memory clock */
	CFGW(OPTION4, (si->ps.option4_reg & 0x6000000f));
	/* set RAM read tap delays and mode register opcode / streamer flow control */
	ACCW(MEMRDBK, si->ps.memrdbk_reg);
	/* b7 v5_mem_type = done by Mark Watson. fixme: still confirm! (unknown bits) */
	maccess = ((((uint32)si->ps.v5_mem_type) & 0x80) >> 1);
	ACCW(MACCESS, maccess);
	/* clear b0-1 and 3, and set b31 in option4: re-enable memory clock */
	CFGW(OPTION4, ((si->ps.option4_reg & 0x60000004) | 0x80000000));
	snooze(250);

	/* if DDR RAM */
	if ((si->ps.v5_mem_type & 0x0060) == 0x0020) 
	{
		/* if not 'EMRSW RAM-option' available */
		if (!(si->ps.v5_mem_type & 0x0100))
		{
			/* clear unknown bits */
			maccess = 0x00000000;
			ACCW(MACCESS, maccess);
			/* clear b12: unknown bit */
			ACCW(MEMRDBK, (si->ps.memrdbk_reg & 0xffffefff));
		}
		else
			/* if not 'DLL RAM-option' available */
			if (!(si->ps.v5_mem_type & 0x0200))
			{
				/* clear b12: unknown bit */
				ACCW(MEMRDBK, (si->ps.memrdbk_reg & 0xffffefff));
			}
	}

	/* create positive flank to generate memory reset */
	//fixme: G550 is still noted as a G450, fix upon trouble..
	if (si->ps.card_type == G450) {
		ACCW(MACCESS, (maccess & 0xffff3fff));
		ACCW(MACCESS, (maccess | 0x0000c000));
	} else { //G550
		ACCW(MACCESS, (maccess & 0xffff7fff));
		ACCW(MACCESS, (maccess | 0x00008000));
	}
	snooze(250);

	/* start memory refresh */
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));

	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
	ACCW(PLNWT,0x00000000);
	ACCW(PLNWT,0xffffffff);

	/* if not 'MEMCASLT RAM-option' available */
	if (!(si->ps.v5_mem_type & 0x0400))
	{
		/* calculate powergraphics CAS-latency from pins CAS-latency, and update register setting */
		ACCW(MCTLWTST,
			((si->ps.mctlwtst_reg & 0xfffffff8) | pwr_cas[(si->ps.mctlwtst_reg & 0x07)]));

	}

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);
	//fixme..
	if (si->ps.secondary_head)
	{
		//MAVW(LOCK,0x01);
		CR2W(DATACTL,0x00000000);
	}

	/* enable primary analog output */
	gx50_general_output_select();

	/* enable 'straight-through' sync outputs on both analog output connectors and
	 * make sure CRTC1 sync outputs are patched through! */
	DXIW(SYNCCTRL,0x00); 

	return B_OK;
}

status_t gx50_general_output_select()
{
	/* make sure this call is warranted */
	if ((si->ps.card_type != G450) && (si->ps.card_type != G550)) return B_ERROR;

	/* choose primary analog outputconnector */
	if (si->ps.primary_dvi && si->ps.secondary_head && si->ps.tvout)
	{
		if (i2c_sec_tv_adapter() == B_OK)
		{
			LOG(4,("INIT: secondary TV-adapter detected, using primary connector\n"));
			DXIW(OUTPUTCONN,0x01); 
			/* signal CRTC2 DPMS which connector to program */
			si->crossed_conns = false;
		}
		else
		{
			LOG(4,("INIT: no secondary TV-adapter detected, using secondary connector\n"));
			DXIW(OUTPUTCONN,0x04); 
			/* signal CRTC2 DPMS which connector to program */
			si->crossed_conns = true;
		}
	}
	else
	{
		LOG(4,("INIT: using primary connector\n"));
		DXIW(OUTPUTCONN,0x01); 
		/* signal CRTC2 DPMS which connector to program */
		si->crossed_conns = false;
	}
	return B_OK;
}

/* connect CRTC(s) to the specified DAC(s) */
status_t gx00_general_dac_select(int dac)
{
	/*MISCCTRL, clock src,...*/
	switch(dac)
	{
		/* G100 - G400 */
		case DS_CRTC1DAC:
		case DS_CRTC1DAC_CRTC2MAVEN:
			/* connect CRTC1 to pixPLL */
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL, connect CRTC1 to internal DAC and
			 * enable CRTC2 external video timing reset signal.
			 * (Setting for MAVEN 'master mode' TVout signal generation.) */
			if (si->ps.secondary_head) CR2W(CTL,(CR2R(CTL)&0xffe00779)|0xD0000002);
			/* disable CRTC1 external video timing reset signal */
			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)&0x77));
			/* select CRTC2 RGB24 MAFC mode: connects CRTC2 to MAVEN DAC */ 
			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x82);
			break;
		case DS_CRTC1MAVEN:
		case DS_CRTC1MAVEN_CRTC2DAC:
			/* connect CRTC1 to vidPLL */
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x2);
			/* connect CRTC2 to pixPLL and internal DAC and
			 * disable CRTC2 external video timing reset signal */
			if (si->ps.secondary_head) CR2W(CTL,(CR2R(CTL)&0x2fe00779)|0x4|(0x1<<20));
			/* enable CRTC1 external video timing reset signal.
			 * note: this is nolonger used as G450/G550 cannot do TVout on CRTC1 */
			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)|0x88));
			/* select CRTC1 RGB24 MAFC mode: connects CRTC1 to MAVEN DAC */
			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x02);
			break;
		/* G450/G550 */
		case DS_CRTC1CON1_CRTC2CON2:
			if (si->ps.card_type < G450) return B_ERROR;
			/* connect CRTC1 to pixPLL */
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL, connect CRTC1 to DAC1, disable CRTC2
			 * external video timing reset signal, set CRTC2 progressive scan mode
			 * and disable TVout mode (b12).
			 * (Setting for MAVEN 'slave mode' TVout signal generation.) */
			//fixme: enable timing resets if TVout is used in master mode!
			//otherwise keep it disabled.
			CR2W(CTL,(CR2R(CTL)&0x2de00779)|0x6|(0x0<<20));
			/* connect DAC1 to CON1, CRTC2/'DAC2' to CON2 (monitor mode) */
			DXIW(OUTPUTCONN,0x09); 
			/* Select 1.5 Volt MAVEN DAC ref. for monitor mode */
			DXIW(GENIOCTRL, DXIR(GENIOCTRL) & ~0x40);
			DXIW(GENIODATA, 0x00);
			/* signal CRTC2 DPMS which connector to program */
			si->crossed_conns = false;
			break;
		//fixme: toggle PLL's below if possible: 
		//       otherwise toggle PLL's for G400 2nd case?
		case DS_CRTC1CON2_CRTC2CON1:
			if (si->ps.card_type < G450) return B_ERROR;
			/* connect CRTC1 to pixPLL */
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL and DAC1, disable CRTC2 external
			 * video timing reset signal, and set CRTC2 progressive scan mode and
			 * disable TVout mode (b12). */
			CR2W(CTL,(CR2R(CTL)&0x2de00779)|0x6|(0x1<<20));
			/* connect DAC1 to CON2 (monitor mode), CRTC2/'DAC2' to CON1 */
			DXIW(OUTPUTCONN,0x05); 
			/* Select 1.5 Volt MAVEN DAC ref. for monitor mode */
			DXIW(GENIOCTRL, DXIR(GENIOCTRL) & ~0x40);
			DXIW(GENIODATA, 0x00);
			/* signal CRTC2 DPMS which connector to program */
			si->crossed_conns = true;
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}

/* basic change of card state from VGA to powergraphics -> should work from BIOS init state*/
static 
status_t gx00_general_bios_to_powergraphics()
{
	LOG(2, ("INIT: Skipping card coldstart!\n"));

	//set to powergraphics etc.
	CFGW(DEVCTRL,(2|CFGR(DEVCTRL))); 
	/*enable device response (already enabled here!)*/

	VGAW_I(CRTC,0x11,0);
	/*allow me to change CRTC*/

	VGAW_I(CRTCEXT,3,0x80);
	/*use powergraphix (+ trash other bits, they are set later)*/

	VGAW(MISCW,0x08);
	/*set only MGA pixel clock in MISC - I don't want to map VGA stuff under this OS*/

	switch (si->ps.card_type)
	{
		case G400:
		case G400MAX:
			/* reset MAVEN so we know the sync polarity is at reset situation (Hpos, Vpos) */
			if (si->ps.tvout)
			{
				ACCW(RST, 0x00000002);
				snooze(1000);
				ACCW(RST, 0x00000000);
			}
			/* makes CRTC2 stable! Matrox specify 8, but use 4 - grrrr! */
			DXIW(MAFCDEL,0x02);
			break;
		case G450:
		case G550:
			/* power up everything except DVI electronics (for now) */
			DXIW(PWRCTRL,0x1b); 
			/* enable 'straight-through' sync outputs on both analog output
			 * connectors and make sure CRTC1 sync outputs are patched through! */
			DXIW(SYNCCTRL,0x00); 
			break;
		default:
			break;
	}

	if (si->ps.card_type >= G100)
	{
		DXIW(MISCCTRL,0x9b);
		/*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/

		DXIW(MULCTRL,0x4);
		/*RGBA direct mode*/
	}
	else
	{ 
		LOG(8, ("INIT: < G100 DAC powerup badly implemented, MISC 0x%02x\n", VGAR(MISCR)));
	} // apsed TODO MIL2

	VGAW_I(SEQ,1,0x00);
	/*enable screen*/

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
//fixme: seperate heads for real dualhead modes:
//CRTC1 and 2 constraints differ!
status_t gx00_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row, bool *acc_mode)
{
	/* Note:
	 * This routine assumes that the CRTC memory pitch granularity is 'smaller than',
	 * or 'equals' the acceleration engine memory pitch granularity! */

	uint32 video_pitch;
	uint32 acc_mask, crtc_mask;
	uint8 depth = 8;

	/* determine pixel multiple based on 2D/3D engine constraints */
	switch (si->ps.card_type)
	{
	case MIL1:
	case MIL2:
		/* see MIL1/2 specs:
		 * these cards always use a 64bit RAMDAC (TVP3026) and interleaved memory */
		switch (target->space)
		{
			case B_CMAP8: acc_mask = 0x7f; depth =  8; break;
			case B_RGB15: acc_mask = 0x3f; depth = 16; break;
			case B_RGB16: acc_mask = 0x3f; depth = 16; break;
			case B_RGB24: acc_mask = 0x7f; depth = 24; break;
			case B_RGB32: acc_mask = 0x1f; depth = 32; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		break;
	default:
		/* see G100 and up specs:
		 * these cards can do 2D as long as multiples of 32 are used.
		 * (Note: don't mix this up with adress linearisation!) */
		switch (target->space)
		{
			case B_CMAP8: depth =  8; break;
			case B_RGB15: depth = 16; break;
			case B_RGB16: depth = 16; break;
			case B_RGB24: depth = 24; break;
			case B_RGB32: depth = 32; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		acc_mask = 0x1f;
		break;
	}

	/* determine pixel multiple based on CRTC memory pitch constraints.
	 * (Note: Don't mix this up with CRTC timing contraints! Those are
	 *        multiples of 8 for horizontal, 1 for vertical timing.) */
	switch (si->ps.card_type)
	{
	case MIL1:
	case MIL2:
		/* see MIL1/2 specs:
		 * these cards always use a 64bit RAMDAC and interleaved memory */
		switch (target->space)
		{
			case B_CMAP8: crtc_mask = 0x7f; break;
			case B_RGB15: crtc_mask = 0x3f; break;
			case B_RGB16: crtc_mask = 0x3f; break;
			/* for B_RGB24 crtc_mask 0x7f is worst case scenario (MIL2 constraint) */
			case B_RGB24: crtc_mask = 0x7f; break; 
			case B_RGB32: crtc_mask = 0x1f; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		break;
	default:
		/* see G100 and up specs */
		switch (target->space)
		{
			case B_CMAP8: crtc_mask = 0x0f; break;
			case B_RGB15: crtc_mask = 0x07; break;
			case B_RGB16: crtc_mask = 0x07; break;
			case B_RGB24: crtc_mask = 0x0f; break; 
			case B_RGB32: crtc_mask = 0x03; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		/* see G400 specs: CRTC2 has different constraints */
		/* Note:
		 * set for RGB and B_YCbCr422 modes. Other modes need larger multiples! */
		if (target->flags & DUALHEAD_BITS)
		{
			switch (target->space)
			{
				case B_RGB16: crtc_mask = 0x1f; break;
				case B_RGB32: crtc_mask = 0x0f; break;
				default:
					LOG(8,("INIT: illegal DH color space: 0x%08x\n", target->space));
					return B_ERROR;
			}
		}
		break;
	}

	/* check if we can setup this mode with acceleration:
	 * Max sizes need to adhere to both the acceleration engine _and_ the CRTC constraints! */
	*acc_mode = true;
	/* check virtual_width */
	switch (si->ps.card_type)
	{
	case MIL1:
	case MIL2:
	case G100:
		/* acc constraint: */
		if (target->virtual_width > 2048) *acc_mode = false;
		break;
	default:
		/* G200-G550 */
		/* acc constraint: */
		if (target->virtual_width > 4096) *acc_mode = false;
		/* for 32bit mode a lower CRTC1 restriction applies! */
		if ((target->space == B_RGB32_LITTLE) && (target->virtual_width > (4092 & ~acc_mask)))
			*acc_mode = false;
		break;
	}
	/* virtual_height */
	if (target->virtual_height > 2048) *acc_mode = false;

	/* now check virtual_size based on CRTC constraints,
	 * making sure virtual_width stays within the 'mask' constraint: which is only
	 * nessesary because of an extra constraint in MIL1/2 cards that exists here. */
	{
		/* virtual_width */
		//fixme for CRTC2 (identical on all G400+ cards):
		//16bit mode: max. virtual_width == 16352 (no extra mask needed);
		//32bit mode: max. virtual_width == 8176 (no extra mask needed);
		//other colordepths are unsupported on CRTC2.
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > (16368 & ~crtc_mask))
				target->virtual_width = (16368 & ~crtc_mask);
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > (8184 & ~crtc_mask))
				target->virtual_width = (8184 & ~crtc_mask);
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > (5456 & ~crtc_mask))
				target->virtual_width = (5456 & ~crtc_mask);
			break;
		case B_RGB32_LITTLE:
			if (target->virtual_width > (4092 & ~crtc_mask))
				target->virtual_width = (4092 & ~crtc_mask);
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
		video_pitch = ((target->virtual_width + acc_mask) & ~acc_mask);
	else
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
