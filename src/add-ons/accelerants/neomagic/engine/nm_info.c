/* setup initialisation information for card */
/* Authors:
   Rudolf Cornelissen 4/2003-6/2004
*/

#define MODULE_BIT 0x00002000

#include "nm_std.h"

static void set_nm2070(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 65;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 65;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 65;
	si->ps.max_dac1_clock_8 = 65;
	si->ps.max_dac1_clock_16 = 65;
	/* 24bit color is not supported */
	si->ps.max_dac1_clock_24 = 0;
	si->ps.memory_size = 896;
	si->ps.curmem_size = 2048;
	si->ps.max_crtc_width = 1024;
	si->ps.max_crtc_height = 1000;
	si->ps.std_engine_clock = 0;
}

static void set_nm2090_nm2093(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 80;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 80;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 80;
	si->ps.max_dac1_clock_8 = 80;
	si->ps.max_dac1_clock_16 = 80;
	si->ps.max_dac1_clock_24 = 65;
	si->ps.memory_size = 1152;
	si->ps.curmem_size = 2048;
	si->ps.max_crtc_width = 1024;
	si->ps.max_crtc_height = 1000;
	si->ps.std_engine_clock = 0;
}

static void set_nm2097(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 80;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 80;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 80;
	si->ps.max_dac1_clock_8 = 80;
	si->ps.max_dac1_clock_16 = 80;
	si->ps.max_dac1_clock_24 = 65;
	si->ps.memory_size = 1152;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1024;
	si->ps.max_crtc_height = 1000;
	si->ps.std_engine_clock = 0;
}

static void set_nm2160(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 90;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 90;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 90;
	si->ps.max_dac1_clock_8 = 90;
	si->ps.max_dac1_clock_16 = 90;
	si->ps.max_dac1_clock_24 = 70;
	si->ps.memory_size = 2048;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1024;
	si->ps.max_crtc_height = 1000;
	si->ps.std_engine_clock = 0;
}

static void set_nm2200(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 110;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 110;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 110;
	si->ps.max_dac1_clock_8 = 110;
	si->ps.max_dac1_clock_16 = 110;
	si->ps.max_dac1_clock_24 = 90;
	si->ps.memory_size = 2560;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1280;
	si->ps.max_crtc_height = 1024;
	si->ps.std_engine_clock = 0;
}

static void set_nm2230(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 110;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 110;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 110;
	si->ps.max_dac1_clock_8 = 110;
	si->ps.max_dac1_clock_16 = 110;
	si->ps.max_dac1_clock_24 = 90;
	si->ps.memory_size = 3008;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1280;
	si->ps.max_crtc_height = 1024;
	si->ps.std_engine_clock = 0;
}

static void set_nm2360(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 110;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 110;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 110;
	si->ps.max_dac1_clock_8 = 110;
	si->ps.max_dac1_clock_16 = 110;
	si->ps.max_dac1_clock_24 = 90;
	si->ps.memory_size = 4096;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1280;
	si->ps.max_crtc_height = 1024;
	si->ps.std_engine_clock = 0;
}

static void set_nm2380(void)
{
	/* setup cardspecs */
	si->ps.f_ref = 14.31818;
	si->ps.max_system_vco = 110;
	si->ps.min_system_vco = 11;
	si->ps.max_pixel_vco = 110;
	si->ps.min_pixel_vco = 11;
	si->ps.max_dac1_clock = 110;
	si->ps.max_dac1_clock_8 = 110;
	si->ps.max_dac1_clock_16 = 110;
	si->ps.max_dac1_clock_24 = 90;
	si->ps.memory_size = 6144;
	si->ps.curmem_size = 1024;
	si->ps.max_crtc_width = 1280;
	si->ps.max_crtc_height = 1024;
	si->ps.std_engine_clock = 0;
}

