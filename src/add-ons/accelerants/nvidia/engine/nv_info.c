/* Read initialisation information from card */
/* some bits are hacks, where PINS is not known */
/* Author:
   Rudolf Cornelissen 7/2003-3/2004
*/

#define MODULE_BIT 0x00002000

#include "nv_std.h"

static void detect_panels(void);
static void pinsnv4_fake(void);
static void pinsnv5_nv5m64_fake(void);
static void pinsnv6_fake(void);
static void pinsnv10_arch_fake(void);
static void pinsnv20_arch_fake(void);
static void pinsnv30_arch_fake(void);
static void getstrap_arch_nv4(void);
static void getstrap_arch_nv10_20_30(void);
static status_t pins5_read(uint8 *pins, uint8 length);

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
			case 5:
				result = pins5_read(pins, pins_len);
				break;
			default:
				LOG(8,("INFO: unknown PINS version\n"));
				return B_ERROR;
				break;
		}
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

/* pins v5 is used by G450 and G550 */
static status_t pins5_read(uint8 *pins, uint8 length)
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
	si->ps.std_memory_clock = 4 * pins[92];

	si->ps.memory_size = ((pins[114] & 0x03) + 1) * 8;
	if ((pins[114] & 0x07) > 3)
	{
		LOG(8,("INFO: unknown RAM size, defaulting to 8Mb\n"));
		si->ps.memory_size = 8;
	}

	if (pins[110] & 0x01) si->ps.f_ref = 14.31818;
	else si->ps.f_ref = 27.00000;

	/* make sure SGRAM functions only get enabled if SGRAM mounted */
//	if ((pins[114] & 0x18) == 0x08) si->ps.sdram = false;
//	else si->ps.sdram = true;

	/* various registers */
	si->ps.secondary_head = (pins[117] & 0x70);
	si->ps.tvout = (pins[117] & 0x40);	
	si->ps.primary_dvi = (pins[117] & 0x02);
	si->ps.secondary_dvi = (pins[117] & 0x20);

	/* not supported: */
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_24 = 0;

	return B_OK;
}

/* fake_pins presumes the card was coldstarted by it's BIOS */
void fake_pins(void)
{
	LOG(8,("INFO: faking PINS\n"));

	/* set failsave speeds */
	switch (si->ps.card_type)
	{
	case NV04:
		pinsnv4_fake();
		break;
	case NV05:
	case NV05M64:
		pinsnv5_nv5m64_fake();
		break;
	case NV06:
		pinsnv6_fake();
		break;
	default:
		switch (si->ps.card_arch)
		{
		case NV10A:
			pinsnv10_arch_fake();
			break;
		case NV20A:
			pinsnv20_arch_fake();
			break;
		case NV30A:
			pinsnv30_arch_fake();
			break;
		default:
			/* 'failsafe' values... */
			pinsnv10_arch_fake();
			break;
		}
		break;
	}

	/* detect RAM amount, reference crystal frequency and dualhead */
	switch (si->ps.card_arch)
	{
	case NV04A:
		getstrap_arch_nv4();
		break;
	default:
		getstrap_arch_nv10_20_30();
		break;
	}

	/* find out if the card has a tvout chip */
	si->ps.tvout = false;
	si->ps.tvout_chip_type = NONE;
//fixme ;-)
/*	if (i2c_maven_probe() == B_OK)
	{
		si->ps.tvout = true;
		si->ps.tvout_chip_bus = ???;
		si->ps.tvout_chip_type = ???;
	}
*/

	/* find out the BIOS preprogrammed panel use status... */
	detect_panels();
}

