/* Read initialisation information from card */
/* some bits are hacks, where PINS is not known */
/* Author:
   Rudolf Cornelissen 7/2003-6/2008
*/

#define MODULE_BIT 0x00002000

#include "nv_std.h"

/* pins V5.16 and up ROM infoblock stuff */
typedef struct {
	uint16	InitScriptTablePtr;		/* ptr to list of ptrs to scripts to exec */
	uint16	MacroIndexTablePtr;		/* ptr to list with indexes and sizes of items in MacroTable */
	uint16	MacroTablePtr;			/* ptr to list with items containing multiple 32bit reg writes */
	uint16	ConditionTablePtr;		/* ptr to list of PCI regs and bits to tst for exec mode */
	uint16	IOConditionTablePtr;	/* ptr to list of ISA regs and bits to tst for exec mode */
	uint16	IOFlagConditionTablePtr;/* ptr to list of ISA regs and bits to tst, ref'd to a matrix, for exec mode */
	uint16	InitFunctionTablePtr;	/* ptr to list of startadresses of fixed ROM init routines */
} PinsTables;

static void detect_panels(void);
static void setup_output_matrix(void);

static void pinsnv30_arch_fake(void);
static void getRAMsize_arch_nv10_20_30_40(void);
static void getstrap_arch_nv10_20_30_40(void);
static status_t pins2_read(uint8 *rom, uint32 offset);
static status_t pins3_5_read(uint8 *rom, uint32 offset);
static status_t coldstart_card(uint8* rom, uint16 init1, uint16 init2, uint16 init_size, uint16 ram_tab);
static status_t coldstart_card_516_up(uint8* rom, PinsTables tabs, uint16 ram_tab);
static status_t exec_type1_script(uint8* rom, uint16 adress, int16* size, uint16 ram_tab);
static status_t exec_type2_script(uint8* rom, uint16 adress, int16* size, PinsTables tabs, uint16 ram_tab);
static status_t exec_type2_script_mode(uint8* rom, uint16* adress, int16* size, PinsTables tabs, uint16 ram_tab, bool* exec);
static void	exec_cmd_39_type2(uint8* rom, uint32 data, PinsTables tabs, bool* exec);
static void log_pll(uint32 reg, uint32 freq);
static void	setup_ram_config(uint8* rom, uint16 ram_tab);
static void	setup_ram_config_nv10_up(uint8* rom);
static status_t translate_ISA_PCI(uint32* reg);
static status_t	nv_crtc_setup_fifo(void);

/* Parse the BIOS PINS structure if there */
status_t parse_pins ()
{
	uint8 *rom;
	uint8 chksum = 0;
	int i;
	uint32 offset;
	status_t result = B_ERROR;

	/* preset PINS read status to failed */
	si->ps.pins_status = B_ERROR;

	/* check the validity of PINS */
	LOG(2,("INFO: Reading PINS info\n"));
	rom = (uint8 *) si->rom_mirror;
	/* check BIOS signature - this is defined in the PCI standard */
	if (rom[0]!=0x55 || rom[1]!=0xaa)
	{
		LOG(8,("INFO: BIOS signature not found\n"));
		return B_ERROR;
	}
	LOG(2,("INFO: BIOS signature $AA55 found OK\n"));

	/* find the PINS struct adress */
	for (offset = 0; offset < 65536; offset++)
	{
		if (rom[offset    ] != 0xff) continue;
		if (rom[offset + 1] != 0x7f) continue;
		if (rom[offset + 2] != 0x4e) continue; /* N */
		if (rom[offset + 3] != 0x56) continue; /* V */
		if (rom[offset + 4] != 0x00) continue;

		LOG(8,("INFO: PINS signature found\n"));
		break;
	}

	if (offset > 65535)
	{
		LOG(8,("INFO: PINS signature not found\n"));
		return B_ERROR;
	}

	/* verify PINS checksum */
	for (i = 0; i < 8; i++)
	{
		chksum += rom[offset + i];
	}
	if (chksum)
	{
		LOG(8,("INFO: PINS checksum error\n"));
		return B_ERROR;
	}

	/* checkout PINS struct version */
	LOG(2,("INFO: PINS checksum is OK; PINS version is %d.%d\n",
		rom[offset + 5], rom[offset + 6]));

	/* update the si->ps struct as far as is possible and coldstart card */
	//fixme: NV40 and up(?) nolonger use this system...
	switch (rom[offset + 5])
	{
	case 2:
		result = pins2_read(rom, offset);
		break;
	case 3:
	case 4:
	case 5:
		result = pins3_5_read(rom, offset);
		break;
	default:
		LOG(8,("INFO: unknown PINS version\n"));
		return B_ERROR;
		break;
	}

	/* check PINS read result */
	if (result == B_ERROR)
	{
		LOG(8,("INFO: PINS read/decode/execute error\n"));
		return B_ERROR;
	}
	/* PINS scan succeeded */
	si->ps.pins_status = B_OK;
	LOG(2,("INFO: PINS scan completed succesfully\n"));
	return B_OK;
}

static status_t pins2_read(uint8 *rom, uint32 offset)
{
	uint16 init1 = rom[offset + 18] + (rom[offset + 19] * 256);
	uint16 init2 = rom[offset + 20] + (rom[offset + 21] * 256);
	uint16 init_size = rom[offset + 22] + (rom[offset + 23] * 256) + 1;
	/* confirmed by comparing cards */
	uint16 ram_tab = init1 - 0x0010;
	/* fixme: PPC BIOSes (might) return NULL pointers for messages here */
	char* signon_msg   = &(rom[(rom[offset + 24] + (rom[offset + 25] * 256))]);
	char* vendor_name  = &(rom[(rom[offset + 40] + (rom[offset + 41] * 256))]);
	char* product_name = &(rom[(rom[offset + 42] + (rom[offset + 43] * 256))]);
	char* product_rev  = &(rom[(rom[offset + 44] + (rom[offset + 45] * 256))]);

	LOG(8,("INFO: cmdlist 1: $%04x, 2: $%04x, max. size $%04x\n", init1, init2, init_size));
	LOG(8,("INFO: signon msg:\n%s\n", signon_msg));
	LOG(8,("INFO: vendor name: %s\n", vendor_name));
	LOG(8,("INFO: product name: %s\n", product_name));
	LOG(8,("INFO: product rev: %s\n", product_rev));

	return coldstart_card(rom, init1, init2, init_size, ram_tab);
}

static status_t pins3_5_read(uint8 *rom, uint32 offset)
{
	uint16 init1 = rom[offset + 18] + (rom[offset + 19] * 256);
	uint16 init2 = rom[offset + 20] + (rom[offset + 21] * 256);
	uint16 init_size = rom[offset + 22] + (rom[offset + 23] * 256) + 1;
	/* confirmed on a TNT2-M64 with pins V5.1 */
	uint16 ram_tab = rom[offset + 24] + (rom[offset + 25] * 256);
	/* fixme: PPC BIOSes (might) return NULL pointers for messages here */
	char* signon_msg   = &(rom[(rom[offset + 30] + (rom[offset + 31] * 256))]);
	char* vendor_name  = &(rom[(rom[offset + 46] + (rom[offset + 47] * 256))]);
	char* product_name = &(rom[(rom[offset + 48] + (rom[offset + 49] * 256))]);
	char* product_rev  = &(rom[(rom[offset + 50] + (rom[offset + 51] * 256))]);

	LOG(8,("INFO: pre PINS 5.16 cmdlist 1: $%04x, 2: $%04x, max. size $%04x\n", init1, init2, init_size));
	LOG(8,("INFO: signon msg:\n%s\n", signon_msg));
	LOG(8,("INFO: vendor name: %s\n", vendor_name));
	LOG(8,("INFO: product name: %s\n", product_name));
	LOG(8,("INFO: product rev: %s\n", product_rev));

	/* pins 5.06 and higher has VCO range info */
	if (((rom[offset + 5]) == 5) && ((rom[offset + 6]) >= 0x06))
	{
		/* get PLL VCO range info */
		uint32 fvco_max = *((uint32*)(&(rom[offset + 67])));
		uint32 fvco_min = *((uint32*)(&(rom[offset + 71])));

		LOG(8,("INFO: PLL VCO range is %dkHz - %dkHz\n", fvco_min, fvco_max));

		/* modify presets to reflect card capability */
		si->ps.min_system_vco = fvco_min / 1000;
		si->ps.max_system_vco = fvco_max / 1000;
		//fixme: enable and modify PLL code...
		//si->ps.min_pixel_vco = fvco_min / 1000;
		//si->ps.max_pixel_vco = fvco_max / 1000;
		//si->ps.min_video_vco = fvco_min / 1000;
		//si->ps.max_video_vco = fvco_max / 1000;
	}

	//fixme: add 'parsing scripts while not actually executing' as warmstart method,
	//       instead of not parsing at all: this will update the driver's speeds
	//       as below, while logging the scripts as well (for our learning pleasure :)

	/* pins 5.16 and higher is more extensive, and works differently from before */
	if (((rom[offset + 5]) == 5) && ((rom[offset + 6]) >= 0x10))
	{
		/* pins 5.16 and up have a more extensive command list table, and have more
		 * commands to choose from as well. */
		PinsTables tabs;
		tabs.InitScriptTablePtr = rom[offset + 75] + (rom[offset + 76] * 256);
		tabs.MacroIndexTablePtr = rom[offset + 77] + (rom[offset + 78] * 256);
		tabs.MacroTablePtr = rom[offset + 79] + (rom[offset + 80] * 256);
		tabs.ConditionTablePtr = rom[offset + 81] + (rom[offset + 82] * 256);
		tabs.IOConditionTablePtr = rom[offset + 83] + (rom[offset + 84] * 256);
		tabs.IOFlagConditionTablePtr = rom[offset + 85] + (rom[offset + 86] * 256);
		tabs.InitFunctionTablePtr = rom[offset + 87] + (rom[offset + 88] * 256);

		LOG(8,("INFO: PINS 5.16 and later cmdlist pointers:\n"));
		LOG(8,("INFO: InitScriptTablePtr: $%04x\n", tabs.InitScriptTablePtr));
		LOG(8,("INFO: MacroIndexTablePtr: $%04x\n", tabs.MacroIndexTablePtr));
		LOG(8,("INFO: MacroTablePtr: $%04x\n", tabs.MacroTablePtr));
		LOG(8,("INFO: ConditionTablePtr: $%04x\n", tabs.ConditionTablePtr));
		LOG(8,("INFO: IOConditionTablePtr: $%04x\n", tabs.IOConditionTablePtr));
		LOG(8,("INFO: IOFlagConditionTablePtr: $%04x\n", tabs.IOFlagConditionTablePtr));
		LOG(8,("INFO: InitFunctionTablePtr: $%04x\n", tabs.InitFunctionTablePtr));

		return coldstart_card_516_up(rom, tabs, ram_tab);
	}
	else
	{
		/* pre 'pins 5.16' still uses the 'old' method in which the command list
		 * table always has two entries. */
		return coldstart_card(rom, init1, init2, init_size, ram_tab);
	}
}

