/* Read initialisation information from card */
/* some bits are hacks, where PINS is not known */
/* Authors:
   Mark Watson 2/2000,
   Rudolf Cornelissen 10/2002
*/

#define MODULE_BIT 0x00002000

#include "mga_std.h"

//general pins stuff!
enum {
	id=0x00,
	length=0x02,
	version=0x04,
	location=0x7FFC
};

//pins v5(!) (as used by G450)
enum {
	p5_option=0x30,
	p5_option2=0x34,
	p5_memctl=0x3e,
	p5_option4=0x42,
	p5_memrd=0x46,
	p5_maccess=0x72
};

//pins v4 (as used by G400)
enum {
	p4_memtype=0x35,
	p4_memctl=0x3d,
	p4_memrd=0x56,
	p4_sdram=0x5c, 
};

//pins v3 (as used by G200/G100?)
enum {
	p3_memtype=0x36,
	p3_memctl=0x30,
	p3_memrd=0x38,
	p3_sdram=0x34,
	p3_membuf=0x3a  //G200 extra?
	};
	
static void dump_card_infos (void)
{
	MSG(("g200_card_info:Memory control word: 0x%08x\n",si->ps.mem_ctl));
	if (si->ps.sdram) MSG(("g200_card_info:is SDRAM card:       1\n"));
	else MSG(("g200_card_info:is SDRAM card:       0\n"));
	MSG(("g200_card_info:Memory config:       0x%08x\n",si->ps.mem_type));
	MSG(("g200_card_info:MEMRDBK setting:     0x%08x\n",si->ps.mem_rd));
	MSG(("g200_card_info:Membuftype:          0x%08x\n",si->ps.membuf));
	MSG(("g200_card_info:mem_rfhcnt:          %d\n",    si->ps.mem_rfhcnt));
}

/*word out card specific information - using PINS where possible (if not substitute sensible defaults)*/
status_t g200_card_info()
{
	uint8 * rom;
	uint8 * pins;
	int i;
	int chksum = 0;

	/*check the validity of PINS*/
	LOG(4,("g200_card_info: Reading PINS info\n"));
	rom = (uint8 *) si->rom_mirror;

	//check sig
	if (rom[0]!=0x55 || rom[1]!=0xaa)
	{
		LOG(8,("g200_card_info:BIOS bad signiture: 0x%02x%02x, expected 0x55aa\n",rom[0],rom[1]));
	}
	LOG(2,("g200_card_info: BIOS signiture $AA55 found OK\n"));
	pins = rom + (rom[location]|(rom[location+1]<<8));
	LOG(2,("g200_card_info: Using PINS v%d.%d structure at 0x%04x\n",pins[version+1],pins[version],pins-rom));
	//check valid
	for (i=0;i<pins[length];i++)
	{
		chksum+=pins[i];
	}
	if(chksum%256)
	{
		si->ps.mem_ctl=0x4244ca1;
		si->ps.mem_type=(4<<10)|(0<<14); //SDRAM memory config 4
		si->ps.mem_rd=0x108;
		si->ps.membuf=0;
	}
	else
	{
		LOG(2,("INFO:PINS checksum is correct - grabbing PINS values\n"));

		/*read memory control word*/
		si->ps.mem_ctl=*((uint32 *)(pins + p3_memctl));

		/*read memory config*/
		si->ps.mem_type=(pins[p3_memtype]&0x7)<<10;
		if (!si->ps.sdram) si->ps.mem_type|= (0x01<<14);

		/*figure out memrdbk settings*/
		si->ps.mem_rd=((pins[p3_memrd+1]&0xf0)>>3)<<24|((pins[p3_memrd+1]&0x03)>>2)<<16; //FIXME - ROR
		si->ps.mem_rd|=((pins[p3_memrd]&0xf0)<<1)|(pins[p3_memrd]&0x0f);
	
		//memory buffer type setting
		si->ps.membuf=pins[p3_membuf]&0x3;
	}

	/*FIXME hardcode a few required values, i.e. ones not found in PINS*/
	/*memory refresh settings (values pinched from windows)*/
	si->ps.mem_rfhcnt=0x78000>>15;

	if (si->settings.logmask & 0x80000000) dump_card_infos();
	return B_OK;
}