void set_specs(void)
{
	uint8 size_outputs, type;

	LOG(8,("INFO: setting cardspecs\n"));

	switch (si->ps.card_type)
	{
		case NM2070:
			set_nm2070();
			break;
		case NM2090:
		case NM2093:
			set_nm2090_nm2093();
			break;
		case NM2097:
			set_nm2097();
			break;
		case NM2160:
			set_nm2160();
			break;
		case NM2200:
			set_nm2200();
			break;
		case NM2230:
			set_nm2230();
			break;
		case NM2360:
			set_nm2360();
			break;
		case NM2380:
			set_nm2380();
			break;
	}

	/* get output properties: */
    /* read panelsize and preselected outputs (via BIOS) */
    size_outputs = ISAGRPHR(PANELCTRL1);
    /* read the panel type */
    type = ISAGRPHR(PANELTYPE);

    /* setup panelspecs */
    switch ((size_outputs & 0x18) >> 3)
    {
    case 0x00 :
		si->ps.panel_width = 640;
		si->ps.panel_height = 480;
		break;
    case 0x01 :
		si->ps.panel_width = 800;
		si->ps.panel_height = 600;
		break;
    case 0x02 :
		si->ps.panel_width = 1024;
		si->ps.panel_height = 768;
		break;
    case 0x03 :
        /* fixme: 1280x1024 panel support still needs to be done */
		si->ps.panel_width = 1280;
		si->ps.panel_height = 1024;
	}
	/* make note of paneltype */
	si->ps.panel_type = (type & 0x12);
	/* make note of preselected outputs (via BIOS) */
	si->ps.outputs = (size_outputs & 0x03);
	/* check for illegal setting */
	if (si->ps.outputs == 0)
	{
		LOG(4, ("INFO: illegal outputmode detected, assuming internal mode!\n"));
		si->ps.outputs = 2;
	}
}

void dump_specs(void)
{
	LOG(2,("INFO: cardspecs and settings follow:\n"));
	LOG(2,("f_ref: %fMhz\n", si->ps.f_ref));
	LOG(2,("max_system_vco: %dMhz\n", si->ps.max_system_vco));
	LOG(2,("min_system_vco: %dMhz\n", si->ps.min_system_vco));
	LOG(2,("max_pixel_vco: %dMhz\n", si->ps.max_pixel_vco));
	LOG(2,("min_pixel_vco: %dMhz\n", si->ps.min_pixel_vco));
	LOG(2,("std_engine_clock: %dMhz\n", si->ps.std_engine_clock));
	LOG(2,("max_dac1_clock: %dMhz\n", si->ps.max_dac1_clock));
	LOG(2,("max_dac1_clock_8: %dMhz\n", si->ps.max_dac1_clock_8));
	LOG(2,("max_dac1_clock_16: %dMhz\n", si->ps.max_dac1_clock_16));
	LOG(2,("max_dac1_clock_24: %dMhz\n", si->ps.max_dac1_clock_24));
	LOG(2,("card memory_size: %dKbytes\n", si->ps.memory_size));
	LOG(2,("card curmem_size: %dbytes\n", si->ps.curmem_size));
	LOG(2,("card max_crtc_width: %d\n", si->ps.max_crtc_width));
	LOG(2,("card max_crtc_height: %d\n", si->ps.max_crtc_height));
	switch (si->ps.panel_type)
	{
	case 0x00:
		LOG(2, ("B/W dualscan LCD panel detected\n"));
		break;
	case 0x02:
		LOG(2, ("color dualscan LCD panel detected\n"));
		break;
	case 0x10:
		LOG(2, ("B/W TFT LCD panel detected\n"));
		break;
	case 0x12:
		LOG(2, ("color TFT LCD panel detected\n"));
		break;
	}
	LOG(2,("internal panel width: %d\n", si->ps.panel_width));
	LOG(2,("internal panel height: %d\n", si->ps.panel_height));
	switch (si->ps.outputs)
	{
	case 0x01:
		LOG(2, ("external CRT only mode preset\n"));
		break;
	case 0x02:
		LOG(2, ("internal LCD only mode preset\n"));
		break;
	case 0x03:
		LOG(2, ("simultaneous LCD/CRT mode preset\n"));
		break;
	}
	LOG(2,("INFO: end cardspecs and settings.\n"));
}
