/* NV Acceleration functions */

/* Author:
   Rudolf Cornelissen 8/2003-6/2010.

   This code was possible thanks to:
    - the Linux XFree86 NV driver,
    - the Linux UtahGLX 3D driver.
*/

#define MODULE_BIT 0x00080000

#include "nv_std.h"

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

static void nv_init_for_3D_dma(void);
static void nv_start_dma(void);
static status_t nv_acc_fifofree_dma(uint16 cmd_size);
static void nv_acc_cmd_dma(uint32 cmd, uint16 offset, uint16 size);
static void nv_acc_set_ch_dma(uint16 ch, uint32 handle);

/* used to track engine DMA stalls */
static uint8 err;

/* wait until engine completely idle */
status_t nv_acc_wait_idle_dma()
{
	/* we'd better check for timeouts on the DMA engine as it's theoretically
	 * breakable by malfunctioning software */
	uint16 cnt = 0;

	/* wait until all upcoming commands are in execution at least. Do this until
	 * we hit a timeout; abort if we failed at least three times before:
	 * if DMA stalls, we have to forget about it alltogether at some point, or
	 * the system will almost come to a complete halt.. */
	/* note:
	 * it doesn't matter which FIFO channel's DMA registers we access, they are in
	 * fact all the same set. It also doesn't matter if the channel was assigned a
	 * command or not. */
	while ((NV_REG32(NVACC_FIFO + NV_GENERAL_DMAGET) != (si->engine.dma.put << 2)) &&
			(cnt < 10000) && (err < 3))
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100);
		cnt++;
	}

	/* log timeout if we had one */
	if (cnt == 10000)
	{
		if (err < 3) err++;
		LOG(4,("ACC_DMA: wait_idle; DMA timeout #%d, engine trouble!\n", err));
	}

	/* wait until execution completed */
	while (ACCR(STATUS))
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100);
	}

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nv_acc_init_dma()
{
	uint32 cnt, tmp;
	uint32 surf_depth, cmd_depth;
	/* reset the engine DMA stalls counter */
	err = 0;

	/* a hanging engine only recovers from a complete power-down/power-up cycle */
	NV_REG32(NV32_PWRUPCTRL) = 0xffff00ff;
	snooze(1000);
	NV_REG32(NV32_PWRUPCTRL) = 0xffffffff;

	/* don't try this on NV20 and later.. */
	/* note:
	 * the specific register that's responsible for the speedfix on NV18 is
	 * $00400ed8: bit 6 needs to be zero for fastest rendering (confirmed). */
	/* note also:
	 * on NV28 the following ranges could be reset (confirmed):
	 * $00400000 upto/incl. $004002fc;
	 * $00400400 upto/incl. $004017fc;
	 * $0040180c upto/incl. $00401948;
	 * $00401994 upto/incl. $00401a80;
	 * $00401a94 upto/incl. $00401ffc.
	 * The intermediate ranges hang the engine upon resetting. */
	if (si->ps.card_arch < NV20A)
	{
		/* actively reset the PGRAPH registerset (acceleration engine) */
		for (cnt = 0x00400000; cnt < 0x00402000; cnt +=4)
		{
			NV_REG32(cnt) = 0x00000000;
		}
	}

	/* setup PTIMER: */
	LOG(4,("ACC_DMA: timer numerator $%08x, denominator $%08x\n", ACCR(PT_NUMERATOR), ACCR(PT_DENOMINATR)));

	/* The NV28 BIOS programs PTIMER like this (see coldstarting in nv_info.c) */
	//ACCW(PT_NUMERATOR, (si->ps.std_engine_clock * 20));
	//ACCW(PT_DENOMINATR, 0x00000271);
	/* Nouveau (march 2009) mentions something like: writing 8 and 3 to these regs breaks the timings
	 * on the LVDS hardware sequencing microcode. A correct solution involves calculations with the GPU PLL. */

	/* For now use BIOS pre-programmed values if there */
	if (!ACCR(PT_NUMERATOR) || !ACCR(PT_DENOMINATR)) {
		/* set timer numerator to 8 (in b0-15) */
		ACCW(PT_NUMERATOR, 0x00000008);
		/* set timer denominator to 3 (in b0-15) */
		ACCW(PT_DENOMINATR, 0x00000003);
	}

	/* disable timer-alarm INT requests (b0) */
	ACCW(PT_INTEN, 0x00000000);
	/* reset timer-alarm INT status bit (b0) */
	ACCW(PT_INTSTAT, 0xffffffff);

	/* enable PRAMIN write access on pre NV10 before programming it! */
	if (si->ps.card_arch == NV04A)
	{
		/* set framebuffer config: type = notiling, PRAMIN write access enabled */
		NV_REG32(NV32_PFB_CONFIG_0) = 0x00001114;
	}
	else
	{
		/* setup acc engine 'source' tile adressranges */
		if ((si->ps.card_type <= NV40) || (si->ps.card_type == NV45))
		{
			ACCW(NV10_FBTIL0AD, 0);
			ACCW(NV10_FBTIL1AD, 0);
			ACCW(NV10_FBTIL2AD, 0);
			ACCW(NV10_FBTIL3AD, 0);
			ACCW(NV10_FBTIL4AD, 0);
			ACCW(NV10_FBTIL5AD, 0);
			ACCW(NV10_FBTIL6AD, 0);
			ACCW(NV10_FBTIL7AD, 0);
			ACCW(NV10_FBTIL0ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL1ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL2ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL3ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL4ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL5ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL6ED, (si->ps.memory_size - 1));
			ACCW(NV10_FBTIL7ED, (si->ps.memory_size - 1));
		}
		else
		{
			/* NV41, 43, 44, G70 and up */
			ACCW(NV41_FBTIL0AD, 0);
			ACCW(NV41_FBTIL1AD, 0);
			ACCW(NV41_FBTIL2AD, 0);
			ACCW(NV41_FBTIL3AD, 0);
			ACCW(NV41_FBTIL4AD, 0);
			ACCW(NV41_FBTIL5AD, 0);
			ACCW(NV41_FBTIL6AD, 0);
			ACCW(NV41_FBTIL7AD, 0);
			ACCW(NV41_FBTIL8AD, 0);
			ACCW(NV41_FBTIL9AD, 0);
			ACCW(NV41_FBTILAAD, 0);
			ACCW(NV41_FBTILBAD, 0);
			ACCW(NV41_FBTIL0ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL1ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL2ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL3ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL4ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL5ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL6ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL7ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL8ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTIL9ED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTILAED, (si->ps.memory_size - 1));
			ACCW(NV41_FBTILBED, (si->ps.memory_size - 1));

			if (si->ps.card_type >= G70)
			{
				ACCW(G70_FBTILCAD, 0);
				ACCW(G70_FBTILDAD, 0);
				ACCW(G70_FBTILEAD, 0);
				ACCW(G70_FBTILCED, (si->ps.memory_size - 1));
				ACCW(G70_FBTILDED, (si->ps.memory_size - 1));
				ACCW(G70_FBTILEED, (si->ps.memory_size - 1));
			}
		}
	}

	/*** PRAMIN ***/
	/* first clear the entire RAMHT (hash-table) space to a defined state. It turns
	 * out at least NV11 will keep the previously programmed handles over resets and
	 * power-outages upto about 15 seconds!! Faulty entries might well hang the
	 * engine (confirmed on NV11).
	 * Note:
	 * this behaviour is not very strange: even very old DRAM chips are known to be
	 * able to do this, even though you should refresh them every few milliseconds or
	 * so. (Large memory cell capacitors, though different cells vary a lot in their
	 * capacity.)
	 * Of course data validity is not certain by a long shot over this large
	 * amount of time.. */
	for(cnt = 0; cnt < 0x0400; cnt++)
		NV_REG32(NVACC_HT_HANDL_00 + (cnt << 2)) = 0;
	/* RAMHT (hash-table) space SETUP FIFO HANDLES */
	/* note:
	 * 'instance' tells you where the engine command is stored in 'PR_CTXx_x' sets
	 * below: instance being b4-19 with baseadress NV_PRAMIN_CTX_0 (0x00700000).
	 * That command is linked to the handle noted here. This handle is then used to
	 * tell the FIFO to which engine command it is connected!
	 * (CTX registers are actually a sort of RAM space.) */
	if (si->ps.card_arch >= NV40A)
	{
		/* (first set) */
		ACCW(HT_HANDL_00, (0x80000000 | NV10_CONTEXT_SURFACES_2D)); /* 32bit handle (not used) */
		ACCW(HT_VALUE_00, 0x0010114c); /* instance $114c, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_01, (0x80000000 | NV_IMAGE_BLIT)); /* 32bit handle */
		ACCW(HT_VALUE_01, 0x00101148); /* instance $1148, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_02, (0x80000000 | NV4_GDI_RECTANGLE_TEXT)); /* 32bit handle */
		ACCW(HT_VALUE_02, 0x0010114a); /* instance $114a, engine = acc engine, CHID = $00 */

		/* (second set) */
		ACCW(HT_HANDL_10, (0x80000000 | NV_ROP5_SOLID)); /* 32bit handle */
		ACCW(HT_VALUE_10, 0x00101142); /* instance $1142, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_11, (0x80000000 | NV_IMAGE_BLACK_RECTANGLE)); /* 32bit handle */
		ACCW(HT_VALUE_11, 0x00101144); /* instance $1144, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_12, (0x80000000 | NV_IMAGE_PATTERN)); /* 32bit handle */
		ACCW(HT_VALUE_12, 0x00101146); /* instance $1146, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_13, (0x80000000 | NV_SCALED_IMAGE_FROM_MEMORY)); /* 32bit handle */
		ACCW(HT_VALUE_13, 0x0010114e); /* instance $114e, engine = acc engine, CHID = $00 */
	}
	else
	{
		/* (first set) */
		ACCW(HT_HANDL_00, (0x80000000 | NV4_SURFACE)); /* 32bit handle */
		ACCW(HT_VALUE_00, 0x80011145); /* instance $1145, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_01, (0x80000000 | NV_IMAGE_BLIT)); /* 32bit handle */
		ACCW(HT_VALUE_01, 0x80011146); /* instance $1146, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_02, (0x80000000 | NV4_GDI_RECTANGLE_TEXT)); /* 32bit handle */
		ACCW(HT_VALUE_02, 0x80011147); /* instance $1147, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_03, (0x80000000 | NV4_CONTEXT_SURFACES_ARGB_ZS)); /* 32bit handle (3D) */
		ACCW(HT_VALUE_03, 0x80011148); /* instance $1148, engine = acc engine, CHID = $00 */

		/* NV4_ and NV10_DX5_TEXTURE_TRIANGLE should be identical */
		ACCW(HT_HANDL_04, (0x80000000 | NV4_DX5_TEXTURE_TRIANGLE)); /* 32bit handle (3D) */
		ACCW(HT_VALUE_04, 0x80011149); /* instance $1149, engine = acc engine, CHID = $00 */

		/* NV4_ and NV10_DX6_MULTI_TEXTURE_TRIANGLE should be identical */
		ACCW(HT_HANDL_05, (0x80000000 | NV4_DX6_MULTI_TEXTURE_TRIANGLE)); /* 32bit handle (not used) */
		ACCW(HT_VALUE_05, 0x8001114a); /* instance $114a, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_06, (0x80000000 | NV1_RENDER_SOLID_LIN)); /* 32bit handle (not used) */
		ACCW(HT_VALUE_06, 0x8001114c); /* instance $114c, engine = acc engine, CHID = $00 */

		/* (second set) */
		ACCW(HT_HANDL_10, (0x80000000 | NV_ROP5_SOLID)); /* 32bit handle */
		ACCW(HT_VALUE_10, 0x80011142); /* instance $1142, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_11, (0x80000000 | NV_IMAGE_BLACK_RECTANGLE)); /* 32bit handle */
		ACCW(HT_VALUE_11, 0x80011143); /* instance $1143, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_12, (0x80000000 | NV_IMAGE_PATTERN)); /* 32bit handle */
		ACCW(HT_VALUE_12, 0x80011144); /* instance $1144, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_13, (0x80000000 | NV_SCALED_IMAGE_FROM_MEMORY)); /* 32bit handle */
		ACCW(HT_VALUE_13, 0x8001114b); /* instance $114b, engine = acc engine, CHID = $00 */

		//2007 3D tests..
		if (si->ps.card_type == NV15)
		{
			ACCW(HT_HANDL_14, (0x80000000 | NV_TCL_PRIMITIVE_3D)); /* 32bit handle */
			ACCW(HT_VALUE_14, 0x8001114d); /* instance $114d, engine = acc engine, CHID = $00 */
		}

	}

	/* program CTX registers: CTX1 is mostly done later (colorspace dependant) */
	/* note:
	 * CTX determines which HT handles point to what engine commands. */
	/* note also:
	 * CTX registers are in fact in the same GPU internal RAM space as the engine's
	 * hashtable. This means that stuff programmed in here also survives resets and
	 * power-outages! (confirmed NV11) */
	if (si->ps.card_arch >= NV40A)
	{
		/* setup a DMA define for use by command defines below. */
		ACCW(PR_CTX0_R, 0x00003000); /* DMA page table present and of linear type;
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_R, (si->ps.memory_size - 1)); /* DMA limit: size is all cardRAM */
		ACCW(PR_CTX2_R, ((0x00000000 & 0xfffff000) | 0x00000002));
									 /* DMA access type is READ_AND_WRITE;
									  * memory starts at start of cardRAM (b12-31):
									  * It's adress needs to be at a 4kb boundary! */
		ACCW(PR_CTX3_R, 0x00000002); /* unknown (looks like this is rubbish/not needed?) */
		/* setup set '0' for cmd NV_ROP5_SOLID */
		ACCW(PR_CTX0_0, 0x02080043); /* NVclass $043, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_0, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_0, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_0, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_1, 0x00000000); /* extra */
		ACCW(PR_CTX1_1, 0x00000000); /* extra */
		/* setup set '1' for cmd NV_IMAGE_BLACK_RECTANGLE */
		ACCW(PR_CTX0_2, 0x02080019); /* NVclass $019, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_2, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_2, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_2, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_3, 0x00000000); /* extra */
		ACCW(PR_CTX1_3, 0x00000000); /* extra */
		/* setup set '2' for cmd NV_IMAGE_PATTERN */
		ACCW(PR_CTX0_4, 0x02080018); /* NVclass $018, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_4, 0x02000000); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_4, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_4, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_5, 0x00000000); /* extra */
		ACCW(PR_CTX1_5, 0x00000000); /* extra */
		/* setup set '4' for cmd NV12_IMAGE_BLIT */
		ACCW(PR_CTX0_6, 0x0208009f); /* NVclass $09f, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_6, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_6, 0x00001140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_6, 0x00001140); /* method trap 0 is $1140, trap 1 disabled */
		ACCW(PR_CTX0_7, 0x00000000); /* extra */
		ACCW(PR_CTX1_7, 0x00000000); /* extra */
		/* setup set '5' for cmd NV4_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_8, 0x0208004a); /* NVclass $04a, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_8, 0x02000000); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_8, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_9, 0x00000000); /* extra */
		ACCW(PR_CTX1_9, 0x00000000); /* extra */
		/* setup set '6' for cmd NV10_CONTEXT_SURFACES_2D */
		ACCW(PR_CTX0_A, 0x02080062); /* NVclass $062, nv10+: little endian */
		ACCW(PR_CTX1_A, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_A, 0x00001140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_A, 0x00001140); /* method trap 0 is $1140, trap 1 disabled */
		ACCW(PR_CTX0_B, 0x00000000); /* extra */
		ACCW(PR_CTX1_B, 0x00000000); /* extra */
		/* setup set '7' for cmd NV_SCALED_IMAGE_FROM_MEMORY */
		ACCW(PR_CTX0_C, 0x02080077); /* NVclass $077, nv10+: little endian */
		ACCW(PR_CTX1_C, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_C, 0x00001140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_C, 0x00001140); /* method trap 0 is $1140, trap 1 disabled */
		ACCW(PR_CTX0_D, 0x00000000); /* extra */
		ACCW(PR_CTX1_D, 0x00000000); /* extra */
		/* setup DMA set pointed at by PF_CACH1_DMAI */
		ACCW(PR_CTX0_E, 0x00003002); /* DMA page table present and of linear type;
									  * DMA class is $002 (b0-11);
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_E, 0x00007fff); /* DMA limit: tablesize is 32k bytes */
		ACCW(PR_CTX2_E, (((si->ps.memory_size - 1) & 0xffff8000) | 0x00000002));
									 /* DMA access type is READ_AND_WRITE;
									  * table is located at end of cardRAM (b12-31):
									  * It's adress needs to be at a 4kb boundary! */
	}
	else
	{
		/* setup a DMA define for use by command defines below. */
		ACCW(PR_CTX0_R, 0x00003000); /* DMA page table present and of linear type;
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_R, (si->ps.memory_size - 1)); /* DMA limit: size is all cardRAM */
		ACCW(PR_CTX2_R, ((0x00000000 & 0xfffff000) | 0x00000002));
									 /* DMA access type is READ_AND_WRITE;
									  * memory starts at start of cardRAM (b12-31):
									  * It's adress needs to be at a 4kb boundary! */
		ACCW(PR_CTX3_R, 0x00000002); /* unknown (looks like this is rubbish/not needed?) */
		/* setup set '0' for cmd NV_ROP5_SOLID */
		ACCW(PR_CTX0_0, 0x01008043); /* NVclass $043, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_0, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_0, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_0, 0x00000000); /* method traps disabled */
		/* setup set '1' for cmd NV_IMAGE_BLACK_RECTANGLE */
		ACCW(PR_CTX0_1, 0x01008019); /* NVclass $019, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_1, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_1, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_1, 0x00000000); /* method traps disabled */
		/* setup set '2' for cmd NV_IMAGE_PATTERN */
		ACCW(PR_CTX0_2, 0x01008018); /* NVclass $018, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_2, 0x00000002); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_2, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_2, 0x00000000); /* method traps disabled */
		/* setup set '3' for ... */
		if(si->ps.card_arch >= NV10A)
		{
			/* ... cmd NV10_CONTEXT_SURFACES_2D */
			ACCW(PR_CTX0_3, 0x01008062); /* NVclass $062, nv10+: little endian */
		}
		else
		{
			/* ... cmd NV4_SURFACE */
			ACCW(PR_CTX0_3, 0x01008042); /* NVclass $042, nv10+: little endian */
		}
		ACCW(PR_CTX1_3, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_3, 0x11401140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_3, 0x00000000); /* method trap 0 is $1140, trap 1 disabled */
		/* setup set '4' for ... */
		if (si->ps.card_type >= NV11)
		{
			/* ... cmd NV12_IMAGE_BLIT */
			ACCW(PR_CTX0_4, 0x0100809f); /* NVclass $09f, patchcfg ROP_AND, nv10+: little endian */
		}
		else
		{
			/* ... cmd NV_IMAGE_BLIT */
			ACCW(PR_CTX0_4, 0x0100805f); /* NVclass $05f, patchcfg ROP_AND, nv10+: little endian */
		}
		ACCW(PR_CTX1_4, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_4, 0x11401140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_4, 0x00000000); /* method trap 0 is $1140, trap 1 disabled */
		/* setup set '5' for cmd NV4_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_5, 0x0100804a); /* NVclass $04a, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_5, 0x00000002); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_5, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_5, 0x00000000); /* method traps disabled */
		/* setup set '6' ... */
		if (si->ps.card_arch >= NV10A)
		{
			/* ... for cmd NV10_CONTEXT_SURFACES_ARGB_ZS */
			ACCW(PR_CTX0_6, 0x00000093); /* NVclass $093, nv10+: little endian */
		}
		else
		{
			/* ... for cmd NV4_CONTEXT_SURFACES_ARGB_ZS */
			ACCW(PR_CTX0_6, 0x00000053); /* NVclass $053, nv10+: little endian */
		}
		ACCW(PR_CTX1_6, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_6, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_6, 0x00000000); /* method traps disabled */
		/* setup set '7' ... */
		if (si->ps.card_arch >= NV10A)
		{
			/* ... for cmd NV10_DX5_TEXTURE_TRIANGLE */
			ACCW(PR_CTX0_7, 0x0300a094); /* NVclass $094, patchcfg ROP_AND, userclip enable,
										  * context surface0 valid, nv10+: little endian */
		}
		else
		{
			/* ... for cmd NV4_DX5_TEXTURE_TRIANGLE */
			ACCW(PR_CTX0_7, 0x0300a054); /* NVclass $054, patchcfg ROP_AND, userclip enable,
										  * context surface0 valid */
		}
		ACCW(PR_CTX1_7, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_7, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_7, 0x00000000); /* method traps disabled */
		/* setup set '8' ... */
		if (si->ps.card_arch >= NV10A)
		{
			/* ... for cmd NV10_DX6_MULTI_TEXTURE_TRIANGLE (not used) */
			ACCW(PR_CTX0_8, 0x0300a095); /* NVclass $095, patchcfg ROP_AND, userclip enable,
										  * context surface0 valid, nv10+: little endian */
		}
		else
		{
			/* ... for cmd NV4_DX6_MULTI_TEXTURE_TRIANGLE (not used) */
			ACCW(PR_CTX0_8, 0x0300a055); /* NVclass $055, patchcfg ROP_AND, userclip enable,
										  * context surface0 valid */
		}
		ACCW(PR_CTX1_8, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_8, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		/* setup set '9' for cmd NV_SCALED_IMAGE_FROM_MEMORY */
		ACCW(PR_CTX0_9, 0x01018077); /* NVclass $077, patchcfg SRC_COPY,
									  * context surface0 valid, nv10+: little endian */
		ACCW(PR_CTX1_9, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_9, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_9, 0x00000000); /* method traps disabled */
		/* setup set 'A' for cmd NV1_RENDER_SOLID_LIN (not used) */
		ACCW(PR_CTX0_A, 0x0300a01c); /* NVclass $01c, patchcfg ROP_AND, userclip enable,
									  * context surface0 valid, nv10+: little endian */
		ACCW(PR_CTX1_A, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_A, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_A, 0x00000000); /* method traps disabled */
		//2007 3D tests..
		/* setup set 'B' ... */
		if (si->ps.card_type == NV15)
		{
			/* ... for cmd NV11_TCL_PRIMITIVE_3D */
			ACCW(PR_CTX0_B, 0x0300a096); /* NVclass $096, patchcfg ROP_AND, userclip enable,
										  * context surface0 valid, nv10+: little endian */
			ACCW(PR_CTX1_B, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
			ACCW(PR_CTX2_B, 0x11401140); /* DMA0, DMA1 instance = $1140 */
			ACCW(PR_CTX3_B, 0x00000000); /* method traps disabled */
		}
		/* setup DMA set pointed at by PF_CACH1_DMAI */
		if (si->engine.agp_mode)
		{
			/* DMA page table present and of linear type;
			 * DMA class is $002 (b0-11);
			 * DMA target node is AGP */
			ACCW(PR_CTX0_C, 0x00033002);
		}
		else
		{
			/* DMA page table present and of linear type;
			 * DMA class is $002 (b0-11);
			 * DMA target node is PCI */
			ACCW(PR_CTX0_C, 0x00023002);
		}
		ACCW(PR_CTX1_C, 0x000fffff); /* DMA limit: tablesize is 1M bytes */
		ACCW(PR_CTX2_C, (((uint32)((uint8 *)(si->dma_buffer_pci))) | 0x00000002));
									 /* DMA access type is READ_AND_WRITE;
									  * table is located in main system RAM (b12-31):
									  * It's adress needs to be at a 4kb boundary! */

		/* set the 3D rendering functions colordepth via BPIXEL's 'depth 2' */
		/* note:
		 * setting a depth to 'invalid' (zero) makes the engine report
		 * ready with drawing 'immediately'. */
		//fixme: NV30A and above (probably) needs to be corrected...
		switch(si->dm.space)
		{
		case B_CMAP8:
			if (si->ps.card_arch < NV30A)
				/* set depth 2: $1 = Y8 */
				ACCW(BPIXEL, 0x00000100);
			else
				/* set depth 0-1: $1 = Y8, $2 = X1R5G5B5_Z1R5G5B5 */
				ACCW(BPIXEL, 0x00000021);
			break;
		case B_RGB15_LITTLE:
			if (si->ps.card_arch < NV30A)
				/* set depth 2: $4 = A1R5G5B5 */
				ACCW(BPIXEL, 0x00000400);
			else
				/* set depth 0-1: $2 = X1R5G5B5_Z1R5G5B5, $4 = A1R5G5B5 */
				ACCW(BPIXEL, 0x00000042);
			break;
		case B_RGB16_LITTLE:
			if (si->ps.card_arch < NV30A)
				/* set depth 2: $5 = R5G6B5 */
				ACCW(BPIXEL, 0x00000500);
			else
				/* set depth 0-1: $5 = R5G6B5, $a = X1A7R8G8B8_O1A7R8G8B8 */
				ACCW(BPIXEL, 0x000000a5);
			break;
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			if (si->ps.card_arch < NV30A)
				/* set depth 2: $c = A8R8G8B8 */
				ACCW(BPIXEL, 0x00000c00);
			else
				/* set depth 0-1: $7 = X8R8G8B8_Z8R8G8B8, $e = V8YB8U8YA8 */
				ACCW(BPIXEL, 0x000000e7);
			break;
		default:
			LOG(8,("ACC: init, invalid bit depth\n"));
			return B_ERROR;
		}
	}

	if (si->ps.card_arch == NV04A)
	{
		/* do a explicit engine reset */
		ACCW(DEBUG0, 0x000001ff);

		/* init some function blocks */
		/* DEBUG0, b20 and b21 should be high, this has a big influence on
		 * 3D rendering speed! (on all cards, confirmed) */
		ACCW(DEBUG0, 0x1230c000);
		/* DEBUG1, b19 = 1 increases 3D rendering speed on TNT2 (M64) a bit,
		 * TNT1 rendering speed stays the same (all cards confirmed) */
		ACCW(DEBUG1, 0x72191101);
		ACCW(DEBUG2, 0x11d5f071);
		ACCW(DEBUG3, 0x0004ff31);
		/* init OP methods */
		ACCW(DEBUG3, 0x4004ff31);

		/* disable all acceleration engine INT reguests */
		ACCW(ACC_INTE, 0x00000000);
		/* reset all acceration engine INT status bits */
		ACCW(ACC_INTS, 0xffffffff);
		/* context control enabled */
		ACCW(NV04_CTX_CTRL, 0x10010100);
		/* all acceleration buffers, pitches and colors are valid */
		ACCW(NV04_ACC_STAT, 0xffffffff);
		/* enable acceleration engine command FIFO */
		ACCW(FIFO_EN, 0x00000001);

		/* setup location of active screen in framebuffer */
		ACCW(OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		/* setup accesible card memory range */
		ACCW(BLIMIT0, (si->ps.memory_size - 1));
		ACCW(BLIMIT1, (si->ps.memory_size - 1));

		/* pattern shape value = 8x8, 2 color */
		//fixme: not needed, unless the engine has a hardware fault (setting via cmd)!
		//ACCW(PAT_SHP, 0x00000000);
		/* Pgraph Beta AND value (fraction) b23-30 */
		ACCW(BETA_AND_VAL, 0xffffffff);
	}
	else
	{
		/* do a explicit engine reset */
		ACCW(DEBUG0, 0xffffffff);
		ACCW(DEBUG0, 0x00000000);
		/* disable all acceleration engine INT reguests */
		ACCW(ACC_INTE, 0x00000000);
		/* reset all acceration engine INT status bits */
		ACCW(ACC_INTS, 0xffffffff);
		/* context control enabled */
		ACCW(NV10_CTX_CTRL, 0x10010100);
		/* all acceleration buffers, pitches and colors are valid */
		ACCW(NV10_ACC_STAT, 0xffffffff);
		/* enable acceleration engine command FIFO */
		ACCW(FIFO_EN, 0x00000001);
		/* setup surface type:
		 * b1-0 = %01 = surface type is non-swizzle;
		 * this is needed to enable 3D on NV1x (confirmed) and maybe others? */
		ACCW(NV10_SURF_TYP, ((ACCR(NV10_SURF_TYP)) & 0x0007ff00));
		ACCW(NV10_SURF_TYP, ((ACCR(NV10_SURF_TYP)) | 0x00020101));
	}

	if (si->ps.card_arch == NV10A)
	{
		/* init some function blocks */
		ACCW(DEBUG1, 0x00118700);
		/* DEBUG2 has a big influence on 3D speed for NV11 and NV15
		 * (confirmed b3 and b18 should both be '1' on both cards!)
		 * (b16 should also be '1', increases 3D speed on NV11 a bit more) */
		ACCW(DEBUG2, 0x24fd2ad9);
		ACCW(DEBUG3, 0x55de0030);
		/* NV10_DEBUG4 has a big influence on 3D speed for NV11, NV15 and NV18
		 * (confirmed b14 and b15 should both be '1' on these cards!)
		 * (confirmed b8 should be '0' on NV18 to prevent complete engine crash!) */
		ACCW(NV10_DEBUG4, 0x0000c000);

		/* copy tile setup stuff from 'source' to acc engine */
		for (cnt = 0; cnt < 32; cnt++)
		{
			NV_REG32(NVACC_NV10_TIL0AD + (cnt << 2)) =
				NV_REG32(NVACC_NV10_FBTIL0AD + (cnt << 2));
		}

		/* setup location of active screen in framebuffer */
		ACCW(OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		/* setup accesible card memory range */
		ACCW(BLIMIT0, (si->ps.memory_size - 1));
		ACCW(BLIMIT1, (si->ps.memory_size - 1));

		/* pattern shape value = 8x8, 2 color */
		//fixme: not needed, unless the engine has a hardware fault (setting via cmd)!
		//ACCW(PAT_SHP, 0x00000000);
		/* Pgraph Beta AND value (fraction) b23-30 */
		ACCW(BETA_AND_VAL, 0xffffffff);
	}

	if (si->ps.card_arch >= NV20A)
	{
		switch (si->ps.card_arch)
		{
		case NV40A:
			/* init some function blocks */
			ACCW(DEBUG1, 0x401287c0);
			ACCW(DEBUG3, 0x60de8051);
			/* disable specific functions, but enable SETUP_SPARE2 register */
			ACCW(NV10_DEBUG4, 0x00008000);
			/* set limit_viol_pix_adress(?): more likely something unknown.. */
			ACCW(NV25_WHAT0, 0x00be3c5f);

			/* setup some unknown serially accessed registers (?) */
			tmp = (NV_REG32(NV32_NV4X_WHAT0) & 0x000000ff);
			for (cnt = 0; (tmp && !(tmp & 0x00000001)); tmp >>= 1, cnt++);
			{
				ACCW(NV4X_WHAT2, cnt);
			}

			/* unknown.. */
			switch (si->ps.card_type)
			{
			case NV40:
			case NV45:
			/* and NV48: but these are pgm'd as NV45 currently */
				ACCW(NV40_WHAT0, 0x83280fff);
				ACCW(NV40_WHAT1, 0x000000a0);
				ACCW(NV40_WHAT2, 0x0078e366);
				ACCW(NV40_WHAT3, 0x0000014c);
				break;
			case NV41:
			/* and ID == 0x012x: but no cards defined yet */
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);
				ACCW(NV40P_WHAT2, 0x007596ff);
				ACCW(NV40P_WHAT3, 0x00000108);
				break;
			case NV43:
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);
				ACCW(NV40P_WHAT2, 0x0072cb77);
				ACCW(NV40P_WHAT3, 0x00000108);
				break;
			case NV44:
			case G72:
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);

				NV_REG32(NV32_NV44_WHAT10) = NV_REG32(NV32_NV10STRAPINFO);
				NV_REG32(NV32_NV44_WHAT11) = 0x00000000;
				NV_REG32(NV32_NV44_WHAT12) = 0x00000000;
				NV_REG32(NV32_NV44_WHAT13) = NV_REG32(NV32_NV10STRAPINFO);

				ACCW(NV44_WHAT2, 0x00000000);
				ACCW(NV44_WHAT3, 0x00000000);
				break;
/*			case NV44 type 2: (cardID 0x022x)
				//fixme if needed: doesn't seem to need the strapinfo thing..
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);

				ACCW(NV44_WHAT2, 0x00000000);
				ACCW(NV44_WHAT3, 0x00000000);
				break;
*/			case G70:
			case G71:
			case G73:
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);
				ACCW(NV40P_WHAT2, 0x07830610);
				ACCW(NV40P_WHAT3, 0x0000016a);
				break;
			default:
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);
				break;
			}

			ACCW(NV10_TIL3PT, 0x2ffff800);
			ACCW(NV10_TIL3ST, 0x00006000);
			ACCW(NV4X_WHAT1, 0x01000000);
			/* engine data source DMA instance = $1140 */
			ACCW(NV4X_DMA_SRC, 0x00001140);
			break;
		case NV30A:
			/* init some function blocks, but most is unknown.. */
			ACCW(DEBUG1, 0x40108700);
			ACCW(NV25_WHAT1, 0x00140000);
			ACCW(DEBUG3, 0xf00e0431);
			ACCW(NV10_DEBUG4, 0x00008000);
			ACCW(NV25_WHAT0, 0xf04b1f36);
			ACCW(NV20_WHAT3, 0x1002d888);
			ACCW(NV25_WHAT2, 0x62ff007f);
			break;
		case NV20A:
			/* init some function blocks, but most is unknown.. */
			ACCW(DEBUG1, 0x00118700);
			ACCW(DEBUG3, 0xf20e0431);
			ACCW(NV10_DEBUG4, 0x00000000);
			ACCW(NV20_WHAT1, 0x00000040);
			if (si->ps.card_type < NV25)
			{
				ACCW(NV20_WHAT2, 0x00080000);
				ACCW(NV10_DEBUG5, 0x00000005);
				ACCW(NV20_WHAT3, 0x45caa208);
				ACCW(NV20_WHAT4, 0x24000000);
				ACCW(NV20_WHAT5, 0x00000040);

				/* copy some fixed RAM(?) configuration info(?) to some indexed registers: */
				/* b16-24 is select; b2-13 is adress in 32-bit words */
				ACCW(RDI_INDEX, 0x00e00038);
				/* data is 32-bit */
				ACCW(RDI_DATA, 0x00000030);
				/* copy some fixed RAM(?) configuration info(?) to some indexed registers: */
				/* b16-24 is select; b2-13 is adress in 32-bit words */
				ACCW(RDI_INDEX, 0x00e10038);
				/* data is 32-bit */
				ACCW(RDI_DATA, 0x00000030);
			}
			else
			{
				ACCW(NV25_WHAT1, 0x00080000);
				ACCW(NV25_WHAT0, 0x304b1fb6);
				ACCW(NV20_WHAT3, 0x18b82880);
				ACCW(NV20_WHAT4, 0x44000000);
				ACCW(NV20_WHAT5, 0x40000080);
				ACCW(NV25_WHAT2, 0x000000ff);
			}
			break;
		}

		/* NV20A, NV30A and NV40A: */
		/* copy tile setup stuff from previous setup 'source' to acc engine
		 * (pattern colorRAM?) */
		if ((si->ps.card_type <= NV40) || (si->ps.card_type == NV45))
		{
			for (cnt = 0; cnt < 32; cnt++)
			{
				/* copy NV10_FBTIL0AD upto/including NV10_FBTIL7ST */
				NV_REG32(NVACC_NV20_WHAT0 + (cnt << 2)) =
					NV_REG32(NVACC_NV10_FBTIL0AD + (cnt << 2));

				/* copy NV10_FBTIL0AD upto/including NV10_FBTIL7ST */
				NV_REG32(NVACC_NV20_2_WHAT0 + (cnt << 2)) =
					NV_REG32(NVACC_NV10_FBTIL0AD + (cnt << 2));
			}
		}
		else
		{
			/* NV41, 43, 44, G70 and later */
			if (si->ps.card_type >= G70)
			{
				for (cnt = 0; cnt < 60; cnt++)
				{
					/* copy NV41_FBTIL0AD upto/including G70_FBTILEST */
					NV_REG32(NVACC_NV41_WHAT0 + (cnt << 2)) =
						NV_REG32(NVACC_NV41_FBTIL0AD + (cnt << 2));

					/* copy NV41_FBTIL0AD upto/including G70_FBTILEST */
					NV_REG32(NVACC_NV20_2_WHAT0 + (cnt << 2)) =
						NV_REG32(NVACC_NV41_FBTIL0AD + (cnt << 2));
				}
			}
			else
			{
				/* NV41, 43, 44 */
				for (cnt = 0; cnt < 48; cnt++)
				{
					/* copy NV41_FBTIL0AD upto/including NV41_FBTILBST */
					NV_REG32(NVACC_NV20_WHAT0 + (cnt << 2)) =
						NV_REG32(NVACC_NV41_FBTIL0AD + (cnt << 2));

					if (si->ps.card_type != NV44)
					{
						/* copy NV41_FBTIL0AD upto/including NV41_FBTILBST */
						NV_REG32(NVACC_NV20_2_WHAT0 + (cnt << 2)) =
							NV_REG32(NVACC_NV41_FBTIL0AD + (cnt << 2));
					}
				}
			}
		}

		if (si->ps.card_arch >= NV40A)
		{
			if ((si->ps.card_type == NV40) || (si->ps.card_type == NV45))
			{
				/* copy some RAM configuration info(?) */
 				ACCW(NV20_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV20_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
				ACCW(NV40_WHAT_T2, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV40_WHAT_T3, NV_REG32(NV32_PFB_CONFIG_1));

				/* setup location of active screen in framebuffer */
				ACCW(NV20_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				ACCW(NV20_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				/* setup accesible card memory range */
				ACCW(NV20_BLIMIT6, (si->ps.memory_size - 1));
				ACCW(NV20_BLIMIT7, (si->ps.memory_size - 1));
			}
			else
			{
				/* NV41, 43, 44, G70 and later */

				/* copy some RAM configuration info(?) */
				if (si->ps.card_type >= G70)
				{
					ACCW(G70_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
					ACCW(G70_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
				}
				else
				{
					/* NV41, 43, 44 */
					ACCW(NV40P_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
					ACCW(NV40P_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
				}
				ACCW(NV40P_WHAT_T2, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV40P_WHAT_T3, NV_REG32(NV32_PFB_CONFIG_1));

				/* setup location of active screen in framebuffer */
				ACCW(NV40P_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				ACCW(NV40P_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				/* setup accesible card memory range */
				ACCW(NV40P_BLIMIT6, (si->ps.memory_size - 1));
				ACCW(NV40P_BLIMIT7, (si->ps.memory_size - 1));
			}
		}
		else /* NV20A and NV30A: */
		{
			/* copy some RAM configuration info(?) */
			ACCW(NV20_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
			ACCW(NV20_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
			/* copy some RAM configuration info(?) to some indexed registers: */
			/* b16-24 is select; b2-13 is adress in 32-bit words */
			ACCW(RDI_INDEX, 0x00ea0000);
			/* data is 32-bit */
			ACCW(RDI_DATA, NV_REG32(NV32_PFB_CONFIG_0));
			/* b16-24 is select; b2-13 is adress in 32-bit words */
			ACCW(RDI_INDEX, 0x00ea0004);
			/* data is 32-bit */
			ACCW(RDI_DATA, NV_REG32(NV32_PFB_CONFIG_1));

			/* setup location of active screen in framebuffer */
			ACCW(NV20_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			ACCW(NV20_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			/* setup accesible card memory range */
			ACCW(NV20_BLIMIT6, (si->ps.memory_size - 1));
			ACCW(NV20_BLIMIT7, (si->ps.memory_size - 1));
		}

		/* NV20A, NV30A and NV40A: */
		/* setup some acc engine tile stuff */
		ACCW(NV10_TIL2AD, 0x00000000);
		ACCW(NV10_TIL0ED, 0xffffffff);
	}

	/* all cards: */
	/* setup clipping: rect size is 32768 x 32768, probably max. setting */
	/* note:
	 * can also be done via the NV_IMAGE_BLACK_RECTANGLE engine command. */
	ACCW(ABS_UCLP_XMIN, 0x00000000);
	ACCW(ABS_UCLP_YMIN, 0x00000000);
	ACCW(ABS_UCLP_XMAX, 0x00007fff);
	ACCW(ABS_UCLP_YMAX, 0x00007fff);

	/* setup sync parameters for NV12_IMAGE_BLIT command for the current mode:
	 * values given are CRTC vertical counter limit values. The NV12 command will wait
	 * for the specified's CRTC's vertical counter to be in between the given values */
	if (si->ps.card_type >= NV11)
	{
		ACCW(NV11_CRTC_LO, si->dm.timing.v_display - 1);
		ACCW(NV11_CRTC_HI, si->dm.timing.v_display + 1);
	}

	/*** PFIFO ***/
	/* (setup caches) */
	/* disable caches reassign */
	ACCW(PF_CACHES, 0x00000000);
	/* PFIFO mode: channel 0 is in DMA mode, channels 1 - 32 are in PIO mode */
	ACCW(PF_MODE, 0x00000001);
	/* cache1 push0 access disabled */
	ACCW(PF_CACH1_PSH0, 0x00000000);
	/* cache1 pull0 access disabled */
	ACCW(PF_CACH1_PUL0, 0x00000000);
	/* cache1 push1 mode = DMA */
	if (si->ps.card_arch >= NV40A)
		ACCW(PF_CACH1_PSH1, 0x00010000);
	else
		ACCW(PF_CACH1_PSH1, 0x00000100);
	/* cache1 DMA Put offset = 0 (b2-28) */
	ACCW(PF_CACH1_DMAP, 0x00000000);
	/* cache1 DMA Get offset = 0 (b2-28) */
	ACCW(PF_CACH1_DMAG, 0x00000000);
	/* cache1 DMA instance adress = $114e (b0-15);
	 * instance being b4-19 with baseadress NV_PRAMIN_CTX_0 (0x00700000). */
	/* note:
	 * should point to a DMA definition in CTX register space (which is sort of RAM).
	 * This define tells the engine where the DMA cmd buffer is and what it's size is.
	 * Inside that cmd buffer you'll find the actual issued engine commands. */
	if (si->ps.card_arch >= NV40A)
		ACCW(PF_CACH1_DMAI, 0x00001150);
	else
		//2007 3d test..
		ACCW(PF_CACH1_DMAI, 0x0000114e);
	/* cache0 push0 access disabled */
	ACCW(PF_CACH0_PSH0, 0x00000000);
	/* cache0 pull0 access disabled */
	ACCW(PF_CACH0_PUL0, 0x00000000);
	/* RAM HT (hash table) baseadress = $10000 (b4-8), size = 4k,
	 * search = 128 (is byte offset between hash 'sets') */
	/* note:
	 * so HT base is $00710000, last is $00710fff.
	 * In this space you define the engine command handles (HT_HANDL_XX), which
	 * in turn points to the defines in CTX register space (which is sort of RAM) */
	ACCW(PF_RAMHT, 0x03000100);
	/* RAM FC baseadress = $11000 (b3-8) (size is fixed to 0.5k(?)) */
	/* note:
	 * so FC base is $00711000, last is $007111ff. (not used?) */
	ACCW(PF_RAMFC, 0x00000110);
	/* RAM RO baseadress = $11200 (b1-8), size = 0.5k */
	/* note:
	 * so RO base is $00711200, last is $007113ff. (not used?) */
	/* note also:
	 * This means(?) the PRAMIN CTX registers are accessible from base $00711400. */
	ACCW(PF_RAMRO, 0x00000112);
	/* PFIFO size: ch0-15 = 512 bytes, ch16-31 = 124 bytes */
	ACCW(PF_SIZE, 0x0000ffff);
	/* cache1 hash instance = $ffff (b0-15) */
	ACCW(PF_CACH1_HASH, 0x0000ffff);
	/* disable all PFIFO INTs */
	ACCW(PF_INTEN, 0x00000000);
	/* reset all PFIFO INT status bits */
	ACCW(PF_INTSTAT, 0xffffffff);
	/* cache0 pull0 engine = acceleration engine (graphics) */
	ACCW(PF_CACH0_PUL1, 0x00000001);
	/* cache1 DMA control: disable some stuff */
	ACCW(PF_CACH1_DMAC, 0x00000000);
	/* cache1 engine 0 upto/including 7 is software (could also be graphics or DVD) */
	ACCW(PF_CACH1_ENG, 0x00000000);
	/* cache1 DMA fetch: trigger at 128 bytes, size is 32 bytes, max requests is 15,
	 * use little endian */
	ACCW(PF_CACH1_DMAF, 0x000f0078);
	/* cache1 DMA push: b0 = 1: access is enabled */
	ACCW(PF_CACH1_DMAS, 0x00000001);
	/* cache1 push0 access enabled */
	ACCW(PF_CACH1_PSH0, 0x00000001);
	/* cache1 pull0 access enabled */
	ACCW(PF_CACH1_PUL0, 0x00000001);
	/* cache1 pull1 engine = acceleration engine (graphics) */
	ACCW(PF_CACH1_PUL1, 0x00000001);
	/* enable PFIFO caches reassign */
	ACCW(PF_CACHES, 0x00000001);

	/* setup 3D specifics */
	nv_init_for_3D_dma();

	/*** init acceleration engine command info ***/
	/* set object handles */
	/* note:
	 * probably depending on some other setup, there are 8 or 32 FIFO channels
	 * available. Assuming the current setup only has 8 channels because the 'rest'
	 * isn't setup here... */
	si->engine.fifo.handle[0] = NV_ROP5_SOLID;
	si->engine.fifo.handle[1] = NV_IMAGE_BLACK_RECTANGLE;
	si->engine.fifo.handle[2] = NV_IMAGE_PATTERN;
	si->engine.fifo.handle[3] = NV4_SURFACE; /* NV10_CONTEXT_SURFACES_2D is identical */
	si->engine.fifo.handle[4] = NV_IMAGE_BLIT;
	si->engine.fifo.handle[5] = NV4_GDI_RECTANGLE_TEXT;
	si->engine.fifo.handle[6] = NV4_CONTEXT_SURFACES_ARGB_ZS;//NV1_RENDER_SOLID_LIN;
	si->engine.fifo.handle[7] = NV4_DX5_TEXTURE_TRIANGLE;
	/* preset no FIFO channels assigned to cmd's */
	for (cnt = 0; cnt < 0x20; cnt++)
	{
		si->engine.fifo.ch_ptr[cnt] = 0;
	}
	/* set handle's pointers to their assigned FIFO channels */
	/* note:
	 * b0-1 aren't used as adressbits. Using b0 to indicate a valid pointer. */
	for (cnt = 0; cnt < 0x08; cnt++)
	{
		si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
												(0x00000001 + (cnt * 0x00002000));
	}

	/*** init DMA command buffer info ***/
	if (si->ps.card_arch >= NV40A) //main mem DMA buf on pre-NV40
	{
		si->dma_buffer = (void *)((char *)si->framebuffer +
			((si->ps.memory_size - 1) & 0xffff8000));
	}
	LOG(4,("ACC_DMA: command buffer is at adress $%08x\n",
		((uint32)(si->dma_buffer))));
	/* we have issued no DMA cmd's to the engine yet */
	si->engine.dma.put = 0;
	/* the current first free adress in the DMA buffer is at offset 0 */
	si->engine.dma.current = 0;
	/* the DMA buffer can hold 8k 32-bit words (it's 32kb in size),
	 * or 256k 32-bit words (1Mb in size) dependant on architecture (for now) */
	/* note:
	 * one word is reserved at the end of the DMA buffer to be able to instruct the
	 * engine to do a buffer wrap-around!
	 * (DMA opcode 'noninc method': issue word $20000000.) */
	if (si->ps.card_arch < NV40A)
		si->engine.dma.max = ((1 * 1024 * 1024) >> 2) - 1;
	else
		si->engine.dma.max = 8192 - 1;
	/* note the current free space we have left in the DMA buffer */
	si->engine.dma.free = si->engine.dma.max - si->engine.dma.current;

	/*** init FIFO via DMA command buffer. ***/
	/* wait for room in fifo for new FIFO assigment cmds if needed: */
	if (si->ps.card_arch >= NV40A)
	{
		if (nv_acc_fifofree_dma(12) != B_OK) return B_ERROR;
	}
	else
	{
		if (nv_acc_fifofree_dma(16) != B_OK) return B_ERROR;
	}

	/* program new FIFO assignments */
	/* Raster OPeration: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH0, si->engine.fifo.handle[0]);
	/* Clip: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH1, si->engine.fifo.handle[1]);
	/* Pattern: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH2, si->engine.fifo.handle[2]);
	/* 2D Surfaces: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH3, si->engine.fifo.handle[3]);
	/* Blit: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH4, si->engine.fifo.handle[4]);
	/* Bitmap: */
	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH5, si->engine.fifo.handle[5]);
	if (si->ps.card_arch < NV40A)
	{
		/* 3D surfaces: (3D related only) */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH6, si->engine.fifo.handle[6]);
		/* Textured Triangle: (3D only) */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH7, si->engine.fifo.handle[7]);
	}

	/*** Set pixel width ***/
	switch(si->dm.space)
	{
	case B_CMAP8:
		surf_depth = 0x00000001;
		cmd_depth = 0x00000003;
		break;
	case B_RGB15_LITTLE:
	case B_RGB16_LITTLE:
		surf_depth = 0x00000004;
		cmd_depth = 0x00000001;
		break;
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		surf_depth = 0x00000006;
		cmd_depth = 0x00000003;
		break;
	default:
		LOG(8,("ACC_DMA: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* wait for room in fifo for surface setup cmd if needed */
	if (nv_acc_fifofree_dma(5) != B_OK) return B_ERROR;
	/* now setup 2D surface (writing 5 32bit words) */
	nv_acc_cmd_dma(NV4_SURFACE, NV4_SURFACE_FORMAT, 4);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = surf_depth; /* Format */
	/* setup screen pitch */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
		((si->fbc.bytes_per_row & 0x0000ffff) | (si->fbc.bytes_per_row << 16)); /* Pitch */
	/* setup screen location */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
		((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer); /* OffsetSource */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
		((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer); /* OffsetDest */

	/* wait for room in fifo for pattern colordepth setup cmd if needed */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* set pattern colordepth (writing 2 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLORFORMAT, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */

	/* wait for room in fifo for bitmap colordepth setup cmd if needed */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* set bitmap colordepth (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_SETCOLORFORMAT, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */

	/* Load our pattern into the engine: */
	/* wait for room in fifo for pattern cmd if needed. */
	if (nv_acc_fifofree_dma(7) != B_OK) return B_ERROR;
	/* now setup pattern (writing 7 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETSHAPE, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000000; /* SetShape: 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLOR0, 4);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xffffffff; /* SetColor0 */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xffffffff; /* SetColor1 */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xffffffff; /* SetPattern[0] */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xffffffff; /* SetPattern[1] */

	/* tell the engine to fetch and execute all (new) commands in the DMA buffer */
	nv_start_dma();

	return B_OK;
}

static void nv_init_for_3D_dma(void)
{
	/* setup PGRAPH unknown registers and modify (pre-cleared) pipe stuff for 3D use */
	if (si->ps.card_arch >= NV10A)
	{
		/* setup unknown PGRAPH stuff */
		ACCW(PGWHAT_00, 0x00000000);
		ACCW(PGWHAT_01, 0x00000000);
		ACCW(PGWHAT_02, 0x00000000);
		ACCW(PGWHAT_03, 0x00000000);

		ACCW(PGWHAT_04, 0x00001000);
		ACCW(PGWHAT_05, 0x00001000);
		ACCW(PGWHAT_06, 0x4003ff80);

		ACCW(PGWHAT_07, 0x00000000);
		ACCW(PGWHAT_08, 0x00000000);
		ACCW(PGWHAT_09, 0x00000000);
		ACCW(PGWHAT_0A, 0x00000000);
		ACCW(PGWHAT_0B, 0x00000000);

		ACCW(PGWHAT_0C, 0x00080008);
		ACCW(PGWHAT_0D, 0x00080008);

		ACCW(PGWHAT_0E, 0x00000000);
		ACCW(PGWHAT_0F, 0x00000000);
		ACCW(PGWHAT_10, 0x00000000);
		ACCW(PGWHAT_11, 0x00000000);
		ACCW(PGWHAT_12, 0x00000000);
		ACCW(PGWHAT_13, 0x00000000);
		ACCW(PGWHAT_14, 0x00000000);
		ACCW(PGWHAT_15, 0x00000000);
		ACCW(PGWHAT_16, 0x00000000);
		ACCW(PGWHAT_17, 0x00000000);
		ACCW(PGWHAT_18, 0x00000000);

		ACCW(PGWHAT_19, 0x10000000);

		ACCW(PGWHAT_1A, 0x00000000);
		ACCW(PGWHAT_1B, 0x00000000);
		ACCW(PGWHAT_1C, 0x00000000);
		ACCW(PGWHAT_1D, 0x00000000);
		ACCW(PGWHAT_1E, 0x00000000);
		ACCW(PGWHAT_1F, 0x00000000);
		ACCW(PGWHAT_20, 0x00000000);
		ACCW(PGWHAT_21, 0x00000000);

		ACCW(PGWHAT_22, 0x08000000);

		ACCW(PGWHAT_23, 0x00000000);
		ACCW(PGWHAT_24, 0x00000000);
		ACCW(PGWHAT_25, 0x00000000);
		ACCW(PGWHAT_26, 0x00000000);

		ACCW(PGWHAT_27, 0x4b7fffff);

		ACCW(PGWHAT_28, 0x00000000);
		ACCW(PGWHAT_29, 0x00000000);
		ACCW(PGWHAT_2A, 0x00000000);

		/* setup window clipping */
		/* b0-11 = min; b16-27 = max.
		 * note:
		 * probably two's complement values, so setting to max range here:
		 * which would be -2048 upto/including +2047. */
		/* horizontal */
		ACCW(WINCLIP_H_0, 0x07ff0800);
		ACCW(WINCLIP_H_1, 0x07ff0800);
		ACCW(WINCLIP_H_2, 0x07ff0800);
		ACCW(WINCLIP_H_3, 0x07ff0800);
		ACCW(WINCLIP_H_4, 0x07ff0800);
		ACCW(WINCLIP_H_5, 0x07ff0800);
		ACCW(WINCLIP_H_6, 0x07ff0800);
		ACCW(WINCLIP_H_7, 0x07ff0800);
		/* vertical */
		ACCW(WINCLIP_V_0, 0x07ff0800);
		ACCW(WINCLIP_V_1, 0x07ff0800);
		ACCW(WINCLIP_V_2, 0x07ff0800);
		ACCW(WINCLIP_V_3, 0x07ff0800);
		ACCW(WINCLIP_V_4, 0x07ff0800);
		ACCW(WINCLIP_V_5, 0x07ff0800);
		ACCW(WINCLIP_V_6, 0x07ff0800);
		ACCW(WINCLIP_V_7, 0x07ff0800);

		/* setup (initialize) pipe:
		 * needed to get valid 3D rendering on (at least) NV1x cards. Without this
		 * those cards produce rubbish instead of 3D, although the engine itself keeps
		 * running and 2D stays OK. */

		/* set eyetype to local, lightning etc. is off */
		ACCW(NV10_XFMOD0, 0x10000000);
		/* disable all lights */
		ACCW(NV10_XFMOD1, 0x00000000);

		/* note: upon writing data into the PIPEDAT register, the PIPEADR is
		 * probably auto-incremented! */
		/* (pipe adress = b2-16, pipe data = b0-31) */
		/* note: pipe adresses IGRAPH registers! */
		ACCW(NV10_PIPEADR, 0x00006740);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x3f800000);

		ACCW(NV10_PIPEADR, 0x00006750);
		ACCW(NV10_PIPEDAT, 0x40000000);
		ACCW(NV10_PIPEDAT, 0x40000000);
		ACCW(NV10_PIPEDAT, 0x40000000);
		ACCW(NV10_PIPEDAT, 0x40000000);

		ACCW(NV10_PIPEADR, 0x00006760);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006770);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006780);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x000067a0);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);

		ACCW(NV10_PIPEADR, 0x00006ab0);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);

		ACCW(NV10_PIPEADR, 0x00006ac0);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006c10);
		ACCW(NV10_PIPEDAT, 0xbf800000);

		ACCW(NV10_PIPEADR, 0x00007030);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007040);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007050);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007060);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007070);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007080);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00007090);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x000070a0);
		ACCW(NV10_PIPEDAT, 0x7149f2ca);

		ACCW(NV10_PIPEADR, 0x00006a80);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x3f800000);

		ACCW(NV10_PIPEADR, 0x00006aa0);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		/* select primitive type that will be drawn (tri's) */
		ACCW(NV10_PIPEADR, 0x00000040);
		ACCW(NV10_PIPEDAT, 0x00000005);

		ACCW(NV10_PIPEADR, 0x00006400);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x4b7fffff);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006410);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006420);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006430);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x000064c0);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x3f800000);
		ACCW(NV10_PIPEDAT, 0x477fffff);
		ACCW(NV10_PIPEDAT, 0x3f800000);

		ACCW(NV10_PIPEADR, 0x000064d0);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0xc5000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x000064e0);
		ACCW(NV10_PIPEDAT, 0xc4fff000);
		ACCW(NV10_PIPEDAT, 0xc4fff000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x000064f0);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);
		ACCW(NV10_PIPEDAT, 0x00000000);

		/* turn lightning on */
		ACCW(NV10_XFMOD0, 0x30000000);
		/* set light 1 to infinite type, other lights remain off */
		ACCW(NV10_XFMOD1, 0x00000004);

		/* Z-buffer state is:
		 * initialized, set to: 'fixed point' (integer?); Z-buffer; 16bits depth */
		/* note:
		 * other options possible are: floating point; 24bits depth; W-buffer */
		ACCW(GLOB_STAT_0, 0x10000000);
		/* set DMA instance 2 and 3 to be invalid */
		ACCW(GLOB_STAT_1, 0x00000000);
	}
}