static status_t coldstart_card(uint8* rom, uint16 init1, uint16 init2, uint16 init_size, uint16 ram_tab)
{
	status_t result = B_OK;
	int16 size = init_size;

	LOG(8,("INFO: now executing coldstart...\n"));

	/* select colormode CRTC registers base adresses */
	NV_REG8(NV8_MISCW) = 0xcb;

	/* unknown.. */
	NV_REG8(NV8_VSE2) = 0x01;

	/* enable access to primary head */
	set_crtc_owner(0);
	/* unlock head's registers for R/W access */
	CRTCW(LOCK, 0x57);
	CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
	/* disable RMA as it's not used */
	/* (RMA is the cmd register for the 32bit port in the GPU to access 32bit registers
	 *  and framebuffer via legacy ISA I/O space.) */
	CRTCW(RMA, 0x00);

	if (si->ps.secondary_head)
	{
		/* enable access to secondary head */
		set_crtc_owner(1);
		/* unlock head's registers for R/W access */
		CRTC2W(LOCK, 0x57);
		CRTC2W(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
	}

	/* turn off both displays and the hardcursors (also disables transfers) */
	nv_crtc_dpms(false, false, false, true);
	nv_crtc_cursor_hide();
	if (si->ps.secondary_head)
	{
		nv_crtc2_dpms(false, false, false, true);
		nv_crtc2_cursor_hide();
	}

	/* execute BIOS coldstart script(s) */
	if (init1 || init2)
	{
		if (init1)
			if (exec_type1_script(rom, init1, &size, ram_tab) != B_OK) result = B_ERROR;
		if (init2 && (result == B_OK))
			if (exec_type1_script(rom, init2, &size, ram_tab) != B_OK) result = B_ERROR;

		/* now enable ROM shadow or the card will remain shut-off! */
		CFGW(ROMSHADOW, (CFGR(ROMSHADOW) |= 0x00000001));

		//temporary: should be called from setmode probably..
		nv_crtc_setup_fifo();
	}
	else
	{
		result = B_ERROR;
	}

	if (result != B_OK)
		LOG(8,("INFO: coldstart failed.\n"));
	else
		LOG(8,("INFO: coldstart execution completed OK.\n"));

	return result;
}

static status_t coldstart_card_516_up(uint8* rom, PinsTables tabs, uint16 ram_tab)
{
	status_t result = B_OK;
	uint16 adress;

	LOG(8,("INFO: now executing coldstart...\n"));

	/* select colormode CRTC registers base adresses */
	NV_REG8(NV8_MISCW) = 0xcb;

	/* unknown.. */
	NV_REG8(NV8_VSE2) = 0x01;

	/* enable access to primary head */
	set_crtc_owner(0);
	/* unlock head's registers for R/W access */
	CRTCW(LOCK, 0x57);
	CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
	/* disable RMA as it's not used */
	/* (RMA is the cmd register for the 32bit port in the GPU to access 32bit registers
	 *  and framebuffer via legacy ISA I/O space.) */
	CRTCW(RMA, 0x00);

	if (si->ps.secondary_head)
	{
		/* enable access to secondary head */
		set_crtc_owner(1);
		/* unlock head's registers for R/W access */
		CRTC2W(LOCK, 0x57);
		CRTC2W(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));
	}

	/* turn off both displays and the hardcursors (also disables transfers) */
	nv_crtc_dpms(false, false, false, true);
	nv_crtc_cursor_hide();
	if (si->ps.secondary_head)
	{
		nv_crtc2_dpms(false, false, false, true);
		nv_crtc2_cursor_hide();
	}

	/* execute all BIOS coldstart script(s) */
	if (tabs.InitScriptTablePtr)
	{
		/* size is nolonger used, keeping it anyway for testing purposes :) */
		int16 size = 32767;
		uint16 index = tabs.InitScriptTablePtr;
	
		adress = *((uint16*)(&(rom[index])));
		if (!adress)
		{
			LOG(8,("INFO: no cmdlist found!\n"));
			result = B_ERROR;
		}

		while (adress && (result == B_OK))
		{
			result = exec_type2_script(rom, adress, &size, tabs, ram_tab);
			/* next command script, please */
			index += 2;
			adress = *((uint16*)(&(rom[index])));
		}

		/* now enable ROM shadow or the card will remain shut-off! */
		CFGW(ROMSHADOW, (CFGR(ROMSHADOW) |= 0x00000001));

		//temporary: should be called from setmode probably..
		nv_crtc_setup_fifo();
	}
	else
	{
		result = B_ERROR;
	}

	if (result != B_OK)
		LOG(8,("INFO: coldstart failed.\n"));
	else
		LOG(8,("INFO: coldstart execution completed OK.\n"));

	return result;
}

/* This routine is complete, and is used for pre-NV10 cards. It's tested on a Elsa
 * Erazor III with TNT2 (NV05) and on two no-name TNT2-M64's. All cards coldstart
 * perfectly. */
static status_t exec_type1_script(uint8* rom, uint16 adress, int16* size, uint16 ram_tab)
{
	status_t result = B_OK;
	bool end = false;
	bool exec = true;
	uint8 index, byte;
	uint32 reg, data, data2, and_out, or_in;

	LOG(8,("\nINFO: executing type1 script at adress $%04x...\n", adress));
	LOG(8,("INFO: ---Executing following command(s):\n"));

	while (!end)
	{
		LOG(8,("INFO: $%04x ($%02x); ", adress, rom[adress]));

		switch (rom[adress])
		{
		case 0x59:
			*size -= 7;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint16*)(&(rom[adress])));
			adress += 2;
			data2 = *((uint16*)(&(rom[data])));
			LOG(8,("cmd 'calculate indirect and set PLL 32bit reg $%08x for %.3fMHz'\n",
				reg, ((float)data2)));
			if (exec)
			{
				float calced_clk;
				uint8 m, n, p;
				nv_dac_sys_pll_find(((float)data2), &calced_clk, &m, &n, &p, 0);
				NV_REG32(reg) = ((p << 16) | (n << 8) | m);
			}
			log_pll(reg, data2);
			break;
		case 0x5a:
			*size -= 7;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint16*)(&(rom[adress])));
			adress += 2;
			data2 = *((uint32*)(&(rom[data])));
			LOG(8,("cmd 'WR indirect 32bit reg' $%08x = $%08x\n", reg, data2));
			if (exec) NV_REG32(reg) = data2;
			break;
		case 0x63:
			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			LOG(8,("cmd 'setup RAM config' (always done)\n"));
			/* always done */
			setup_ram_config(rom, ram_tab);
			break;
		case 0x65:
			*size -= 13;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint32*)(&(rom[adress])));
			adress += 4;
			data2 = *((uint32*)(&(rom[adress])));
			adress += 4;
			LOG(8,("cmd 'WR 32bit reg $%08x = $%08x, then = $%08x' (always done)\n",
				reg, data, data2));
			/* always done */
			NV_REG32(reg) = data;
			NV_REG32(reg) = data2;
			CFGW(ROMSHADOW, (CFGR(ROMSHADOW) & 0xfffffffe));
			break;
		case 0x69:
			*size -= 5;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint16*)(&(rom[adress])));
			adress += 2;
			and_out = *((uint8*)(&(rom[adress])));
			adress += 1;
			or_in = *((uint8*)(&(rom[adress])));
			adress += 1;
			LOG(8,("cmd 'RD 8bit ISA reg $%04x, AND-out = $%02x, OR-in = $%02x, WR-bk'\n",
				reg, and_out, or_in));
			if (exec)
			{
				translate_ISA_PCI(&reg);
				byte = NV_REG8(reg);
				byte &= (uint8)and_out;
				byte |= (uint8)or_in;
				NV_REG8(reg) = byte;
			}
			break;
		case 0x6d:
			*size -= 3;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			data = NV_REG32(NV32_NV4STRAPINFO);
			and_out = *((uint8*)(&(rom[adress])));
			adress += 1;
			byte = *((uint8*)(&(rom[adress])));
			adress += 1;
			data &= (uint32)and_out;
			LOG(8,("cmd 'CHK bits AND-out $%02x RAMCFG for $%02x'\n",
				and_out, byte));
			if (((uint8)data) != byte)
			{
				LOG(8,("INFO: ---No match: not executing following command(s):\n"));
				exec = false;
			}
			else
			{
				LOG(8,("INFO: ---Match, so this cmd has no effect.\n"));
			}
			break;
		case 0x6e:
			*size -= 13;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			and_out = *((uint32*)(&(rom[adress])));
			adress += 4;
			or_in = *((uint32*)(&(rom[adress])));
			adress += 4;
			LOG(8,("cmd 'RD 32bit reg $%08x, AND-out = $%08x, OR-in = $%08x, WR-bk'\n",
				reg, and_out, or_in));
			if (exec)
			{
				data = NV_REG32(reg);
				data &= and_out;
				data |= or_in;
				NV_REG32(reg) = data;
			}
			break;
		case 0x71:
			LOG(8,("cmd 'END', execution completed.\n\n"));
			end = true;

			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}
			break;
		case 0x72:
			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}

			/* execute */
			adress += 1;
			LOG(8,("cmd 'PGM commands'\n"));
			LOG(8,("INFO: ---Executing following command(s):\n"));
			exec = true;
			break;
		case 0x73:
			*size -= 9;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			data = NV_REG32(NV32_NVSTRAPINFO2);
			and_out = *((uint32*)(&(rom[adress])));
			adress += 4;
			data2 = *((uint32*)(&(rom[adress])));
			adress += 4;
			data &= and_out;
			LOG(8,("cmd 'CHK bits AND-out $%08x STRAPCFG2 for $%08x'\n",
				and_out, data2));
			if (data != data2)
			{
				LOG(8,("INFO: ---No match: not executing following command(s):\n"));
				exec = false;
			}
			else
			{
				LOG(8,("INFO: ---Match, so this cmd has no effect.\n"));
			}
			break;
		case 0x74:
			*size -= 3;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			data = *((uint16*)(&(rom[adress])));
			adress += 2;
			LOG(8,("cmd 'SNOOZE for %d ($%04x) microSeconds' (always done)\n", data, data));
			/* always done */
			snooze(data);
			break;
		case 0x77:
			*size -= 7;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint16*)(&(rom[adress])));
			adress += 2;
			LOG(8,("cmd 'WR 32bit reg' $%08x = $%08x (b31-16 = '0', b15-0 = data)\n",
				reg, data));
			if (exec) NV_REG32(reg) = data;
			break;
		case 0x78:
			*size -= 6;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint16*)(&(rom[adress])));
			adress += 2;
			index = *((uint8*)(&(rom[adress])));
			adress += 1;
			and_out = *((uint8*)(&(rom[adress])));
			adress += 1;
			or_in = *((uint8*)(&(rom[adress])));
			adress += 1;
			LOG(8,("cmd 'RD idx ISA reg $%02x via $%04x, AND-out = $%02x, OR-in = $%02x, WR-bk'\n",
				index, reg, and_out, or_in));
			if (exec)
			{
				translate_ISA_PCI(&reg);
				NV_REG8(reg) = index;
				byte = NV_REG8(reg + 1);
				byte &= (uint8)and_out;
				byte |= (uint8)or_in;
				NV_REG8(reg + 1) = byte;
			}
			break;
		case 0x79:
			*size -= 7;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint16*)(&(rom[adress])));
			adress += 2;
			LOG(8,("cmd 'calculate and set PLL 32bit reg $%08x for %.3fMHz'\n", reg, (data / 100.0)));
			if (exec)
			{
				float calced_clk;
				uint8 m, n, p;
				nv_dac_sys_pll_find((data / 100.0), &calced_clk, &m, &n, &p, 0);
				NV_REG32(reg) = ((p << 16) | (n << 8) | m);
			}
			log_pll(reg, (data / 100));
			break;
		case 0x7a:
			*size -= 9;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			adress += 1;
			reg = *((uint32*)(&(rom[adress])));
			adress += 4;
			data = *((uint32*)(&(rom[adress])));
			adress += 4;
			LOG(8,("cmd 'WR 32bit reg' $%08x = $%08x\n", reg, data));
			if (exec) NV_REG32(reg) = data;
			break;
		default:
			LOG(8,("unknown cmd, aborting!\n\n"));
			end = true;
			result = B_ERROR;
			break;
		}
	}

	return result;
}