static void detect_panels()
{
	/* detect if the BIOS enabled LCD's (internal panels or DVI) or TVout */
	{
		/* both external TMDS transmitters (used for LCD/DVI) and external TVencoders
		 * use the CRTC's in slaved mode. */
		/* Note:
		 * It looks like GeForceFX cards have on die TMDS encoders that nolonger
		 * require the CRTC to be slaved. */
		bool slaved_for_dev1 = false, slaved_for_dev2 = false;
		bool tvout1 = false, tvout2 = false;

		/* check primary head: */
		/* enable access to CRTC1 on dualhead cards */
		if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

		/* unlock CRTC1 */
		CRTCW(LOCK, 0x57);
		CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
		/* detect active slave device (if any) */
		slaved_for_dev1 = (CRTCR(PIXEL) & 0x80);
		if (slaved_for_dev1)
		{
			/* if the panel isn't selected, tvout is.. */
			tvout1 = !(CRTCR(LCD) & 0x01);
		}

		if (si->ps.secondary_head)
		{
			/* check secondary head: */
			/* enable access to CRTC2 */
			CRTCW(OWNER, 0x03);
			/* unlock CRTC2 */
			CRTCW(LOCK, 0x57);
			CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
			/* detect active slave device (if any) */
			slaved_for_dev2 = (CRTCR(PIXEL) & 0x80);
			if (slaved_for_dev2)
			{
				/* if the panel isn't selected, tvout is.. */
				tvout2 = !(CRTCR(LCD) & 0x01);
			}
		}

		/* do some presets */
		si->ps.panel1_width = 0;
		si->ps.panel1_height = 0;
		si->ps.panel2_width = 0;
		si->ps.panel2_height = 0;
		si->ps.slaved_tmds1 = false;
		si->ps.slaved_tmds2 = false;
		si->ps.master_tmds1 = false;
		si->ps.master_tmds2 = false;
		si->ps.tmds1_active = false;
		si->ps.tmds2_active = false;
		/* determine the situation we are in... (regarding flatpanels) */
		/* fixme: add VESA DDC EDID stuff one day... */
		/* fixme: find out how to program those transmitters one day instead of
		 * relying on the cards BIOS to do it. This adds TVout options where panels
		 * are used!
		 * Currently we'd loose the panel setup while not being able to restore it. */
		if (slaved_for_dev1 && !tvout1)
		{
			uint16 width = ((DACR(FP_HDISPEND) & 0x0000ffff) + 1);
			uint16 height = ((DACR(FP_VDISPEND) & 0x0000ffff) + 1);
			if ((width >= 640) && (height >= 480))
			{
				si->ps.slaved_tmds1 = true;
				si->ps.tmds1_active = true;
				si->ps.panel1_width = width;
				si->ps.panel1_height = height;
			}
		}
		if (si->ps.secondary_head && slaved_for_dev2 && !tvout2)
		{
			uint16 width = ((DAC2R(FP_HDISPEND) & 0x0000ffff) + 1);
			uint16 height = ((DAC2R(FP_VDISPEND) & 0x0000ffff) + 1);
			if ((width >= 640) && (height >= 480))
			{
				si->ps.slaved_tmds2 = true;
				si->ps.tmds2_active = true;
				si->ps.panel2_width = width;
				si->ps.panel2_height = height;
			}
		}
		if (!si->ps.slaved_tmds1 && !tvout1)
		{
			uint16 width = ((DACR(FP_HDISPEND) & 0x0000ffff) + 1);
			uint16 height = ((DACR(FP_VDISPEND) & 0x0000ffff) + 1);
			if ((width >= 640) && (height >= 480))
			{
				si->ps.master_tmds1 = true;
				si->ps.tmds1_active = true;
				si->ps.panel1_width = width;
				si->ps.panel1_height = height;
			}
		}
		if (si->ps.secondary_head && !si->ps.slaved_tmds2 && !tvout2)
		{
			uint16 width = ((DAC2R(FP_HDISPEND) & 0x0000ffff) + 1);
			uint16 height = ((DAC2R(FP_VDISPEND) & 0x0000ffff) + 1);
			if ((width >= 640) && (height >= 480))
			{
				si->ps.master_tmds2 = true;
				si->ps.tmds2_active = true;
				si->ps.panel2_width = width;
				si->ps.panel2_height = height;
			}
		}
	}

	/* dump some panel configuration registers... */
	LOG(2,("INFO: Dumping flatpanel registers:\n"));
	LOG(2,("DAC1: FP_HDISPEND: $%08x = (dec) %d\n", DACR(FP_HDISPEND),DACR(FP_HDISPEND)));
	LOG(2,("DAC1: FP_HTOTAL: $%08x = (dec) %d\n", DACR(FP_HTOTAL),DACR(FP_HTOTAL)));
	LOG(2,("DAC1: FP_HCRTC: $%08x = (dec) %d\n", DACR(FP_HCRTC),DACR(FP_HCRTC)));
	LOG(2,("DAC1: FP_HSYNC_S: $%08x = (dec) %d\n", DACR(FP_HSYNC_S),DACR(FP_HSYNC_S)));
	LOG(2,("DAC1: FP_HSYNC_E: $%08x = (dec) %d\n", DACR(FP_HSYNC_E),DACR(FP_HSYNC_E)));
	LOG(2,("DAC1: FP_HVALID_S: $%08x = (dec) %d\n", DACR(FP_HVALID_S),DACR(FP_HVALID_S)));
	LOG(2,("DAC1: FP_HVALID_E: $%08x = (dec) %d\n", DACR(FP_HVALID_E),DACR(FP_HVALID_E)));

	LOG(2,("DAC1: FP_VDISPEND: $%08x = (dec) %d\n", DACR(FP_VDISPEND),DACR(FP_VDISPEND)));
	LOG(2,("DAC1: FP_VTOTAL: $%08x = (dec) %d\n", DACR(FP_VTOTAL),DACR(FP_VTOTAL)));
	LOG(2,("DAC1: FP_VCRTC: $%08x = (dec) %d\n", DACR(FP_VCRTC),DACR(FP_VCRTC)));
	LOG(2,("DAC1: FP_VSYNC_S: $%08x = (dec) %d\n", DACR(FP_VSYNC_S),DACR(FP_VSYNC_S)));
	LOG(2,("DAC1: FP_VSYNC_E: $%08x = (dec) %d\n", DACR(FP_VSYNC_E),DACR(FP_VSYNC_E)));
	LOG(2,("DAC1: FP_VVALID_S: $%08x = (dec) %d\n", DACR(FP_VVALID_S),DACR(FP_VVALID_S)));
	LOG(2,("DAC1: FP_VVALID_E: $%08x = (dec) %d\n", DACR(FP_VVALID_E),DACR(FP_VVALID_E)));

	LOG(2,("DAC1: FP_CHKSUM: $%08x = (dec) %d\n", DACR(FP_CHKSUM),DACR(FP_CHKSUM)));
	LOG(2,("DAC1: FP_TST_CTRL: $%08x = (dec) %d\n", DACR(FP_TST_CTRL),DACR(FP_TST_CTRL)));
	LOG(2,("DAC1: FP_TG_CTRL: $%08x = (dec) %d\n", DACR(FP_TG_CTRL),DACR(FP_TG_CTRL)));
	LOG(2,("DAC1: FP_DEBUG0: $%08x = (dec) %d\n", DACR(FP_DEBUG0),DACR(FP_DEBUG0)));
	LOG(2,("DAC1: FP_DEBUG1: $%08x = (dec) %d\n", DACR(FP_DEBUG1),DACR(FP_DEBUG1)));
	LOG(2,("DAC1: FP_DEBUG2: $%08x = (dec) %d\n", DACR(FP_DEBUG2),DACR(FP_DEBUG2)));
	LOG(2,("DAC1: FP_DEBUG3: $%08x = (dec) %d\n", DACR(FP_DEBUG3),DACR(FP_DEBUG3)));

	if(si->ps.secondary_head)
	{
		LOG(2,("DAC2: FP_HDISPEND: $%08x = (dec) %d\n", DAC2R(FP_HDISPEND),DAC2R(FP_HDISPEND)));
		LOG(2,("DAC2: FP_HTOTAL: $%08x = (dec) %d\n", DAC2R(FP_HTOTAL),DAC2R(FP_HTOTAL)));
		LOG(2,("DAC2: FP_HCRTC: $%08x = (dec) %d\n", DAC2R(FP_HCRTC),DAC2R(FP_HCRTC)));
		LOG(2,("DAC2: FP_HSYNC_S: $%08x = (dec) %d\n", DAC2R(FP_HSYNC_S),DAC2R(FP_HSYNC_S)));
		LOG(2,("DAC2: FP_HSYNC_E: $%08x = (dec) %d\n", DAC2R(FP_HSYNC_E),DAC2R(FP_HSYNC_E)));
		LOG(2,("DAC2: FP_HVALID_S: $%08x = (dec) %d\n", DAC2R(FP_HVALID_S),DAC2R(FP_HVALID_S)));
		LOG(2,("DAC2: FP_HVALID_E: $%08x = (dec) %d\n", DAC2R(FP_HVALID_E),DAC2R(FP_HVALID_E)));

		LOG(2,("DAC2: FP_VDISPEND: $%08x = (dec) %d\n", DAC2R(FP_VDISPEND),DAC2R(FP_VDISPEND)));
		LOG(2,("DAC2: FP_VTOTAL: $%08x = (dec) %d\n", DAC2R(FP_VTOTAL),DAC2R(FP_VTOTAL)));
		LOG(2,("DAC2: FP_VCRTC: $%08x = (dec) %d\n", DAC2R(FP_VCRTC),DAC2R(FP_VCRTC)));
		LOG(2,("DAC2: FP_VSYNC_S: $%08x = (dec) %d\n", DAC2R(FP_VSYNC_S),DAC2R(FP_VSYNC_S)));
		LOG(2,("DAC2: FP_VSYNC_E: $%08x = (dec) %d\n", DAC2R(FP_VSYNC_E),DAC2R(FP_VSYNC_E)));
		LOG(2,("DAC2: FP_VVALID_S: $%08x = (dec) %d\n", DAC2R(FP_VVALID_S),DAC2R(FP_VVALID_S)));
		LOG(2,("DAC2: FP_VVALID_E: $%08x = (dec) %d\n", DAC2R(FP_VVALID_E),DAC2R(FP_VVALID_E)));

		LOG(2,("DAC2: FP_CHKSUM: $%08x = (dec) %d\n", DAC2R(FP_CHKSUM),DAC2R(FP_CHKSUM)));
		LOG(2,("DAC2: FP_TST_CTRL: $%08x = (dec) %d\n", DAC2R(FP_TST_CTRL),DAC2R(FP_TST_CTRL)));
		LOG(2,("DAC2: FP_TG_CTRL: $%08x = (dec) %d\n", DAC2R(FP_TG_CTRL),DAC2R(FP_TG_CTRL)));
		LOG(2,("DAC2: FP_DEBUG0: $%08x = (dec) %d\n", DAC2R(FP_DEBUG0),DAC2R(FP_DEBUG0)));
		LOG(2,("DAC2: FP_DEBUG1: $%08x = (dec) %d\n", DAC2R(FP_DEBUG1),DAC2R(FP_DEBUG1)));
		LOG(2,("DAC2: FP_DEBUG2: $%08x = (dec) %d\n", DAC2R(FP_DEBUG2),DAC2R(FP_DEBUG2)));
		LOG(2,("DAC2: FP_DEBUG3: $%08x = (dec) %d\n", DAC2R(FP_DEBUG3),DAC2R(FP_DEBUG3)));
	}
	LOG(2,("INFO: End flatpanel registers dump.\n"));
}

