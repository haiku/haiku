/* Read initialisation information from card */
/* some bits are hacks, where PINS is not known */
/* Authors:
   Mark Watson 2/2000,
   Rudolf Cornelissen 10/2002-5/2006
*/

#define MODULE_BIT 0x00002000

#include "mga_std.h"

/* Parse the BIOS PINS structure if there */
status_t parse_pins ()
{
	uint8 pins_len = 0;
	uint8 *rom;
	uint8 *pins;
	uint8 chksum = 0;
	int i;
	status_t result = B_ERROR;

	/* preset PINS read status to failed */
	si->ps.pins_status = B_ERROR;

	/* check the validity of PINS */
	LOG(2,("INFO: Reading PINS info\n"));
	rom = (uint8 *) si->rom_mirror;
	/* check BIOS signature */
	if (rom[0]!=0x55 || rom[1]!=0xaa)
	{
		LOG(8,("INFO: BIOS signature not found\n"));
		return B_ERROR;
	}
	LOG(2,("INFO: BIOS signature $AA55 found OK\n"));
	/* check for a valid PINS struct adress */
	pins = rom + (rom[0x7FFC]|(rom[0x7FFD]<<8));
	if ((pins - rom) > 0x7F80)
	{
		LOG(8,("INFO: invalid PINS adress\n"));
		return B_ERROR;
	}
	/* checkout new PINS struct version if there */
	if ((pins[0] == 0x2E) && (pins[1] == 0x41))
	{
		pins_len = pins[2];
		if (pins_len < 3 || pins_len > 128)
		{
			LOG(8,("INFO: invalid PINS size\n"));
			return B_ERROR;
		}
		
		/* calculate PINS checksum */
		for (i = 0; i < pins_len; i++)
		{
			chksum += pins[i];
		}
		if (chksum)
		{
			LOG(8,("INFO: PINS checksum error\n"));
			return B_ERROR;
		}
		LOG(2,("INFO: new PINS, version %u.%u, length %u\n", pins[5], pins[4], pins[2]));
		/* fill out the si->ps struct if possible */
		switch (pins[5])
		{
			case 2:
				result = pins2_read(pins, pins_len);
				break;
			case 3:
				result = pins3_read(pins, pins_len);
				break;
			case 4:
				result = pins4_read(pins, pins_len);
				break;
			case 5:
				result = pins5_read(pins, pins_len);
				break;
			default:
				LOG(8,("INFO: unknown PINS version\n"));
				return B_ERROR;
				break;
		}
	}
	/* checkout old 64 byte PINS struct version if there */
	else if ((pins[0] == 0x40) && (pins[1] == 0x00))
	{
		pins_len = 0x40;
		/* this PINS version has no checksum */

		LOG(2,("INFO: old PINS found\n"));
		/* fill out the si->ps struct */
		result = pins1_read(pins, pins_len);
	}
	/* no valid PINS signature found */
	else
	{
		LOG(8,("INFO: no PINS signature found\n"));
		return B_ERROR;
	}
	/* check PINS read result */
	if (result == B_ERROR)
	{
		LOG(8,("INFO: PINS read/decode error\n"));
		return B_ERROR;
	}
	/* PINS scan succeeded */
	si->ps.pins_status = B_OK;
	LOG(2,("INFO: PINS scan completed succesfully\n"));
	return B_OK;
}