static void log_pll(uint32 reg, uint32 freq)
{
	switch (reg)
	{
	case NV32_MEMPLL:
		LOG(8,("INFO: ---Memory PLL accessed.\n"));
		/* update the card's specs */
		si->ps.std_memory_clock = freq;
		break;
	case NV32_COREPLL:
		LOG(8,("INFO: ---Core PLL accessed.\n"));
		/* update the card's specs */
		si->ps.std_engine_clock = freq;
		break;
	case NVDAC_PIXPLLC:
		LOG(8,("INFO: ---DAC1 PLL accessed.\n"));
		break;
	case NVDAC2_PIXPLLC:
		LOG(8,("INFO: ---DAC2 PLL accessed.\n"));
		break;
	/* unexpected cases, here for learning goals... */
	case NV32_MEMPLL2:
		LOG(8,("INFO: ---NV31/NV36 extension to memory PLL accessed only!\n"));
		break;
	case NV32_COREPLL2:
		LOG(8,("INFO: ---NV31/NV36 extension to core PLL accessed only!\n"));
		break;
	case NVDAC_PIXPLLC2:
		LOG(8,("INFO: ---NV31/NV36 extension to DAC1 PLL accessed only!\n"));
		break;
	case NVDAC2_PIXPLLC2:
		LOG(8,("INFO: ---NV31/NV36 extension to DAC2 PLL accessed only!\n"));
		break;
	default:
		LOG(8,("INFO: ---Unknown PLL accessed!\n"));
		break;
	}
}

static void	setup_ram_config(uint8* rom, uint16 ram_tab)
{
	uint32 ram_cfg, data;
	uint8 cnt;

	/* set MRS = 256 */
	NV_REG32(NV32_PFB_DEBUG_0) &= 0xffffffef;
	/* read RAM config hardware(?) strap */
	ram_cfg = ((NV_REG32(NV32_NVSTRAPINFO2) >> 2) & 0x0000000f);
	LOG(8,("INFO: ---RAM config strap is $%01x\n", ram_cfg));
	/* use it as a pointer in a BIOS table for prerecorded RAM configurations */
	ram_cfg = *((uint16*)(&(rom[(ram_tab + (ram_cfg * 2))])));
	/* log info */
	switch (ram_cfg & 0x00000003)
	{
	case 0:
		LOG(8,("INFO: ---32Mb RAM should be connected\n"));
		break;
	case 1:
		LOG(8,("INFO: ---4Mb RAM should be connected\n"));
		break;
	case 2:
		LOG(8,("INFO: ---8Mb RAM should be connected\n"));
		break;
	case 3:
		LOG(8,("INFO: ---16Mb RAM should be connected\n"));
		break;
	}
	if (ram_cfg & 0x00000004)
		LOG(8,("INFO: ---RAM should be 128bits wide\n"));
	else
		LOG(8,("INFO: ---RAM should be 64bits wide\n"));
	switch ((ram_cfg & 0x00000038) >> 3)
	{
	case 0:
		LOG(8,("INFO: ---RAM type: 8Mbit SGRAM\n"));
		break;
	case 1:
		LOG(8,("INFO: ---RAM type: 16Mbit SGRAM\n"));
		break;
	case 2:
		LOG(8,("INFO: ---RAM type: 4 banks of 16Mbit SGRAM\n"));
		break;
	case 3:
		LOG(8,("INFO: ---RAM type: 16Mbit SDRAM\n"));
		break;
	case 4:
		LOG(8,("INFO: ---RAM type: 64Mbit SDRAM\n"));
		break;
	case 5:
		LOG(8,("INFO: ---RAM type: 64Mbit x16 SDRAM\n"));
		break;
	}
	/* set RAM amount, width and type */
	data = (NV_REG32(NV32_NV4STRAPINFO) & 0xffffffc0);
	NV_REG32(NV32_NV4STRAPINFO) = (data | (ram_cfg & 0x0000003f));
	/* setup write to read delay (?) */
	data = (NV_REG32(NV32_PFB_CONFIG_1) & 0xff8ffffe);
	data |= ((ram_cfg & 0x00000700) << 12);
	/* force update via b0 = 0... */
	NV_REG32(NV32_PFB_CONFIG_1) = data;
	/* ... followed by b0 = 1(?) */
	NV_REG32(NV32_PFB_CONFIG_1) = (data | 0x00000001);

	/* do RAM width test to confirm RAM width set to be correct */
	/* write testpattern to first 128 bits of graphics memory... */
	data = 0x4e563541;
	for (cnt = 0; cnt < 4; cnt++)
		((volatile uint32 *)si->framebuffer)[cnt] = data;
	/* ... if second 64 bits does not contain the testpattern we are apparantly
	 * set to 128bits width while we should be set to 64bits width, so correct. */
	if (((volatile uint32 *)si->framebuffer)[3] != data)
	{
		LOG(8,("INFO: ---RAM width tested: width is 64bits, correcting settings.\n"));
		NV_REG32(NV32_NV4STRAPINFO) &= ~0x00000004;
	}
	else
	{
		LOG(8,("INFO: ---RAM width tested: access is OK.\n"));
	}

	/* do RAM size test to confirm RAM size set to be correct */
	ram_cfg = (NV_REG32(NV32_NV4STRAPINFO) & 0x00000003);
	data = 0x4e563542;
	/* first check for 32Mb... */
	if (!ram_cfg)
	{
		/* write testpattern to just above the 16Mb boundary */
		((volatile uint32 *)si->framebuffer)[(16 * 1024 * 1024) >> 2] = data;
		/* check if pattern reads back */
		if (((volatile uint32 *)si->framebuffer)[(16 * 1024 * 1024) >> 2] == data)
		{
			/* write second testpattern to base adress */
			data = 0x4135564e;
			((volatile uint32 *)si->framebuffer)[0] = data;
			if (((volatile uint32 *)si->framebuffer)[0] == data)
			{
				LOG(8,("INFO: ---RAM size tested: size was set OK (32Mb).\n"));
				return;
			}
		}
		/* one of the two tests for 32Mb failed, we must have 16Mb */
		ram_cfg = 0x00000003;
		LOG(8,("INFO: ---RAM size tested: size is 16Mb, correcting settings.\n"));
		NV_REG32(NV32_NV4STRAPINFO) =
			 (((NV_REG32(NV32_NV4STRAPINFO)) & 0xfffffffc) | ram_cfg);
		return;
	}
	/* ... now check for 16Mb... */
	if (ram_cfg == 0x00000003)
	{
		/* increment testpattern */
		data++;
		/* write testpattern to just above the 8Mb boundary */
		((volatile uint32 *)si->framebuffer)[(8 * 1024 * 1024) >> 2] = data;
		/* check if pattern reads back */
		if (((volatile uint32 *)si->framebuffer)[(8 * 1024 * 1024) >> 2] == data)
		{
			LOG(8,("INFO: ---RAM size tested: size was set OK (16Mb).\n"));
			return;
		}
		else
		{
			/* assuming 8Mb: retesting below! */
			ram_cfg = 0x00000002;
			LOG(8,("INFO: ---RAM size tested: size is NOT 16Mb, testing for 8Mb...\n"));
			NV_REG32(NV32_NV4STRAPINFO) =
				 (((NV_REG32(NV32_NV4STRAPINFO)) & 0xfffffffc) | ram_cfg);
		}
	}
	/* ... and now check for 8Mb! (ram_cfg will be 'pre'set to 4Mb or 8Mb here) */
	{
		/* increment testpattern (again) */
		data++;
		/* write testpattern to just above the 4Mb boundary */
		((volatile uint32 *)si->framebuffer)[(4 * 1024 * 1024) >> 2] = data;
		/* check if pattern reads back */
		if (((volatile uint32 *)si->framebuffer)[(4 * 1024 * 1024) >> 2] == data)
		{
			/* we have 8Mb, make sure this is set. */
			ram_cfg = 0x00000002;
			LOG(8,("INFO: ---RAM size tested: size is 8Mb, setting 8Mb.\n"));
			/* fixme? assuming this should be done here! */
			NV_REG32(NV32_NV4STRAPINFO) =
				 (((NV_REG32(NV32_NV4STRAPINFO)) & 0xfffffffc) | ram_cfg);
			return;
		}
		else
		{
			/* we must have 4Mb, make sure this is set. */
			ram_cfg = 0x00000001;
			LOG(8,("INFO: ---RAM size tested: size is 4Mb, setting 4Mb.\n"));
			NV_REG32(NV32_NV4STRAPINFO) =
				 (((NV_REG32(NV32_NV4STRAPINFO)) & 0xfffffffc) | ram_cfg);
			return;
		}
	}
}

/* this routine is used for NV10 and later */
static status_t exec_type2_script(uint8* rom, uint16 adress, int16* size, PinsTables tabs, uint16 ram_tab)
{
	bool exec = true;

	LOG(8,("\nINFO: executing type2 script at adress $%04x...\n", adress));
	LOG(8,("INFO: ---Executing following command(s):\n"));

	return exec_type2_script_mode(rom, &adress, size, tabs, ram_tab, &exec);
}

/* this routine is used for NV10 and later. It's tested on a GeForce2 MX400 (NV11),
 * GeForce4 MX440 (NV18), GeForce4 Ti4200 (NV28) and a GeForceFX 5200 (NV34).
 * These cards coldstart perfectly. */