static void pinsnv4_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 256;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 256;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 0;
	si->ps.min_video_vco = 0;
	si->ps.max_dac1_clock = 250;
	si->ps.max_dac1_clock_8 = 250;
	si->ps.max_dac1_clock_16 = 250;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 220;
	si->ps.max_dac1_clock_32 = 180;
	si->ps.max_dac1_clock_32dh = 180;
	/* secondary head */
	si->ps.max_dac2_clock = 0;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 0;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 0;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 0;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 90;
	si->ps.std_memory_clock = 110;
}

static void pinsnv5_nv5m64_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 300;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 300;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 0;
	si->ps.min_video_vco = 0;
	si->ps.max_dac1_clock = 300;
	si->ps.max_dac1_clock_8 = 300;
	si->ps.max_dac1_clock_16 = 300;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 270;
	si->ps.max_dac1_clock_32 = 230;
	si->ps.max_dac1_clock_32dh = 230;
	/* secondary head */
	si->ps.max_dac2_clock = 0;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 0;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 0;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 0;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 125;
	si->ps.std_memory_clock = 150;
}

static void pinsnv6_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 300;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 300;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 0;
	si->ps.min_video_vco = 0;
	si->ps.max_dac1_clock = 300;
	si->ps.max_dac1_clock_8 = 300;
	si->ps.max_dac1_clock_16 = 300;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 270;
	si->ps.max_dac1_clock_32 = 230;
	si->ps.max_dac1_clock_32dh = 230;
	/* secondary head */
	si->ps.max_dac2_clock = 0;
	si->ps.max_dac2_clock_8 = 0;
	si->ps.max_dac2_clock_16 = 0;
	si->ps.max_dac2_clock_24 = 0;
	si->ps.max_dac2_clock_32 = 0;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 0;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 100;
	si->ps.std_memory_clock = 125;
}