static void nv_start_dma(void)
{
	uint32 dummy;

	if (si->engine.dma.current != si->engine.dma.put)
	{
		si->engine.dma.put = si->engine.dma.current;
		/* flush used caches so we know for sure the DMA cmd buffer received all data. */
		if (si->ps.card_arch < NV40A)
		{
			/* some CPU's support out-of-order processing (WinChip/Cyrix). Flush them. */
			__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");
			/* read a non-cached adress to flush the cash */
			dummy = ACCR(STATUS);
		}
		else
		{
			/* dummy read the first adress of the framebuffer to flush MTRR-WC buffers */
			dummy = *((volatile uint32 *)(si->framebuffer));
		}

		/* actually start DMA to execute all commands now in buffer */
		/* note:
		 * it doesn't matter which FIFO channel's DMA registers we access, they are in
		 * fact all the same set. It also doesn't matter if the channel was assigned a
		 * command or not. */
		/* note also:
		 * NV_GENERAL_DMAPUT is a write-only register on some cards (confirmed NV11). */
		NV_REG32(NVACC_FIFO + NV_GENERAL_DMAPUT) = (si->engine.dma.put << 2);
	}
}

/* this routine does not check the engine's internal hardware FIFO, but the DMA
 * command buffer. You can see this as a FIFO as well, that feeds the hardware FIFO.
 * The hardware FIFO state is checked by the DMA hardware automatically. */