static status_t exec_type2_script_mode(uint8* rom, uint16* adress, int16* size, PinsTables tabs, uint16 ram_tab, bool* exec)
{
	status_t result = B_OK;
	bool end = false;
	uint8 index, byte, byte2, shift;
	uint32 reg, reg2, data, data2, and_out, and_out2, or_in, or_in2, safe32, offset32, size32;

	while (!end)
	{
		LOG(8,("INFO: $%04x ($%02x); ", *adress, rom[*adress]));

		/* all commands are here (verified NV11 and NV28) */
		switch (rom[*adress])
		{
		case 0x31: /* new */
			*size -= (15 + ((*((uint8*)(&(rom[(*adress + 10)])))) << 2));
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			and_out = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			shift = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			size32 = ((*((uint8*)(&(rom[*adress])))) << 2);
			*adress += 1;
			reg2 = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'RD 32bit reg $%08x, AND-out = $%08x, shift-right = $%02x,\n",
				reg, and_out, shift));
			LOG(8,("INFO: (cont.) RD 32bit data from subtable with size $%04x, at offset (result << 2),\n",
				size32));
			LOG(8,("INFO: (cont.) then WR result data to 32bit reg $%08x'\n", reg2));
			if (*exec && reg2)
			{
				data = NV_REG32(reg);
				data &= and_out;
				data >>= shift;
				data2 = *((uint32*)(&(rom[(*adress + (data << 2))])));
				NV_REG32(reg2) = data2;
			}
			*adress += size32;
			break;
		case 0x32: /* new */
			*size -= (11 + ((*((uint8*)(&(rom[(*adress + 6)])))) << 2));
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			index = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			and_out = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			byte2 = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			size32 = ((*((uint8*)(&(rom[*adress])))) << 2);
			*adress += 1;
			reg2 = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'RD idx ISA reg $%02x via $%04x, AND-out = $%02x, shift-right = $%02x,\n",
				index, reg, and_out, byte2));
			LOG(8,("INFO: (cont.) RD 32bit data from subtable with size $%04x, at offset (result << 2),\n",
				size32));
			LOG(8,("INFO: (cont.) then WR result data to 32bit reg $%08x'\n", reg2));
			if (*exec && reg2)
			{
				translate_ISA_PCI(&reg);
				NV_REG8(reg) = index;
				byte = NV_REG8(reg + 1);
				byte &= (uint8)and_out;
				byte >>= byte2;
				offset32 = (byte << 2);
				data = *((uint32*)(&(rom[(*adress + offset32)])));
				NV_REG32(reg2) = data;
			}
			*adress += size32;
			break;
		case 0x33: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			size32 = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			/* executed 1-256 times */
			if (!size32) size32 = 256;
			/* remember where to start each time */
			safe32 = *adress;
			LOG(8,("cmd 'execute following part of this script $%03x times' (always done)\n", size32));
			for (offset32 = 0; offset32 < size32; offset32++)
			{
				LOG(8,("\nINFO: (#$%02x) executing part of type2 script at adress $%04x...\n",
					offset32, *adress));
				LOG(8,("INFO: ---Not touching 'execution' mode at this time:\n"));
				*adress = safe32;
				result = exec_type2_script_mode(rom, adress, size, tabs, ram_tab, exec);
			}
			LOG(8,("INFO: ---Continuing script:\n"));
			break;
		case 0x34: /* new */
			*size -= (12 + ((*((uint8*)(&(rom[(*adress + 7)])))) << 1));
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			index = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			and_out = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			shift = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			offset32 = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			size32 = ((*((uint8*)(&(rom[*adress])))) << 1);
			*adress += 1;
			reg2 = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'RD idx ISA reg $%02x via $%04x, AND-out = $%02x, shift-right = $%02x,\n",
				index, reg, and_out, shift));
			LOG(8,("INFO: (cont.) RD 16bit PLL frequency to pgm from subtable with size $%04x, at offset (result << 1),\n",
				size32));
			LOG(8,("INFO: (cont.) RD table-index ($%02x) for cmd $39'\n",
				offset32));
			translate_ISA_PCI(&reg);
			NV_REG8(reg) = index;
			byte = NV_REG8(reg + 1);
			byte &= (uint8)and_out;
			data = (byte >> shift);
			data <<= 1;
			data2 = *((uint16*)(&(rom[(*adress + data)])));
			if (offset32 < 0x80)
			{
				bool double_f = true;
				LOG(8,("INFO: Do subcmd ($39); "));
				exec_cmd_39_type2(rom, offset32, tabs, &double_f);
				LOG(8,("INFO: (cont. cmd $34) Doubling PLL frequency to be set for cmd $34.\n"));
				if (double_f) data2 <<= 1;
				LOG(8,("INFO: ---Reverting to pre-subcmd ($39) 'execution' mode.\n"));
			}
			else
			{
				LOG(8,("INFO: table index is negative, not executing subcmd ($39).\n"));
			}
			LOG(8,("INFO: (cont.) 'calc and set PLL 32bit reg $%08x for %.3fMHz'\n",
				reg2, (data2 / 100.0)));
			if (*exec && reg2)
			{
				float calced_clk;
				uint8 m, n, p;
				nv_dac_sys_pll_find((data2 / 100.0), &calced_clk, &m, &n, &p, 0);
				/* programming the PLL needs to be done in steps! (confirmed NV28) */
				data = NV_REG32(reg2);
				NV_REG32(reg2) = ((data & 0xffff0000) | (n << 8) | m);
				data = NV_REG32(reg2);
				NV_REG32(reg2) = ((p << 16) | (n << 8) | m);
//fixme?
				/* program 2nd set N and M scalers if they exist (b31=1 enables them) */
//				if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36))
//					DACW(PIXPLLC2, 0x80000401);
			}
			log_pll(reg2, (data2 / 100));
			*adress += size32;
			break;
		case 0x35: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			byte = *((uint8*)(&(rom[*adress])));
 			*adress += 1;
			offset32 = (byte << 1);
			offset32 += tabs.InitFunctionTablePtr;
			LOG(8,("cmd 'execute fixed VGA BIOS routine #$%02x at adress $%04x'\n",
				byte, offset32));
			/* note:
			 * This command is BIOS/'pins' version specific. Confirmed a NV28 having NO
			 * entries at all in InitFunctionTable!
			 * (BIOS version 4.28.20.05.11; 'pins' version 5.21) */
			//fixme: impl. if it turns out this cmd is used.. (didn't see that yet)
			if (*exec)
			{
				//fixme: add BIOS/'pins' version dependancy...
				switch(byte)
				{
				default:
					LOG(8,("\n\nINFO: WARNING: function not implemented, skipping!\n\n"));
					break;
				}
			}
			break;
		case 0x37: /* new */
			*size -= 11;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			byte2 = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			and_out = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			reg2 = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			index = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			and_out2 = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			LOG(8,("cmd 'RD 32bit reg $%08x, shift-right = $%02x, AND-out lsb = $%02x,\n",
				reg, byte2, and_out));
			LOG(8,("INFO: (cont.) RD 8bit ISA reg $%02x via $%04x, AND-out = $%02x, OR-in lsb result 32bit, WR-bk'\n",
				index, reg2, and_out2));
			if (*exec)
			{
				data = NV_REG32(reg);
				if (byte2 < 0x80)
				{
					data >>= byte2;
				}
				else
				{
					data <<= (0x0100 - byte2);
				}
				data &= and_out;
				translate_ISA_PCI(&reg2);
				NV_REG8(reg2) = index;
				byte = NV_REG8(reg2 + 1);
				byte &= (uint8)and_out2;
				byte |= (uint8)data;
				NV_REG8(reg2 + 1) = byte;
			}
			break;
		case 0x38: /* new */
			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			LOG(8,("cmd 'invert current mode'\n"));
			*exec = !(*exec);
			if (*exec)
				LOG(8,("INFO: ---Executing following command(s):\n"));
			else
				LOG(8,("INFO: ---Not executing following command(s):\n"));
			break;
		case 0x39: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			data = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			exec_cmd_39_type2(rom, data, tabs, exec);
			break;
		case 0x49: /* new */
			size32 = *((uint8*)(&(rom[*adress + 17])));
			if (!size32) size32 = 256;
			*size -= (18 + (size32 << 1));
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			reg2 = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			and_out = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			or_in = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			size32 = *((uint8*)(&(rom[*adress])));
			if (!size32) size32 = 256;
			*adress += 1;
			LOG(8,("cmd 'do following cmd structure $%03x time(s)':\n", size32));
			for (offset32 = 0; offset32 < size32; offset32++)
			{
				or_in2 = *((uint8*)(&(rom[(*adress + (offset32 << 1))])));
				data2 = *((uint8*)(&(rom[(*adress + (offset32 << 1) + 1)])));
				LOG(8,("INFO (cont.) (#$%02x) cmd 'WR 32bit reg $%08x = $%08x, RD 32bit reg $%08x,\n",
					offset32, reg2, data2, reg));
				LOG(8,("INFO (cont.) AND-out $%08x, OR-in $%08x, OR-in $%08x, WR-bk'\n",
					and_out, or_in, or_in2));
			}
			if (*exec)
			{
				for (index = 0; index < size32; index++)
				{
					or_in2 = *((uint8*)(&(rom[*adress])));
					*adress += 1;
					data2 = *((uint8*)(&(rom[*adress])));
					*adress += 1;
					NV_REG32(reg2) = data2;
					data = NV_REG32(reg);
					data &= and_out;
					data |= or_in;
					data |= or_in2;
					NV_REG32(reg) = data;
				}
			}
			else
			{
				*adress += (size32 << 1);
			}
			break;
		case 0x61: /* new */
			*size -= 4;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			byte = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			LOG(8,("cmd 'WR ISA reg $%04x = $%02x'\n", reg, byte));
			if (*exec)
			{
				translate_ISA_PCI(&reg);
				NV_REG8(reg) = byte;
			}
			break;
		case 0x62: /* new */
			*size -= 5;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			index = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			byte = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			LOG(8,("cmd 'WR idx ISA reg $%02x via $%04x = $%02x'\n", index, reg, byte));
			if (*exec)
			{
				translate_ISA_PCI(&reg);
				NV_REG16(reg) = ((((uint16)byte) << 8) | index);
			}
			break;
		case 0x63: /* new setup compared to pre-NV10 version */
			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			LOG(8,("cmd 'setup RAM config' (always done)\n"));
			/* always done */
			setup_ram_config_nv10_up(rom);
			break;
		case 0x65: /* identical to type1 */
			*size -= 13;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			data = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			data2 = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'WR 32bit reg $%08x = $%08x, then = $%08x' (always done)\n",
				reg, data, data2));
			/* always done */
			NV_REG32(reg) = data;
			NV_REG32(reg) = data2;
			CFGW(ROMSHADOW, (CFGR(ROMSHADOW) & 0xfffffffe));
			break;
		case 0x69: /* identical to type1 */
			*size -= 5;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			and_out = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			or_in = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			LOG(8,("cmd 'RD 8bit ISA reg $%04x, AND-out = $%02x, OR-in = $%02x, WR-bk'\n",
				reg, and_out, or_in));
			if (*exec)
			{
				translate_ISA_PCI(&reg);
				byte = NV_REG8(reg);
				byte &= (uint8)and_out;
				byte |= (uint8)or_in;
				NV_REG8(reg) = byte;
			}
			break;
		case 0x6a: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			data = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			data2 = *((uint16*)(&(rom[(tabs.InitScriptTablePtr + (data << 1))])));
			LOG(8,("cmd 'jump to script #$%02x at adress $%04x'\n", data, data2));
			if (*exec)
			{
				*adress = data2;
				LOG(8,("INFO: ---Jumping; not touching 'execution' mode.\n"));
			}
			break;
		case 0x6b: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			data = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			data2 = *((uint16*)(&(rom[(tabs.InitScriptTablePtr + (data << 1))])));
			LOG(8,("cmd 'gosub script #$%02x at adress $%04x'\n", data, data2));
			if (*exec && data2)
			{
				result = exec_type2_script(rom, data2, size, tabs, ram_tab);
				LOG(8,("INFO: ---Reverting to pre-gosub 'execution' mode.\n"));
			}
			break;
		case 0x6e: /* identical to type1 */
			*size -= 13;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			and_out = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			or_in = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'RD 32bit reg $%08x, AND-out = $%08x, OR-in = $%08x, WR-bk'\n",
				reg, and_out, or_in));
			if (*exec)
			{
				data = NV_REG32(reg);
				data &= and_out;
				data |= or_in;
				NV_REG32(reg) = data;
			}
			break;
		case 0x6f: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			byte = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			data = tabs.MacroIndexTablePtr + (byte << 1);
			offset32 = (*((uint8*)(&(rom[data]))) << 3);
			size32 = *((uint8*)(&(rom[(data + 1)])));
			offset32 += tabs.MacroTablePtr;
			/* note: min 1, max 255 commands can be requested */
			LOG(8,("cmd 'do $%02x time(s) a 32bit reg WR with 32bit data' (MacroIndexTable idx = $%02x):\n",
				size32, byte));
			safe32 = 0;
			while (safe32 < size32)
			{
				reg2 = *((uint32*)(&(rom[(offset32 + (safe32 << 3))])));
				data2 = *((uint32*)(&(rom[(offset32 + (safe32 << 3) + 4)])));
				LOG(8,("INFO: (cont.) (#$%02x) cmd 'WR 32bit reg' $%08x = $%08x\n",
					safe32, reg2, data2));
				safe32++;
 			}
			if (*exec)
			{
				safe32 = 0;
				while (safe32 < size32)
				{
					reg2 = *((uint32*)(&(rom[(offset32 + (safe32 << 3))])));
					data2 = *((uint32*)(&(rom[(offset32 + (safe32 << 3) + 4)])));
					NV_REG32(reg2) = data2;
					safe32++;
				}
			}
			break;
		case 0x36: /* new */
		case 0x66: /* new */
		case 0x67: /* new */
		case 0x68: /* new */
		case 0x6c: /* new */
		case 0x71: /* identical to type1 */
			LOG(8,("cmd 'END', execution completed.\n\n"));
			end = true;

			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}

			/* execute */
			*adress += 1; /* needed to make cmd #$33 work correctly! */
			break;
		case 0x72: /* identical to type1 */
			*size -= 1;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}

			/* execute */
			*adress += 1;
			LOG(8,("cmd 'PGM commands'\n"));
			LOG(8,("INFO: ---Executing following command(s):\n"));
			*exec = true;
			break;
		case 0x74: /* identical to type1 */
			//fixme? on at least NV28 this cmd hammers the CRTC PCI-timeout register
			//'data' number of times instead of snoozing.
			//Couldn't see any diff in behaviour though!
			*size -= 3;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			data = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			LOG(8,("cmd 'SNOOZE for %d ($%04x) microSeconds' (always done)\n", data, data));
			/* always done */
			snooze(data);
			break;
		case 0x75: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}

			/* execute */
			*adress += 1;
			data = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			data *= 12;
			data += tabs.ConditionTablePtr;
			reg = *((uint32*)(&(rom[data])));
			and_out = *((uint32*)(&(rom[(data + 4)])));
			data2 = *((uint32*)(&(rom[(data + 8)])));
			data = NV_REG32(reg);
			data &= and_out;
			LOG(8,("cmd 'CHK bits AND-out $%08x reg $%08x for $%08x'\n",
				and_out, reg, data2));
			if (data != data2)
			{
				LOG(8,("INFO: ---No match: not executing following command(s):\n"));
				*exec = false;
			}
			else
			{
				LOG(8,("INFO: ---Match, so this cmd has no effect.\n"));
			}
			break;
		case 0x76: /* new */
			*size -= 2;
			if (*size < 0)
			{
				LOG(8,("script size error!\n\n"));
				result = B_ERROR;
			}

			/* execute */
			*adress += 1;
			data = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			data *= 5;
			data += tabs.IOConditionTablePtr;
			reg = *((uint16*)(&(rom[data])));
			index = *((uint8*)(&(rom[(data + 2)])));
			and_out = *((uint8*)(&(rom[(data + 3)])));
			byte2 = *((uint8*)(&(rom[(data + 4)])));
			LOG(8,("cmd 'CHK bits AND-out $%02x idx ISA reg $%02x via $%04x for $%02x'\n",
				and_out, index, reg, byte2));
			translate_ISA_PCI(&reg);
			NV_REG8(reg) = index;
			byte = NV_REG8(reg + 1);
			byte &= (uint8)and_out;
			if (byte != byte2)
			{
				LOG(8,("INFO: ---No match: not executing following command(s):\n"));
				*exec = false;
			}
			else
			{
				LOG(8,("INFO: ---Match, so this cmd has no effect.\n"));
			}
			break;
		case 0x78: /* identical to type1 */
			*size -= 6;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			index = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			and_out = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			or_in = *((uint8*)(&(rom[*adress])));
			*adress += 1;
			LOG(8,("cmd 'RD idx ISA reg $%02x via $%04x, AND-out = $%02x, OR-in = $%02x, WR-bk'\n",
				index, reg, and_out, or_in));
			if (*exec)
			{
				translate_ISA_PCI(&reg);
				NV_REG8(reg) = index;
				byte = NV_REG8(reg + 1);
				byte &= (uint8)and_out;
				byte |= (uint8)or_in;
				NV_REG8(reg + 1) = byte;
			}
			break;
		case 0x79:
			*size -= 7;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			data = *((uint16*)(&(rom[*adress])));
			*adress += 2;
			LOG(8,("cmd 'calculate and set PLL 32bit reg $%08x for %.3fMHz'\n", reg, (data / 100.0)));
			if (*exec)
			{
				float calced_clk;
				uint8 m, n, p;
				nv_dac_sys_pll_find((data / 100.0), &calced_clk, &m, &n, &p, 0);
				/* programming the PLL needs to be done in steps! (confirmed NV28) */
				data2 = NV_REG32(reg);
				NV_REG32(reg) = ((data2 & 0xffff0000) | (n << 8) | m);
				data2 = NV_REG32(reg);
				NV_REG32(reg) = ((p << 16) | (n << 8) | m);
//fixme?
				/* program 2nd set N and M scalers if they exist (b31=1 enables them) */
//				if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36))
//					DACW(PIXPLLC2, 0x80000401);
			}
			log_pll(reg, (data / 100));
			break;
		case 0x7a: /* identical to type1 */
			*size -= 9;
			if (*size < 0)
			{
				LOG(8,("script size error, aborting!\n\n"));
				end = true;
				result = B_ERROR;
				break;
			}

			/* execute */
			*adress += 1;
			reg = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			data = *((uint32*)(&(rom[*adress])));
			*adress += 4;
			LOG(8,("cmd 'WR 32bit reg' $%08x = $%08x\n", reg, data));
			if (*exec) NV_REG32(reg) = data;
			break;
		default:
			LOG(8,("unknown cmd, aborting!\n\n"));
			end = true;
			result = B_ERROR;
			break;
		}
	}

	return result;
}

