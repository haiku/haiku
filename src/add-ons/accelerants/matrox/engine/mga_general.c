/* Authors:
   Mark Watson 12/1999,
   Apsed,
   Rudolf Cornelissen 10/2002
*/

#define MODULE_BIT 0x00008000

#include "mga_std.h"
//apsed #include "memory"
//#include "mga_init.c" //Nicole's test stuff.

status_t test_ram();
static status_t mil2_general_powerup (void);
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
	uint32 class;

	//detect card type and powerup
	switch(CFGR(DEVID))
	{
	case 0x0519102b: //MGA-2064 Millenium PCI
	case 0x051a102b: //MGA-1064 Mystic PCI
		LOG(8,("POWERUP: unimplemented Matrox device %08x\n",CFGR(DEVID)));
		return B_ERROR;
	case 0x051b102b:case 0x051f102b: //MGA-2164 Millenium 2 PCI/AGP
		si->ps.card_type=MIL2;
		LOG(4,("POWERUP:Detected MGA-2164 Millennium 2\n"));
		status = mil2_general_powerup();
		break;
	case 0x1000102b:case 0x1001102b: //G100
		si->ps.card_type=G100;
		LOG(4,("POWERUP:Detected G100\n"));
		status = g100_general_powerup();
		break;
	case 0x0520102b:case 0x0521102b: //G200
		si->ps.card_type=G200;
		LOG(4,("POWERUP:Detected G200\n"));
		status = g200_general_powerup();
		break;
	case 0x0525102b: //G400
		LOG(4,("POWERUP:Detected G4"));
		//Check if it is a G450...
		class = 0xff&CFGR(CLASS);
		if (class & 0x80) //G450
		{
			si->ps.card_type=G450;
			LOG(4, ("50 revision %x\n", class&0x7f));
			status = g450_general_powerup();
		}
		else //G400
		{
			si->ps.card_type=G400;
			LOG(4, ("00 revision %x\n", class&0x7f));
			status = g400_general_powerup();
		}
		break;
	case 0x2527102b://G550 patch from Jean-Michel Batto
		si->ps.card_type=G450;
		LOG(4,("POWERUP:Detected G550\n"));
		status = g450_general_powerup();
		break;	
	default:
		LOG(8,("POWERUP:Failed to detect valid card 0x%08x\n",CFGR(DEVID)));
		return B_ERROR;
	}

	/*override memory if requested by user*/
	// even if detection works on the G400
	if (si->settings.memory != 0) // apsed
		si->ps.memory_size = si->settings.memory;

	return status;
}