status_t pins1_read(uint8 *pins, uint8 length)
{
	if (length != 64)
	{
		LOG(8,("INFO: wrong PINS length, expected 64, got %d\n", length));
		return B_ERROR;
	}

//reset all for test:
//float:
	si->ps.f_ref = 0;
//uint32:
	si->ps.max_system_vco = 0;
	si->ps.min_system_vco = 0;
	si->ps.min_pixel_vco = 0;
	si->ps.min_video_vco = 0;
	si->ps.std_engine_clock_dh = 0;
	si->ps.max_dac1_clock_32 = 0;
	si->ps.max_dac1_clock_32dh = 0;
	si->ps.memory_size = 0;
	si->ps.mctlwtst_reg = 0;
	si->ps.memrdbk_reg = 0;
	si->ps.option2_reg = 0;
	si->ps.option3_reg = 0;
	si->ps.option4_reg = 0;
//uint8:
	si->ps.v3_option2_reg = 0;
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
//uint16:
	si->ps.v5_mem_type = 0;
//bools:
	si->ps.secondary_head = false;
	si->ps.tvout = false;
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	si->ps.sdram = true;

//experimental: checkout!
	si->ps.max_dac1_clock_32 = pins[22];//ramdac
	si->ps.max_pixel_vco = (pins[25] << 8) | pins[24];//PCLK
	si->ps.std_engine_clock = (pins[29] << 8) | pins[28];
	if ((uint32)((pins[31] << 8) | pins[30]) < si->ps.std_engine_clock)
		si->ps.std_engine_clock = (pins[31] << 8) | pins[30];
	if ((uint32)((pins[33] << 8) | pins[32]) < si->ps.std_engine_clock)
		si->ps.std_engine_clock = (pins[33] << 8) | pins[32];

//temp. test to see some vals..
	si->ps.max_video_vco = (pins[27] << 8) | pins[26];//LCLK
	//feature flags:
	si->ps.option_reg = (pins[53] << 24) | (pins[52] << 16) | (pins[51] << 8) | pins [50];

	si->ps.max_dac2_clock = (pins[35] << 8) | pins[34];//clkmod
	si->ps.max_dac2_clock_8 = (pins[37] << 8) | pins[36];//testclk
	si->ps.max_dac2_clock_16 = (pins[39] << 8) | pins[38];//vgafreq1
	si->ps.max_dac2_clock_24 = (pins[41] << 8) | pins[40];//vgafreq2
	si->ps.max_dac2_clock_32 = (pins[55] << 8) | pins[54];//vga clock
	si->ps.max_dac2_clock_32dh = pins[58];//vid ctrl

	si->ps.max_dac1_clock = (pins[29] << 8) | pins[28];//clkbase
	si->ps.max_dac1_clock_8 = (pins[31] << 8) | pins[30];//4mb
	si->ps.max_dac1_clock_16 = (pins[33] << 8) | pins[32];//8mb
	si->ps.max_dac1_clock_24 = pins[23];//ramdac type

//test! Don't actually use the reported settings for now...
	return B_OK;
}

status_t pins2_read(uint8 *pins, uint8 length)
{
	if (length != 64)
	{
		LOG(8,("INFO: wrong PINS length, expected 64, got %d\n", length));
		return B_ERROR;
	}

	LOG(2,("INFO: PINS version 2 details not yet known\n"));
	return B_ERROR;
}

/* pins v3 is used by G100 and G200. */
status_t pins3_read(uint8 *pins, uint8 length)
{
	/* used to calculate RAM refreshrate */
	float mclk_period;
	uint32 rfhcnt;

	if (length != 64)
	{
		LOG(8,("INFO: wrong PINS length, expected 64, got %d\n", length));
		return B_ERROR;
	}

	/* fill out the shared info si->ps struct */
	si->ps.max_pixel_vco = pins[36] + 100;

	si->ps.max_dac1_clock_8 = pins[37] + 100;
	si->ps.max_dac1_clock_16 = pins[38] + 100;
	si->ps.max_dac1_clock_24 = pins[39] + 100;
	si->ps.max_dac1_clock_32 = pins[40] + 100;

	si->ps.std_engine_clock = pins[44];
	if (pins [45] < si->ps.std_engine_clock) si->ps.std_engine_clock = pins[45];
	if (pins [46] < si->ps.std_engine_clock) si->ps.std_engine_clock = pins[46];
	if (pins [47] < si->ps.std_engine_clock) si->ps.std_engine_clock = pins[47];
	if ((si->ps.card_type == G200) && (pins[58] & 0x04))
	{
		/* G200 can work without divisor */
		si->ps.std_engine_clock *= 1;
	}
	else
	{
		if (pins[52] & 0x01)
			si->ps.std_engine_clock *= 3;
		else
			si->ps.std_engine_clock *= 2;
	}

	if (pins[52] & 0x20) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	/* G100 and G200 support 2-16Mb RAM */
	si->ps.memory_size = 2 << ((pins[55] & 0xc0) >> 6);
	/* more memory specifics */
	si->ps.mctlwtst_reg = (pins[51] << 24) | (pins[50] << 16) | (pins[49] << 8) | pins [48];
	si->ps.memrdbk_reg = 
		(pins[56] & 0x0f) | ((pins[56] & 0xf0) << 1) | ((pins[57] & 0x03) << 22) | ((pins[57] & 0xf0) << 21);
	/* Mark did this as one step in the above stuff, which must be wrong:
	((pins[p3_memrd+1]&0x03)>>2)<<16; //FIXME - ROR */

	si->ps.v3_clk_div = pins[52];
	si->ps.v3_mem_type = pins[54];
	si->ps.v3_option2_reg = pins[58];

	/* for cards using this version of PINS both functions are in maven */
	si->ps.tvout = !(pins[59] & 0x01);
	/* beware of TVout add-on boards: test for the MAVEN ourself! */
	if (i2c_maven_probe() == B_OK)
	{
		si->ps.tvout = true;
	}

	/* G200 and earlier cards are always singlehead cards */
	si->ps.secondary_head = false;
	if (si->ps.card_type >= G400) si->ps.secondary_head = !(pins[59] & 0x01);

	/* setup via gathered info from pins */
	si->ps.option_reg = 0;
	/* calculate refresh timer info-bits for 15uS interval (or shorter). See G100/G200 specs */
	/* calculate std memory clock period (nS) */
	if ((si->ps.card_type == G200) && (pins[58] & 0x08))
	{
		/* G200 can work without Mclk divisor */
		mclk_period = 1000.0 / si->ps.std_engine_clock;
	}
	else
	{
		if (pins[52] & 0x02)
			/* this factor is only used on G200, not on G100 */
			mclk_period = 3000.0 / si->ps.std_engine_clock;
		else
			mclk_period = 2000.0 / si->ps.std_engine_clock;
	}
	/* calculate needed setting, 'round-down' result! */
	rfhcnt = (uint32)(((15000 / mclk_period) - 1) / 64);
	/* check for register limit */
	if (rfhcnt > 0x3f) rfhcnt = 0x3f;
	/* add to option register */
	si->ps.option_reg |= (rfhcnt << 15);
	/* the rest of the OPTION info for pins v3 comes via 'v3_clk_div' and 'v3_mem_type'. */

	/* assuming the only possible panellink will be on the first head */
	si->ps.primary_dvi = !(pins[59] & 0x40);
	/* logical consequence of the above */
	si->ps.secondary_dvi = false;

	/* indirect logical consequences, see also G100 and G200 specs */
	si->ps.max_system_vco = si->ps.max_pixel_vco;
	si->ps.max_dac1_clock = si->ps.max_dac1_clock_8;
	si->ps.max_dac1_clock_32dh = si->ps.max_dac1_clock_32;
	si->ps.std_engine_clock_dh = si->ps.std_engine_clock;
	si->ps.sdram = (si->ps.v3_clk_div & 0x10);
	/* not supported: */
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_24 = 0;
	/* see G100, G200 and G400 specs */
	si->ps.min_system_vco = 50;
	si->ps.min_pixel_vco = 50;
	/* fixme: ehhh, no specs: confirm/tune these by testing?! */
	si->ps.max_video_vco = si->ps.max_pixel_vco;
	si->ps.min_video_vco = 50;
	/* assuming G100, G200 MAVEN has same specs as G400 MAVEN */
	si->ps.max_dac2_clock = 136;
	si->ps.max_dac2_clock_16 = 136;
	si->ps.max_dac2_clock_32dh = 136;
	si->ps.max_dac2_clock_32 = 136;

	/* not used here: */
	si->ps.option2_reg = 0;
	si->ps.option3_reg = 0;
	si->ps.option4_reg = 0;
	si->ps.v5_mem_type = 0;
	return B_OK;
}