/*word out card specific information - using PINS where possible (if not substitute sensible defaults)*/
status_t g400_card_info()
{
	uint8 * rom;
	uint8 * pins;
	int i;
	int chksum = 0;

	LOG(4,("INFO:Getting G400 card info\n"));
	/*check the validity of PINS*/
	LOG(2,("INFO:Reading PINS info\n"));
	rom = (uint8 *) si->rom_mirror;
	//check sig
	if (rom[0]!=0x55 || rom[1]!=0xaa)
	{
		LOG(8,("INFO:BIOS signiture not found\n"));
	}
	LOG(2,("INFO:BIOS signiture $AA55 found OK\n"));
	pins = rom + (rom[location]|(rom[location+1]<<8));
	LOG(2,("INFO:Using PINS v%d.%d structure at: %x\n",pins[version+1],pins[version],pins-rom));
	//check valid 
	for (i=0;i<pins[length];i++)
	{
		chksum+=pins[i];
	}

	if((chksum%256)) //if pins is invalid make up some stuff
	{
		LOG(8,("INFO:PINS checksum is incorrect - using default values\n"));
		LOG(8,("INFO:AFAIK these should work with most G400s, but they are untested\n"));
		si->ps.mem_ctl=0x24045491;
		si->ps.mem_type=1<<14; //SDRAM memory config 0
		si->ps.mem_rd=0x108;
		si->ps.sdram=true;
	}
	else
	{
		LOG(2,("INFO:PINS checksum is correct - grabbing PINS values\n"));

		/*read memory control word*/
		si->ps.mem_ctl=*((uint32 *)(pins + p4_memctl));
		LOG(2,("INFO:Memory control word: %x\n",si->ps.mem_ctl));

		/*read memory config*/
		si->ps.sdram = (pins[p4_sdram]&0x10);
		si->ps.mem_type=(pins[p4_memtype]&0x38)<<7;
		if (!si->ps.sdram) si->ps.mem_type |= (0x01 << 14);
		LOG(2,("INFO:Memory config: %x\n",si->ps.mem_type));

		/*figure out memrdbk settings*/
		si->ps.mem_rd=((pins[p4_memrd+1]&0xf0)>>3)<<24|((pins[p4_memrd+1]&0x03)>>2)<<16; //FIXME - ROR
		si->ps.mem_rd|=((pins[p4_memrd]&0xf0)<<1)|(pins[p4_memrd]&0x0f);
		LOG(2,("INFO:MEMRDBK setting: %x\n",si->ps.mem_rd));
	
		/*figure out if it is a G400MAX*/
		/*FIXME, use the correct ID method!*/
		if (si->ps.mem_ctl==0x20049911)
		{
			LOG(2,("INFO:MAX\n"));
			si->ps.card_type=G400MAX;
		}
	}

	/*FIXME hardcode a few required values, i.e. ones not found in PINS*/
	/*memory refresh settings (values pinched from windows)*/
	if (si->ps.card_type==G400)
	{
		si->ps.mem_rfhcnt=0x27;
	}
	else if (si->ps.card_type==G400MAX)
	{
		si->ps.mem_rfhcnt=0x2e;
	}
	if (si->settings.logmask & 0x80000000) dump_card_infos();

	return B_OK;
}

