/* Authors:
   Mark Watson 12/1999,
   Apsed,
   Rudolf Cornelissen 10/2002-12/2003
*/

#define MODULE_BIT 0x00008000

#include "nv_std.h"
//apsed #include "memory"

status_t test_ram();
static status_t nvxx_general_powerup (void);
static status_t nv_general_bios_to_powergraphics(void);

static void nv_dump_configuration_space (void)
{
#define DUMP_CFG(reg, type) if (si->ps.card_type >= type) do { \
	uint32 value = CFGR(reg); \
	MSG(("configuration_space 0x%02x %20s 0x%08x\n", \
		NVCFG_##reg, #reg, value)); \
} while (0)
	DUMP_CFG (DEVID,	0);
	DUMP_CFG (DEVCTRL,	0);
	DUMP_CFG (CLASS,	0);
	DUMP_CFG (HEADER,	0);
	DUMP_CFG (BASE1REGS,0);
	DUMP_CFG (BASE2FB,	0);
	DUMP_CFG (BASE3,	0);
	DUMP_CFG (BASE4,	0);
	DUMP_CFG (BASE5,	0);
	DUMP_CFG (BASE6,	0);
	DUMP_CFG (BASE7,	0);
	DUMP_CFG (SUBSYSID1,0);
	DUMP_CFG (ROMBASE,	0);
	DUMP_CFG (CFG_0,	0);
	DUMP_CFG (CFG_1,	0);
	DUMP_CFG (INTERRUPT,0);
	DUMP_CFG (SUBSYSID2,0);
	DUMP_CFG (AGPREF,	0);
	DUMP_CFG (AGPSTAT,	0);
	DUMP_CFG (AGPCMD,	0);
	DUMP_CFG (ROMSHADOW,0);
	DUMP_CFG (VGA,		0);
	DUMP_CFG (SCHRATCH,	0);
	DUMP_CFG (CFG_10,	0);
	DUMP_CFG (CFG_11,	0);
	DUMP_CFG (CFG_12,	0);
	DUMP_CFG (CFG_13,	0);
	DUMP_CFG (CFG_14,	0);
	DUMP_CFG (CFG_15,	0);
	DUMP_CFG (CFG_16,	0);
	DUMP_CFG (CFG_17,	0);
	DUMP_CFG (GF2IGPU,	0);
	DUMP_CFG (CFG_19,	0);
	DUMP_CFG (GF4MXIGPU,0);
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

status_t nv_general_powerup()
{
	status_t status;

	LOG(1,("POWERUP: nVidia (open)BeOS Accelerant 0.07 running.\n"));

	/* preset no laptop */
	si->ps.laptop = false;

	/* detect card type and power it up */
	switch(CFGR(DEVID))
	{
	/* Vendor Nvidia */
	case 0x002010de: /* Nvidia TNT1 */
		si->ps.card_type = NV04;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia TNT1 (NV04)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002810de: /* Nvidia TNT2 (pro) */
	case 0x002910de: /* Nvidia TNT2 Ultra */
	case 0x002a10de: /* Nvidia TNT2 */
	case 0x002b10de: /* Nvidia TNT2 */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia TNT2 (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002c10de: /* Nvidia Vanta (Lt) */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia Vanta (Lt) (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002d10de: /* Nvidia TNT2-M64 (Pro) */
		si->ps.card_type = NV05M64;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia TNT2-M64 (Pro) (NV05M64)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002e10de: /* Nvidia NV06 Vanta */
	case 0x002f10de: /* Nvidia NV06 Vanta */
		si->ps.card_type = NV06;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia Vanta (NV06)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x00a010de: /* Nvidia Aladdin TNT2 */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia Aladdin TNT2 (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x010010de: /* Nvidia GeForce256 SDR */
	case 0x010110de: /* Nvidia GeForce256 DDR */
	case 0x010210de: /* Nvidia GeForce256 Ultra */
		si->ps.card_type = NV10;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce256 (NV10)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x010310de: /* Nvidia Quadro */
		si->ps.card_type = NV10;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia Quadro (NV10)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x011010de: /* Nvidia GeForce2 MX/MX400 */
	case 0x011110de: /* Nvidia GeForce2 MX100/MX200 DDR */
		si->ps.card_type = NV11;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce2 MX (NV11)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x011210de: /* Nvidia GeForce2 Go */
		si->ps.card_type = NV11;
		si->ps.card_arch = NV10A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce2 Go (NV11)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x011310de: /* Nvidia Quadro2 MXR/EX/Go */
		si->ps.card_type = NV11;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia Quadro2 MXR/EX/Go (NV11)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x015010de: /* Nvidia GeForce2 GTS/Pro */
	case 0x015110de: /* Nvidia GeForce2 Ti DDR */
	case 0x015210de: /* Nvidia GeForce2 Ultra */
		si->ps.card_type = NV15;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce2 (NV15)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x015310de: /* Nvidia Quadro2 Pro */
		si->ps.card_type = NV15;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia Quadro2 Pro (NV15)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x017010de: /* Nvidia GeForce4 MX 460 */
	case 0x017110de: /* Nvidia GeForce4 MX 440 */
	case 0x017210de: /* Nvidia GeForce4 MX 420 */ 
	case 0x017310de: /* Nvidia GeForce4 MX 440SE */ 
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 MX (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x017410de: /* Nvidia GeForce4 440 Go */ 
	case 0x017510de: /* Nvidia GeForce4 420 Go */
	case 0x017610de: /* Nvidia GeForce4 420 Go 32M */
	case 0x017710de: /* Nvidia GeForce4 460 Go */
	case 0x017910de: /* Nvidia GeForce4 440 Go 64M */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 Go (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x017810de: /* Nvidia Quadro4 500 XGL/550 XGL */
	case 0x017a10de: /* Nvidia Quadro4 200 NVS/400 NVS */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia Quadro4 (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x017c10de: /* Nvidia Quadro4 500 GoGL */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia Quadro4 500 GoGL (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x017d10de: /* Nvidia unknown 4 Go */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown 4 Go (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x018010de: /* Nvidia GeForce4 MX 440 AGP8X */
	case 0x018110de: /* Nvidia GeForce4 MX 440SE AGP8X */
	case 0x018210de: /* Nvidia GeForce4 MX 420 AGP8X */
		si->ps.card_type = NV18;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 MX AGP8X (NV18)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x018610de: /* Nvidia GeForce4 448 Go */
	case 0x018710de: /* Nvidia GeForce4 488 Go */
		si->ps.card_type = NV18;
		si->ps.card_arch = NV10A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 Go (NV18)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x018810de: /* Nvidia Quadro4 580 XGL */
	case 0x018a10de: /* Nvidia Quadro4 280 NVS */
	case 0x018b10de: /* Nvidia Quadro4 380 XGL */
		si->ps.card_type = NV18;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia Quadro4 (NV18)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x01a010de: /* Nvidia GeForce2 Integrated GPU */
		si->ps.card_type = NV11;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce2 Integrated GPU (CRUSH, NV11)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x01f010de: /* Nvidia GeForce4 MX Integrated GPU */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 MX Integrated GPU (NFORCE2, NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x020010de: /* Nvidia GeForce3 */
	case 0x020110de: /* Nvidia GeForce3 Ti 200 */
	case 0x020210de: /* Nvidia GeForce3 Ti 500 */
		si->ps.card_type = NV20;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia GeForce3 (NV20)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x020310de: /* Nvidia Quadro DCC */
		si->ps.card_type = NV20;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia Quadro DCC (NV20)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x025010de: /* Nvidia GeForce4 Ti 4600 */
	case 0x025110de: /* Nvidia GeForce4 Ti 4400 */
	case 0x025210de: /* Nvidia GeForce4 Ti 4600 */
	case 0x025310de: /* Nvidia GeForce4 Ti 4200 */
		si->ps.card_type = NV25;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 Ti (NV25)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x025810de: /* Nvidia Quadro4 900 XGL */
	case 0x025910de: /* Nvidia Quadro4 750 XGL */
	case 0x025b10de: /* Nvidia Quadro4 700 XGL */
		si->ps.card_type = NV25;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia Quadro4 XGL (NV25)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x028010de: /* Nvidia GeForce4 Ti 4600 AGP8X */
	case 0x028110de: /* Nvidia GeForce4 Ti 4200 AGP8X */
		si->ps.card_type = NV28;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 Ti AGP8X (NV28)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x028210de: /* Nvidia GeForce4 Ti 4800SE */
		si->ps.card_type = NV28;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 Ti 4800SE (NV28)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x028610de: /* Nvidia GeForce4 4200 Go */
		si->ps.card_type = NV28;
		si->ps.card_arch = NV20A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce4 4200 Go (NV28)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x028810de: /* Nvidia Quadro4 980 XGL */
	case 0x028910de: /* Nvidia Quadro4 780 XGL */
		si->ps.card_type = NV28;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia Quadro4 XGL (NV28)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x028c10de: /* Nvidia unknown 4 Go */
		si->ps.card_type = NV28;
		si->ps.card_arch = NV20A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown 4 Go (NV28)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x02a010de: /* Nvidia GeForce3 Integrated GPU */
		si->ps.card_type = NV20;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Nvidia GeForce3 Integrated GPU (XBOX, NV20)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x030110de: /* Nvidia GeForce FX 5800 Ultra */
	case 0x030210de: /* Nvidia GeForce FX 5800 */
		si->ps.card_type = NV30;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5800 (NV30)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x030810de: /* Nvidia Quadro FX 2000 */
	case 0x030910de: /* Nvidia Quadro FX 1000 */
		si->ps.card_type = NV30;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia Quadro FX (NV30)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x031110de: /* Nvidia GeForce FX 5600 Ultra */
	case 0x031210de: /* Nvidia GeForce FX 5600 */
		si->ps.card_type = NV31;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5600 (NV31)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x031610de: /* Nvidia unknown FX Go */
	case 0x031710de: /* Nvidia unknown FX Go */
		si->ps.card_type = NV31;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown FX Go (NV31)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x031a10de: /* Nvidia GeForce FX 5600 Go */
		si->ps.card_type = NV31;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5600 Go (NV31)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x031b10de: /* Nvidia unknown FX Go */
	case 0x031c10de: /* Nvidia unknown FX Go */
	case 0x031d10de: /* Nvidia unknown FX Go */
	case 0x031e10de: /* Nvidia unknown FX Go */
	case 0x031f10de: /* Nvidia unknown FX Go */
		si->ps.card_type = NV31;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown FX Go (NV31)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x032110de: /* Nvidia GeForce FX 5200 Ultra */
	case 0x032210de: /* Nvidia GeForce FX 5200 */
		si->ps.card_type = NV34;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5200 (NV34)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x032410de: /* Nvidia GeForce FX 5200 Go */
		si->ps.card_type = NV34;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5200 Go (NV34)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x032610de: /* Nvidia unknown FX Go */
		si->ps.card_type = NV34;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown FX Go (NV34)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x032b10de: /* Nvidia Quadro FX 500 */
		si->ps.card_type = NV34;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia Quadro FX 500 (NV34)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x032e10de: /* Nvidia unknown FX Go */
		si->ps.card_type = NV34;
		si->ps.card_arch = NV30A;
		si->ps.laptop = true;
		LOG(4,("POWERUP: Detected Nvidia unknown FX Go (NV34)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x033010de: /* Nvidia GeForce FX 5900 Ultra */
	case 0x033110de: /* Nvidia GeForce FX 5900 */
		si->ps.card_type = NV35;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia GeForce FX 5900 (NV35)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x033810de: /* Nvidia Quadro FX 3000 */
		si->ps.card_type = NV35;
		si->ps.card_arch = NV30A;
		LOG(4,("POWERUP: Detected Nvidia Quadro FX 3000 (NV35)\n"));
		status = nvxx_general_powerup();
		break;
	/* Vendor Elsa GmbH */
	case 0x0c601048: /* Elsa Gladiac Geforce2 MX */
		si->ps.card_type = NV11;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Elsa Gladiac Geforce2 MX (NV11)\n"));
		status = nvxx_general_powerup();
		break;
	/* Vendor Nvidia STB/SGS-Thompson */
	case 0x002012d2: /* Nvidia STB/SGS-Thompson TNT1 */
		si->ps.card_type = NV04;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson TNT1 (NV04)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002812d2: /* Nvidia STB/SGS-Thompson TNT2 (pro) */
	case 0x002912d2: /* Nvidia STB/SGS-Thompson TNT2 Ultra */
	case 0x002a12d2: /* Nvidia STB/SGS-Thompson TNT2 */
	case 0x002b12d2: /* Nvidia STB/SGS-Thompson TNT2 */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson TNT2 (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002c12d2: /* Nvidia STB/SGS-Thompson Vanta (Lt) */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson Vanta (Lt) (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002d12d2: /* Nvidia STB/SGS-Thompson TNT2-M64 (Pro) */
		si->ps.card_type = NV05M64;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson TNT2-M64 (Pro) (NV05M64)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x002e12d2: /* Nvidia STB/SGS-Thompson NV06 Vanta */
	case 0x002f12d2: /* Nvidia STB/SGS-Thompson NV06 Vanta */
		si->ps.card_type = NV06;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson Vanta (NV06)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x00a012d2: /* Nvidia STB/SGS-Thompson Aladdin TNT2 */
		si->ps.card_type = NV05;
		si->ps.card_arch = NV04A;
		LOG(4,("POWERUP: Detected Nvidia STB/SGS-Thompson Aladdin TNT2 (NV05)\n"));
		status = nvxx_general_powerup();
		break;
	/* Vendor Varisys Limited */
	case 0x35031888: /* Varisys GeForce4 MX440 */
		si->ps.card_type = NV17;
		si->ps.card_arch = NV10A;
		LOG(4,("POWERUP: Detected Varisys GeForce4 MX440 (NV17)\n"));
		status = nvxx_general_powerup();
		break;
	case 0x35051888: /* Varisys GeForce4 Ti 4200 */
		si->ps.card_type = NV25;
		si->ps.card_arch = NV20A;
		LOG(4,("POWERUP: Detected Varisys GeForce4 Ti 4200 (NV25)\n"));
		status = nvxx_general_powerup();
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
 * (CAS latency is dependant on NV setup on some (DRAM) boards) */
status_t nv_set_cas_latency()
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
	case G550:
			if (!si->ps.sdram)
			{
				LOG(4,("INIT: G100 SGRAM CAS tuning not permitted, aborting.\n"));
				return B_OK;
			}
			/* SDRAM card */
			for (latency = 4; latency >= 2; latency-- )
			{
				/* MCTLWTST is a write-only register! */
//				ACCW(MCTLWTST, ((si->ps.mctlwtst_reg & 0xfffffffc) | (latency - 2)));
				result = test_ram();
				if (result == B_OK) break;
			}
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

static status_t nvxx_general_powerup()
{
	status_t result;

	LOG(4, ("INIT: NV powerup\n"));
	if (si->settings.logmask & 0x80000000) nv_dump_configuration_space();

	/* initialize the shared_info PINS struct */
	result = parse_pins();
	if (result != B_OK) fake_pins();

	/* log the PINS struct settings */
	dump_pins();

	/* if the user doesn't want a coldstart OR the BIOS pins info could not be found warmstart */
//temp:
return nv_general_bios_to_powergraphics();
	if (si->settings.usebios || (result != B_OK)) return nv_general_bios_to_powergraphics();

	/*power up the PLLs,LUT,DAC*/
	LOG(2,("INIT: PLL/LUT/DAC powerup\n"));
	/* turn off both displays and the hardcursor (also disables transfers) */
	nv_crtc_dpms(false, false, false);
	nv_crtc_cursor_hide();
	/* G200 SGRAM and SDRAM use external pix and dac refs, do *not* activate internals!
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
	snooze(250);
	/* re-enable pixelclock oscillations */
//	DXIW(PIXCLKCTRL, (DXIR(PIXCLKCTRL) & 0xfb));

	/* setup i2c bus */
	i2c_init();

	/*make sure card is in powergraphics mode*/
//	VGAW_I(CRTCEXT,3,0x80);      

	/*set the system clocks to powergraphics speed*/
	LOG(2,("INIT: Setting system PLL to powergraphics speeds\n"));
	g400_dac_set_sys_pll();

	/* 'official' RAM initialisation */
	LOG(2,("INIT: RAM init\n"));
	/* disable hardware plane write mask if SDRAM card */
//	if (si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) & 0xffffbfff));
	/* disable plane write mask (needed for SDRAM): actual change needed to get it sent to RAM */
//	ACCW(PLNWT,0x00000000);
//	ACCW(PLNWT,0xffffffff);
	/* program memory control waitstates */
//	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* set memory configuration including:
	 * - SDRAM / SGRAM special functions select. */
//	CFGW(OPTION,(CFGR(OPTION)&0xFFFF83FF) | ((si->ps.v3_mem_type & 0x07) << 10));
//	if (!si->ps.sdram) CFGW(OPTION,(CFGR(OPTION) | (0x01 << 14)));
	/* set memory buffer type */
//	CFGW(OPTION2,(CFGR(OPTION2)&0xFFFFCFFF)|((si->ps.v3_option2_reg & 0x03) << 12));
	/* set mode register opcode and streamer flow control */	
//	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0x0000FFFF)|(si->ps.memrdbk_reg & 0xffff0000));
	/* set RAM read tap delays */
//	ACCW(MEMRDBK,(ACCR(MEMRDBK)&0xFFFF0000)|(si->ps.memrdbk_reg & 0x0000ffff));
	/* wait 200uS minimum */
	snooze(250);

	/* reset memory (MACCESS is a write only register!) */
//	ACCW(MACCESS, 0x00000000);
	/* perform actual RAM reset */
//	ACCW(MACCESS, 0x00008000);
	snooze(250);
	/* start memory refresh */
//	CFGW(OPTION,(CFGR(OPTION)&0xffe07fff) | (si->ps.option_reg & 0x001f8000));
	/* set memory control waitstate again AFTER the RAM reset */
//	ACCW(MCTLWTST,si->ps.mctlwtst_reg);
	/* end 'official' RAM initialisation. */

	/* Bus parameters: enable retries, use advanced read */
//	CFGW(OPTION,(CFGR(OPTION)|(1<<22)|(0<<29)));

	/*enable writing to crtc registers*/
//	VGAW_I(CRTC,0x11,0);

	/* turn on display one */
	nv_crtc_dpms(true , true, true);

	return B_OK;
}

status_t gx50_general_output_select()
{
	/* make sure this call is warranted */
	if ((si->ps.card_type != NV11) && (si->ps.card_type != NV17)) return B_ERROR;

	/* choose primary analog outputconnector */
	if ((si->ps.primary_dvi) && (si->ps.secondary_head) && (si->ps.tvout))
	{
		if (i2c_sec_tv_adapter() == B_OK)
		{
			LOG(4,("INIT: secondary TV-adapter detected, using primary connector\n"));
//			DXIW(OUTPUTCONN,0x01); 
		}
		else
		{
			LOG(4,("INIT: no secondary TV-adapter detected, using secondary connector\n"));
//			DXIW(OUTPUTCONN,0x04); 
		}
	}
	else
	{
		LOG(4,("INIT: using primary connector\n"));
//		DXIW(OUTPUTCONN,0x01); 
	}
	return B_OK;
}

/*connect CRTC1 to the specified DAC*/
status_t nv_general_dac_select(int dac)
{
	if (!si->ps.secondary_head)
		return B_ERROR;

	/*MISCCTRL, clock src,...*/
	switch(dac)
	{
		/* G400 */
		case DS_CRTC1DAC_CRTC2MAVEN:
			/* connect CRTC1 to pixPLL */
//			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL, connect CRTC1 to internal DAC and
			 * enable CRTC2 external video timing reset signal.
			 * (Setting for MAVEN 'master mode' TVout signal generation.) */
//			CR2W(CTL,(CR2R(CTL)&0xffe00779)|0xD0000002);
			/* disable CRTC1 external video timing reset signal */
//			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)&0x77));
			/* select CRTC2 RGB24 MAFC mode: connects CRTC2 to MAVEN DAC */ 
//			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x82);
			break;
		case DS_CRTC1MAVEN_CRTC2DAC:
			/* connect CRTC1 to vidPLL */
//			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x2);
			/* connect CRTC2 to pixPLL and internal DAC and
			 * disable CRTC2 external video timing reset signal */
//			CR2W(CTL,(CR2R(CTL)&0x2fe00779)|0x4|(0x1<<20));
			/* enable CRTC1 external video timing reset signal.
			 * note: this is nolonger used as G450/G550 cannot do TVout on CRTC1 */
//			VGAW_I(CRTCEXT,1,(VGAR_I(CRTCEXT,1)|0x88));
			/* select CRTC1 RGB24 MAFC mode: connects CRTC1 to MAVEN DAC */
//			DXIW(MISCCTRL,(DXIR(MISCCTRL)&0x19)|0x02);
			break;
		/* G450/G550 */
		case DS_CRTC1CON1_CRTC2CON2:
			/* connect CRTC1 to pixPLL */
//			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL, connect CRTC1 to DAC1, disable CRTC2
			 * external video timing reset signal, set CRTC2 progressive scan mode
			 * and disable TVout mode (b12).
			 * (Setting for MAVEN 'slave mode' TVout signal generation.) */
			//fixme: enable timing resets if TVout is used in master mode!
			//otherwise keep it disabled.
//			CR2W(CTL,(CR2R(CTL)&0x2de00779)|0x6|(0x0<<20));
			/* connect DAC1 to CON1, CRTC2/'DAC2' to CON2 (monitor mode) */
//			DXIW(OUTPUTCONN,0x09); 
			/* Select 1.5 Volt MAVEN DAC ref. for monitor mode */
//			DXIW(GENIOCTRL, DXIR(GENIOCTRL) & ~0x40);
//			DXIW(GENIODATA, 0x00);
			break;
		//fixme: toggle PLL's below if possible: 
		//       otherwise toggle PLL's for G400 2nd case?
		case DS_CRTC1CON2_CRTC2CON1:
			/* connect CRTC1 to pixPLL */
//			DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0xc)|0x1);
			/* connect CRTC2 to vidPLL and DAC1, disable CRTC2 external
			 * video timing reset signal, and set CRTC2 progressive scan mode and
			 * disable TVout mode (b12). */
//			CR2W(CTL,(CR2R(CTL)&0x2de00779)|0x6|(0x1<<20));
			/* connect DAC1 to CON2 (monitor mode), CRTC2/'DAC2' to CON1 */
//			DXIW(OUTPUTCONN,0x05); 
			/* Select 1.5 Volt MAVEN DAC ref. for monitor mode */
//			DXIW(GENIOCTRL, DXIR(GENIOCTRL) & ~0x40);
//			DXIW(GENIODATA, 0x00);
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}

/* basic change of card state from VGA to enhanced mode:
 * Should work from VGA BIOS POST init state. */
static 
status_t nv_general_bios_to_powergraphics()
{
	LOG(2, ("INIT: Skipping card coldstart!\n"));

	/* unlock card registers for R/W access */
	CRTCW(LOCK, 0x57);
	CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));

	/* turn off both displays and the hardcursor (also disables transfers) */
	nv_crtc_dpms(false, false, false);
	nv_crtc_cursor_hide();

	/* let acc engine make power off/power on cycle to start 'fresh' */
	NV_REG32(NV32_PWRUPCTRL) = 0x13110011;
	snooze(1000);

	/* power-up all nvidia hardware function blocks */
	/* bit 28: OVERLAY ENGINE (BES),
	 * bit 25: CRTC2, (> NV04A)
	 * bit 24: CRTC1,
	 * bit 20: framebuffer,
	 * bit 16: PPMI,
	 * bit 12: PGRAPH,
	 * bit  8: PFIFO,
	 * bit  4: PMEDIA,
	 * bit  0: TVOUT. (> NV04A) */
	NV_REG32(NV32_PWRUPCTRL) = 0x13111111;

	if (si->ps.card_arch >= NV10A)
	{ 
		/* switch overlay engine to head 1 */
		//fixme: add other function blocks...
		NV_REG32(NV32_FUNCSEL) |= 0x00001000;
		NV_REG32(NV32_2FUNCSEL) &= ~0x00001000;
	}

	/* enable 'enhanced mode', enable Vsync & Hsync,
	 * set DAC palette to 8-bit width, disable large screen */
	CRTCW(REPAINT1, 0x04);

	/* turn on display */
	nv_crtc_dpms(true, true, true);

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
status_t nv_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row)
{
	/* Note:
	 * This routine assumes that the CRTC memory pitch granularity is 'smaller than',
	 * or 'equals' the acceleration engine memory pitch granularity! */

	uint32 video_pitch;
	uint32 acc_mask, crtc_mask;
	uint8 depth = 8;

	/* determine pixel multiple based on 2D/3D engine constraints */
	switch (si->ps.card_arch)
	{
	default:
		/* confirmed for:
		 * TNT1, TNT2, TNT2-M64, GeForce2 MX400, GeForce4 MX440, GeForceFX 5200 */
		switch (target->space)
		{
			case B_CMAP8: acc_mask = 0x0f; depth =  8; break;
			case B_RGB15: acc_mask = 0x07; depth = 16; break;
			case B_RGB16: acc_mask = 0x07; depth = 16; break;
			case B_RGB24: acc_mask = 0x0f; depth = 24; break;
			case B_RGB32: acc_mask = 0x03; depth = 32; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		/* NV31 (confirmed GeForceFX 5600) has NV20A granularity!
		 * So let it fall through... */
		if (si->ps.card_type != NV31) break;
	case NV20A:
		/* confirmed for:
		 * GeForce4 Ti4200 */
		switch (target->space)
		{
			case B_CMAP8: acc_mask = 0x3f; depth =  8; break;
			case B_RGB15: acc_mask = 0x1f; depth = 16; break;
			case B_RGB16: acc_mask = 0x1f; depth = 16; break;
			case B_RGB24: acc_mask = 0x3f; depth = 24; break;
			case B_RGB32: acc_mask = 0x0f; depth = 32; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
		break;
	}

	/* determine pixel multiple based on CRTC memory pitch constraints.
	 * (Note: Don't mix this up with CRTC timing contraints! Those are
	 *        multiples of 8 for horizontal, 1 for vertical timing.) */
	switch (si->ps.card_arch)
	{
	default:
		/* all NV cards */
		switch (target->space)
		{
			case B_CMAP8: crtc_mask = 0x07; break;
			case B_RGB15: crtc_mask = 0x03; break;
			case B_RGB16: crtc_mask = 0x03; break;
			case B_RGB24: crtc_mask = 0x07; break;
			case B_RGB32: crtc_mask = 0x01; break;
			default:
				LOG(8,("INIT: unknown color space: 0x%08x\n", target->space));
				return B_ERROR;
		}
//fixme for NV..
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
//end fixme.
		break;
	}

	/* check if we can setup this mode with acceleration:
	 * Max sizes need to adhere to both the acceleration engine _and_ the CRTC constraints! */
	si->acc_mode = true;
	/* check virtual_width */
	switch (si->ps.card_arch)
	{
	case NV04A:
		/* confirmed for:
		 * TNT1, TNT2, TNT2-M64 */
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > 8176) si->acc_mode = false;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > 4088) si->acc_mode = false;
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > 2720) si->acc_mode = false;
			break;
		case B_RGB32_LITTLE:
			if (target->virtual_width > 2044) si->acc_mode = false;
			break;
		}
		break;
	default:
		/* confirmed for:
		 * GeForce2 MX400, GeForce4 MX440, GeForceFX 5200 */
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > 16368) si->acc_mode = false;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > 8184) si->acc_mode = false;
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > 5456) si->acc_mode = false;
			break;
		case B_RGB32_LITTLE:
			if (target->virtual_width > 4092) si->acc_mode = false;
			break;
		}
		/* NV31 (confirmed GeForceFX 5600) has NV20A granularity!
		 * So let it fall through... */
		if (si->ps.card_type != NV31) break;
	case NV20A:
		/* confirmed for:
		 * GeForce4 Ti4200 */
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > 16320) si->acc_mode = false;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > 8160) si->acc_mode = false;
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > 5440) si->acc_mode = false;
			break;
		case B_RGB32_LITTLE:
			if (target->virtual_width > 4080) si->acc_mode = false;
			break;
		}
		break;
	}
	/* virtual_height */
	/* (NV cards can even do more than this(?)...
	 *  but 4096 is confirmed on all cards at max. accelerated width.) */
	if (target->virtual_height > 4096) si->acc_mode = false;

	/* now check NV virtual_size based on CRTC constraints */
	{
		/* virtual_width */
		//fixme for NV CRTC2?...:
		switch(target->space)
		{
		case B_CMAP8:
			if (target->virtual_width > 16376)
				target->virtual_width = 16376;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			if (target->virtual_width > 8188)
				target->virtual_width = 8188;
			break;
		case B_RGB24_LITTLE:
			if (target->virtual_width > 5456)
				target->virtual_width = 5456;
			break;
		case B_RGB32_LITTLE:
			if (target->virtual_width > 4094)
				target->virtual_width = 4094;
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
	if (si->acc_mode)
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