/* pins v4 is used by G400 and G400MAX */
status_t pins4_read(uint8 *pins, uint8 length)
{
	/* used to calculate RAM refreshrate */
	float mclk_period;
	uint32 rfhcnt;

	if (length != 128)
	{
		LOG(8,("INFO: wrong PINS length, expected 128, got %d\n", length));
		return B_ERROR;
	}

	/* fill out the shared info si->ps struct */
	if (pins[39] == 0xff) si->ps.max_pixel_vco = 230;
	else si->ps.max_pixel_vco = 4 * pins[39];

	if (pins[38] == 0xff) si->ps.max_system_vco = si->ps.max_pixel_vco;
	else si->ps.max_system_vco = 4 * pins[38];

	if (pins[40] == 0xff) si->ps.max_dac1_clock_8 = si->ps.max_pixel_vco;
	else si->ps.max_dac1_clock_8 = 4 * pins[40];

	if (pins[41] == 0xff) si->ps.max_dac1_clock_16 = si->ps.max_dac1_clock_8;
	else si->ps.max_dac1_clock_16 = 4 * pins[41];

	if (pins[42] == 0xff) si->ps.max_dac1_clock_24 = si->ps.max_dac1_clock_16;
	else si->ps.max_dac1_clock_24 = 4 * pins[42];

	if (pins[43] == 0xff) si->ps.max_dac1_clock_32 = si->ps.max_dac1_clock_24;
	else si->ps.max_dac1_clock_32 = 4 * pins[43];

	if (pins[44] == 0xff) si->ps.max_dac2_clock_16 = si->ps.max_pixel_vco;
	else si->ps.max_dac2_clock_16 = 4 * pins[44];

	if (pins[45] == 0xff) si->ps.max_dac2_clock_32 = si->ps.max_dac2_clock_16;
	else si->ps.max_dac2_clock_32 = 4 * pins[45];

	/* verified against windows driver: */
	si->ps.std_engine_clock = 2 * pins[65];
	
	if (pins[92] & 0x01) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	si->ps.memory_size = 4 << ((pins[92] >> 2) & 0x03);
	/* more memory specifics */
	si->ps.mctlwtst_reg = (pins[74] << 24) | (pins[73] << 16) | (pins[72] << 8) | pins [71];
	si->ps.option3_reg = (pins[70] << 24) | (pins[69] << 16) | (pins[68] << 8) | pins [67];
	/* mrsopcod field, msb is always zero.. */
	si->ps.memrdbk_reg = 
		(pins[86] & 0x0f) | ((pins[86] & 0xf0) << 1) | ((pins[87] & 0x03) << 22) | ((pins[87] & 0xf0) << 21);
	si->ps.sdram = (pins[92] & 0x10);

	/* setup via gathered info from pins */
	si->ps.option_reg = ((pins[53] & 0x38) << 7) | ((pins[53] & 0x40) << 22) | ((pins[53] & 0x80) << 15);
	/* calculate refresh timer info-bits for 15uS interval (or shorter). See G400 specs;
	 * the 15uS value was confirmed by Mark Watson for both G400 and G400MAX */
	/* calculate std memory clock period (nS) */
	switch ((si->ps.option3_reg & 0x0000e000) >> 13)
	{
	case 0:
		mclk_period = 3000.0 / (si->ps.std_engine_clock * 1);
		break;
	case 1:
		mclk_period = 5000.0 / (si->ps.std_engine_clock * 2);
		break;
	case 2:
		mclk_period = 9000.0 / (si->ps.std_engine_clock * 4);
		break;
	case 3:
		mclk_period = 2000.0 / (si->ps.std_engine_clock * 1);
		break;
	case 4:
		mclk_period = 3000.0 / (si->ps.std_engine_clock * 2);
		break;
	case 5:
		mclk_period = 1000.0 / (si->ps.std_engine_clock * 1);
		break;
	default:
		/* we choose the lowest refreshcount that could be needed (so assuming slowest clocked memory) */
		mclk_period = 3000.0 / (si->ps.std_engine_clock * 1);
		LOG(8,("INFO: undefined/unknown memory clock divider select, using failsafe for refresh\n"));
		break;	
	}
	/* calculate needed setting, 'round-down' result! */
	rfhcnt = (uint32)(((15000 / mclk_period) - 1) / 64);
	/* check for register limit */
	if (rfhcnt > 0x3f) rfhcnt = 0x3f;
	/* add to option register */
	si->ps.option_reg |= (rfhcnt << 15);

	/* for cards using this version of PINS both functions are in maven */
	si->ps.tvout = !(pins[91] & 0x01);
	/* beware of TVout add-on boards: test for the MAVEN ourself! */
	if (i2c_maven_probe() == B_OK)
	{
		si->ps.tvout = true;
	}

	/* G200 and earlier cards are always singlehead cards */
	si->ps.secondary_head = false;
	if (si->ps.card_type >= G400) si->ps.secondary_head = !(pins[91] & 0x01);

	/* assuming the only possible panellink will be on the first head */
	si->ps.primary_dvi = !(pins[91] & 0x40);
	/* logical consequence of the above */
	si->ps.secondary_dvi = false;

	/* indirect logical consequences, see also G100, G200 and G400 specs */
	si->ps.max_dac1_clock = si->ps.max_dac1_clock_8;
	si->ps.max_dac2_clock = si->ps.max_dac2_clock_16;
	si->ps.max_dac1_clock_32dh = si->ps.max_dac1_clock_32;
	si->ps.max_dac2_clock_32dh = si->ps.max_dac2_clock_32;
	si->ps.std_engine_clock_dh = si->ps.std_engine_clock;
	/* not supported: */
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_24 = 0;
	/* see G100, G200 and G400 specs */
	si->ps.min_system_vco = 50;
	si->ps.min_pixel_vco = 50;
	/* fixme: ehhh, no specs: confirm/tune these by testing?! */
	si->ps.max_video_vco = si->ps.max_pixel_vco;
	si->ps.min_video_vco = 50;

	/* not used here: */
	si->ps.option2_reg = 0;
	si->ps.option4_reg = 0;
	si->ps.v3_option2_reg = 0;
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
	si->ps.v5_mem_type = 0;

	/* check for a G400MAX card */
	/* fixme: use the PCI configspace ID method if it exists... */
	if (si->ps.max_dac1_clock > 300)
	{
		si->ps.card_type = G400MAX;
		LOG(2,("INFO: G400MAX detected\n"));
	}
	return B_OK;
}