status_t test_ram()
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
			/* G450 and G550 tune CAS latency via a predefined table at powerup time */			
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
status_t mil2_general_powerup()
{
	status_t result;

	LOG(4, ("INIT: Millenium II powerup\n"));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

//remove this:
	// various sensible defaults for MIL2
	si->ps.sdram = true;
	si->ps.memory_size = 2; //can override
	si->ps.memory_size = 4; //can override my mil2

	// apsed TODO MIL2 TVP 3026 may be 135, 175, 220 or 250MHz 
	// chip on my Millenium2 is TVP3026-250CPCE, 250MHz
	// rudolf: works in Mhz now
	si->ps.max_dac1_clock=250; // TVP3026 
//end remove this.

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
//restore this line:
//	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	//set to powergraphics etc.
	LOG(2, ("INIT: Skipping card coldstart!\n"));
	mil2_dac_init();

	VGAW_I(SEQ,1,0x00);
	/*enable screen*/
return B_OK;

	return B_ERROR; // apsed TODO MIL2 taken from G100, avoid DXIR/W DACR/W 
	
	/*power up the PLLs,LUT,DAC*/
	/*this bit should not be needed if BIOS has initialised it*/
	LOG(2,("INIT:PLL/LUT/DAC powerup\n"));
	DXIW(VREFCTRL,0x3F);                /*set voltage reference - using DAC reference block*/
	delay(100000);                      /*wait for 100ms for voltage reference to stabalise*/
	CFGW(OPTION,CFGR(OPTION)|0x20);     /*power up the SYSPLL - sets syspllpdN to 1*/
	while(!(DXIR(SYSPLLSTAT)&0x40));    /*wait for the SYSPLL frequency to lock*/
	LOG(2,("INIT: SYS PLL locked\n"));
	DXIW(PIXCLKCTRL,0x08);              /*power up the PIXPLL - sets pixpllpdN to 1*/
	while(!(DXIR(PIXPLLSTAT)&0x40));    /*wait for the PIXPLL frequency to lock*/
	LOG(2,("INIT: PIX PLL locked\n"));
	DXIW(MISCCTRL,0x1b);                /*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/

	/* setup i2c bus */
	i2c_init();

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT:Setting SYS/PIX plls to powergraphics speeds\n"));
	g100_dac_set_sys_pll();

	/*RAM initialisation*/
	LOG(2,("INIT:RAM init\n"));
	gx00_crtc_dpms(0,0,0);						/*turn off both displays*/
	ACCW(MCTLWTST,si->ps.mem_ctl);                                  /*set memory wait states*/
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF)|si->ps.mem_type);         /*set RAM type and config*/
	CFGW(OPTION2,(CFGR(OPTION2)&0xC100)|(si->ps.mem_rd));           /*set MEMRDCLK*/
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|(si->ps.membuf<<12));   /*set the memory buffer type*/
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MACCESS,ACCR(MACCESS)&0xFFFF7FFF);                         /*reset memory*/          
	delay(250);
	ACCW(MACCESS,ACCR(MACCESS)|0xC000);              		/*sets JEDEC as well*/
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MACCESS,ACCR(MACCESS)&0xFFFF3FFF);
	delay(250);
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.mem_rd&0xFFFF));/*set tap delays*/
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff)|(si->ps.mem_rfhcnt<<15)); /*start memory refresh*/

	/*Bus parameters*/
	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));                    /*enable retries, use advanced read*/

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);

	/*turn on display one*/
	gx00_crtc_dpms(1,1,1);

	return B_OK;
}

static 
status_t g100_general_powerup()
{
	status_t result;
	
	LOG(4, ("INIT: G100 powerup\n"));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	/*this bit should not be needed if BIOS has initialised it*/
	LOG(2,("INIT: PLL/LUT/DAC powerup\n"));
	/* G100 SGRAM and SDRAM use external pix and dac refs, do *not* activate internals!
	 * (this would create electrical shortcuts,
	 * resulting in extra chip heat and distortions visible on screen */
	DXIW(VREFCTRL,0x03);                /*set voltage reference - using DAC reference block partly */
	delay(100000);                      /*wait for 100ms for voltage reference to stabalise*/
	CFGW(OPTION,CFGR(OPTION)|0x20);     /*power up the SYSPLL - sets syspllpdN to 1*/
	while(!(DXIR(SYSPLLSTAT)&0x40));    /*wait for the SYSPLL frequency to lock*/
	LOG(2,("INIT: SYS PLL locked\n"));
	DXIW(PIXCLKCTRL,0x08);              /*power up the PIXPLL - sets pixpllpdN to 1*/
	while(!(DXIR(PIXPLLSTAT)&0x40));    /*wait for the PIXPLL frequency to lock*/
	LOG(2,("INIT: PIX PLL locked\n"));
	DXIW(MISCCTRL,0x1b);                /*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/

	/* setup i2c bus */
	i2c_init();

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g100_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* turn off both displays (also disables transfers) */
	gx00_crtc_dpms(0,0,0);
	/* disable plane write mask (needed for SDRAM) */
	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* set memory configuration:
	 * - no split framebuffer,
	 * - Mark says b14 (G200) should be done also though not defined for G100 in spec,
	 * - b3 v3_mem_type was included by Mark for memconfig setup: but looks like not defined */
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF8FFF) | ((si->ps.v3_mem_type & 0x04) << 10));
	/* set memory buffer type:
	 * - Mark says: if((v3_mem_type & 0x03) == 0x03) then do not or-in bits in option2;
	 *   but looks like v3_mem_type b1 is not defined,
	 * - Mark also says: place v3_mem_type b1 in option2 bit13 (if not 0x03) but b13 = reserved. */
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|((si->ps.v3_mem_type & 0x01) << 12));
	/* set mode register opcode to $0 */	