static void pinsnv10_arch_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 350;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 350;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 350;
	si->ps.min_video_vco = 128;
	si->ps.max_dac1_clock = 350;
	si->ps.max_dac1_clock_8 = 350;
	si->ps.max_dac1_clock_16 = 350;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 320;
	si->ps.max_dac1_clock_32 = 280;
	si->ps.max_dac1_clock_32dh = 250;
	/* secondary head */
	//fixme? assuming...
	si->ps.max_dac2_clock = 200;
	si->ps.max_dac2_clock_8 = 200;
	si->ps.max_dac2_clock_16 = 200;
	si->ps.max_dac2_clock_24 = 200;
	si->ps.max_dac2_clock_32 = 200;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 180;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 120;
	si->ps.std_memory_clock = 150;
}

static void pinsnv20_arch_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 350;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 350;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 350;
	si->ps.min_video_vco = 128;
	si->ps.max_dac1_clock = 350;
	si->ps.max_dac1_clock_8 = 350;
	si->ps.max_dac1_clock_16 = 350;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 320;
	si->ps.max_dac1_clock_32 = 280;
	si->ps.max_dac1_clock_32dh = 250;
	/* secondary head */
	//fixme? assuming...
	si->ps.max_dac2_clock = 200;
	si->ps.max_dac2_clock_8 = 200;
	si->ps.max_dac2_clock_16 = 200;
	si->ps.max_dac2_clock_24 = 200;
	si->ps.max_dac2_clock_32 = 200;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 180;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 175;
	si->ps.std_memory_clock = 200;
}