static status_t nv_acc_fifofree_dma(uint16 cmd_size)
{
	uint32 dmaget;

	/* we'd better check for timeouts on the DMA engine as it's theoretically
	 * breakable by malfunctioning software */
	uint16 cnt = 0;

	/* check if the DMA buffer has enough room for the command.
	 * note:
	 * engine.dma.free is 'cached' */
	while ((si->engine.dma.free < cmd_size) && (cnt < 10000) && (err < 3))
	{
		/* see where the engine is currently fetching from the buffer */
		/* note:
		 * read this only once in the code as accessing registers is relatively slow */
		/* note also:
		 * it doesn't matter which FIFO channel's DMA registers we access, they are in
		 * fact all the same set. It also doesn't matter if the channel was assigned a
		 * command or not. */
		dmaget = ((NV_REG32(NVACC_FIFO + NV_GENERAL_DMAGET)) >> 2);

		/* update timeout counter: on NV11 on a Pentium4 2.8Ghz max reached count
		 * using BeRoMeter 1.2.6 was about 600; so counting 10000 before generating
		 * a timeout should definately do it. Snooze()-ing cannot be done without a
		 * serious speed penalty, even if done for only 1 microSecond. */
		cnt++;

		/* where's the engine fetching viewed from us issuing? */
		if (si->engine.dma.put >= dmaget)
		{
			/* engine is fetching 'behind us', the last piece of the buffer is free */

			/* note the 'updated' free space we have in the DMA buffer */
			si->engine.dma.free = si->engine.dma.max - si->engine.dma.current;
			/* if it's enough after all we exit this routine immediately. Else: */
			if (si->engine.dma.free < cmd_size)
			{
				/* not enough room left, so instruct DMA engine to reset the buffer
				 * when it's reaching the end of it */
				((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x20000000;
				/* reset our buffer pointer, so new commands will be placed at the
				 * beginning of the buffer. */
				si->engine.dma.current = 0;
				/* tell the engine to fetch the remaining command(s) in the DMA buffer
				 * that where not executed before. */
				nv_start_dma();

				/* NOW the engine is fetching 'in front of us', so the first piece
				 * of the buffer is free */

				/* note the updated current free space we have in the DMA buffer */
				si->engine.dma.free = dmaget - si->engine.dma.current;
				/* mind this pittfall:
				 * Leave some room between where the engine is fetching and where we
				 * put new commands. Otherwise the engine will crash on heavy loads.
				 * A crash can be forced best in 640x480x32 mode with BeRoMeter 1.2.6.
				 * (confirmed on NV11 and NV43 with less than 256 words forced freespace.)
				 * Note:
				 * The engine is DMA triggered for fetching chunks every 128 bytes,
				 * maybe this is the reason for this behaviour.
				 * Note also:
				 * it looks like the space that needs to be kept free is coupled
				 * with the size of the DMA buffer. */
				if (si->engine.dma.free < 256)
					si->engine.dma.free = 0;
				else
					si->engine.dma.free -= 256;
			}
		}
		else
		{
			/* engine is fetching 'in front of us', so the first piece of the buffer
			 * is free */

			/* note the updated current free space we have in the DMA buffer */
			si->engine.dma.free = dmaget - si->engine.dma.current;
			/* mind this pittfall:
			 * Leave some room between where the engine is fetching and where we
			 * put new commands. Otherwise the engine will crash on heavy loads.
			 * A crash can be forced best in 640x480x32 mode with BeRoMeter 1.2.6.
			 * (confirmed on NV11 and NV43 with less than 256 words forced freespace.)
			 * Note:
			 * The engine is DMA triggered for fetching chunks every 128 bytes,
			 * maybe this is the reason for this behaviour.
			 * Note also:
			 * it looks like the space that needs to be kept free is coupled
			 * with the size of the DMA buffer. */
			if (si->engine.dma.free < 256)
				si->engine.dma.free = 0;
			else
				si->engine.dma.free -= 256;
		}
	}

	/* log timeout if we had one */
	if (cnt == 10000)
	{
		if (err < 3) err++;
		LOG(4,("ACC_DMA: fifofree; DMA timeout #%d, engine trouble!\n", err));
	}

	/* we must make the acceleration routines abort or the driver will hang! */
	if (err >= 3) return B_ERROR;

	return B_OK;
}

static void nv_acc_cmd_dma(uint32 cmd, uint16 offset, uint16 size)
{
	/* NV_FIFO_DMA_OPCODE: set number of cmd words (b18 - 28); set FIFO offset for
	 * first cmd word (b2 - 15); set DMA opcode = method (b29 - 31).
	 * a 'NOP' is the opcode word $00000000. */
	/* note:
	 * possible DMA opcodes:
	 * b'000' is 'method' (execute cmd);
	 * b'001' is 'jump';
	 * b'002' is 'noninc method' (execute buffer wrap-around);
	 * b'003' is 'call': return is executed by opcode word $00020000 (b17 = 1). */
	/* note also:
	 * this system uses auto-increments for the FIFO offset adresses. Make sure
	 * to set a new adress if a gap exists between the previous one and the new one. */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = ((size << 18) |
		((si->engine.fifo.ch_ptr[cmd] + offset) & 0x0000fffc));

	/* space left after issuing the current command is the cmd AND it's arguments less */
	si->engine.dma.free -= (size + 1);
}

static void nv_acc_set_ch_dma(uint16 ch, uint32 handle)
{
	/* issue FIFO channel assign cmd */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = ((1 << 18) | ch);
	/* set new assignment */
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = (0x80000000 | handle);

	/* space left after issuing the current command is the cmd AND it's arguments less */
	si->engine.dma.free -= 2;
}

/* note:
 * switching fifo channel assignments this way has no noticable slowdown:
 * measured 0.2% with Quake2. */
void nv_acc_assert_fifo_dma(void)
{
	/* does every engine cmd this accelerant needs have a FIFO channel? */
	//fixme: can probably be optimized for both speed and channel selection...
	if (!si->engine.fifo.ch_ptr[NV_ROP5_SOLID] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLACK_RECTANGLE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_PATTERN] ||
		!si->engine.fifo.ch_ptr[NV4_SURFACE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLIT] ||
		!si->engine.fifo.ch_ptr[NV4_GDI_RECTANGLE_TEXT] ||
		!si->engine.fifo.ch_ptr[NV_SCALED_IMAGE_FROM_MEMORY])
	{
		uint16 cnt;

		/* free the FIFO channels we want from the currently assigned cmd's */
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[0]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[1]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[2]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[3]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[4]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[5]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[6]] = 0;

		/* set new object handles */
		si->engine.fifo.handle[0] = NV_ROP5_SOLID;
		si->engine.fifo.handle[1] = NV_IMAGE_BLACK_RECTANGLE;
		si->engine.fifo.handle[2] = NV_IMAGE_PATTERN;
		si->engine.fifo.handle[3] = NV4_SURFACE;
		si->engine.fifo.handle[4] = NV_IMAGE_BLIT;
		si->engine.fifo.handle[5] = NV4_GDI_RECTANGLE_TEXT;
		si->engine.fifo.handle[6] = NV_SCALED_IMAGE_FROM_MEMORY;

		/* set handle's pointers to their assigned FIFO channels */
		/* note:
		 * b0-1 aren't used as adressbits. Using b0 to indicate a valid pointer. */
		for (cnt = 0; cnt < 0x08; cnt++)
		{
			si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
				(0x00000001 + (cnt * 0x00002000));
		}

		/* wait for room in fifo for new FIFO assigment cmds if needed. */
		if (nv_acc_fifofree_dma(14) != B_OK) return;

		/* program new FIFO assignments */
		/* Raster OPeration: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH0, si->engine.fifo.handle[0]);
		/* Clip: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH1, si->engine.fifo.handle[1]);
		/* Pattern: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH2, si->engine.fifo.handle[2]);
		/* 2D Surface: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH3, si->engine.fifo.handle[3]);
		/* Blit: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH4, si->engine.fifo.handle[4]);
		/* Bitmap: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH5, si->engine.fifo.handle[5]);
		/* Scaled and fitered Blit: */
		nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH6, si->engine.fifo.handle[6]);

		/* tell the engine to fetch and execute all (new) commands in the DMA buffer */
		nv_start_dma();
	}
}