/* pins v5 is used by G450 and G550 */
status_t pins5_read(uint8 *pins, uint8 length)
{
	unsigned int m_factor = 6;

	if (length != 128)
	{
		LOG(8,("INFO: wrong PINS length, expected 128, got %d\n", length));
		return B_ERROR;
	}

	/* fill out the shared info si->ps struct */
	if (pins[4] == 0x01) m_factor = 8;
	if (pins[4] >= 0x02) m_factor = 10;

	si->ps.max_system_vco = m_factor * pins[36];
	si->ps.max_video_vco = m_factor * pins[37];
	si->ps.max_pixel_vco = m_factor * pins[38];
	si->ps.min_system_vco = m_factor * pins[121];
	si->ps.min_video_vco = m_factor * pins[122];
	si->ps.min_pixel_vco = m_factor * pins[123];

	if (pins[39] == 0xff) si->ps.max_dac1_clock_8 = si->ps.max_pixel_vco;
	else si->ps.max_dac1_clock_8 = 4 * pins[39];

	if (pins[40] == 0xff) si->ps.max_dac1_clock_16 = si->ps.max_dac1_clock_8;
	else si->ps.max_dac1_clock_16 = 4 * pins[40];

	if (pins[41] == 0xff) si->ps.max_dac1_clock_24 = si->ps.max_dac1_clock_16;
	else si->ps.max_dac1_clock_24 = 4 * pins[41];

	if (pins[42] == 0xff) si->ps.max_dac1_clock_32 = si->ps.max_dac1_clock_24;
	else si->ps.max_dac1_clock_32 = 4 * pins[42];

	if (pins[124] == 0xff) si->ps.max_dac1_clock_32dh = si->ps.max_dac1_clock_32;
	else si->ps.max_dac1_clock_32dh = 4 * pins[124];

	if (pins[43] == 0xff) si->ps.max_dac2_clock_16 = si->ps.max_video_vco;
	else si->ps.max_dac2_clock_16 = 4 * pins[43];

	if (pins[44] == 0xff) si->ps.max_dac2_clock_32 = si->ps.max_dac2_clock_16;
	else si->ps.max_dac2_clock_32 = 4 * pins[44];

	if (pins[125] == 0xff) si->ps.max_dac2_clock_32dh = si->ps.max_dac2_clock_32;
	else si->ps.max_dac2_clock_32dh = 4 * pins[125];

	if (pins[118] == 0xff) si->ps.max_dac1_clock = si->ps.max_dac1_clock_8;
	else si->ps.max_dac1_clock = 4 * pins[118];

	if (pins[119] == 0xff) si->ps.max_dac2_clock = si->ps.max_dac1_clock;
	else si->ps.max_dac2_clock = 4 * pins[119];

	si->ps.std_engine_clock = 4 * pins[74];
	si->ps.std_engine_clock_dh = 4 * pins[92];

	si->ps.memory_size = ((pins[114] & 0x03) + 1) * 8;
	if ((pins[114] & 0x07) > 3)
	{
		LOG(8,("INFO: unknown RAM size, defaulting to 8Mb\n"));
		si->ps.memory_size = 8;
	}

	if (pins[110] & 0x01) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	/* make sure SGRAM functions only get enabled if SGRAM mounted */
	if ((pins[114] & 0x18) == 0x08) si->ps.sdram = false;
	else si->ps.sdram = true;
	/* more memory specifics */
	si->ps.v5_mem_type = (pins[115] << 8) | pins [114];

	/* various registers */
	si->ps.option_reg = (pins[51] << 24) | (pins[50] << 16) | (pins[49] << 8) | pins [48];
	si->ps.option2_reg = (pins[55] << 24) | (pins[54] << 16) | (pins[53] << 8) | pins [52];
	si->ps.option3_reg = (pins[79] << 24) | (pins[78] << 16) | (pins[77] << 8) | pins [76];
	si->ps.option4_reg = (pins[87] << 24) | (pins[86] << 16) | (pins[85] << 8) | pins [84];
	si->ps.mctlwtst_reg = (pins[83] << 24) | (pins[82] << 16) | (pins[81] << 8) | pins [80];
	si->ps.memrdbk_reg = (pins[91] << 24) | (pins[90] << 16) | (pins[89] << 8) | pins [88];

	/* both the secondary head and MAVEN are on die, (so) no add-on boards exist */
	si->ps.secondary_head = (pins[117] & 0x70);
	si->ps.tvout = (pins[117] & 0x40);

	si->ps.primary_dvi = (pins[117] & 0x02);
	si->ps.secondary_dvi = (pins[117] & 0x20);

	/* not supported: */
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_24 = 0;

	/* not used here: */
	si->ps.v3_option2_reg = 0;
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
	return B_OK;
}