static void	exec_cmd_39_type2(uint8* rom, uint32 data, PinsTables tabs, bool* exec)
{
	uint8 index, byte, byte2, safe, shift;
	uint32 reg, and_out, and_out2, offset32;

	data *= 9;
	data += tabs.IOFlagConditionTablePtr;
	reg = *((uint16*)(&(rom[data])));
	index = *((uint8*)(&(rom[(data + 2)])));
	and_out = *((uint8*)(&(rom[(data + 3)])));
	shift = *((uint8*)(&(rom[(data + 4)])));
	offset32 = *((uint16*)(&(rom[data + 5])));
	and_out2 = *((uint8*)(&(rom[(data + 7)])));
	byte2 = *((uint8*)(&(rom[(data + 8)])));
	LOG(8,("cmd 'AND-out bits $%02x idx ISA reg $%02x via $%04x, shift-right = $%02x,\n",
		and_out, index, reg, shift));
	translate_ISA_PCI(&reg);
	NV_REG8(reg) = index;
	byte = NV_REG8(reg + 1);
	byte &= (uint8)and_out;
	offset32 += (byte >> shift);
	safe = byte = *((uint8*)(&(rom[offset32])));
	byte &= (uint8)and_out2;
	LOG(8,("INFO: (cont.) use result as index in table to get data $%02x,\n",
		safe));
	LOG(8,("INFO: (cont.) then chk bits AND-out $%02x of data for $%02x'\n",
		and_out2, byte2));
	if (byte != byte2)
	{
		LOG(8,("INFO: ---No match: not executing following command(s):\n"));
		*exec = false;
	}
	else
	{
		LOG(8,("INFO: ---Match, so this cmd has no effect.\n"));
	}
}

static void	setup_ram_config_nv10_up(uint8* rom)
{
	/* note:
	 * After writing data to RAM a snooze is required to make the test work.
	 * Confirmed a NV11: without snooze it worked OK on a low-voltage AGP2.0 slot,
	 * but on a higher-voltage AGP 1.0 slot it failed to identify errors correctly!!
	 * Adding the snooze fixed that. */

	uint32 data, dummy;
	uint8 cnt = 0;
	status_t stat = B_ERROR;

	/* set 'refctrl is valid' */
	NV_REG32(NV32_PFB_REFCTRL) = 0x80000000;

	/* check RAM for 256bits buswidth(?) */
	while ((cnt < 4) && (stat != B_OK))
	{
		/* reset RAM bits at offset 224-255 bits four times */
		((volatile uint32 *)si->framebuffer)[0x07] = 0x00000000;
		snooze(10);
		((volatile uint32 *)si->framebuffer)[0x07] = 0x00000000;
		snooze(10);
		((volatile uint32 *)si->framebuffer)[0x07] = 0x00000000;
		snooze(10);
		((volatile uint32 *)si->framebuffer)[0x07] = 0x00000000;
		snooze(10);
		/* write testpattern */
		((volatile uint32 *)si->framebuffer)[0x07] = 0x4e563131;
		snooze(10);
		/* reset RAM bits at offset 480-511 bits */
		((volatile uint32 *)si->framebuffer)[0x0f] = 0x00000000;
		snooze(10);
		/* check testpattern to have survived */
		if (((volatile uint32 *)si->framebuffer)[0x07] == 0x4e563131) stat = B_OK;
		cnt++;
	}

	/* if pattern did not hold modify RAM-type setup */
	if (stat != B_OK)
	{
		LOG(8,("INFO: ---RAM test #1 done: access errors, modified setup.\n"));
		data = NV_REG32(NV32_PFB_CONFIG_0);
		if (data & 0x00000010)
		{
			data &= 0xffffffcf;
		}
		else
		{
			data &= 0xffffffcf;
			data |= 0x00000020;
		}
		NV_REG32(NV32_PFB_CONFIG_0) = data;
	}
	else
	{
		LOG(8,("INFO: ---RAM test #1 done: access is OK.\n"));
	}

	/* check RAM bankswitching stuff(?) */
	cnt = 0;
	stat = B_ERROR;
	while ((cnt < 4) && (stat != B_OK))
	{
		/* read RAM size */
		data = NV_REG32(NV32_NV10STRAPINFO);
		/* subtract 1MB */
		data -= 0x00100000;
		/* write testpattern at generated RAM adress */
		((volatile uint32 *)si->framebuffer)[(data >> 2)] = 0x4e564441;
		snooze(10);
		/* reset first RAM adress */
		((volatile uint32 *)si->framebuffer)[0x00] = 0x00000000;
		snooze(10);
		/* dummyread first RAM adress four times */
		dummy = ((volatile uint32 *)si->framebuffer)[0x00];
		dummy = ((volatile uint32 *)si->framebuffer)[0x00];
		dummy = ((volatile uint32 *)si->framebuffer)[0x00];
		dummy = ((volatile uint32 *)si->framebuffer)[0x00];
		/* check testpattern to have survived */
		if (((volatile uint32 *)si->framebuffer)[(data >> 2)] == 0x4e564441) stat = B_OK;
		cnt++;
	}

	/* if pattern did not hold modify RAM-type setup */
	if (stat != B_OK)
	{
		LOG(8,("INFO: ---RAM test #2 done: access errors, modified setup.\n"));
		NV_REG32(NV32_PFB_CONFIG_0) &= 0xffffefff;
	}
	else
	{
		LOG(8,("INFO: ---RAM test #2 done: access is OK.\n"));
	}
}