/*
	note:
	moved acceleration 'top-level' routines to be integrated in the engine:
	it is costly to call the engine for every single function within a loop!
	(measured with BeRoMeter 1.2.6: upto 15% speed increase on all CPU's.)

	note also:
	splitting up each command list into sublists (see routines below) prevents
	a lot more nested calls, further increasing the speed with upto 70%.
	
	finally:
	sending the sublist to just one single engine command even further increases
	speed with upto another 10%. This can't be done for blits though, as this engine-
	command's hardware does not support multiple objects.
*/

/* screen to screen blit - i.e. move windows around and scroll within them. */
void SCREEN_TO_SCREEN_BLIT_DMA(engine_token *et, blit_params *list, uint32 count)
{
	uint32 i = 0;
	uint16 subcnt;

	/*** init acc engine for blit function ***/
	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return;
	/* now setup ROP (writing 2 32bit words) for GXcopy */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xcc; /* SetRop5 */

	/*** do each blit ***/
	/* Note:
	 * blit-copy direction is determined inside nvidia hardware: no setup needed */
	while (count)
	{
		/* break up the list in sublists to minimize calls, while making sure long
		 * lists still get executed without trouble */
		subcnt = 32;
		if (count < 32) subcnt = count;
		count -= subcnt;

		/* wait for room in fifo for blit cmd if needed. */
		if (nv_acc_fifofree_dma(4 * subcnt) != B_OK) return;

		while (subcnt--)
		{
			/* now setup blit (writing 4 32bit words) */
			nv_acc_cmd_dma(NV_IMAGE_BLIT, NV_IMAGE_BLIT_SOURCEORG, 3);
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].src_top) << 16) | (list[i].src_left)); /* SourceOrg */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].dest_top) << 16) | (list[i].dest_left)); /* DestOrg */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				((((list[i].height) + 1) << 16) | ((list[i].width) + 1)); /* HeightWidth */

			i++;
		}

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}