static void pinsnv30_arch_fake(void)
{
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 350;
	si->ps.min_system_vco = 128;
	si->ps.max_pixel_vco = 350;
	si->ps.min_pixel_vco = 128;
	si->ps.max_video_vco = 350;
	si->ps.min_video_vco = 128;
	si->ps.max_dac1_clock = 350;
	si->ps.max_dac1_clock_8 = 350;
	si->ps.max_dac1_clock_16 = 350;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 320;
	si->ps.max_dac1_clock_32 = 280;
	si->ps.max_dac1_clock_32dh = 250;
	/* secondary head */
	//fixme? assuming...
	si->ps.max_dac2_clock = 200;
	si->ps.max_dac2_clock_8 = 200;
	si->ps.max_dac2_clock_16 = 200;
	si->ps.max_dac2_clock_24 = 200;
	si->ps.max_dac2_clock_32 = 200;
	/* 'failsave' values */
	si->ps.max_dac2_clock_32dh = 180;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 190;
	si->ps.std_memory_clock = 190;
}

static void getstrap_arch_nv4(void)
{
	uint32 strapinfo = NV_REG32(NV32_NV4STRAPINFO);

	if (strapinfo & 0x00000100)
	{
		/* Unified memory architecture used */
		si->ps.memory_size =
			((((strapinfo & 0x0000f000) >> 12) * 2) + 2);

		LOG(8,("INFO: NV4 architecture chip with UMA detected\n"));
	}
	else
	{
		/* private memory architecture used */
		switch (strapinfo & 0x00000003)
		{
		case 0:
			si->ps.memory_size = 32;
			break;
		case 1:
			si->ps.memory_size = 4;
			break;
		case 2:
			si->ps.memory_size = 8;
			break;
		case 3:
			si->ps.memory_size = 16;
			break;
		}
	}

	strapinfo = NV_REG32(NV32_NVSTRAPINFO2);

	/* determine PLL reference crystal frequency */
	if (strapinfo & 0x00000040)
		si->ps.f_ref = 14.31818;
	else
		si->ps.f_ref = 13.50000;

	/* these cards are always singlehead */
	si->ps.secondary_head = false;
}