/* fake_pins presumes the card was coldstarted by it's BIOS */
void fake_pins(void)
{
	LOG(8,("INFO: faking PINS\n"));

	switch (si->ps.card_type)
	{
		case MIL1:
			pinsmil1_fake();
			break;
		case MIL2:
			pinsmil2_fake();
			break;
		case G100:
			pinsg100_fake();
			break;
		case G200:
			pinsg200_fake();
			break;
		case G400:
			pinsg400_fake();
			break;
		case G400MAX:
			pinsg400max_fake();
			break;
		case G450:
			pinsg450_fake();
			break;
		case G550:
			pinsg550_fake();
			break;
	}

	/* find out if the card has a maven */
	si->ps.tvout = false;
	si->ps.secondary_head = false;
	/* only do I2C probe if the card has a chance */
	if (si->ps.card_type >= G100)
	{
		if (i2c_maven_probe() == B_OK)
		{
			si->ps.tvout = true;
			/* G200 and earlier cards are always singlehead cards */
			if (si->ps.card_type >= G400) si->ps.secondary_head = true;
		}
	}

	/* not used because no coldstart will be attempted */
	si->ps.std_engine_clock = 0;
	si->ps.std_engine_clock_dh = 0;
	si->ps.mctlwtst_reg = 0;
	si->ps.memrdbk_reg = 0;
	si->ps.option_reg = 0;
	si->ps.option2_reg = 0;
	si->ps.option3_reg = 0;
	si->ps.option4_reg = 0;
	si->ps.v3_option2_reg = 0;
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
	si->ps.v5_mem_type = 0;
}