/* scaled and filtered screen to screen blit - i.e. video playback without overlay */
/* note: source and destination may not overlap. */
//fixme? checkout NV5 and NV10 version of cmd: faster?? (or is 0x77 a 'autoselect' version?)
void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT_DMA(engine_token *et, scaled_blit_params *list, uint32 count)
{
	uint32 i = 0;
	uint16 subcnt;
	uint32 cmd_depth;
	uint8 bpp;

	/*** init acc engine for scaled filtered blit function ***/
	/* Set pixel width */
	switch(si->dm.space)
	{
	case B_RGB15_LITTLE:
		cmd_depth = 0x00000002;
		bpp = 2;
		break;
	case B_RGB16_LITTLE:
		cmd_depth = 0x00000007;
		bpp = 2;
		break;
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		cmd_depth = 0x00000004;
		bpp = 4;
		break;
	/* fixme sometime:
	 * we could do the spaces below if this function would be modified to be able
	 * to use a source outside of the desktop, i.e. using offscreen bitmaps... */
	case B_YCbCr422:
		cmd_depth = 0x00000005;
		bpp = 2;
		break;
	case B_YUV422:
		cmd_depth = 0x00000006;
		bpp = 2;
		break;
	default:
		/* note: this function does not support src or dest in the B_CMAP8 space! */
		//fixme: the NV10 version of this cmd supports B_CMAP8 src though... (checkout)
		LOG(8,("ACC_DMA: scaled_filtered_blit, invalid bit depth\n"));
		return;
	}

	/* modify surface depth settings for 15-bit colorspace so command works as intended */
	if (si->dm.space == B_RGB15_LITTLE)
	{
		/* wait for room in fifo for surface setup cmd if needed */
		if (nv_acc_fifofree_dma(2) != B_OK) return;
		/* now setup 2D surface (writing 1 32bit word) */
		nv_acc_cmd_dma(NV4_SURFACE, NV4_SURFACE_FORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000002; /* Format */
	}

	/* TNT1 has fixed operation mode 'SRCcopy' while the rest can be programmed: */
	if (si->ps.card_type != NV04)
	{
		/* wait for room in fifo for cmds if needed. */
		if (nv_acc_fifofree_dma(5) != B_OK) return;
		/* now setup source bitmap colorspace */
		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT, 2);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */
		/* now setup operation mode to SRCcopy */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000003; /* SetOperation */
	}
	else
	{
		/* wait for room in fifo for cmd if needed. */
		if (nv_acc_fifofree_dma(4) != B_OK) return;
		/* now setup source bitmap colorspace */
		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */
		/* TNT1 has fixed operation mode SRCcopy */
	}
	/* now setup fill color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000000; /* Color1A */

	/*** do each blit ***/
	while (count)
	{
		/* break up the list in sublists to minimize calls, while making sure long
		 * lists still get executed without trouble */
		subcnt = 16;
		if (count < 16) subcnt = count;
		count -= subcnt;

		/* wait for room in fifo for blit cmd if needed. */
		if (nv_acc_fifofree_dma(12 * subcnt) != B_OK) return;

		while (subcnt--)
		{
			/* now setup blit (writing 12 32bit words) */
			nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCEORG, 6);
			/* setup dest clipping ref for blit (not used) (b0-15 = left, b16-31 = top) */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0; /* SourceOrg */
			/* setup dest clipping size for blit */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].dest_height + 1) << 16) | (list[i].dest_width + 1)); /* SourceHeightWidth */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			/* setup destination location and size for blit */
				(((list[i].dest_top) << 16) | (list[i].dest_left)); /* DestOrg */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].dest_height + 1) << 16) | (list[i].dest_width + 1)); /* DestHeightWidth */
			//fixme: findout scaling limits... (although the current cmd interface doesn't support them.)
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].src_width + 1) << 20) / (list[i].dest_width + 1)); /* HorInvScale (in 12.20 format) */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].src_height + 1) << 20) / (list[i].dest_height + 1)); /* VerInvScale (in 12.20 format) */

			nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCESIZE, 4);
			/* setup horizontal and vertical source (fetching) ends.
			 * note:
			 * horizontal granularity is 2 pixels, vertical granularity is 1 pixel.
			 * look at Matrox or Neomagic bes engines code for usage example. */
			//fixme: tested 15, 16 and 32-bit RGB depth, verify other depths...
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].src_height + 1) << 16) |
				 (((list[i].src_width + 1) + 0x0001) & ~0x0001)); /* SourceHeightWidth */
			/* setup source pitch (b0-15). Set 'format origin center' (b16-17) and
			 * select 'format interpolator foh (bilinear filtering)' (b24). */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(si->fbc.bytes_per_row | (1 << 16) | (1 << 24)); /* SourcePitch */
			/* setup source surface location */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				((uint32)((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer)) +
				(list[i].src_top * si->fbc.bytes_per_row) +	(list[i].src_left * bpp); /* Offset */
			/* setup source start: first (sub)pixel contributing to output picture */
			/* note:
			 * clipping is not asked for.
			 * look at nVidia NV10+ bes engine code for useage example. */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				0; /* SourceRef (b0-15 = hor, b16-31 = ver: both in 12.4 format) */

			i++;
		}

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* reset surface depth settings so the other engine commands works as intended */
	if (si->dm.space == B_RGB15_LITTLE)
	{
		/* wait for room in fifo for surface setup cmd if needed */
		if (nv_acc_fifofree_dma(2) != B_OK) return;
		/* now setup 2D surface (writing 1 32bit word) */
		nv_acc_cmd_dma(NV4_SURFACE, NV4_SURFACE_FORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000004; /* Format */

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}

/* scaled and filtered screen to screen blit - i.e. video playback without overlay */
/* note: source and destination may not overlap. */
//fixme? checkout NV5 and NV10 version of cmd: faster?? (or is 0x77 a 'autoselect' version?)
void OFFSCREEN_TO_SCREEN_SCALED_FILTERED_BLIT_DMA(
	engine_token *et, offscreen_buffer_config *config, clipped_scaled_blit_params *list, uint32 count)
{
	uint32 i = 0;
	uint32 cmd_depth;
	uint8 bpp;

	LOG(4,("ACC_DMA: offscreen src buffer location $%08x\n", (uint32)((uint8*)(config->buffer))));

	/*** init acc engine for scaled filtered blit function ***/
	/* Set pixel width */
	switch(config->space)
	{
	case B_RGB15_LITTLE:
		cmd_depth = 0x00000002;
		bpp = 2;
		break;
	case B_RGB16_LITTLE:
		cmd_depth = 0x00000007;
		bpp = 2;
		break;
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		cmd_depth = 0x00000004;
		bpp = 4;
		break;
	/* fixme sometime:
	 * we could do the spaces below if this function would be modified to be able
	 * to use a source outside of the desktop, i.e. using offscreen bitmaps... */
	case B_YCbCr422:
		cmd_depth = 0x00000005;
		bpp = 2;
		break;
	case B_YUV422:
		cmd_depth = 0x00000006;
		bpp = 2;
		break;
	default:
		/* note: this function does not support src or dest in the B_CMAP8 space! */
		//fixme: the NV10 version of this cmd supports B_CMAP8 src though... (checkout)
		LOG(8,("ACC_DMA: scaled_filtered_blit, invalid bit depth\n"));
		return;
	}

	/* modify surface depth settings for 15-bit colorspace so command works as intended */
	if (si->dm.space == B_RGB15_LITTLE)
	{
		/* wait for room in fifo for surface setup cmd if needed */
		if (nv_acc_fifofree_dma(2) != B_OK) return;
		/* now setup 2D surface (writing 1 32bit word) */
		nv_acc_cmd_dma(NV4_SURFACE, NV4_SURFACE_FORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000002; /* Format */
	}

	/* TNT1 has fixed operation mode 'SRCcopy' while the rest can be programmed: */
	if (si->ps.card_type != NV04)
	{
		/* wait for room in fifo for cmds if needed. */
		if (nv_acc_fifofree_dma(5) != B_OK) return;
		/* now setup source bitmap colorspace */
		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT, 2);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */
		/* now setup operation mode to SRCcopy */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000003; /* SetOperation */
	}
	else
	{
		/* wait for room in fifo for cmd if needed. */
		if (nv_acc_fifofree_dma(4) != B_OK) return;
		/* now setup source bitmap colorspace */
		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */
		/* TNT1 has fixed operation mode SRCcopy */
	}
	/* now setup fill color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000000; /* Color1A */

	/*** do each blit ***/
	while (count--)
	{
		uint32 j = 0;
		uint16 clipcnt = list[i].dest_clipcount;

		LOG(4,("ACC_DMA: offscreen src left %d, top %d\n", list[i].src_left, list[i].src_top));
		LOG(4,("ACC_DMA: offscreen src width %d, height %d\n", list[i].src_width + 1, list[i].src_height + 1));
		LOG(4,("ACC_DMA: offscreen dest left %d, top %d\n", list[i].dest_left, list[i].dest_top));
		LOG(4,("ACC_DMA: offscreen dest width %d, height %d\n", list[i].dest_width + 1, list[i].dest_height + 1));

		/* wait for room in fifo for blit cmd if needed. */
		if (nv_acc_fifofree_dma(9 + (5 * clipcnt)) != B_OK) return;

		/* now setup blit (writing 12 32bit words) */
		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCEORG + 8, 4);
		/* setup destination location and size for blit */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			((list[i].dest_top << 16) | list[i].dest_left); /* DestTopLeftOutputRect */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(((list[i].dest_height + 1) << 16) | (list[i].dest_width + 1)); /* DestHeightWidthOutputRect */
		/* setup scaling */
		//fixme: findout scaling limits... (although the current cmd interface doesn't support them.)
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(((list[i].src_width + 1) << 20) / (list[i].dest_width + 1)); /* HorInvScale (in 12.20 format) */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(((list[i].src_height + 1) << 20) / (list[i].dest_height + 1)); /* VerInvScale (in 12.20 format) */

		nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCESIZE, 3);
		/* setup horizontal and vertical source (fetching) ends.
		 * note:
		 * horizontal granularity is 2 pixels, vertical granularity is 1 pixel.
		 * look at Matrox or Neomagic bes engines code for usage example. */
		//fixme: tested 15, 16 and 32-bit RGB depth, verify other depths...
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(((list[i].src_height + 1) << 16) |
			 (((list[i].src_width + 1) + 0x0001) & ~0x0001)); /* SourceHeightWidth */
		/* setup source pitch (b0-15). Set 'format origin center' (b16-17) and
		 * select 'format interpolator foh (bilinear filtering)' (b24). */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(config->bytes_per_row | (1 << 16) | (1 << 24)); /* SourcePitch */

		/* setup source surface location */
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
			(uint32)((uint8*)config->buffer - (uint8*)si->framebuffer +
			(list[i].src_top * config->bytes_per_row) +	(list[i].src_left * bpp)); /* Offset */

		while (clipcnt--)
		{
			LOG(4,("ACC_DMA: offscreen clip left %d, top %d\n",
				list[i].dest_cliplist[j].left, list[i].dest_cliplist[j].top));
			LOG(4,("ACC_DMA: offscreen clip width %d, height %d\n",
				list[i].dest_cliplist[j].width + 1, list[i].dest_cliplist[j].height + 1));

			/* now setup blit (writing 12 32bit words) */
			nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCEORG, 2);
			/* setup dest clipping rect for blit (b0-15 = left, b16-31 = top) */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
					(list[i].dest_cliplist[j].top << 16) | list[i].dest_cliplist[j].left; /* DestTopLeftClipRect */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
					((list[i].dest_cliplist[j].height + 1) << 16) | (list[i].dest_cliplist[j].width + 1); /* DestHeightWidthClipRect */

			nv_acc_cmd_dma(NV_SCALED_IMAGE_FROM_MEMORY, NV_SCALED_IMAGE_FROM_MEMORY_SOURCESIZE + 12, 1);
			/* setup source start: first (sub)pixel contributing to output picture */
			/* note:
			 * clipping is not asked for.
			 * look at nVidia NV10+ bes engine code for useage example. */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				0; /* SourceRef (b0-15 = hor, b16-31 = ver: both in 12.4 format) */

			j++;
		}

		i++;
	}

	/* tell the engine to fetch the commands in the DMA buffer that where not
	 * executed before. */
	nv_start_dma();

	/* reset surface depth settings so the other engine commands works as intended */
	if (si->dm.space == B_RGB15_LITTLE)
	{
		/* wait for room in fifo for surface setup cmd if needed */
		if (nv_acc_fifofree_dma(2) != B_OK) return;
		/* now setup 2D surface (writing 1 32bit word) */
		nv_acc_cmd_dma(NV4_SURFACE, NV4_SURFACE_FORMAT, 1);
		((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000004; /* Format */

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}

/* rectangle fill - i.e. workspace and window background color */
void FILL_RECTANGLE_DMA(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count)
{
	uint32 i = 0;
	uint16 subcnt;

	/*** init acc engine for fill function ***/
	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP and bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(4) != B_OK) return;
	/* now setup ROP (writing 2 32bit words) for GXcopy */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xcc; /* SetRop5 */
	/* now setup fill color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = colorIndex; /* Color1A */

	/*** draw each rectangle ***/
	while (count)
	{
		/* break up the list in sublists to minimize calls, while making sure long
		 * lists still get executed without trouble */
		subcnt = 32;
		if (count < 32) subcnt = count;
		count -= subcnt;

		/* wait for room in fifo for bitmap cmd if needed. */
		if (nv_acc_fifofree_dma(1 + (2 * subcnt)) != B_OK) return;

		/* issue fill command once... */
		nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, (2 * subcnt));
		/* ... and send multiple rects (engine cmd supports 32 max) */
		while (subcnt--)
		{
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].left) << 16) | ((list[i].top) & 0x0000ffff)); /* Unclipped Rect 0 LeftTop */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((((list[i].right)+1) - (list[i].left)) << 16) |
				(((list[i].bottom-list[i].top)+1) & 0x0000ffff)); /* Unclipped Rect 0 WidthHeight */

			i++;
		}

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}