static void getstrap_arch_nv10_20_30(void)
{
	uint32 dev_manID = CFGR(DEVID);
	uint32 strapinfo = NV_REG32(NV32_NV10STRAPINFO);

	switch (dev_manID)
	{
	case 0x01a010de: /* Nvidia GeForce2 Integrated GPU */
		//fixme: need kerneldriver function to readout other device PCI config space!?!
		//linux: int amt = pciReadLong(pciTag(0, 0, 1), 0x7C);
		si->ps.memory_size = (((CFGR(GF2IGPU) & 0x000007c0) >> 6) + 1);
		break;
	case 0x01f010de: /* Nvidia GeForce4 MX Integrated GPU */
		//fixme: need kerneldriver function to readout other device PCI config space!?!
		//linux: int amt = pciReadLong(pciTag(0, 0, 1), 0x84);
		si->ps.memory_size = (((CFGR(GF4MXIGPU) & 0x000007f0) >> 4) + 1);
		break;
	default:
		LOG(8,("INFO: (Memory detection) Strapinfo value is: $%08x\n", strapinfo));

		switch ((strapinfo & 0x1ff00000) >> 20)
		{
		case 2:
			si->ps.memory_size = 2;
			break;
		case 4:
			si->ps.memory_size = 4;
			break;
		case 8:
			si->ps.memory_size = 8;
			break;
		case 16:
			si->ps.memory_size = 16;
			break;
		case 32:
			si->ps.memory_size = 32;
			break;
		case 64:
			si->ps.memory_size = 64;
			break;
		case 128:
			si->ps.memory_size = 128;
			break;
		case 256:
			si->ps.memory_size = 256;
			break;
		default:
			si->ps.memory_size = 16;

			LOG(8,("INFO: NV10/20/30 architecture chip with unknown RAM amount detected;\n"));
			LOG(8,("INFO: Setting 16Mb\n"));
			break;
		}
	}

	strapinfo = NV_REG32(NV32_NVSTRAPINFO2);

	/* determine PLL reference crystal frequency: three types are used... */
	if (strapinfo & 0x00000040)
		si->ps.f_ref = 14.31818;
	else
		si->ps.f_ref = 13.50000;

	switch (dev_manID & 0xfff0ffff)
	{
	/* Nvidia cards: */
	case 0x017010de:
	case 0x018010de:
	case 0x01f010de:
	case 0x025010de:
	case 0x028010de:
	case 0x030010de:
	case 0x031010de:
	case 0x032010de:
	case 0x033010de:
	case 0x034010de:
	/* Varisys cards: */
	case 0x35001888:
		if (strapinfo & 0x00400000) si->ps.f_ref = 27.00000;
		break;
	default:
		break;
	}

	/* determine if we have a dualhead card */
	switch (dev_manID & 0xfff0ffff)
	{
	/* Nvidia cards: */
	case 0x011010de:
	case 0x017010de:
	case 0x018010de:
	case 0x01f010de:
	case 0x025010de:
	case 0x028010de:
	case 0x030010de:
	case 0x031010de:
	case 0x032010de:
	case 0x033010de:
	case 0x034010de:
	/* Varisys cards: */
	case 0x35001888:
		si->ps.secondary_head = true;
		break;
	default:
		si->ps.secondary_head = false;
		break;
	}
}