void pinsmil1_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	si->ps.f_ref = 14.31818;
	/* see MIL1 specs */
	si->ps.max_system_vco = 220;
	si->ps.min_system_vco = 110;
	si->ps.max_pixel_vco = 220;
	si->ps.min_pixel_vco = 110;
	/* no specs, assuming these */
	si->ps.max_video_vco = 0;
	si->ps.min_video_vco = 0;
	/* see MIL1 specs */
	si->ps.max_dac1_clock = 220;
	si->ps.max_dac1_clock_8 = 220;
	si->ps.max_dac1_clock_16 = 200;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 180;
	si->ps.max_dac1_clock_32 = 136;
	si->ps.max_dac1_clock_32dh = 0;
	/* see specs */
	si->ps.max_dac2_clock = 0;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 0;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 0;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 0;
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 2Mb RAM mounted */
	//fixme: see if we can get this from OPTION or so...
	si->ps.memory_size = 2;
	//fixme: should be overrule-able via mga.settings for MIL1.
	//fail-safe mode for now:
	si->ps.sdram = true;
}

void pinsmil2_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	si->ps.f_ref = 14.31818;
	/* see MIL2 specs */
	si->ps.max_system_vco = 220;
	si->ps.min_system_vco = 110;
	si->ps.max_pixel_vco = 220;
	si->ps.min_pixel_vco = 110;
	/* no specs, assuming these */
	si->ps.max_video_vco = 0;
	si->ps.min_video_vco = 0;
	/* see MIL2 specs */
	si->ps.max_dac1_clock = 220;
	si->ps.max_dac1_clock_8 = 220;
	si->ps.max_dac1_clock_16 = 200;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 180;
	si->ps.max_dac1_clock_32 = 136;
	si->ps.max_dac1_clock_32dh = 0;
	/* see specs */
	si->ps.max_dac2_clock = 0;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 0;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 0;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 0;
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 4Mb RAM mounted */
	//fixme: see if we can get this from OPTION or so...
	si->ps.memory_size = 4;
	//fixme: should be overrule-able via mga.settings for MIL2.
	//fail-safe mode for now:
	si->ps.sdram = true;
}

void pinsg100_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G100 specs */
	si->ps.max_system_vco = 230;
	si->ps.min_system_vco = 50;
	si->ps.max_pixel_vco = 230;
	si->ps.min_pixel_vco = 50;
	/* no specs, assuming these */
	si->ps.max_video_vco = 230;
	si->ps.min_video_vco = 50;
	/* see G100 specs */
	si->ps.max_dac1_clock = 230;
	si->ps.max_dac1_clock_8 = 230;
	si->ps.max_dac1_clock_16 = 230;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 180;
	si->ps.max_dac1_clock_32 = 136;
	si->ps.max_dac1_clock_32dh = 136;
	/* see specs */
	si->ps.max_dac2_clock = 136;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 136;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 136;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 136;
	/* assuming the only possible panellink will be on the first head */
	//fixme: primary_dvi should be overrule-able via mga.settings for G100.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 2Mb RAM mounted */
	si->ps.memory_size = 2;
	//fixme: should be overrule-able via mga.settings for G100.
	//fail-safe mode for now:
	si->ps.sdram = true;
}