static status_t translate_ISA_PCI(uint32* reg)
{
	switch (*reg)
	{
	case 0x03c0:
		*reg = NV8_ATTRDATW;
		break;
	case 0x03c1:
		*reg = NV8_ATTRDATR;
		break;
	case 0x03c2:
		*reg = NV8_MISCW;
		break;
	case 0x03c4:
		*reg = NV8_SEQIND;
		break;
	case 0x03c5:
		*reg = NV8_SEQDAT;
		break;
	case 0x03c6:
		*reg = NV8_PALMASK;
		break;
	case 0x03c7:
		*reg = NV8_PALINDR;
		break;
	case 0x03c8:
		*reg = NV8_PALINDW;
		break;
	case 0x03c9:
		*reg = NV8_PALDATA;
		break;
	case 0x03cc:
		*reg = NV8_MISCR;
		break;
	case 0x03ce:
		*reg = NV8_GRPHIND;
		break;
	case 0x03cf:
		*reg = NV8_GRPHDAT;
		break;
	case 0x03d4:
		*reg = NV8_CRTCIND;
		break;
	case 0x03d5:
		*reg = NV8_CRTCDAT;
		break;
	case 0x03da:
		*reg = NV8_INSTAT1;
		break;
	default:
		LOG(8,("\n\nINFO: WARNING: ISA->PCI register adress translation failed!\n\n"));
		return B_ERROR;
		break;
	}

	return B_OK;
}

void set_pll(uint32 reg, uint32 req_clk)
{
	uint32 data;
	float calced_clk;
	uint8 m, n, p;
	nv_dac_sys_pll_find(req_clk, &calced_clk, &m, &n, &p, 0);
	/* programming the PLL needs to be done in steps! (confirmed NV28) */
	data = NV_REG32(reg);
	NV_REG32(reg) = ((data & 0xffff0000) | (n << 8) | m);
	data = NV_REG32(reg);
	NV_REG32(reg) = ((p << 16) | (n << 8) | m);

//fixme?
	/* program 2nd set N and M scalers if they exist (b31=1 enables them) */
	if (si->ps.ext_pll)
	{
		if (reg == NV32_COREPLL) NV_REG32(NV32_COREPLL2) = 0x80000401;
		if (reg == NV32_MEMPLL) NV_REG32(NV32_MEMPLL2) = 0x80000401;
	}

	log_pll(reg, req_clk);
}

/* doing general fail-safe default setup here */
static status_t	nv_crtc_setup_fifo()
{
	/* enable access to primary head */
	set_crtc_owner(0);

	/* set CRTC FIFO burst size to 256 */
	CRTCW(FIFO, 0x03);

	/* set CRTC FIFO low watermark to 32 */
	CRTCW(FIFO_LWM, 0x20);

	return B_OK;
}

/* (pre)set 'fixed' card specifications */
void set_specs(void)
{
	LOG(8,("INFO: setting up card specifications\n"));

	/* set failsave speeds */
	switch (si->ps.card_arch)
	{
	case NV40A:
		pinsnv30_arch_fake();
		break;
	default:
		/* 'failsafe' values... */
		pinsnv30_arch_fake();
		break;
	}

	/* detect reference crystal frequency and dualhead */
	switch (si->ps.card_arch)
	{
	default:
		getstrap_arch_nv10_20_30_40();
		break;
	}
}

/* this routine presumes the card was coldstarted by the card's BIOS for panel stuff */
void fake_panel_start(void)
{
	LOG(8,("INFO: detecting RAM size\n"));

	/* detect RAM amount */
	switch (si->ps.card_arch)
	{
	default:
		getRAMsize_arch_nv10_20_30_40();
		break;
	}

	/* override memory detection if requested by user */
	if (si->settings.memory != 0)
	{
		LOG(2,("INFO: forcing memory size (specified in settings file)\n"));
		si->ps.memory_size = si->settings.memory * 1024 * 1024;
	}

	i2c_init();

	LOG(8,("INFO: faking panel startup\n"));

	/* find out the BIOS preprogrammed panel use status... */
	detect_panels();

	/* determine and setup output devices and heads */
	setup_output_matrix();

	/* select other CRTC for primary head use if specified by user in settings file */
	if (si->ps.secondary_head && si->settings.switchhead)
	{
		LOG(2,("INFO: inverting head use (specified in settings file)\n"));
		si->ps.crtc2_prim = !si->ps.crtc2_prim;
	}
}