//	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xE1FFFFFF)); /* G200 only */
	/* set RAM read tap delay G100 */
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFFFF0) | ((si->ps.v3_mem_type & 0xf0) >> 4));
//	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|the_setting); /* G200 version */
	/* wait 200uS minimum */
	snooze(250);

	/* reset memory */
	ACCW(MACCESS,ACCR(MACCESS)&0xFFFF7FFF);
	/* select JEDEC reset method */
	ACCW(MACCESS,ACCR(MACCESS)|0x4000);
	/* perform actual RAM reset */
	ACCW(MACCESS,ACCR(MACCESS)|0x8000);
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

	/*turn on display one*/
	gx00_crtc_dpms(1,1,1);

	return B_OK;
}

static 
status_t g200_general_powerup()
{
	status_t result;
	
	LOG(4, ("INIT: G200 powerup\n"));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	/*this bit should not be needed if BIOS has initialised it*/
	LOG(2,("INIT: PLL/LUT/DAC powerup\n"));
//rudolf: check from here on:
	DXIW(VREFCTRL,0x3f);                /*set voltage reference - using DAC reference block*/
	delay(100000);                      /*wait for 100ms for voltage reference to stabalise*/
	CFGW(OPTION,CFGR(OPTION)|0x20);     /*power up the SYSPLL - sets syspllpdN to 1*/
	while(!(DXIR(SYSPLLSTAT)&0x40));    /*wait for the SYSPLL frequency to lock*/
	LOG(2,("INIT: SYS PLL locked\n"));
	DXIW(PIXCLKCTRL,0x08);              /*power up the PIXPLL - sets pixpllpdN to 1*/
	while(!(DXIR(PIXPLLSTAT)&0x40));    /*wait for the PIXPLL frequency to lock*/
	LOG(2,("INIT: PIX PLL locked\n"));
	DXIW(MISCCTRL,0x1b);                /*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/
//until here

	/* setup i2c bus */
	i2c_init();

//rudolf: remove:
	/*read the PINS and other stuff*/
	if (g200_card_info()==B_ERROR)
		return B_ERROR;
//until here

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting SYS/PIX plls to powergraphics speeds\n"));
	g200_dac_set_sys_pll();

	/*RAM initialisation*/
	LOG(2,("INIT:RAM init\n"));
	gx00_crtc_dpms(0,0,0);						/*turn off both displays*/
	ACCW(MCTLWTST,si->ps.mem_ctl);                                  /*set memory wait states*/
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF)|si->ps.mem_type);         /*set RAM type and config*/
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF)|(si->ps.mem_rd&0xFFFF0000));/*set MEMRDBK - mrsopcode*/
	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|(si->ps.membuf<<12));   /*set the memory buffer type*/
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MACCESS,ACCR(MACCESS)&0xFFFF7FFF);                         /*reset memory*/          
	ACCW(MACCESS,ACCR(MACCESS)|0x8000);
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.mem_rd&0xFFFF));/*set tap delays*/
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff)|(si->ps.mem_rfhcnt<<15)); /*start memory refresh*/

	/*Bus parameters*/
	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));                    /*enable retries, use advanced read*/

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);

	/*turn on display one*/
	gx00_crtc_dpms(1,1,1);

	return B_OK;
}