void pinsg200_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G200 specs */
	si->ps.max_system_vco = 250;
	si->ps.min_system_vco = 50;
	si->ps.max_pixel_vco = 250;
	si->ps.min_pixel_vco = 50;
	/* no specs, assuming these */
	si->ps.max_video_vco = 250;
	si->ps.min_video_vco = 50;
	/* see G200 specs */
	si->ps.max_dac1_clock = 250;
	si->ps.max_dac1_clock_8 = 250;
	si->ps.max_dac1_clock_16 = 250;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 180;
	si->ps.max_dac1_clock_32 = 136;
	si->ps.max_dac1_clock_32dh = 136;
	/* see specs */
	si->ps.max_dac2_clock = 136;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 136;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 136;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 136;
	/* assuming the only possible panellink will be on the first head */
	//fixme: primary_dvi should be overrule-able via mga.settings for G100.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 2Mb RAM mounted */
	si->ps.memory_size = 2;
	/* ask the G200 what type of RAM it has been set to by it's BIOS */
	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
}

void pinsg400_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G400 specs */
	si->ps.max_system_vco = 300;
	si->ps.min_system_vco = 50;
	si->ps.max_pixel_vco = 300;
	si->ps.min_pixel_vco = 50;
	/* no specs, assuming these */
	si->ps.max_video_vco = 300;
	si->ps.min_video_vco = 50;
	/* see G400 specs */
	si->ps.max_dac1_clock = 300;
	si->ps.max_dac1_clock_8 = 300;
	si->ps.max_dac1_clock_16 = 300;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 230;
	si->ps.max_dac1_clock_32 = 180;
	si->ps.max_dac1_clock_32dh = 136;
	/* see specs */
	si->ps.max_dac2_clock = 136;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 136;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 136;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 136;
	/* assuming the only possible panellink will be on the first head */
	//fixme: primary_dvi should be overrule-able via mga.settings for G400.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 4Mb RAM mounted */
	si->ps.memory_size = 4;
	/* ask the G400 what type of RAM it has been set to by it's BIOS */
	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
}

/* this routine is currently unused, because G400MAX is detected via pins! */
void pinsg400max_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G400MAX specs */
	si->ps.max_system_vco = 360;
	si->ps.min_system_vco = 50;
	si->ps.max_pixel_vco = 360;
	si->ps.min_pixel_vco = 50;
	/* no specs, assuming these */
	si->ps.max_video_vco = 360;
	si->ps.min_video_vco = 50;
	/* see G400MAX specs */
	si->ps.max_dac1_clock = 360;
	si->ps.max_dac1_clock_8 = 360;
	si->ps.max_dac1_clock_16 = 360;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 280;
	si->ps.max_dac1_clock_32 = 230;
	si->ps.max_dac1_clock_32dh = 136;
	/* see specs */
	si->ps.max_dac2_clock = 136;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 136;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 136;
	/* 'failsave' value */
	si->ps.max_dac2_clock_32dh = 136;
	/* assuming the only possible panellink will be on the first head */
	//fixme: primary_dvi should be overrule-able via mga.settings for G400MAX.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 4Mb RAM mounted */
	si->ps.memory_size = 4;
	/* ask the G400MAX what type of RAM it has been set to by it's BIOS */
	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
}

void pinsg450_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G450 pins readouts for max ranges, then use a bit smaller ones */
	/* carefull not to take to high lower limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 640;
	si->ps.min_system_vco = 320;
	si->ps.max_pixel_vco = 640;
	si->ps.min_pixel_vco = 320;
	si->ps.max_video_vco = 640;
	si->ps.min_video_vco = 320;
	si->ps.max_dac1_clock = 360;
	si->ps.max_dac1_clock_8 = 360;
	si->ps.max_dac1_clock_16 = 360;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 280;
	si->ps.max_dac1_clock_32 = 230;
	si->ps.max_dac1_clock_32dh = 180;
	/* see G450 pins readouts */
	si->ps.max_dac2_clock = 232;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 232;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 232;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 180;
	//fixme: primary & secondary_dvi should be overrule-able via mga.settings for G450.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 8Mb RAM mounted */
	si->ps.memory_size = 8;
	/* ask the G450 what type of RAM it has been set to by it's BIOS */
//todo:
//	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
//fail-safe mode for now:
	si->ps.sdram = true;
}

void pinsg550_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G550 pins readouts for max ranges, then use a bit smaller ones */
	/* carefull not to take to high lower limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 768;
	si->ps.min_system_vco = 384;
	si->ps.max_pixel_vco = 960;
	si->ps.min_pixel_vco = 320;
	si->ps.max_video_vco = 960;
	si->ps.min_video_vco = 320;
	si->ps.max_dac1_clock = 360;
	si->ps.max_dac1_clock_8 = 360;
	si->ps.max_dac1_clock_16 = 360;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 280;
	si->ps.max_dac1_clock_32 = 230;
	si->ps.max_dac1_clock_32dh = 180;
	/* see G550 pins readouts */
	si->ps.max_dac2_clock = 232;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 232;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 232;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 180;
	//fixme: primary & secondary_dvi should be overrule-able via mga.settings for G550.
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/*  presume 8Mb RAM mounted */
	si->ps.memory_size = 8;
	/* ask the G550 what type of RAM it has been set to by it's BIOS */