static void detect_panels()
{
	/* detect if the BIOS enabled LCD's (internal panels or DVI) */

	/* both external TMDS transmitters (used for LCD/DVI) and external TVencoders
	 * (can) use the CRTC's in slaved mode. */
	/* Note:
	 * DFP's are programmed with standard VESA modelines by the card's BIOS! */
	bool slaved_for_dev1 = false, slaved_for_dev2 = false;

	/* check primary head: */
	/* enable access to primary head */
	set_crtc_owner(0);

	/* unlock head's registers for R/W access */
	CRTCW(LOCK, 0x57);
	CRTCW(VSYNCE ,(CRTCR(VSYNCE) & 0x7f));

	LOG(2,("INFO: Dumping flatpanel related CRTC registers:\n"));
	/* related info PIXEL register:
	 * b7: 1 = slaved mode										(all cards). */
	LOG(2,("CRTC1: PIXEL register: $%02x\n", CRTCR(PIXEL)));
	/* info LCD register:
	 * b7: 1 = stereo view (shutter glasses use)				(all cards),
	 * b5: 1 = power ext. TMDS (or something)/0 = TVout	use	(?)	(confirmed NV17, NV28),
	 * b4: 1 = power ext. TMDS (or something)/0 = TVout use	(?)	(confirmed NV34),
	 * b3: 1 = ??? (not panel related probably!)				(confirmed NV34),
	 * b1: 1 = power ext. TMDS (or something) (?)				(confirmed NV05?, NV17),
	 * b0: 1 = select panel encoder / 0 = select TVout encoder	(all cards). */
	LOG(2,("CRTC1: LCD register: $%02x\n", CRTCR(LCD)));
	/* info 0x59 register:
	 * b0: 1 = enable ext. TMDS clock (DPMS)					(confirmed NV28, NV34). */
	LOG(2,("CRTC1: register $59: $%02x\n", CRTCR(0x59)));
	/* info 0x9f register:
	 * b4: 0 = TVout use (?). */
	LOG(2,("CRTC1: register $9f: $%02x\n", CRTCR(0x9f)));

	/* detect active slave device (if any) */
	slaved_for_dev1 = (CRTCR(PIXEL) & 0x80);

	if (si->ps.secondary_head)
	{
		/* check secondary head: */
		/* enable access to secondary head */
		set_crtc_owner(1);
		/* unlock head's registers for R/W access */
		CRTC2W(LOCK, 0x57);
		CRTC2W(VSYNCE ,(CRTC2R(VSYNCE) & 0x7f));

		LOG(2,("CRTC2: PIXEL register: $%02x\n", CRTC2R(PIXEL)));
		LOG(2,("CRTC2: LCD register: $%02x\n", CRTC2R(LCD)));
		LOG(2,("CRTC2: register $59: $%02x\n", CRTC2R(0x59)));
		LOG(2,("CRTC2: register $9f: $%02x\n", CRTC2R(0x9f)));

		/* detect active slave device (if any) */
		slaved_for_dev2 = (CRTC2R(PIXEL) & 0x80);
	}

	LOG(2,("INFO: End flatpanel related CRTC registers dump.\n"));

	/* do some presets */
	si->ps.p1_timing.h_display = 0;
	si->ps.p1_timing.v_display = 0;
	si->ps.panel1_aspect = 0;
	si->ps.p2_timing.h_display = 0;
	si->ps.p2_timing.v_display = 0;
	si->ps.panel2_aspect = 0;
	si->ps.slaved_tmds1 = false;
	si->ps.slaved_tmds2 = false;
	si->ps.master_tmds1 = false;
	si->ps.master_tmds2 = false;
	si->ps.tmds1_active = false;
	si->ps.tmds2_active = false;
	/* determine the situation we are in... (regarding flatpanels) */
	/* fixme: add VESA DDC EDID stuff one day... */
	/* fixme: find out how to program those transmitters one day instead of
	 * relying on the cards BIOS to do it.
	 * Currently we'd loose the panel setup while not being able to restore it. */

	/* note: (facts)
	 * -> a register-set's FP_TG_CTRL register, bit 31 tells you if a LVDS panel is
	 *    connected to the primary head (0), or to the secondary head (1);
	 * -> for LVDS panels both registersets are programmed identically by the card's
	 *    BIOSes;
	 * -> the programmed set of registers tells you where a TMDS (DVI) panel is
	 *    connected;
	 * -> On all cards a CRTC is used in slaved mode when a panel is connected. */
	/* note also:
	 * external TMDS encoders are only used for logic-level translation: it's 
	 * modeline registers are not used. Instead the GPU's internal modeline registers
	 * are used. The external encoder is not connected to a I2C bus (confirmed NV34). */
	if (slaved_for_dev1)
	{
		uint16 width = ((DACR(FP_HDISPEND) & 0x0000ffff) + 1);
		uint16 height = ((DACR(FP_VDISPEND) & 0x0000ffff) + 1);
		if ((width >= 640) && (height >= 480))
		{
			si->ps.slaved_tmds1 = true;
			si->ps.tmds1_active = true;
			si->ps.p1_timing.h_display = width;
			si->ps.p1_timing.v_display = height;
		}
	}

	if (si->ps.secondary_head && slaved_for_dev2)
	{
		uint16 width = ((DAC2R(FP_HDISPEND) & 0x0000ffff) + 1);
		uint16 height = ((DAC2R(FP_VDISPEND) & 0x0000ffff) + 1);
		if ((width >= 640) && (height >= 480))
		{
			si->ps.slaved_tmds2 = true;
			si->ps.tmds2_active = true;
			si->ps.p2_timing.h_display = width;
			si->ps.p2_timing.v_display = height;
		}
	}

	//fixme...:
	//we are assuming that no DVI is used as external monitor on laptops;
	//otherwise we probably get into trouble here if the checked specs match.
	if (si->ps.laptop && si->ps.tmds1_active && si->ps.tmds2_active &&
		((DACR(FP_TG_CTRL) & 0x80000000) == (DAC2R(FP_TG_CTRL) & 0x80000000)) &&
		(si->ps.p1_timing.h_display == si->ps.p2_timing.h_display) &&
		(si->ps.p1_timing.v_display == si->ps.p2_timing.v_display))
	{
		LOG(2,("INFO: correcting double detection of single panel!\n"));

		if (DACR(FP_TG_CTRL) & 0x80000000)
		{
			/* LVDS panel is on CRTC2, so clear false primary detection */
			si->ps.slaved_tmds1 = false;
			si->ps.master_tmds1 = false;
			si->ps.tmds1_active = false;
			si->ps.p1_timing.h_display = 0;
			si->ps.p1_timing.v_display = 0;
		}
		else
		{
			/* LVDS panel is on CRTC1, so clear false secondary detection */
			si->ps.slaved_tmds2 = false;
			si->ps.master_tmds2 = false;
			si->ps.tmds2_active = false;
			si->ps.p2_timing.h_display = 0;
			si->ps.p2_timing.v_display = 0;
		}
	}

	/* fetch panel(s) modeline(s) */
	if (si->ps.tmds1_active)
	{
		/* determine panel aspect ratio */
		si->ps.panel1_aspect =
			(si->ps.p1_timing.h_display / ((float)si->ps.p1_timing.v_display));
		/* force widescreen type if requested */
		if (si->settings.force_ws) si->ps.panel1_aspect = 1.60;
		/* horizontal timing */
		si->ps.p1_timing.h_sync_start = (DACR(FP_HSYNC_S) & 0x0000ffff) + 1;
		si->ps.p1_timing.h_sync_end = (DACR(FP_HSYNC_E) & 0x0000ffff) + 1;
		si->ps.p1_timing.h_total = (DACR(FP_HTOTAL) & 0x0000ffff) + 1;
		/* vertical timing */
		si->ps.p1_timing.v_sync_start = (DACR(FP_VSYNC_S) & 0x0000ffff) + 1;
		si->ps.p1_timing.v_sync_end = (DACR(FP_VSYNC_E) & 0x0000ffff) + 1;
		si->ps.p1_timing.v_total = (DACR(FP_VTOTAL) & 0x0000ffff) + 1;
		/* sync polarity */
		si->ps.p1_timing.flags = 0;
		if (DACR(FP_TG_CTRL) & 0x00000001) si->ps.p1_timing.flags |= B_POSITIVE_VSYNC;
		if (DACR(FP_TG_CTRL) & 0x00000010) si->ps.p1_timing.flags |= B_POSITIVE_HSYNC;
		/* display enable polarity (not an official flag) */
		if (DACR(FP_TG_CTRL) & 0x10000000) si->ps.p1_timing.flags |= B_BLANK_PEDESTAL;
		/* refreshrate:
		 * fix a DVI or laptop flatpanel to 60Hz refresh! */
		si->ps.p1_timing.pixel_clock =
			(si->ps.p1_timing.h_total * si->ps.p1_timing.v_total * 60) / 1000;
	}
	if (si->ps.tmds2_active)
	{
		/* determine panel aspect ratio */
		si->ps.panel2_aspect =
			(si->ps.p2_timing.h_display / ((float)si->ps.p2_timing.v_display));
		/* force widescreen type if requested */
		if (si->settings.force_ws) si->ps.panel2_aspect = 1.60;
		/* horizontal timing */
		si->ps.p2_timing.h_sync_start = (DAC2R(FP_HSYNC_S) & 0x0000ffff) + 1;
		si->ps.p2_timing.h_sync_end = (DAC2R(FP_HSYNC_E) & 0x0000ffff) + 1;
		si->ps.p2_timing.h_total = (DAC2R(FP_HTOTAL) & 0x0000ffff) + 1;
		/* vertical timing */
		si->ps.p2_timing.v_sync_start = (DAC2R(FP_VSYNC_S) & 0x0000ffff) + 1;
		si->ps.p2_timing.v_sync_end = (DAC2R(FP_VSYNC_E) & 0x0000ffff) + 1;
		si->ps.p2_timing.v_total = (DAC2R(FP_VTOTAL) & 0x0000ffff) + 1;
		/* sync polarity */
		si->ps.p2_timing.flags = 0;
		if (DAC2R(FP_TG_CTRL) & 0x00000001) si->ps.p2_timing.flags |= B_POSITIVE_VSYNC;
		if (DAC2R(FP_TG_CTRL) & 0x00000010) si->ps.p2_timing.flags |= B_POSITIVE_HSYNC;
		/* display enable polarity (not an official flag) */
		if (DAC2R(FP_TG_CTRL) & 0x10000000) si->ps.p2_timing.flags |= B_BLANK_PEDESTAL;
		/* refreshrate:
		 * fix a DVI or laptop flatpanel to 60Hz refresh! */
		si->ps.p2_timing.pixel_clock =
			(si->ps.p2_timing.h_total * si->ps.p2_timing.v_total * 60) / 1000;
	}

	/* dump some panel configuration registers... */
	LOG(2,("INFO: Dumping flatpanel registers:\n"));
	LOG(2,("DUALHEAD_CTRL: $%08x\n", NV_REG32(NV32_DUALHEAD_CTRL)));
	LOG(2,("DAC1: FP_HDISPEND: %d\n", DACR(FP_HDISPEND)));
	LOG(2,("DAC1: FP_HTOTAL: %d\n", DACR(FP_HTOTAL)));
	LOG(2,("DAC1: FP_HCRTC: %d\n", DACR(FP_HCRTC)));
	LOG(2,("DAC1: FP_HSYNC_S: %d\n", DACR(FP_HSYNC_S)));
	LOG(2,("DAC1: FP_HSYNC_E: %d\n", DACR(FP_HSYNC_E)));
	LOG(2,("DAC1: FP_HVALID_S: %d\n", DACR(FP_HVALID_S)));
	LOG(2,("DAC1: FP_HVALID_E: %d\n", DACR(FP_HVALID_E)));

	LOG(2,("DAC1: FP_VDISPEND: %d\n", DACR(FP_VDISPEND)));
	LOG(2,("DAC1: FP_VTOTAL: %d\n", DACR(FP_VTOTAL)));
	LOG(2,("DAC1: FP_VCRTC: %d\n", DACR(FP_VCRTC)));
	LOG(2,("DAC1: FP_VSYNC_S: %d\n", DACR(FP_VSYNC_S)));
	LOG(2,("DAC1: FP_VSYNC_E: %d\n", DACR(FP_VSYNC_E)));
	LOG(2,("DAC1: FP_VVALID_S: %d\n", DACR(FP_VVALID_S)));
	LOG(2,("DAC1: FP_VVALID_E: %d\n", DACR(FP_VVALID_E)));

	LOG(2,("DAC1: FP_CHKSUM: $%08x = (dec) %d\n", DACR(FP_CHKSUM),DACR(FP_CHKSUM)));
	LOG(2,("DAC1: FP_TST_CTRL: $%08x\n", DACR(FP_TST_CTRL)));
	LOG(2,("DAC1: FP_TG_CTRL: $%08x\n", DACR(FP_TG_CTRL)));
	LOG(2,("DAC1: FP_DEBUG0: $%08x\n", DACR(FP_DEBUG0)));
	LOG(2,("DAC1: FP_DEBUG1: $%08x\n", DACR(FP_DEBUG1)));
	LOG(2,("DAC1: FP_DEBUG2: $%08x\n", DACR(FP_DEBUG2)));
	LOG(2,("DAC1: FP_DEBUG3: $%08x\n", DACR(FP_DEBUG3)));

	LOG(2,("DAC1: FUNCSEL: $%08x\n", NV_REG32(NV32_FUNCSEL)));
	LOG(2,("DAC1: PANEL_PWR: $%08x\n", NV_REG32(NV32_PANEL_PWR)));

	if(si->ps.secondary_head)
	{
		LOG(2,("DAC2: FP_HDISPEND: %d\n", DAC2R(FP_HDISPEND)));
		LOG(2,("DAC2: FP_HTOTAL: %d\n", DAC2R(FP_HTOTAL)));
		LOG(2,("DAC2: FP_HCRTC: %d\n", DAC2R(FP_HCRTC)));
		LOG(2,("DAC2: FP_HSYNC_S: %d\n", DAC2R(FP_HSYNC_S)));
		LOG(2,("DAC2: FP_HSYNC_E: %d\n", DAC2R(FP_HSYNC_E)));
		LOG(2,("DAC2: FP_HVALID_S:%d\n", DAC2R(FP_HVALID_S)));
		LOG(2,("DAC2: FP_HVALID_E: %d\n", DAC2R(FP_HVALID_E)));

		LOG(2,("DAC2: FP_VDISPEND: %d\n", DAC2R(FP_VDISPEND)));
		LOG(2,("DAC2: FP_VTOTAL: %d\n", DAC2R(FP_VTOTAL)));
		LOG(2,("DAC2: FP_VCRTC: %d\n", DAC2R(FP_VCRTC)));
		LOG(2,("DAC2: FP_VSYNC_S: %d\n", DAC2R(FP_VSYNC_S)));
		LOG(2,("DAC2: FP_VSYNC_E: %d\n", DAC2R(FP_VSYNC_E)));
		LOG(2,("DAC2: FP_VVALID_S: %d\n", DAC2R(FP_VVALID_S)));
		LOG(2,("DAC2: FP_VVALID_E: %d\n", DAC2R(FP_VVALID_E)));

		LOG(2,("DAC2: FP_CHKSUM: $%08x = (dec) %d\n", DAC2R(FP_CHKSUM),DAC2R(FP_CHKSUM)));
		LOG(2,("DAC2: FP_TST_CTRL: $%08x\n", DAC2R(FP_TST_CTRL)));
		LOG(2,("DAC2: FP_TG_CTRL: $%08x\n", DAC2R(FP_TG_CTRL)));
		LOG(2,("DAC2: FP_DEBUG0: $%08x\n", DAC2R(FP_DEBUG0)));
		LOG(2,("DAC2: FP_DEBUG1: $%08x\n", DAC2R(FP_DEBUG1)));
		LOG(2,("DAC2: FP_DEBUG2: $%08x\n", DAC2R(FP_DEBUG2)));
		LOG(2,("DAC2: FP_DEBUG3: $%08x\n", DAC2R(FP_DEBUG3)));

		LOG(2,("DAC2: FUNCSEL: $%08x\n", NV_REG32(NV32_2FUNCSEL)));
		LOG(2,("DAC2: PANEL_PWR: $%08x\n", NV_REG32(NV32_2PANEL_PWR)));
	}

	/* determine flatpanel type(s) */
	//fixme?: linux checks on (only and) all dualhead cards, and only on DAC1...
//fixme: testing...
	if (/*si->ps.tmds1_active && */1)
	{
		/* Read a indexed register to see if it indicates LVDS or TMDS panel presence.
		 * b0-7 = adress, b16 = 1 = write_enable */
		DACW(FP_TMDS_CTRL, ((1 << 16) | 0x04));
		/* (b0-7 = data) */
		if (DACR(FP_TMDS_DATA) & 0x01)
			LOG(2,("INFO: Flatpanel on head 1 is LVDS type\n"));
		else
			LOG(2,("INFO: Flatpanel on head 1 is TMDS type\n"));
	}
//fixme: testing...
//	if (si->ps.tmds2_active)
	if (si->ps.secondary_head)
	{
		/* Read a indexed register to see if it indicates LVDS or TMDS panel presence.
		 * b0-7 = adress, b16 = 1 = write_enable */
		DAC2W(FP_TMDS_CTRL, ((1 << 16) | 0x04));
		/* (b0-7 = data) */
		if (DAC2R(FP_TMDS_DATA) & 0x01)
			LOG(2,("INFO: Flatpanel on head 2 is LVDS type\n"));
		else
			LOG(2,("INFO: Flatpanel on head 2 is TMDS type\n"));
	}

	LOG(2,("INFO: End flatpanel registers dump.\n"));
}