/*word out card specific information - using PINS where possible (if not substitute sensible defaults)*/
status_t g450_card_info()
{
	uint8 * rom;
	uint8 * pins;
	int i;
	int chksum = 0;

	LOG(4,("INFO:Getting G450 card info\n"));
	/*check the validity of PINS*/
	LOG(2,("INFO:Reading PINS info\n"));
	rom = (uint8 *) si->rom_mirror;
	//check sig
	if (rom[0]!=0x55 || rom[1]!=0xaa)
	{
		LOG(8,("INFO:BIOS signiture not found\n"));
	}
	LOG(2,("INFO:BIOS signiture $AA55 found OK\n"));
	pins = rom + (rom[location]|(rom[location+1]<<8));
	LOG(2,("INFO:Using PINS v%d.%d structure at: %x\n",pins[version+1],pins[version],pins-rom));
	//check valid 
	for (i=0;i<pins[length];i++)
	{
		chksum+=pins[i];
	}

	if((chksum%256)) //if pins is invalid make up some stuff
	{
		LOG(8,("INFO:PINS checksum is incorrect - using default values\n"));
		LOG(8,("INFO:AFAIK these should work with most G450s, but they are untested\n"));
		//FIXME
		si->ps.mem_ctl=0x24045491;
		si->ps.mem_type=1<<14; //SDRAM memory config 0
		si->ps.mem_rd=0x108;
		si->ps.sdram=true;
	}
	else
	{
		LOG(2,("INFO:PINS checksum is correct - grabbing PINS values\n"));

		/*read memory control word*/
		si->ps.mem_ctl=*((uint32 *)(pins + p5_memctl));
		LOG(2,("INFO:Memory control word: %x\n",si->ps.mem_ctl));

		/*read useful registers*/
		si->ps.option=*((uint32 *)(pins + p5_option));
		si->ps.option2=*((uint32 *)(pins + p5_option2));
		si->ps.option4=*((uint32 *)(pins + p5_option4));
		si->ps.maccess=*((uint32 *)(pins + p5_maccess));

		/*figure out memrdbk settings*/
		si->ps.mem_rd=*((uint32 *)(pins + p5_memrd));
	}

	if (si->settings.logmask & 0x80000000) dump_card_infos();
	return B_OK;
}

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
		LOG(8,("INFO: BIOS signiture not found\n"));
		return B_ERROR;
	}
	LOG(2,("INFO: BIOS signiture $AA55 found OK\n"));
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
//remove later on here:
//		float f_ref;				/* PLL reference-oscillator frequency */
//		uint32 max_system_vco;		/* graphics engine PLL VCO limits */
//		uint32 min_system_vco;
//		uint32 max_pixel_vco;		/* dac1 PLL VCO limits */
//		uint32 min_pixel_vco;
//		uint32 max_video_vco;		/* dac2, maven PLL VCO limits */
//		uint32 min_video_vco;
//		uint32 std_engine_clock;	/* graphics engine clock speed needed */
//		uint32 std_engine_clock_dh;
//		uint32 max_dac1_clock;		/* dac1 limits */
//		uint32 max_dac1_clock_8;	/* dac1 limits correlated to RAMspeed limits */
//		uint32 max_dac1_clock_16;
//		uint32 max_dac1_clock_24;
//		uint32 max_dac1_clock_32;
//		uint32 max_dac1_clock_32dh;
//		uint32 max_dac2_clock;		/* dac2 limits */
//		uint32 max_dac2_clock_8;	/* dac2, maven limits correlated to RAMspeed limits */
//		uint32 max_dac2_clock_16;
//		uint32 max_dac2_clock_24;
//		uint32 max_dac2_clock_32;
//		uint32 max_dac2_clock_32dh;
//		bool secondary_head;		/* presence of functions */
//		bool secondary_tvout;
//		bool primary_dvi;
//		bool secondary_dvi;
//		uint32 memory_size;			/* memory in Mb */
//		uint32 mctlwtst_reg;		/* memory control waitstate register */
//		uint32 option_reg;			/* option register */
//		uint8 v3_clk_div;			/* pins v3 memory and system clock division factors */
//		uint8 v3_mem_type;			/* pins v3 memory type info */
//		bool sdram;
//end remove later on here.

	//fixme: implement this..
	return B_ERROR;
}

status_t pins2_read(uint8 *pins, uint8 length)
{
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
	if (pins[52] & 0x01)
		si->ps.std_engine_clock *= 3;
	else
		si->ps.std_engine_clock *= 2;

	if (pins[52] & 0x20) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	/* G100 and G200 support 2-16Mb RAM */
	si->ps.memory_size = 2 << ((pins[55] & 0xc0) >> 6);
	/* more memory specifics */
	si->ps.mctlwtst_reg = (pins[51] << 24) | (pins[50] << 16) | (pins[49] << 8) | pins [48];
	si->ps.v3_clk_div = pins[52];
	si->ps.v3_mem_type = pins[54];

	/* for cards using this version of PINS both functions are in maven */
	si->ps.secondary_head = !(pins[59] & 0x01);
	si->ps.secondary_tvout = !(pins[59] & 0x01);

	/* setup via gathered info from pins with some fixed values added which are not in pins */
	si->ps.option_reg = 0;
	/* calculate refresh timer info-bits for 15uS interval (or shorter). See G100/G200 specs */
	/* calculate std memory clock period (nS) */
	if (pins[52] & 0x02)
		/* only used on G200, not on G100 */
		mclk_period = 3000.0 / si->ps.std_engine_clock;
	else
		mclk_period = 2000.0 / si->ps.std_engine_clock;
	/* calculate needed setting, 'round-down' result! */
	rfhcnt = (uint32)(((15000 / mclk_period) - 1) / 64);
	/* check for register limit */
	if (rfhcnt > 0x3f) rfhcnt = 0x3f;
	/* add to option register */
	si->ps.option_reg |= (rfhcnt << 15);
	/* the rest of the OPTION info currently comes via 'v3_clk_div' and 'v3_mem_type'. */

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
	return B_OK;
}