//todo:
//	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
//fail-safe mode for now:
	si->ps.sdram = true;
}

void dump_pins(void)
{
	LOG(2,("INFO: pinsdump follows:\n"));
	LOG(2,("f_ref: %fMhz\n", si->ps.f_ref));
	LOG(2,("max_system_vco: %dMhz\n", si->ps.max_system_vco));
	LOG(2,("min_system_vco: %dMhz\n", si->ps.min_system_vco));
	LOG(2,("max_pixel_vco: %dMhz\n", si->ps.max_pixel_vco));
	LOG(2,("min_pixel_vco: %dMhz\n", si->ps.min_pixel_vco));
	LOG(2,("max_video_vco: %dMhz\n", si->ps.max_video_vco));
	LOG(2,("min_video_vco: %dMhz\n", si->ps.min_video_vco));
	LOG(2,("std_engine_clock: %dMhz\n", si->ps.std_engine_clock));
	LOG(2,("std_engine_clock_dh: %dMhz\n", si->ps.std_engine_clock_dh));
	LOG(2,("max_dac1_clock: %dMhz\n", si->ps.max_dac1_clock));
	LOG(2,("max_dac1_clock_8: %dMhz\n", si->ps.max_dac1_clock_8));
	LOG(2,("max_dac1_clock_16: %dMhz\n", si->ps.max_dac1_clock_16));
	LOG(2,("max_dac1_clock_24: %dMhz\n", si->ps.max_dac1_clock_24));
	LOG(2,("max_dac1_clock_32: %dMhz\n", si->ps.max_dac1_clock_32));
	LOG(2,("max_dac1_clock_32dh: %dMhz\n", si->ps.max_dac1_clock_32dh));
	LOG(2,("max_dac2_clock: %dMhz\n", si->ps.max_dac2_clock));
	LOG(2,("max_dac2_clock_8: %dMhz\n", si->ps.max_dac2_clock_8));
	LOG(2,("max_dac2_clock_16: %dMhz\n", si->ps.max_dac2_clock_16));
	LOG(2,("max_dac2_clock_24: %dMhz\n", si->ps.max_dac2_clock_24));
	LOG(2,("max_dac2_clock_32: %dMhz\n", si->ps.max_dac2_clock_32));
	LOG(2,("max_dac2_clock_32dh: %dMhz\n", si->ps.max_dac2_clock_32dh));
	LOG(2,("secondary_head: "));
	if (si->ps.secondary_head) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("tvout: "));
	if (si->ps.tvout) LOG(2,("present\n")); else LOG(2,("absent\n"));
	//fixme: probably only valid for pre-G400 cards...(?)
	if ((si->ps.tvout) && (si->ps.card_type < G450))
	{
		if (si->ps.card_type < G400)
			LOG(2,("MGA_TVO version: "));
		else
			LOG(2,("MAVEN version: "));
		if ((MAVR(VERSION)) < 20)
			LOG(2,("rev. B\n"));
		else
			LOG(2,("rev. C\n"));
	}
	LOG(2,("primary_dvi: "));
	if (si->ps.primary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("secondary_dvi: "));
	if (si->ps.secondary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("card memory_size: %dMb\n", si->ps.memory_size));
	LOG(2,("mctlwtst register: $%08x\n", si->ps.mctlwtst_reg));
	LOG(2,("memrdbk register: $%08x\n", si->ps.memrdbk_reg));
	LOG(2,("option register: $%08x\n", si->ps.option_reg));
	LOG(2,("option2 register: $%08x\n", si->ps.option2_reg));
	LOG(2,("option3 register: $%08x\n", si->ps.option3_reg));
	LOG(2,("option4 register: $%08x\n", si->ps.option4_reg));
	LOG(2,("v3_option2_reg: $%02x\n", si->ps.v3_option2_reg));
	LOG(2,("v3_clock_div: $%02x\n", si->ps.v3_clk_div));
	LOG(2,("v3_mem_type: $%02x\n", si->ps.v3_mem_type));
	LOG(2,("v5_mem_type: $%04x\n", si->ps.v5_mem_type));
	LOG(2,("sdram: "));
	if (si->ps.sdram) LOG(2,("SDRAM card\n")); else LOG(2,("SGRAM card\n"));
	LOG(2,("INFO: end pinsdump.\n"));
}