static 
status_t g400_general_powerup()
{
	status_t result;
	
	//fully functional G400 powerup -> uses settings from my card if no PINS
	LOG(4, ("INIT: G400 powerup\n"));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	/*this bit should not be needed if BIOS has initialised it*/
	LOG(4,("INIT: G400 PLL/LUT/DAC powerup\n"));
	DXIW(VREFCTRL,0x30);                /*set voltage reference - using DAC reference block*/
	delay(100000);                      /*wait for 100ms for voltage reference to stabalise*/
	CFGW(OPTION,CFGR(OPTION)|0x20);     /*power up the SYSPLL - sets syspllpdN to 1*/
	while(!(DXIR(SYSPLLSTAT)&0x40));    /*wait for the SYSPLL frequency to lock*/
	LOG(2,("INIT: SYS PLL locked\n"));
	DXIW(PIXCLKCTRL,0x08);              /*power up the PIXPLL - sets pixpllpdN to 1*/
	while(!(DXIR(PIXPLLSTAT)&0x40));    /*wait for the PIXPLL frequency to lock*/
	LOG(2,("INIT: PIX PLL locked\n"));
	DXIW(MISCCTRL,0x9b);                /*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/
	DXIW(MAFCDEL,0x2);                  /*makes CRTC2 stable! Matrox specify 8, but use 4 - grrrr!*/
	DXIW(PANELMODE,0x00);               /*eclipse panellink*/

	/* setup i2c bus */
	i2c_init();

//rudolf: remove
	/*read the PINS and other stuff*/
	if (g400_card_info()==B_ERROR)
		return B_ERROR;
//end remove

	/*make sure card is in powergraphics mode*/
	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting SYS/PIX plls to powergraphics speeds\n"));
	g400_dac_set_sys_pll();

	/*RAM initialisation*/
	LOG(2,("INIT:RAM init\n"));
	gx00_crtc_dpms(0,0,0);						/*turn off both displays*/
	g400_crtc2_dpms(0,0,0);
	ACCW(MCTLWTST,si->ps.mem_ctl);                                  /*set memory wait states*/
	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF)|si->ps.mem_type);         /*set RAM type and config*/
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF)|(si->ps.mem_rd&0xFFFF0000));/*set MEMRDBK - mrsopcode*/
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MACCESS,ACCR(MACCESS)&0xFFFF7FFF);                         /*reset memory*/          
	ACCW(MACCESS,ACCR(MACCESS)|0x8000);             
	delay(250);                                                     /*wait for 250microseconds*/
	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.mem_rd&0xFFFF));/*set tap delays*/
	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff)|(si->ps.mem_rfhcnt<<15)); /*start memory refresh*/

	/*Bus parameters*/
	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));                    /*enable retries, use advanced read*/

	/*enable writing to crtc registers*/
	VGAW_I(CRTC,0x11,0);
	if (si->ps.secondary_head)
	{
		MAVW(LOCK,0x01);
		CR2W(DATACTL,0x00000000);
	}

	/*turn on display one*/
	gx00_crtc_dpms(1,1,1);
	return B_OK;
}