/* pins v4 is used by G400 */
status_t pins4_read(uint8 *pins, uint8 length)
{
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

	/* for cards using this version of PINS both functions are in maven */
	si->ps.secondary_head = !(pins[91] & 0x01);
	si->ps.secondary_tvout = !(pins[91] & 0x01);

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

//todo:
//		uint32 mctlwtst_reg;
//		uint32 option_reg;
//		bool sdram;

	/* not used here: */
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
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
	if (pins[4]) m_factor = 8;

	si->ps.max_system_vco = m_factor * pins[36];
	si->ps.max_video_vco = m_factor * pins[37];
	/* pixelVCO multiplier is 10 if pins V5.2 */
	if (pins[4] == 0x02) si->ps.max_pixel_vco = 10 * pins[38];
	else si->ps.max_pixel_vco = m_factor * pins[38];
	si->ps.min_system_vco = m_factor * pins[121];
	si->ps.min_video_vco = m_factor * pins[122];
	/* pixelVCO multiplier is 10 if pins V5.2 */
	if (pins[4] == 0x02) si->ps.min_pixel_vco = 10 * pins[123];
	else si->ps.min_pixel_vco = m_factor * pins[123];

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
		LOG(2,("INFO: unknown RAM size, defaulting to 8Mb\n"));
		si->ps.memory_size = 8;
	}

	if (pins[110] & 0x01) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	si->ps.secondary_head = (pins[117] & 0x70);
	si->ps.secondary_tvout = (pins[117] & 0x40);	
	si->ps.primary_dvi = (pins[117] & 0x02);
	si->ps.secondary_dvi = (pins[117] & 0x20);

	/* not supported: */
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_24 = 0;

//todo:
//		uint32 mctlwtst_reg;
//		uint32 option_reg;
//		bool sdram;

	/* not used here: */
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
	if (i2c_maven_probe() == B_OK)
	{
		si->ps.secondary_tvout = true;
		si->ps.secondary_head = true;
	}
	else
	{
		si->ps.secondary_tvout = false;
		si->ps.secondary_head = false;
	}

	/* not used because no coldstart will be attempted */
	si->ps.std_engine_clock = 0;
	si->ps.std_engine_clock_dh = 0;
	si->ps.mctlwtst_reg = 0;
	si->ps.option_reg = 0;
	si->ps.v3_clk_div = 0;
	si->ps.v3_mem_type = 0;
}

void pinsmil2_fake(void)
{
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
//todo:
//	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
//end todo.
}

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
//todo:
//	si->ps.sdram = !(CFGR(OPTION) & 0x00004000);
}

void pinsg450_fake(void)
{
	/* 'worst case' scenario defaults, overrule-able via mga.settings if needed */

	//fixme: should be overrule-able via mga.settings.
	si->ps.f_ref = 27.000;
	/* see G450 pins readouts for max ranges, then use a bit smaller ones */
	/* carefull not to take to high lower limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 600;
	si->ps.min_system_vco = 256;
	si->ps.max_pixel_vco = 640;
	si->ps.min_pixel_vco = 320;
	si->ps.max_video_vco = 600;
	si->ps.min_video_vco = 256;
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
	si->ps.max_video_vco = 600;
	si->ps.min_video_vco = 256;
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
	LOG(2,("secondary_tvout: "));
	if (si->ps.secondary_tvout) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("primary_dvi: "));
	if (si->ps.primary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("secondary_dvi: "));
	if (si->ps.secondary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("card memory_size: %dMb\n", si->ps.memory_size));
	LOG(2,("mctlwtst register: $%08x\n", si->ps.mctlwtst_reg));
	LOG(2,("option register: $%08x\n", si->ps.option_reg));
	LOG(2,("v3_clock_div: $%02x\n", si->ps.v3_clk_div));
	LOG(2,("v3_mem_type: $%02x\n", si->ps.v3_mem_type));
	LOG(2,("sdram: "));
	if (si->ps.sdram) LOG(2,("SDRAM card\n")); else LOG(2,("SGRAM card\n"));
	LOG(2,("INFO: end pinsdump.\n"));
}