/* span fill - i.e. (selected) menuitem background color (Dano) */
void FILL_SPAN_DMA(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count)
{
	uint32 i = 0;
	uint16 subcnt;

	/*** init acc engine for fill function ***/
	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP and bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(4) != B_OK) return;
	/* now setup ROP (writing 2 32bit words) for GXcopy */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0xcc; /* SetRop5 */
	/* now setup fill color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = colorIndex; /* Color1A */

	/*** draw each span ***/
	while (count)
	{
		/* break up the list in sublists to minimize calls, while making sure long
		 * lists still get executed without trouble */
		subcnt = 32;
		if (count < 32) subcnt = count;
		count -= subcnt;

		/* wait for room in fifo for bitmap cmd if needed. */
		if (nv_acc_fifofree_dma(1 + (2 * subcnt)) != B_OK) return;

		/* issue fill command once... */
		nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, (2 * subcnt));
		/* ... and send multiple rects (spans) (engine cmd supports 32 max) */
		while (subcnt--)
		{
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i+1]) << 16) | ((list[i]) & 0x0000ffff)); /* Unclipped Rect 0 LeftTop */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				((((list[i+2]+1) - (list[i+1])) << 16) | 0x00000001); /* Unclipped Rect 0 WidthHeight */

			i+=3;
		}

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}