static 
status_t g450_general_powerup()
{
	uint32 temp;
	status_t result;

	LOG(4, ("INIT: G450 powerup\n"));
	if (si->settings.logmask & 0x80000000) mga_dump_configuration_space();
	
	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

//rudolf: remove if impl in PINS above:
	// various sensible defaults for G450
	si->ps.sdram = true;
//end remove

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
	if (si->settings.usebios || (result != B_OK)) return gx00_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
//rudolf: checkout coldstart from here:

	//In case you are interested here is some of the G450 powerup stuff -> nonfunction as yet
		
	//These are the values of OPTION->OPTION4 used in Linux
	//rudolf: get from PINS...
	CFGW(OPTION, 0x400a1160);
	CFGW(OPTION2, 0x100ac00);
	CFGW(OPTION3, 0x90a409);
	CFGW(OPTION4, 0x80000004);

//rudolf: remove
	/*read the PINS and other stuff*/
	if (g450_card_info()==B_ERROR)
		return B_ERROR;
//end remove

	//various init (as bios)
	CFGW(OPTION, ((CFGR(OPTION)&0xf8404164) | (si->ps.option&0x207e00)) );
	CFGW(OPTION2, (CFGR(OPTION2) | (0xfc00&si->ps.option2)));
	ACCW(MCTLWTST, si->ps.mem_ctl);
	CFGW(OPTION4, (si->ps.option4&0x6000000f));
	ACCW(MEMRDBK, si->ps.mem_rd);

	ACCW(MACCESS, ((si->ps.maccess&0x80)>>1) );

	CFGW(OPTION4,((si->ps.option4&0x60000004)|0x80000000));

	delay(250);

	if 
	(
		((si->ps.option&0x200000) == 0) &&
		((si->ps.maccess&0x200) == 0)
	)
	{
			ACCW(MEMRDBK, ((si->ps.mem_rd)&0xffffefff));

			if ((si->ps.maccess&0x100) == 0)
			{
					ACCW(MACCESS, ACCR(MACCESS)&0xffff00ff);
			}
	}

	temp = ACCR(MACCESS);
	temp &=0xffff80ff;
	temp = temp|((temp&0x8000)>>1)|0x8000;
	ACCW(MACCESS, temp);

	temp &= 0xffff7fff;
	ACCW(MACCESS, temp);

	delay(250);

	if ((si->ps.maccess&0x400) == 0)
	{
			temp = si->ps.mem_ctl;
			temp = (temp &0x7) + 3;
			temp |= si->ps.mem_ctl &0xfffffff8;
			ACCW(MCTLWTST, temp);
	}

	temp = CFGR(OPTION);
	temp &=0xffe07fff;
	temp |= si->ps.option&0x1f8000;
	CFGW(OPTION, temp);

	return B_OK;
}

/*connect CRTC1 to the specified DAC*/
status_t gx00_general_dac_select(int dac)
{
	if (!si->ps.secondary_head)
		return B_ERROR;

	/*MISCCTRL, clock src,...*/
	switch(dac)
	{
		case DS_CRTCDAC_CRTC2MAVEN: /*CRTC->DAC,CRTC2->MAFC*/
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1); /*internal clk*/
			CR2W(CTL,(CR2R(CTL)&0xffe00779)|0xD0000002); /*external clk*/
			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)&0x77)); 
			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x82);
			break;
		case DS_CRTCMAVEN_CRTC2DAC:  /*CRTC->MAVEN,CRTC2->DAC*/
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x2); /*external clk*/
			CR2W(CTL,(CR2R(CTL)&0x2fe00779)|0x4|(0x1<<20)); /*internal clk*/
			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)|0x88));
			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x02);
			break;
		case DS_CRTCDAC_CRTC2DAC2:
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1); /*internal clk*/
			CR2W(CTL,(CR2R(CTL)&0x2fe00779)|0x4|(0x1<<20)); /*internal clk - gets DAC*/
	//FIXME		VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)&0x77)); 
	//FIXME		DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x82);
			break;
		case DS_CRTCDAC_CRTCDAC2:
			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1); /*internal clk*/
			CR2W(CTL,(CR2R(CTL)&0x2fe00779)|0x4|(0x0<<20)); /*internal clk - no DAC PTR*/
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}

/*busy wait until retrace!*/
status_t gx00_general_wait_retrace()
{
	while (!(ACCR(STATUS)&0x8));
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

	if (si->ps.card_type >= G100) {
		DXIW(MISCCTRL,0x9b);
		/*CRTC2->MAFC, 8-bit DAC, CLUT enabled, enable DAC*/

		DXIW(MULCTRL,0x4);
		/*RGBA direct mode*/
	} else { 
		LOG(8, ("INIT: < G100 DAC powerup badly implemented, MISC 0x%02x\n", VGAR(MISCR)));
	} // apsed TODO MIL2

	VGAW_I(SEQ,1,0x00);
	/*enable screen*/

	return B_OK;
}
