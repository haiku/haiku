/* NV Acceleration functions */

/* Author:
   Rudolf Cornelissen 8/2003-2/2005.

   This code was possible thanks to:
    - the Linux XFree86 NV driver,
    - the Linux UtahGLX 3D driver.
*/

/*
	note:
	attempting DMA because without it I can't get NV40 and higher going ATM.
	Maybe later we can forget about the non-DMA version: that depends on
	3D acceleration attempts).
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
	uint16 cnt;
	uint32 surf_depth, cmd_depth;
	/* reset the engine DMA stalls counter */
	err = 0;

	/* a hanging engine only recovers from a complete power-down/power-up cycle */
	NV_REG32(NV32_PWRUPCTRL) = 0x13110011;
	snooze(1000);
	NV_REG32(NV32_PWRUPCTRL) = 0x13111111;

	/* setup PTIMER: */
	//fixme? how about NV28 setup as just after coldstarting? (see nv_info.c)
	/* set timer numerator to 8 (in b0-15) */
	ACCW(PT_NUMERATOR, 0x00000008);
	/* set timer denominator to 3 (in b0-15) */
	ACCW(PT_DENOMINATR, 0x00000003);

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
		ACCW(HT_VALUE_01, 0x00101148); /* instance $1146, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_02, (0x80000000 | NV4_GDI_RECTANGLE_TEXT)); /* 32bit handle */
		ACCW(HT_VALUE_02, 0x0010114a); /* instance $1147, engine = acc engine, CHID = $00 */

		/* (second set) */
		ACCW(HT_HANDL_10, (0x80000000 | NV_ROP5_SOLID)); /* 32bit handle */
		ACCW(HT_VALUE_10, 0x00101142); /* instance $1142, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_11, (0x80000000 | NV_IMAGE_BLACK_RECTANGLE)); /* 32bit handle */
		ACCW(HT_VALUE_11, 0x00101144); /* instance $1143, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_12, (0x80000000 | NV_IMAGE_PATTERN)); /* 32bit handle */
		ACCW(HT_VALUE_12, 0x00101146); /* instance $1144, engine = acc engine, CHID = $00 */
	}
	else
	{
		/* (first set) */
		ACCW(HT_HANDL_00, (0x80000000 | NV4_SURFACE)); /* 32bit handle */
		ACCW(HT_VALUE_00, 0x8001114c); /* instance $114c, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_01, (0x80000000 | NV_IMAGE_BLIT)); /* 32bit handle */
		ACCW(HT_VALUE_01, 0x80011148); /* instance $1146, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_02, (0x80000000 | NV4_GDI_RECTANGLE_TEXT)); /* 32bit handle */
		ACCW(HT_VALUE_02, 0x8001114a); /* instance $1147, engine = acc engine, CHID = $00 */

		/* (second set) */
		ACCW(HT_HANDL_10, (0x80000000 | NV_ROP5_SOLID)); /* 32bit handle */
		ACCW(HT_VALUE_10, 0x80011142); /* instance $1142, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_11, (0x80000000 | NV_IMAGE_BLACK_RECTANGLE)); /* 32bit handle */
		ACCW(HT_VALUE_11, 0x80011144); /* instance $1143, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_12, (0x80000000 | NV_IMAGE_PATTERN)); /* 32bit handle */
		ACCW(HT_VALUE_12, 0x80011146); /* instance $1144, engine = acc engine, CHID = $00 */
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
		/* setup set '4' for cmd NV_IMAGE_BLIT */
		ACCW(PR_CTX0_6, 0x0208005f); /* NVclass $05f, patchcfg ROP_AND, nv10+: little endian */
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
		/* setup DMA set pointed at by PF_CACH1_DMAI */
		ACCW(PR_CTX0_C, 0x00003002); /* DMA page table present and of linear type;
									  * DMA class is $002 (b0-11);
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_C, 0x00007fff); /* DMA limit: tablesize is 32k bytes */
		ACCW(PR_CTX2_C, (((si->ps.memory_size - 1) & 0xffff8000) | 0x00000002));
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
		ACCW(PR_CTX0_2, 0x01008019); /* NVclass $019, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_2, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_2, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_2, 0x00000000); /* method traps disabled */
		/* setup set '2' for cmd NV_IMAGE_PATTERN */
		ACCW(PR_CTX0_4, 0x01008018); /* NVclass $018, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_4, 0x00000002); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_4, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_4, 0x00000000); /* method traps disabled */
		/* setup set '4' for cmd NV_IMAGE_BLIT */
		ACCW(PR_CTX0_6, 0x0100805f); /* NVclass $05f, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_6, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_6, 0x11401140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_6, 0x00000000); /* method trap 0 is $1140, trap 1 disabled */
		/* setup set '5' for cmd NV4_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_8, 0x0100804a); /* NVclass $04a, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_8, 0x00000002); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_8, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		/* setup set '6' for ... */
		if(si->ps.card_arch >= NV10A)
		{
			/* ... cmd NV10_CONTEXT_SURFACES_2D */
			ACCW(PR_CTX0_A, 0x01008062); /* NVclass $062, nv10+: little endian */
		}
		else
		{
			/* ... cmd NV4_SURFACE */
			ACCW(PR_CTX0_A, 0x01008042); /* NVclass $042, nv10+: little endian */
		}
		ACCW(PR_CTX1_A, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_A, 0x11401140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_A, 0x00000000); /* method trap 0 is $1140, trap 1 disabled */
		/* setup DMA set pointed at by PF_CACH1_DMAI */
		ACCW(PR_CTX0_C, 0x00003002); /* DMA page table present and of linear type;
									  * DMA class is $002 (b0-11);
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_C, 0x00007fff); /* DMA limit: tablesize is 32k bytes */
		ACCW(PR_CTX2_C, (((si->ps.memory_size - 1) & 0xffff8000) | 0x00000002));
									 /* DMA access type is READ_AND_WRITE;
									  * table is located at end of cardRAM (b12-31):
									  * It's adress needs to be at a 4kb boundary! */
	}

	if (si->ps.card_arch == NV04A)
	{
/*
if(TNT1)
{
//cmd buffer DMA update: look at the DMA cmd buffer on cardRAM via main mem...
  PR_CTX0_C |= 0x00020000; //select NV_DMA_TARGET_NODE_PCI
  PR_CTX2_C += pNv->FbAddress; //set main mem adress ptr to the buf (workaround!)

//info: pNv->FbAddress = pNv->PciInfo->memBase[1] & 0xff800000;
//membase[1] is the virtual (high) adress where the buffer is really mapped probably..
}
*/

		/* do a explicit engine reset */
		ACCW(DEBUG0, 0x000001ff);

		/* init some function blocks */
		ACCW(DEBUG0, 0x1230c000);
		ACCW(DEBUG1, 0x72111101);
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
		//fixme: setting this here means that we don't need to provide the acc
		//commands with it. But have other architectures this pre-programmed
		//explicitly??? I don't think so!
		ACCW(PAT_SHP, 0x00000000);
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
		/* setup surface type */
		ACCW(NV10_SURF_TYP, ((ACCR(NV10_SURF_TYP)) & 0x0007ff00));
		ACCW(NV10_SURF_TYP, ((ACCR(NV10_SURF_TYP)) | 0x00020100));
	}

	if (si->ps.card_arch == NV10A)
	{
		/* init some function blocks */
		ACCW(DEBUG1, 0x00118700);
		ACCW(DEBUG2, 0x24e00810);
		ACCW(DEBUG3, 0x55de0030);

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
		//fixme: setting this here means that we don't need to provide the acc
		//commands with it. But have other architectures this pre-programmed
		//explicitly??? I don't think so!
		ACCW(PAT_SHP, 0x00000000);
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

			/* unknown.. */
			switch (si->ps.card_type)
			{
			case NV40:
			case NV45:
				ACCW(NV40_WHAT0, 0x83280fff);
				ACCW(NV40_WHAT1, 0x000000a0);
				ACCW(NV40_WHAT2, 0x0078e366);
				ACCW(NV40_WHAT3, 0x0000014c);
//      	    pNv->PFB[0x033C/4] &= 0xffff7fff;//0x00100000 :<<<< NV_PFB_CLOSE_PAGE2, bits unknown
				break;
			case NV41:
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
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);

				NV_REG32(NV32_NV44_WHAT10) = NV_REG32(NV32_NV10STRAPINFO);
				NV_REG32(NV32_NV44_WHAT11) = 0x00000000;
				NV_REG32(NV32_NV44_WHAT12) = 0x00000000;
				NV_REG32(NV32_NV44_WHAT13) = NV_REG32(NV32_NV10STRAPINFO);

				ACCW(NV44_WHAT2, 0x00000000);
				ACCW(NV44_WHAT3, 0x00000000);
//schakelt screrm signaal uit op NV43, maar timing blijft werken<<<<<<<<
//      	    pNv->PRAMDAC[0x0608/4] |= 0x00100000;//0x00680608==NVDAC_TSTCTRL haiku
              									//b20=1=DACTM_TEST ON (termination?)
              									//how about: NVDAC2_TSTCTRL????
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
		/* copy tile setup stuff from 'source' to acc engine (pattern colorRAM?) */
		for (cnt = 0; cnt < 32; cnt++)
		{
			NV_REG32(NVACC_NV20_WHAT0 + (cnt << 2)) =
				NV_REG32(NVACC_NV10_FBTIL0AD + (cnt << 2));
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
				/* copy some RAM configuration info(?) */
				ACCW(NV40P_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV40P_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
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
	si->engine.fifo.handle[6] = NV1_RENDER_SOLID_LIN;
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
	si->engine.dma.cmdbuffer = (uint32 *)((char *)si->framebuffer +
		((si->ps.memory_size - 1) & 0xffff8000));
	LOG(4,("ACC_DMA: command buffer is at adress $%08x\n",
		((uint32)(si->engine.dma.cmdbuffer))));
	/* we have issued no DMA cmd's to the engine yet */
	si->engine.dma.put = 0;
	/* the current first free adress in the DMA buffer is at offset 0 */
	si->engine.dma.current = 0;
	/* the DMA buffer can hold 8k 32-bit words (it's 32kb in size) */
	/* note:
	 * one word is reserved at the end of the DMA buffer to be able to instruct the
	 * engine to do a buffer wrap-around!
	 * (DMA opcode 'noninc method': issue word $20000000.) */
	si->engine.dma.max = 8192 - 1;
	/* note the current free space we have left in the DMA buffer */
	si->engine.dma.free = si->engine.dma.max - si->engine.dma.current;

	/*** init FIFO via DMA command buffer. ***/
	/* wait for room in fifo for new FIFO assigment cmds if needed: */
//fixme if CH6 and CH7 are assigned..
//	if (nv_acc_fifofree_dma(16) != B_OK) return B_ERROR;
	if (nv_acc_fifofree_dma(12) != B_OK) return B_ERROR;

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
	/* Line: (not used or 3D only?) */
//fixme..
//	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH6, si->engine.fifo.handle[6]);
	/* Textured Triangle: (3D only) */
//fixme..
//	nv_acc_set_ch_dma(NV_GENERAL_FIFO_CH7, si->engine.fifo.handle[7]);

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
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = surf_depth; /* Format */
	/* setup screen pitch */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		((si->fbc.bytes_per_row & 0x0000ffff) | (si->fbc.bytes_per_row << 16)); /* Pitch */
	/* setup screen location */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer); /* OffsetSource */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer); /* OffsetDest */

	/* wait for room in fifo for pattern colordepth setup cmd if needed */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* set pattern colordepth (writing 2 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLORFORMAT, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */

	/* wait for room in fifo for bitmap colordepth setup cmd if needed */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* set bitmap colordepth (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_SETCOLORFORMAT, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = cmd_depth; /* SetColorFormat */

	/* tell the engine to fetch and execute all (new) commands in the DMA buffer */
	nv_start_dma();

	return B_OK;
}

static void nv_start_dma(void)
{
	uint8 dummy;

	if (si->engine.dma.current != si->engine.dma.put)
	{
		si->engine.dma.put = si->engine.dma.current;
		/* dummy read the first adress of the framebuffer: flushes MTRR-WC buffers so
		 * we know for sure the DMA command buffer received all data. */
		dummy = *((char *)(si->framebuffer));
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
				si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x20000000;
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
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = ((size << 18) |
		((si->engine.fifo.ch_ptr[cmd] + offset) & 0x0000fffc));

	/* space left after issuing the current command is the cmd AND it's arguments less */
	si->engine.dma.free -= (size + 1);
}

static void nv_acc_set_ch_dma(uint16 ch, uint32 handle)
{
	/* issue FIFO channel assign cmd */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = ((1 << 18) | ch);
	/* set new assignment */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = (0x80000000 | handle);

	/* space left after issuing the current command is the cmd AND it's arguments less */
	si->engine.dma.free -= 2;
}

/* fixme? (check this out..)
 * Looks like this stuff can be very much simplified and speed-up, as it seems it's not
 * nessesary to wait for the engine to become idle before re-assigning channels.
 * Because the cmd handles are actually programmed _inside_ the fifo channels, it might
 * well be that the assignment is buffered along with the commands that still have to 
 * be executed!
 * (sounds very plausible to me :) */
void nv_acc_assert_fifo_dma(void)
{
	/* does every engine cmd this accelerant needs have a FIFO channel? */
	//fixme: can probably be optimized for both speed and channel selection...
	if (!si->engine.fifo.ch_ptr[NV_ROP5_SOLID] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLACK_RECTANGLE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_PATTERN] ||
		!si->engine.fifo.ch_ptr[NV4_SURFACE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLIT] ||
		!si->engine.fifo.ch_ptr[NV4_GDI_RECTANGLE_TEXT])
	{
		uint16 cnt;

		/* no, wait until the engine is idle before re-assigning the FIFO */
		nv_acc_wait_idle_dma();

		/* free the FIFO channels we want from the currently assigned cmd's */
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[0]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[1]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[2]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[3]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[4]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[5]] = 0;

		/* set new object handles */
		si->engine.fifo.handle[0] = NV_ROP5_SOLID;
		si->engine.fifo.handle[1] = NV_IMAGE_BLACK_RECTANGLE;
		si->engine.fifo.handle[2] = NV_IMAGE_PATTERN;
		si->engine.fifo.handle[3] = NV4_SURFACE;
		si->engine.fifo.handle[4] = NV_IMAGE_BLIT;
		si->engine.fifo.handle[5] = NV4_GDI_RECTANGLE_TEXT;

		/* set handle's pointers to their assigned FIFO channels */
		/* note:
		 * b0-1 aren't used as adressbits. Using b0 to indicate a valid pointer. */
		for (cnt = 0; cnt < 0x08; cnt++)
		{
			si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
				(0x00000001 + (cnt * 0x00002000));
		}

		/* wait for room in fifo for new FIFO assigment cmds if needed. */
		if (nv_acc_fifofree_dma(12) != B_OK) return;

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

		/* tell the engine to fetch and execute all (new) commands in the DMA buffer */
		nv_start_dma();
	}
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nv_acc_setup_blit_dma()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed. */
	if (nv_acc_fifofree_dma(7) != B_OK) return B_ERROR;
	/* now setup pattern (writing 7 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETSHAPE, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x00000000; /* SetShape: 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLOR0, 4);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor0 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor1 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[0] */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[1] */
	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;

	/* now setup ROP (writing 2 32bit words) for GXcopy */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xcc; /* SetRop5 */

	return B_OK;
}

status_t nv_acc_blit_dma(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* Note: blit-copy direction is determined inside riva hardware: no setup needed */

	/* instruct engine what to blit:
	 * wait for room in fifo for blit cmd if needed. */
	if (nv_acc_fifofree_dma(4) != B_OK) return B_ERROR;
	/* now setup blit (writing 4 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_BLIT, NV_IMAGE_BLIT_SOURCEORG, 3);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = ((ys << 16) | xs); /* SourceOrg */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = ((yd << 16) | xd); /* DestOrg */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = (((h + 1) << 16) | (w + 1)); /* HeightWidth */

	/* tell the engine to fetch the commands in the DMA buffer that where not
	 * executed before. At this time the setup done by nv_acc_setup_blit_dma() is
	 * also executed on the first call of nv_acc_blit_dma(). */
	nv_start_dma();

	return B_OK;
}

/* rectangle fill - i.e. workspace and window background color */
/* span fill - i.e. (selected) menuitem background color (Dano) */
status_t nv_acc_setup_rectangle_dma(uint32 color)
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed. */
	if (nv_acc_fifofree_dma(7) != B_OK) return B_ERROR;
	/* now setup pattern (writing 7 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETSHAPE, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x00000000; /* SetShape: 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLOR0, 4);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor0 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor1 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[0] */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[1] */

	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* now setup ROP (writing 2 32bit words) for GXcopy */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xcc; /* SetRop5 */

	/* setup fill color:
	 * wait for room in fifo for bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* now setup color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = color; /* Color1A */

	return B_OK;
}