/* rectangle invert - i.e. text cursor and text selection */
void INVERT_RECTANGLE_DMA(engine_token *et, fill_rect_params *list, uint32 count)
{
	uint32 i = 0;
	uint16 subcnt;

	/*** init acc engine for invert function ***/
	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP and bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(4) != B_OK) return;
	/* now setup ROP (writing 2 32bit words) for GXinvert */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x55; /* SetRop5 */
	/* now reset fill color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	((uint32*)(si->dma_buffer))[si->engine.dma.current++] = 0x00000000; /* Color1A */

	/*** invert each rectangle ***/
	while (count)
	{
		/* break up the list in sublists to minimize calls, while making sure long
		 * lists still get executed without trouble */
		subcnt = 32;
		if (count < 32) subcnt = count;
		count -= subcnt;

		/* wait for room in fifo for bitmap cmd if needed. */
		if (nv_acc_fifofree_dma(1 + (2 * subcnt)) != B_OK) return;

		/* issue fill command once... */
		nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, (2 * subcnt));
		/* ... and send multiple rects (engine cmd supports 32 max) */
		while (subcnt--)
		{
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((list[i].left) << 16) | ((list[i].top) & 0x0000ffff)); /* Unclipped Rect 0 LeftTop */
			((uint32*)(si->dma_buffer))[si->engine.dma.current++] =
				(((((list[i].right)+1) - (list[i].left)) << 16) |
				(((list[i].bottom-list[i].top)+1) & 0x0000ffff)); /* Unclipped Rect 0 WidthHeight */

			i++;
		}

		/* tell the engine to fetch the commands in the DMA buffer that where not
		 * executed before. */
		nv_start_dma();
	}

	/* tell 3D add-ons that they should reload their rendering states and surfaces */
	si->engine.threeD.reload = 0xffffffff;
}