void dump_pins(void)
{
	char *msg = "";

	LOG(2,("INFO: pinsdump follows:\n"));
	LOG(2,("f_ref: %fMhz\n", si->ps.f_ref));
	LOG(2,("max_system_vco: %dMhz\n", si->ps.max_system_vco));
	LOG(2,("min_system_vco: %dMhz\n", si->ps.min_system_vco));
	LOG(2,("max_pixel_vco: %dMhz\n", si->ps.max_pixel_vco));
	LOG(2,("min_pixel_vco: %dMhz\n", si->ps.min_pixel_vco));
	LOG(2,("max_video_vco: %dMhz\n", si->ps.max_video_vco));
	LOG(2,("min_video_vco: %dMhz\n", si->ps.min_video_vco));
	LOG(2,("std_engine_clock: %dMhz\n", si->ps.std_engine_clock));
	LOG(2,("std_memory_clock: %dMhz\n", si->ps.std_memory_clock));
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
	/* setup TVout logmessage text */
	switch (si->ps.tvout_chip_type)
	{
	case NONE:
		msg = "No";
		break;
	case CH7003:
		msg = "Chrontel CH7003";
		break;
	case CH7004:
		msg = "Chrontel CH7004";
		break;
	case CH7005:
		msg = "Chrontel CH7005";
		break;
	case CH7006:
		msg = "Chrontel CH7006";
		break;
	case CH7007:
		msg = "Chrontel CH7007";
		break;
	case CH7008:
		msg = "Chrontel CH7008";
		break;
	case SAA7102:
		msg = "Philips SAA7102";
		break;
	case SAA7103:
		msg = "Philips SAA7103";
		break;
	case SAA7104:
		msg = "Philips SAA7104";
		break;
	case SAA7105:
		msg = "Philips SAA7105";
		break;
	case BT868:
		msg = "Brooktree/Conexant BT868";
		break;
	case BT869:
		msg = "Brooktree/Conexant BT869";
		break;
	case CX25870:
		msg = "Conexant CX25870";
		break;
	case CX25871:
		msg = "Conexant CX25871";
		break;
	case NVIDIA:
		msg = "Nvidia internal";
		break;
	default:
		msg = "Unknown";
		break;
	}
	LOG(2, ("%s TVout chip detected\n", msg));
//	LOG(2,("primary_dvi: "));
//	if (si->ps.primary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
//	LOG(2,("secondary_dvi: "));
//	if (si->ps.secondary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("card memory_size: %dMb\n", si->ps.memory_size));
	LOG(2,("laptop: "));
	if (si->ps.laptop) LOG(2,("yes\n")); else LOG(2,("no\n"));
	if (si->ps.tmds1_active)
	{
		LOG(2,("found DFP (digital flatpanel) on CRTC1; CRTC1 is "));
		if (si->ps.slaved_tmds1) LOG(2,("slaved\n")); else LOG(2,("master\n"));
		LOG(2,("panel width: %d, height: %d\n",
			si->ps.panel1_width, si->ps.panel1_height));
	}
	if (si->ps.tmds2_active)
	{
		LOG(2,("found DFP (digital flatpanel) on CRTC2; CRTC2 is "));
		if (si->ps.slaved_tmds2) LOG(2,("slaved\n")); else LOG(2,("master\n"));
		LOG(2,("panel width: %d, height: %d\n",
			si->ps.panel2_width, si->ps.panel2_height));
	}
	LOG(2,("INFO: end pinsdump.\n"));
}