status_t nv_acc_rectangle_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to fill:
	 * wait for room in fifo for bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(3) != B_OK) return B_ERROR;
	/* now setup fill (writing 3 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, 2);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		((xs << 16) | (ys & 0x0000ffff)); /* Unclipped Rect 0 LeftTop */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		(((xe - xs) << 16) | (yl & 0x0000ffff)); /* Unclipped Rect 0 WidthHeight */

	/* tell the engine to fetch the commands in the DMA buffer that where not
	 * executed before. At this time the setup done by nv_acc_setup_rectangle_dma() is
	 * also executed on the first call of nv_acc_rectangle_dma(). */
	nv_start_dma();

	return B_OK;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t nv_acc_setup_rect_invert_dma()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed. */
	if (nv_acc_fifofree_dma(7) != B_OK) return B_ERROR;
	/* now setup pattern (writing 7 32bit words) */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETSHAPE, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x00000000; /* SetShape: 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_acc_cmd_dma(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLOR0, 4);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor0 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetColor1 */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[0] */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xffffffff; /* SetPattern[1] */

	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* now setup ROP (writing 2 32bit words) for GXinvert */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x55; /* SetRop5 */

	/* reset fill color:
	 * wait for room in fifo for bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;
	/* now reset color (writing 2 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0x00000000; /* Color1A */

	return B_OK;
}

status_t nv_acc_rectangle_invert_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to fill:
	 * wait for room in fifo for bitmap cmd if needed. */
	if (nv_acc_fifofree_dma(3) != B_OK) return B_ERROR;
	/* now setup fill (writing 3 32bit words) */
	nv_acc_cmd_dma(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, 2);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		((xs << 16) | (ys & 0x0000ffff)); /* Unclipped Rect 0 LeftTop */
	si->engine.dma.cmdbuffer[si->engine.dma.current++] =
		(((xe - xs) << 16) | (yl & 0x0000ffff)); /* Unclipped Rect 0 WidthHeight */

	/* tell the engine to fetch the commands in the DMA buffer that where not
	 * executed before. At this time the setup done by nv_acc_setup_rectangle_dma() is
	 * also executed on the first call of nv_acc_rectangle_dma(). */
	nv_start_dma();

	return B_OK;
}