static void setup_output_matrix()
{
	/* setup defaults: */
	/* no monitors (output devices) detected */
	si->ps.monitors = 0x00;
	/* head 1 will be the primary head */
	si->ps.crtc2_prim = false;

	/* setup output devices and heads */
	if (si->ps.secondary_head)
	{
		/* setup defaults: */
		/* connect analog outputs straight through */
		nv_general_output_select(false);

		/* presetup by the card's BIOS, we can't change this (lack of info) */
		if (si->ps.tmds1_active) si->ps.monitors |= 0x01;
		if (si->ps.tmds2_active) si->ps.monitors |= 0x10;
		/* detect analog monitors (confirmed working OK on NV18, NV28 and NV34): */
		/* sense analog monitor on primary connector */
		if (nv_dac_crt_connected()) si->ps.monitors |= 0x02;
		/* sense analog monitor on secondary connector */
		if (nv_dac2_crt_connected()) si->ps.monitors |= 0x20;

		/* setup correct output and head use */
		switch (si->ps.monitors)
		{
		case 0x00: /* no monitor found at all */
			LOG(2,("INFO: head 1 has nothing connected;\n"));
			LOG(2,("INFO: head 2 has nothing connected:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x01: /* digital panel on head 1, nothing on head 2 */
			LOG(2,("INFO: head 1 has a digital panel;\n"));
			LOG(2,("INFO: head 2 has nothing connected:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x02: /* analog panel or CRT on head 1, nothing on head 2 */
			LOG(2,("INFO: head 1 has an analog panel or CRT;\n"));
			LOG(2,("INFO: head 2 has nothing connected:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x03: /* both types on head 1, nothing on head 2 */
			LOG(2,("INFO: head 1 has a digital panel AND an analog panel or CRT;\n"));
			LOG(2,("INFO: head 2 has nothing connected:\n"));
			LOG(2,("INFO: correcting...\n"));
			/* cross connect analog outputs so analog panel or CRT gets head 2 */
			nv_general_output_select(true);
			LOG(2,("INFO: head 1 has a digital panel;\n"));
			LOG(2,("INFO: head 2 has an analog panel or CRT:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x10: /* nothing on head 1, digital panel on head 2 */
			LOG(2,("INFO: head 1 has nothing connected;\n"));
			LOG(2,("INFO: head 2 has a digital panel:\n"));
			LOG(2,("INFO: defaulting to head 2 for primary use.\n"));
			si->ps.crtc2_prim = true;
			break;
		case 0x20: /* nothing on head 1, analog panel or CRT on head 2 */
			LOG(2,("INFO: head 1 has nothing connected;\n"));
			LOG(2,("INFO: head 2 has an analog panel or CRT:\n"));
			LOG(2,("INFO: defaulting to head 2 for primary use.\n"));
			si->ps.crtc2_prim = true;
			break;
		case 0x30: /* nothing on head 1, both types on head 2 */
			LOG(2,("INFO: head 1 has nothing connected;\n"));
			LOG(2,("INFO: head 2 has a digital panel AND an analog panel or CRT:\n"));
			LOG(2,("INFO: correcting...\n"));
			/* cross connect analog outputs so analog panel or CRT gets head 1 */
			nv_general_output_select(true);
			LOG(2,("INFO: head 1 has an analog panel or CRT;\n"));
			LOG(2,("INFO: head 2 has a digital panel:\n"));
			LOG(2,("INFO: defaulting to head 2 for primary use.\n"));
			si->ps.crtc2_prim = true;
			break;
		case 0x11: /* digital panels on both heads */
			LOG(2,("INFO: head 1 has a digital panel;\n"));
			LOG(2,("INFO: head 2 has a digital panel:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x12: /* analog panel or CRT on head 1, digital panel on head 2 */
			LOG(2,("INFO: head 1 has an analog panel or CRT;\n"));
			LOG(2,("INFO: head 2 has a digital panel:\n"));
			LOG(2,("INFO: defaulting to head 2 for primary use.\n"));
			si->ps.crtc2_prim = true;
			break;
		case 0x21: /* digital panel on head 1, analog panel or CRT on head 2 */
			LOG(2,("INFO: head 1 has a digital panel;\n"));
			LOG(2,("INFO: head 2 has an analog panel or CRT:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x22: /* analog panel(s) or CRT(s) on both heads */
			LOG(2,("INFO: head 1 has an analog panel or CRT;\n"));
			LOG(2,("INFO: head 2 has an analog panel or CRT:\n"));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		case 0x32: /* more than two monitors connected to just two outputs: illegal! */
			//general fixme:
			//NV40 architecture contains (an) additional switch(es) to
			//connect a CRTC/DAC combination to a connector. We can't work as
			//usual (yet) because this interferes via BIOS card pre-programming.
			//
			//Also: it looks as if each pixelclock PLL can select different CRTC's
			//as well now via a new register: one PLL can be driving both CRTC's
			//and there's nothing we can do about that (yet). (DVI/dualhead trouble)
		default: /* more than two monitors connected to just two outputs: illegal! */
			LOG(2,("INFO: illegal monitor setup ($%02x):\n", si->ps.monitors));
			LOG(2,("INFO: defaulting to head 1 for primary use.\n"));
			break;
		}
	}
	else /* singlehead cards */
	{
		/* presetup by the card's BIOS, we can't change this (lack of info) */
		if (si->ps.tmds1_active) si->ps.monitors |= 0x01;
		/* detect analog monitor (confirmed working OK on all cards): */
		/* sense analog monitor on primary connector */
		if (nv_dac_crt_connected()) si->ps.monitors |= 0x02;
	}
}

void get_panel_modes(display_mode *p1, display_mode *p2, bool *pan1, bool *pan2)
{
	if (si->ps.tmds1_active)
	{
		/* timing ('modeline') */
		p1->timing = si->ps.p1_timing;
		/* setup the rest */
		p1->space = B_CMAP8;
		p1->virtual_width = p1->timing.h_display;
		p1->virtual_height = p1->timing.v_display;
		p1->h_display_start = 0;
		p1->v_display_start = 0;
		p1->flags = 0;
		*pan1 = true;
	}
	else
		*pan1 = false;

	if (si->ps.tmds2_active)
	{
		/* timing ('modeline') */
		p2->timing = si->ps.p2_timing;
		/* setup the rest */
		p2->space = B_CMAP8;
		p2->virtual_width = p2->timing.h_display;
		p2->virtual_height = p2->timing.v_display;
		p2->h_display_start = 0;
		p2->v_display_start = 0;
		p2->flags = 0;
		*pan2 = true;
	}
	else
		*pan2 = false;
}

static void pinsnv30_arch_fake(void)
{
	/* we have a extended PLL */
	si->ps.ext_pll = true;
	/* carefull not to take to high limits, and high should be >= 2x low. */
	si->ps.max_system_vco = 350;
	si->ps.min_system_vco = 128;
	if (si->ps.ext_pll)
	{
		si->ps.max_pixel_vco = 600;
		si->ps.min_pixel_vco = 220;
		si->ps.max_video_vco = 600;
		si->ps.min_video_vco = 220;
	}
	else
	{
		si->ps.max_pixel_vco = 350;
		si->ps.min_pixel_vco = 128;
		si->ps.max_video_vco = 350;
		si->ps.min_video_vco = 128;
	}
	si->ps.max_dac1_clock = 350;
	si->ps.max_dac1_clock_8 = 350;
	si->ps.max_dac1_clock_16 = 350;
	/* 'failsave' values */
	si->ps.max_dac1_clock_24 = 320;
	si->ps.max_dac1_clock_32 = 280;
	si->ps.max_dac1_clock_32dh = 250;
	/* secondary head */
	/* GeForceFX cards have dual integrated DACs with identical capability */
	/* (called nview technology) */
	si->ps.max_dac2_clock = 350;
	si->ps.max_dac2_clock_8 = 350;
	si->ps.max_dac2_clock_16 = 350;
	/* 'failsave' values */
	si->ps.max_dac2_clock_24 = 320;
	si->ps.max_dac2_clock_32 = 280;
	si->ps.max_dac2_clock_32dh = 250;
	//fixme: primary & secondary_dvi should be overrule-able via nv.settings
	si->ps.primary_dvi = false;
	si->ps.secondary_dvi = false;
	/* not used (yet) because no coldstart will be attempted (yet) */
	si->ps.std_engine_clock = 190;
	si->ps.std_memory_clock = 190;
}

static void getRAMsize_arch_nv10_20_30_40(void)
{
	uint32 dev_manID = CFGR(DEVID);
	uint32 strapinfo = NV_REG32(NV32_NV10STRAPINFO);

	switch (dev_manID)
	{
	case 0x01a010de: /* Nvidia GeForce2 Integrated GPU */
	case 0x01f010de: /* Nvidia GeForce4 MX Integrated GPU */
		/* the kerneldriver already determined the amount of RAM these cards have at
		 * their disposal (UMA, values read from PCI config space in other device) */
		LOG(8,("INFO: nVidia GPU with UMA detected\n"));
		break;
	default:
		LOG(8,("INFO: (Memory detection) Strapinfo value is: $%08x\n", strapinfo));

		switch ((strapinfo & 0x3ff00000) >> 20)
		{
		case 2:
			si->ps.memory_size = 2 * 1024 * 1024;
			break;
		case 4:
			si->ps.memory_size = 4 * 1024 * 1024;
			break;
		case 8:
			si->ps.memory_size = 8 * 1024 * 1024;
			break;
		case 16:
			si->ps.memory_size = 16 * 1024 * 1024;
			break;
		case 32:
			si->ps.memory_size = 32 * 1024 * 1024;
			break;
		case 64:
			si->ps.memory_size = 64 * 1024 * 1024;
			break;
		case 128:
			si->ps.memory_size = 128 * 1024 * 1024;
			break;
		case 256:
			si->ps.memory_size = 256 * 1024 * 1024;
			break;
		case 512:
			si->ps.memory_size = 512 * 1024 * 1024;
			break;
		default:
			si->ps.memory_size = 16 * 1024 * 1024;

			LOG(8,("INFO: NV10/20/30 architecture chip with unknown RAM amount detected;\n"));
			LOG(8,("INFO: Setting 16Mb\n"));
			break;
		}
	}
}

static void getstrap_arch_nv10_20_30_40(void)
{
	uint32 strapinfo = NV_REG32(NV32_NVSTRAPINFO2);

	/* all cards are dualhead cards these days */
	si->ps.secondary_head = true;

	/* determine PLL reference crystal frequency: three types are used... */
	if (strapinfo & 0x00000040)
		si->ps.f_ref = 14.31818;
	else
		si->ps.f_ref = 13.50000;

	if (si->ps.secondary_head)
	{
		if (strapinfo & 0x00400000) si->ps.f_ref = 27.00000;
	}
}

void dump_pins(void)
{
	LOG(2,("INFO: pinsdump follows:\n"));
	LOG(2,("PLL type: "));
	if (si->ps.ext_pll) LOG(2,("extended\n")); else LOG(2,("standard\n"));
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
//	LOG(2,("primary_dvi: "));
//	if (si->ps.primary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
//	LOG(2,("secondary_dvi: "));
//	if (si->ps.secondary_dvi) LOG(2,("present\n")); else LOG(2,("absent\n"));
	LOG(2,("card memory_size: %3.3fMb\n", (si->ps.memory_size / (1024.0 * 1024.0))));
	LOG(2,("laptop: "));
	if (si->ps.laptop) LOG(2,("yes\n")); else LOG(2,("no\n"));
	if (si->ps.tmds1_active)
	{
		LOG(2,("found DFP (digital flatpanel) on CRTC1; CRTC1 is "));
		if (si->ps.slaved_tmds1) LOG(2,("slaved\n")); else LOG(2,("master\n"));
		LOG(2,("panel width: %d, height: %d, aspect ratio: %1.2f\n",
			si->ps.p1_timing.h_display, si->ps.p1_timing.v_display, si->ps.panel1_aspect));
	}
	if (si->ps.tmds2_active)
	{
		LOG(2,("found DFP (digital flatpanel) on CRTC2; CRTC2 is "));
		if (si->ps.slaved_tmds2) LOG(2,("slaved\n")); else LOG(2,("master\n"));
		LOG(2,("panel width: %d, height: %d, aspect ratio: %1.2f\n",
			si->ps.p2_timing.h_display, si->ps.p2_timing.v_display, si->ps.panel2_aspect));
	}
	LOG(2,("monitor (output devices) setup matrix: $%02x\n", si->ps.monitors));
	LOG(2,("INFO: end pinsdump.\n"));
}
