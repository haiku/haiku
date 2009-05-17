/* NV Acceleration functions */
/* Author:
   Rudolf Cornelissen 8/2003-5/2009.

   This code was possible thanks to:
    - the Linux XFree86 NV driver,
    - the Linux UtahGLX 3D driver.
*/

/*
	note:
	Can't get NV40 and higher going using this PIO mode acceleration system ATM.
	Here's the problem:
	The FIFO is not functioning correctly: the proof of this is that you can only
	readout	the PIO FIFO fill-level register (FifoFree) once before it stops responding
	(returns only zeros on all reads after the first one).
	You can see the issued commands are actually placed in the FIFO because the first
	read of FifoFree corresponds to what you'd expect.
	There is no visual confirmation of any command actually being executed by the
	acceleration engine, so we don't know if the FIFO places commands in the engine.
	BTW:
	The FifoFree register exhibits the exact same behaviour in DMA acceleration mode.
	It's no problem there because we use the DMAPut and DMAGet registers instead.

	The non-functioning Fifo in PIO mode might have one of these reasons:
	- lack of specs: maybe additional programming is required.
	- hardware fault: as probably no-one uses PIO mode acceleration anymore these days,
	  nVidia might not care about this any longer.

	note also:
	Keeping this PIO mode acceleration stuff here for now to guarantee compatibility
	with current 3D acceleration attempts: the utahGLX 3D driver cooperated with the
	PIO mode acceleration functions in the XFree drivers (upto/including XFree 4.2.0).
*/

#define MODULE_BIT 0x00080000

#include "nv_std.h"

static void nv_init_for_3D(void);

/*acceleration notes*/

/*functions Be's app_server uses:
fill span (horizontal only)
fill rectangle (these 2 are very similar)
invert rectangle 
blit
*/

/*
	nVidia hardware info:
	We should be able to do FIFO assignment setup changes on-the-fly now, using
	all the engine-command-handles that are pre-defined on any FIFO channel.
	Also we should be able to setup new additional handles to previously unused
	engine commands now.
*/

/* FIFO channel pointers */
/* note:
 * every instance of the accelerant needs to have it's own pointers, as the registers
 * are cloned to different adress ranges for each one */
static cmd_nv_rop5_solid* nv_rop5_solid_ptr;
static cmd_nv_image_black_rectangle* nv_image_black_rectangle_ptr;
static cmd_nv_image_pattern* nv_image_pattern_ptr;
static cmd_nv_image_blit* nv_image_blit_ptr;
static cmd_nv3_gdi_rectangle_text* nv3_gdi_rectangle_text_ptr;

status_t nv_acc_wait_idle()
{
	/* wait until engine completely idle */
	while (ACCR(STATUS))
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100); 
	}

	return B_OK;
}

/* AFAIK this must be done for every new screenmode.
 * Engine required init. */
status_t nv_acc_init()
{
	uint16 cnt;

	/* a hanging engine only recovers from a complete power-down/power-up cycle */
	NV_REG32(NV32_PWRUPCTRL) = 0xffff00ff;
	snooze(1000);
	NV_REG32(NV32_PWRUPCTRL) = 0xffffffff;

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

	/*** PFIFO ***/
	/* (setup caches) */
	/* disable caches reassign */
	ACCW(PF_CACHES, 0x00000000);
	/* PFIFO mode for all 32 channels is PIO (instead of DMA) */
	ACCW(PF_MODE, 0x00000000);
	/* cache1 push0 access disabled */
	ACCW(PF_CACH1_PSH0, 0x00000000);
	/* cache1 pull0 access disabled */
	ACCW(PF_CACH1_PUL0, 0x00000000);
	/* cache1 push1 mode = pio (disable DMA use) */
	ACCW(PF_CACH1_PSH1, 0x00000000);
	/* cache1 DMA Put offset = 0 (b2-28) */
	ACCW(PF_CACH1_DMAP, 0x00000000);
	/* cache1 DMA Get offset = 0 (b2-28) */
	ACCW(PF_CACH1_DMAG, 0x00000000);
	/* cache1 DMA instance adress = none (b0-15);
	 * instance being b4-19 with baseadress NV_PRAMIN_CTX_0 (0x00700000). */
	/* note:
	 * should point to a DMA definition in CTX register space (which is sort of RAM).
	 * This define tells the engine where the DMA cmd buffer is and what it's size is.
	 * Inside that cmd buffer you'll find the actual issued engine commands. */
	ACCW(PF_CACH1_DMAI, 0x00000000);
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
	/* cache1 DMA push: b0=0 is access disabled */
	ACCW(PF_CACH1_DMAS, 0x00000000);
	/* cache1 push0 access enabled */
	ACCW(PF_CACH1_PSH0, 0x00000001);
	/* cache1 pull0 access enabled */
	ACCW(PF_CACH1_PUL0, 0x00000001);
	/* cache1 pull1 engine = acceleration engine (graphics) */
	ACCW(PF_CACH1_PUL1, 0x00000001);
	/* enable PFIFO caches reassign */
	ACCW(PF_CACHES, 0x00000001);

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
	/* RAMHT space (hash-table) SETUP FIFO HANDLES */
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
		ACCW(HT_VALUE_06, 0x8001114b); /* instance $114b, engine = acc engine, CHID = $00 */

		/* (second set) */
		ACCW(HT_HANDL_10, (0x80000000 | NV_ROP5_SOLID)); /* 32bit handle */
		ACCW(HT_VALUE_10, 0x80011142); /* instance $1142, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_11, (0x80000000 | NV_IMAGE_BLACK_RECTANGLE)); /* 32bit handle */
		ACCW(HT_VALUE_11, 0x80011143); /* instance $1143, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_12, (0x80000000 | NV_IMAGE_PATTERN)); /* 32bit handle */
		ACCW(HT_VALUE_12, 0x80011144); /* instance $1144, engine = acc engine, CHID = $00 */
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
		ACCW(PR_CTX2_0, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_0, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_1, 0x00000000); /* extra */
		ACCW(PR_CTX1_1, 0x00000000); /* extra */
		/* setup set '1' for cmd NV_IMAGE_BLACK_RECTANGLE */
		ACCW(PR_CTX0_2, 0x02080019); /* NVclass $019, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_2, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_2, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_3, 0x00000000); /* extra */
		ACCW(PR_CTX1_3, 0x00000000); /* extra */
		/* setup set '2' for cmd NV_IMAGE_PATTERN */
		ACCW(PR_CTX0_4, 0x02080018); /* NVclass $018, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_4, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_4, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_5, 0x00000000); /* extra */
		ACCW(PR_CTX1_5, 0x00000000); /* extra */
		/* setup set '4' for cmd NV_IMAGE_BLIT */
		ACCW(PR_CTX0_6, 0x0208005f); /* NVclass $05f, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_6, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_6, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_7, 0x00000000); /* extra */
		ACCW(PR_CTX1_7, 0x00000000); /* extra */
		/* setup set '5' for cmd NV4_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_8, 0x0208004a); /* NVclass $04a, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_8, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_9, 0x00000000); /* extra */
		ACCW(PR_CTX1_9, 0x00000000); /* extra */
		/* setup set '6' for cmd NV10_CONTEXT_SURFACES_2D */
		ACCW(PR_CTX0_A, 0x02080062); /* NVclass $062, nv10+: little endian */
		ACCW(PR_CTX2_A, 0x00001140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_A, 0x00001140); /* method trap 0 is $1140, trap 1 disabled */
		ACCW(PR_CTX0_B, 0x00000000); /* extra */
		ACCW(PR_CTX1_B, 0x00000000); /* extra */
	}
	else
	{
		/* setup a DMA define for use by command defines below.
		 * (would currently be used by CTX 'sets' 0x6 upto/including 0xe: 3D stuff.) */
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
		ACCW(PR_CTX2_0, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_0, 0x00000000); /* method traps disabled */
		/* setup set '1' for cmd NV_IMAGE_BLACK_RECTANGLE */
		ACCW(PR_CTX0_1, 0x01008019); /* NVclass $019, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_1, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_1, 0x00000000); /* method traps disabled */
		/* setup set '2' for cmd NV_IMAGE_PATTERN */
		ACCW(PR_CTX0_2, 0x01008018); /* NVclass $018, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_2, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_2, 0x00000000); /* method traps disabled */
//fixme: update 3D add-on and this code for the NV4_SURFACE command.
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
		/* setup set '4' for cmd NV_IMAGE_BLIT */
		ACCW(PR_CTX0_4, 0x0100805f); /* NVclass $05f, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_4, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_4, 0x00000000); /* method traps disabled */
		/* setup set '5' for cmd NV4_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_5, 0x0100804a); /* NVclass $04a, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX2_5, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_5, 0x00000000); /* method traps disabled */
		/* setup set '6' ... */
		if (si->ps.card_arch != NV04A)
		{
			/* ... for cmd NV10_CONTEXT_SURFACES_ARGB_ZS */
			ACCW(PR_CTX0_6, 0x00000093); /* NVclass $093, nv10+: little endian */
		}
		else
		{
			/* ... for cmd NV4_CONTEXT_SURFACES_ARGB_ZS */
			ACCW(PR_CTX0_6, 0x00000053); /* NVclass $053, nv10+: little endian */
		}
		ACCW(PR_CTX2_6, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_6, 0x00000000); /* method traps disabled */
		/* setup set '7' ... */
		if (si->ps.card_arch != NV04A)
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
		ACCW(PR_CTX1_7, 0x00000d01); /* format is A8RGB24, MSB mono */
		ACCW(PR_CTX2_7, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_7, 0x00000000); /* method traps disabled */
		/* setup set '8' ... */
		if (si->ps.card_arch != NV04A)
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
		ACCW(PR_CTX1_8, 0x00000d01); /* format is A8RGB24, MSB mono */
		ACCW(PR_CTX2_8, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		/* setup set '9' for cmd NV1_RENDER_SOLID_LIN (not used) */
		ACCW(PR_CTX0_9, 0x0300a01c); /* NVclass $01c, patchcfg ROP_AND, userclip enable,
									  * context surface0 valid, nv10+: little endian */
		ACCW(PR_CTX2_9, 0x11401140); /* DMA0, DMA1 instance = $1140 */
		ACCW(PR_CTX3_9, 0x00000000); /* method traps disabled */
//fixme: update 3D add-on and this code for the NV4_SURFACE command.
		/* setup set '9' for cmd NV3_SURFACE_0 */
//		ACCW(PR_CTX0_9, 0x00000058); /* NVclass $058, nv10+: little endian */
//		ACCW(PR_CTX2_9, 0x11401140); /* DMA0, DMA1 instance = $1140 */
//		ACCW(PR_CTX3_9, 0x00000000); /* method traps disabled */
		/* setup set 'A' for cmd NV3_SURFACE_1 */
//		ACCW(PR_CTX0_A, 0x00000059); /* NVclass $059, nv10+: little endian */
//		ACCW(PR_CTX2_A, 0x11401140); /* DMA0, DMA1 instance = $1140 */
//		ACCW(PR_CTX3_A, 0x00000000); /* method traps disabled */
	}

	/*** PGRAPH ***/
	switch (si->ps.card_arch)
	{
	case NV40A:
		/* set resetstate for most function blocks */
		ACCW(DEBUG0, 0x0003ffff);//?
		/* init some function blocks */
		ACCW(DEBUG1, 0x401287c0);
		ACCW(DEBUG2, 0x24f82ad9);//?
		ACCW(DEBUG3, 0x60de8051);
		/* end resetstate for the function blocks */
		ACCW(DEBUG0, 0x00000000);//?
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
			ACCW(NV44_WHAT2, 0x00000000);
			ACCW(NV44_WHAT3, 0x00000000);
			/* unknown.. */
			NV_REG32(NV32_NV44_WHAT10) = NV_REG32(NV32_NV10STRAPINFO);
			NV_REG32(NV32_NV44_WHAT11) = 0x00000000;
			NV_REG32(NV32_NV44_WHAT12) = 0x00000000;
			NV_REG32(NV32_NV44_WHAT13) = NV_REG32(NV32_NV10STRAPINFO);
			break;
		default:
			ACCW(NV40P_WHAT0, 0x83280eff);
			ACCW(NV40P_WHAT1, 0x000000a0);
			break;
		}
		break;
	case NV04A:
		/* init some function blocks */
		ACCW(DEBUG0, 0x1231c001);
		ACCW(DEBUG1, 0x72111101);
		ACCW(DEBUG2, 0x11d5f071);
		ACCW(DEBUG3, 0x10d4ff31);
		break;
	default:
		/* set resetstate for most function blocks */
		ACCW(DEBUG0, 0x0003ffff);
		/* init some function blocks */
		ACCW(DEBUG1, 0x00118701);
		ACCW(DEBUG2, 0x24f82ad9);
		ACCW(DEBUG3, 0x55de0030);
		/* end resetstate for the function blocks */
		ACCW(DEBUG0, 0x00000000);
		/* disable specific functions */
		ACCW(NV10_DEBUG4, 0);
		break;
	}

	/* reset all cache sets */
	ACCW(CACHE1_1, 0);
	ACCW(CACHE1_2, 0);
	ACCW(CACHE1_3, 0);
	ACCW(CACHE1_4, 0);
	ACCW(CACHE1_5, 0);
	ACCW(CACHE2_1, 0);
	ACCW(CACHE2_2, 0);
	ACCW(CACHE2_3, 0);
	ACCW(CACHE2_4, 0);
	ACCW(CACHE2_5, 0);
	ACCW(CACHE3_1, 0);
	ACCW(CACHE3_2, 0);
	ACCW(CACHE3_3, 0);
	ACCW(CACHE3_4, 0);
	ACCW(CACHE3_5, 0);
	ACCW(CACHE4_1, 0);
	ACCW(CACHE4_2, 0);
	ACCW(CACHE4_3, 0);
	ACCW(CACHE4_4, 0);
	ACCW(CACHE4_5, 0);
	if (si->ps.card_arch != NV04A)
		ACCW(NV10_CACHE5_1, 0);
	ACCW(CACHE5_2, 0);
	ACCW(CACHE5_3, 0);
	ACCW(CACHE5_4, 0);
	ACCW(CACHE5_5, 0);
	if (si->ps.card_arch != NV04A)
		ACCW(NV10_CACHE6_1, 0);
	ACCW(CACHE6_2, 0);
	ACCW(CACHE6_3, 0);
	ACCW(CACHE6_4, 0);
	ACCW(CACHE6_5, 0);
	if (si->ps.card_arch != NV04A)
		ACCW(NV10_CACHE7_1, 0);
	ACCW(CACHE7_2, 0);
	ACCW(CACHE7_3, 0);
	ACCW(CACHE7_4, 0);
	ACCW(CACHE7_5, 0);
	if (si->ps.card_arch != NV04A)
		ACCW(NV10_CACHE8_1, 0);
	ACCW(CACHE8_2, 0);
	ACCW(CACHE8_3, 0);
	ACCW(CACHE8_4, 0);
	ACCW(CACHE8_5, 0);

	if (si->ps.card_arch != NV04A)
	{
		/* reset (disable) context switch stuff */
		ACCW(NV10_CTX_SW1, 0);
		ACCW(NV10_CTX_SW2, 0);
		ACCW(NV10_CTX_SW3, 0);
		ACCW(NV10_CTX_SW4, 0);
		ACCW(NV10_CTX_SW5, 0);
	}

	/* setup accesible card memory range for acc engine */
	ACCW(BBASE0, 0x00000000);
	ACCW(BBASE1, 0x00000000);
	ACCW(BBASE2, 0x00000000);
	ACCW(BBASE3, 0x00000000);
	ACCW(BLIMIT0, (si->ps.memory_size - 1));
	ACCW(BLIMIT1, (si->ps.memory_size - 1));
	ACCW(BLIMIT2, (si->ps.memory_size - 1));
	ACCW(BLIMIT3, (si->ps.memory_size - 1));
	if (si->ps.card_arch >= NV10A)
	{
		ACCW(NV10_BBASE4, 0x00000000);
		ACCW(NV10_BBASE5, 0x00000000);
		ACCW(NV10_BLIMIT4, (si->ps.memory_size - 1));
		ACCW(NV10_BLIMIT5, (si->ps.memory_size - 1));
	}
	if (si->ps.card_arch >= NV20A)
	{
		if ((si->ps.card_type > NV40) && (si->ps.card_type != NV45))
		{
			ACCW(NV40P_BLIMIT6, (si->ps.memory_size - 1));
			ACCW(NV40P_BLIMIT7, (si->ps.memory_size - 1));
		}
		else
		{
			/* fixme(?): assuming more BLIMIT registers here: Then how about BBASE6-9? */
			ACCW(NV20_BLIMIT6, (si->ps.memory_size - 1));
			ACCW(NV20_BLIMIT7, (si->ps.memory_size - 1));
			if (si->ps.card_type < NV40)
			{
				ACCW(NV20_BLIMIT8, (si->ps.memory_size - 1));
				ACCW(NV20_BLIMIT9, (si->ps.memory_size - 1));
			}
		}
	}

	/* disable all acceleration engine INT reguests */
	ACCW(ACC_INTE, 0x00000000);

	/* reset all acceration engine INT status bits */
	ACCW(ACC_INTS, 0xffffffff);
	if (si->ps.card_arch != NV04A)
	{
		/* context control enabled */
		ACCW(NV10_CTX_CTRL, 0x10010100);
		/* all acceleration buffers, pitches and colors are valid */
		ACCW(NV10_ACC_STAT, 0xffffffff);
	}
	else
	{
		/* context control enabled */
		ACCW(NV04_CTX_CTRL, 0x10010100);
		/* all acceleration buffers, pitches and colors are valid */
		ACCW(NV04_ACC_STAT, 0xffffffff);
	}
	/* enable acceleration engine command FIFO */
	ACCW(FIFO_EN, 0x00000001);
	/* pattern shape value = 8x8, 2 color */
	ACCW(PAT_SHP, 0x00000000);
	if (si->ps.card_arch != NV04A)
	{
		/* surface type is non-swizzle */
		ACCW(NV10_SURF_TYP, 0x00000001);
	}
	else
	{
		/* surface type is non-swizzle */
		ACCW(NV04_SURF_TYP, 0x00000001);
	}

	/*** Set pixel width and format ***/
	//info:
	//the BPIXEL register holds the colorspaces for different engine 'contexts' or so.
	//B0-3 is 'channel' 0, b4-7 is 'channel '1', etc.
	//It looks like we are only using channel 0, so the settings for other channels
	//shouldn't matter yet.
	//When for instance rect_fill is going to be used on other buffers than the actual
	//screen, it's colorspace should be corrected. When the engine is setup in 32bit
	//desktop mode for example, the pixel's alpha channel doesn't get touched currently.
	//choose mode $d (which is Y32) to get alpha filled too.
	switch(si->dm.space)
	{
	case B_CMAP8:
		/* acc engine */
		ACCW(FORMATS, 0x00001010);
		if (si->ps.card_arch < NV30A)
			/* set depth 0-5: $1 = Y8 */
			ACCW(BPIXEL, 0x00111111);
		else
			/* set depth 0-1: $1 = Y8, $2 = X1R5G5B5_Z1R5G5B5 */
			ACCW(BPIXEL, 0x00000021);
		ACCW(STRD_FMT, 0x03020202);
		/* PRAMIN */
		if (si->ps.card_arch < NV40A)
		{
			ACCW(PR_CTX1_0, 0x00000302); /* format is X24Y8, LSB mono */
			ACCW(PR_CTX1_1, 0x00000302); /* format is X24Y8, LSB mono */
			ACCW(PR_CTX1_2, 0x00000202); /* format is X16A8Y8, LSB mono */
			ACCW(PR_CTX1_3, 0x00000302); /* format is X24Y8, LSB mono */
			ACCW(PR_CTX1_4, 0x00000302); /* format is X24Y8, LSB mono */
			ACCW(PR_CTX1_5, 0x00000302); /* format is X24Y8, LSB mono */
			if (si->ps.card_arch == NV04A)
			{
				ACCW(PR_CTX1_6, 0x00000302); /* format is X24Y8, LSB mono */
			}
			else
			{
				ACCW(PR_CTX1_6, 0x00000000); /* format is invalid */
			}
			ACCW(PR_CTX1_9, 0x00000302); /* format is X24Y8, LSB mono */
//fixme: update 3D add-on and this code for the NV4_SURFACE command.
//old surf0 and 1:
//			ACCW(PR_CTX1_9, 0x00000302); /* format is X24Y8, LSB mono */
//			ACCW(PR_CTX2_9, 0x00000302); /* dma_instance 0 valid, instance 1 invalid */
		}
		else
		{
			//fixme: select colorspace here (and in other depths), or add
			//the appropriate SURFACE command(s).
			ACCW(PR_CTX1_0, 0x00000000); /* NV_ROP5_SOLID */
			ACCW(PR_CTX1_2, 0x00000000); /* NV_IMAGE_BLACK_RECTANGLE */
			ACCW(PR_CTX1_4, 0x02000000); /* NV_IMAGE_PATTERN */
			ACCW(PR_CTX1_6, 0x00000000); /* NV_IMAGE_BLIT */
			ACCW(PR_CTX1_8, 0x02000000); /* NV4_GDI_RECTANGLE_TEXT */
			ACCW(PR_CTX1_A, 0x02000000); /* NV10_CONTEXT_SURFACES_2D */
		}
		break;
	case B_RGB15_LITTLE:
		/* acc engine */
		ACCW(FORMATS, 0x00002071);
		if (si->ps.card_arch < NV30A)
			/* set depth 0-5: $2 = X1R5G5B5_Z1R5G5B5, $6 = Y16 */
			ACCW(BPIXEL, 0x00226222);
		else
			/* set depth 0-1: $2 = X1R5G5B5_Z1R5G5B5, $4 = A1R5G5B5 */
			ACCW(BPIXEL, 0x00000042);
		ACCW(STRD_FMT, 0x09080808);
		/* PRAMIN */
		ACCW(PR_CTX1_0, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_1, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_2, 0x00000802); /* format is X16A1RGB15, LSB mono */
		ACCW(PR_CTX1_3, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_4, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_5, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_6, 0x00000902); /* format is X17RGB15, LSB mono */
		ACCW(PR_CTX1_9, 0x00000902); /* format is X17RGB15, LSB mono */
//old surf0 and 1:
//		ACCW(PR_CTX1_9, 0x00000902); /* format is X17RGB15, LSB mono */
//		ACCW(PR_CTX2_9, 0x00000902); /* dma_instance 0 valid, instance 1 invalid */
		break;
	case B_RGB16_LITTLE:
		/* acc engine */
		ACCW(FORMATS, 0x000050C2);
		if (si->ps.card_arch < NV30A)
			/* set depth 0-5: $5 = R5G6B5, $6 = Y16 */
			ACCW(BPIXEL, 0x00556555);
		else
			/* set depth 0-1: $5 = R5G6B5, $a = X1A7R8G8B8_O1A7R8G8B8 */
			ACCW(BPIXEL, 0x000000a5);
		if (si->ps.card_arch == NV04A)
			ACCW(STRD_FMT, 0x0c0b0b0b);
		else
			ACCW(STRD_FMT, 0x000b0b0c);
		/* PRAMIN */
		ACCW(PR_CTX1_0, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_1, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_2, 0x00000b02); /* format is A16RGB16, LSB mono */
		ACCW(PR_CTX1_3, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_4, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_5, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_6, 0x00000c02); /* format is X16RGB16, LSB mono */
		ACCW(PR_CTX1_9, 0x00000c02); /* format is X16RGB16, LSB mono */
//old surf0 and 1:
//		ACCW(PR_CTX1_9, 0x00000c02); /* format is X16RGB16, LSB mono */
//		ACCW(PR_CTX2_9, 0x00000c02); /* dma_instance 0 valid, instance 1 invalid */
		break;
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
		/* acc engine */
		ACCW(FORMATS, 0x000070e5);
		if (si->ps.card_arch < NV30A)
			/* set depth 0-5: $7 = X8R8G8B8_Z8R8G8B8, $d = Y32 */
			ACCW(BPIXEL, 0x0077d777);
		else
			/* set depth 0-1: $7 = X8R8G8B8_Z8R8G8B8, $e = V8YB8U8YA8 */
			ACCW(BPIXEL, 0x000000e7);
		ACCW(STRD_FMT, 0x0e0d0d0d);
		/* PRAMIN */
		ACCW(PR_CTX1_0, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_1, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_2, 0x00000d02); /* format is A8RGB24, LSB mono */
		ACCW(PR_CTX1_3, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_4, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_5, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_6, 0x00000e02); /* format is X8RGB24, LSB mono */
		ACCW(PR_CTX1_9, 0x00000e02); /* format is X8RGB24, LSB mono */
//old surf0 and 1:
//		ACCW(PR_CTX1_9, 0x00000e02); /* format is X8RGB24, LSB mono */
//		ACCW(PR_CTX2_9, 0x00000e02); /* dma_instance 0 valid, instance 1 invalid */
		break;
	default:
		LOG(8,("ACC: init, invalid bit depth\n"));
		return B_ERROR;
	}

	/* setup some extra stuff for NV30A and later */
	if (si->ps.card_arch >= NV30A)
	{
		/* activate Zcullflush(?) */
		ACCW(DEBUG3, (ACCR(DEBUG3) | 0x00000001));
		/* unknown */
		ACCW(NV25_WHAT1, (ACCR(NV25_WHAT1) | 0x00040000));
	}

	/*** setup screen location and pitch ***/
	switch (si->ps.card_arch)
	{
	case NV04A:
	case NV10A:
		/* location of active screen in framebuffer */
		/* (confirmed NV05: OFFSET0 is 2D destination buffer offset) */
		ACCW(OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		/* (confirmed NV05: OFFSET1 is 2D source buffer offset) */
		ACCW(OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		/* (confirmed NV05: OFFSET2 is 3D color buffer offset) */
		ACCW(OFFSET2, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		/* (confirmed NV05: OFFSET3 is 3D depth buffer offset) */
		ACCW(OFFSET3, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(OFFSET4, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(OFFSET5, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));

		/* setup buffer pitch */
		/* (confirmed NV05: PITCH0 is 2D destination buffer pitch) */
		ACCW(PITCH0, (si->fbc.bytes_per_row & 0x0000ffff));
		/* (confirmed NV05: PITCH1 is 2D source buffer pitch) */
		ACCW(PITCH1, (si->fbc.bytes_per_row & 0x0000ffff));
		/* (confirmed NV05: PITCH2 is 3D color buffer pitch) */
		ACCW(PITCH2, (si->fbc.bytes_per_row & 0x0000ffff));
		/* (confirmed NV05: PITCH3 is 3D depth buffer pitch) */
		ACCW(PITCH3, (si->fbc.bytes_per_row & 0x0000ffff));
		ACCW(PITCH4, (si->fbc.bytes_per_row & 0x0000ffff));
		break;
	case NV20A:
	case NV30A:
		/* location of active screen in framebuffer */
		ACCW(NV20_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(NV20_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(NV20_OFFSET2, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
		ACCW(NV20_OFFSET3, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));

		/* setup buffer pitch */
		ACCW(NV20_PITCH0, (si->fbc.bytes_per_row & 0x0000ffff));
		ACCW(NV20_PITCH1, (si->fbc.bytes_per_row & 0x0000ffff));
		ACCW(NV20_PITCH2, (si->fbc.bytes_per_row & 0x0000ffff));
		ACCW(NV20_PITCH3, (si->fbc.bytes_per_row & 0x0000ffff));
		break;
	case NV40A:
		if ((si->ps.card_type == NV40) || (si->ps.card_type == NV45))
		{
			/* location of active screen in framebuffer */
			ACCW(NV20_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			ACCW(NV20_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			//ACCW(NV20_OFFSET2, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			//ACCW(NV20_OFFSET3, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));

			/* setup buffer pitch */
			//fixme?
			ACCW(NV20_PITCH0, (si->fbc.bytes_per_row & 0x0000ffff));
			ACCW(NV20_PITCH1, (si->fbc.bytes_per_row & 0x0000ffff));
			ACCW(NV20_PITCH2, (si->fbc.bytes_per_row & 0x0000ffff));
			ACCW(NV20_PITCH3, (si->fbc.bytes_per_row & 0x0000ffff));
		}
		else
		{
			/* location of active screen in framebuffer */
			ACCW(NV40P_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
			ACCW(NV40P_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));

			/* setup buffer pitch */
			//fixme?
			ACCW(NV40P_PITCH0, (si->fbc.bytes_per_row & 0x0000ffff));
			ACCW(NV40P_PITCH1, (si->fbc.bytes_per_row & 0x0000ffff));
		}
		break;
	}

	/*** setup tile and pipe stuff ***/
	if (si->ps.card_arch >= NV10A)
	{
		/* setup acc engine tile stuff: */
		/* reset tile adresses */
		ACCW(NV10_FBTIL0AD, 0);
		ACCW(NV10_FBTIL1AD, 0);
		ACCW(NV10_FBTIL2AD, 0);
		ACCW(NV10_FBTIL3AD, 0);
		ACCW(NV10_FBTIL4AD, 0);
		ACCW(NV10_FBTIL5AD, 0);
		ACCW(NV10_FBTIL6AD, 0);
		ACCW(NV10_FBTIL7AD, 0);
		/* copy some RAM configuration info(?) */
		if (si->ps.card_arch >= NV20A)
		{
			if ((si->ps.card_type > NV40) && (si->ps.card_type != NV45))
			{
				ACCW(NV40P_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV40P_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
				ACCW(NV40P_WHAT_T2, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV40P_WHAT_T3, NV_REG32(NV32_PFB_CONFIG_1));
			}
			else
			{
				ACCW(NV20_WHAT_T0, NV_REG32(NV32_PFB_CONFIG_0));
				ACCW(NV20_WHAT_T1, NV_REG32(NV32_PFB_CONFIG_1));
				if ((si->ps.card_type == NV40) || (si->ps.card_type == NV45))
				{
					ACCW(NV40_WHAT_T2, NV_REG32(NV32_PFB_CONFIG_0));
					ACCW(NV40_WHAT_T3, NV_REG32(NV32_PFB_CONFIG_1));
				}
			}
		}
		/* copy tile setup stuff from 'source' to acc engine */
		/* tile 0: */
		/* tile invalid, tile adress = $00000 (18bit) */
		ACCW(NV10_TIL0AD, ACCR(NV10_FBTIL0AD));
		/* set tile end adress (18bit) */
		ACCW(NV10_TIL0ED, ACCR(NV10_FBTIL0ED));
		/* set tile size pitch (8bit: b8-15) */
		ACCW(NV10_TIL0PT, ACCR(NV10_FBTIL0PT));
		/* set tile status */
		ACCW(NV10_TIL0ST, ACCR(NV10_FBTIL0ST));
		/* tile 1: */
		ACCW(NV10_TIL1AD, ACCR(NV10_FBTIL1AD));
		ACCW(NV10_TIL1ED, ACCR(NV10_FBTIL1ED));
		ACCW(NV10_TIL1PT, ACCR(NV10_FBTIL1PT));
		ACCW(NV10_TIL1ST, ACCR(NV10_FBTIL1ST));
		/* tile 2: */
		ACCW(NV10_TIL2AD, ACCR(NV10_FBTIL2AD));
		ACCW(NV10_TIL2ED, ACCR(NV10_FBTIL2ED));
		ACCW(NV10_TIL2PT, ACCR(NV10_FBTIL2PT));
		ACCW(NV10_TIL2ST, ACCR(NV10_FBTIL2ST));
		/* tile 3: */
		ACCW(NV10_TIL3AD, ACCR(NV10_FBTIL3AD));
		ACCW(NV10_TIL3ED, ACCR(NV10_FBTIL3ED));
		if (si->ps.card_arch >= NV40A)
		{
			ACCW(NV10_TIL3PT, 0x2ffff800);
			ACCW(NV10_TIL3ST, 0x00006000);
		}
		else
		{
			ACCW(NV10_TIL3PT, ACCR(NV10_FBTIL3PT));
			ACCW(NV10_TIL3ST, ACCR(NV10_FBTIL3ST));
		}
		/* tile 4: */
		ACCW(NV10_TIL4AD, ACCR(NV10_FBTIL4AD));
		ACCW(NV10_TIL4ED, ACCR(NV10_FBTIL4ED));
		ACCW(NV10_TIL4PT, ACCR(NV10_FBTIL4PT));
		ACCW(NV10_TIL4ST, ACCR(NV10_FBTIL4ST));
		/* tile 5: */
		ACCW(NV10_TIL5AD, ACCR(NV10_FBTIL5AD));
		ACCW(NV10_TIL5ED, ACCR(NV10_FBTIL5ED));
		ACCW(NV10_TIL5PT, ACCR(NV10_FBTIL5PT));
		ACCW(NV10_TIL5ST, ACCR(NV10_FBTIL5ST));
		/* tile 6: */
		ACCW(NV10_TIL6AD, ACCR(NV10_FBTIL6AD));
		ACCW(NV10_TIL6ED, ACCR(NV10_FBTIL6ED));
		ACCW(NV10_TIL6PT, ACCR(NV10_FBTIL6PT));
		ACCW(NV10_TIL6ST, ACCR(NV10_FBTIL6ST));
		/* tile 7: */
		ACCW(NV10_TIL7AD, ACCR(NV10_FBTIL7AD));
		ACCW(NV10_TIL7ED, ACCR(NV10_FBTIL7ED));
		ACCW(NV10_TIL7PT, ACCR(NV10_FBTIL7PT));
		ACCW(NV10_TIL7ST, ACCR(NV10_FBTIL7ST));

		if (si->ps.card_arch >= NV40A)
		{
			/* unknown.. */
			ACCW(NV4X_WHAT1, 0x01000000);
			/* engine data source DMA instance is invalid */
			ACCW(NV4X_DMA_SRC, 0x00000000);
		}

		/* setup (clear) pipe */
		/* set eyetype to local, lightning is off */
		ACCW(NV10_XFMOD0, 0x10000000);
		/* disable all lights */
		ACCW(NV10_XFMOD1, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00000040);
		ACCW(NV10_PIPEDAT, 0x00000008);

		/* note: upon writing data into the PIPEDAT register, the PIPEADR is
		 * probably auto-incremented! */
		ACCW(NV10_PIPEADR, 0x00000200);
		for (cnt = 0; cnt < (3 * 16); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00000040);
		ACCW(NV10_PIPEDAT, 0x00000000);

		//fixme: this 'set' seems to hang the NV43 engine if executed:
		//status remains 'busy' forever in this case.
		if (si->ps.card_arch < NV40A)
		{
			ACCW(NV10_PIPEADR, 0x00000800);
			for (cnt = 0; cnt < (16 * 16); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);
		}

		/* turn lightning on */
		ACCW(NV10_XFMOD0, 0x30000000);
		/* set light 1 to infinite type, other lights remain off */
		ACCW(NV10_XFMOD1, 0x00000004);

		ACCW(NV10_PIPEADR, 0x00006400);
		for (cnt = 0; cnt < (59 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006800);
		for (cnt = 0; cnt < (47 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00006c00);
		for (cnt = 0; cnt < (3 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00007000);
		for (cnt = 0; cnt < (19 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00007400);
		for (cnt = 0; cnt < (12 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00007800);
		for (cnt = 0; cnt < (12 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00004400);
		for (cnt = 0; cnt < (8 * 4); cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00000000);
		for (cnt = 0; cnt < 16; cnt++) ACCW(NV10_PIPEDAT, 0x00000000);

		ACCW(NV10_PIPEADR, 0x00000040);
		for (cnt = 0; cnt < 4; cnt++) ACCW(NV10_PIPEDAT, 0x00000000);
	}

	/* setup 3D specifics */
	nv_init_for_3D();

	/*** setup acceleration engine command shortcuts (so via fifo) ***/
	/* set object handles (b31 = 1 selects 'config' function?) */
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
	if (si->ps.card_arch < NV40A)
	{
		si->engine.fifo.handle[6] = NV1_RENDER_SOLID_LIN;
		si->engine.fifo.handle[7] = NV4_DX5_TEXTURE_TRIANGLE;
	}
	/* preset no FIFO channels assigned to cmd's */
	for (cnt = 0; cnt < 0x20; cnt++)
	{
		si->engine.fifo.ch_ptr[cnt] = 0;
	}
	/* set handle's pointers to their assigned FIFO channels */
	for (cnt = 0; cnt < 0x08; cnt++)
	{
		si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
			(NVACC_FIFO + (cnt * 0x00002000));
	}
	/* program FIFO assignments */
	ACCW(FIFO_CH0, (0x80000000 | si->engine.fifo.handle[0])); /* Raster OPeration */
	ACCW(FIFO_CH1, (0x80000000 | si->engine.fifo.handle[1])); /* Clip */
	ACCW(FIFO_CH2, (0x80000000 | si->engine.fifo.handle[2])); /* Pattern */
	ACCW(FIFO_CH3, (0x80000000 | si->engine.fifo.handle[3])); /* 2D Surface */
	ACCW(FIFO_CH4, (0x80000000 | si->engine.fifo.handle[4])); /* Blit */
	ACCW(FIFO_CH5, (0x80000000 | si->engine.fifo.handle[5])); /* Bitmap */
	if (si->ps.card_arch < NV40A)
	{
		ACCW(FIFO_CH6, (0x80000000 | si->engine.fifo.handle[6])); /* Line (not used) */
		ACCW(FIFO_CH7, (0x80000000 | si->engine.fifo.handle[7])); /* Textured Triangle (3D only) */
	}

	/* initialize our local pointers */
	nv_acc_assert_fifo();

	/* do first actual acceleration engine command:
	 * setup clipping region (workspace size) to 32768 x 32768 pixels:
	 * wait for room in fifo for clipping cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_image_black_rectangle_ptr->FifoFree) >> 2) < 2)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup clipping (writing 2 32bit words) */
	nv_image_black_rectangle_ptr->TopLeft = 0x00000000;
	nv_image_black_rectangle_ptr->HeightWidth = 0x80008000;

	return B_OK;
}

static void nv_init_for_3D(void)
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

		/* setup (initialize) pipe */
		/* set eyetype to local, lightning etc. is off */
		ACCW(NV10_XFMOD0, 0x10000000);
		/* disable all lights */
		ACCW(NV10_XFMOD1, 0x00000000);

		/* note: upon writing data into the PIPEDAT register, the PIPEADR is
		 * probably auto-incremented! */
		/* (pipe adress = b2-16, pipe data = b0-31) */
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
		 * other options possible are: floating point; 24bits depth; W-buffer(?) */
		ACCW(GLOB_STAT_0, 0x10000000);
		/* set DMA instance 2 and 3 to be invalid */
		ACCW(GLOB_STAT_1, 0x00000000);
	}
}

/* fixme? (check this out..)
 * Looks like this stuff can be very much simplified and speed-up, as it seems it's not
 * nessesary to wait for the engine to become idle before re-assigning channels.
 * Because the cmd handles are actually programmed _inside_ the fifo channels, it might
 * well be that the assignment is buffered along with the commands that still have to 
 * be executed!
 * (sounds very plausible to me :) */
void nv_acc_assert_fifo(void)
{
	/* does every engine cmd this accelerant needs have a FIFO channel? */
	//fixme: can probably be optimized for both speed and channel selection...
	if (!si->engine.fifo.ch_ptr[NV_ROP5_SOLID] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLACK_RECTANGLE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_PATTERN] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLIT] ||
		!si->engine.fifo.ch_ptr[NV4_GDI_RECTANGLE_TEXT])
	{
		uint16 cnt;

		/* no, wait until the engine is idle before re-assigning the FIFO */
		nv_acc_wait_idle();

		/* free the FIFO channels we want from the currently assigned cmd's */
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[0]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[1]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[2]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[4]] = 0;
		si->engine.fifo.ch_ptr[si->engine.fifo.handle[5]] = 0;

		/* set new object handles */
		si->engine.fifo.handle[0] = NV_ROP5_SOLID;
		si->engine.fifo.handle[1] = NV_IMAGE_BLACK_RECTANGLE;
		si->engine.fifo.handle[2] = NV_IMAGE_PATTERN;
		si->engine.fifo.handle[4] = NV_IMAGE_BLIT;
		si->engine.fifo.handle[5] = NV4_GDI_RECTANGLE_TEXT;

		/* set handle's pointers to their assigned FIFO channels */
		for (cnt = 0; cnt < 0x08; cnt++)
		{
			si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
				(NVACC_FIFO + (cnt * 0x00002000));
		}

		/* program new FIFO assignments */
		ACCW(FIFO_CH0, (0x80000000 | si->engine.fifo.handle[0])); /* Raster OPeration */
		ACCW(FIFO_CH1, (0x80000000 | si->engine.fifo.handle[1])); /* Clip */
		ACCW(FIFO_CH2, (0x80000000 | si->engine.fifo.handle[2])); /* Pattern */
		ACCW(FIFO_CH4, (0x80000000 | si->engine.fifo.handle[4])); /* Blit */
		ACCW(FIFO_CH5, (0x80000000 | si->engine.fifo.handle[5])); /* Bitmap */
	}

	/* update our local pointers */
	nv_rop5_solid_ptr = (cmd_nv_rop5_solid*)
		&(regs[(si->engine.fifo.ch_ptr[NV_ROP5_SOLID]) >> 2]);

	nv_image_black_rectangle_ptr = (cmd_nv_image_black_rectangle*)
		&(regs[(si->engine.fifo.ch_ptr[NV_IMAGE_BLACK_RECTANGLE]) >> 2]);

	nv_image_pattern_ptr = (cmd_nv_image_pattern*)
		&(regs[(si->engine.fifo.ch_ptr[NV_IMAGE_PATTERN]) >> 2]);

	nv_image_blit_ptr = (cmd_nv_image_blit*)
		&(regs[(si->engine.fifo.ch_ptr[NV_IMAGE_BLIT]) >> 2]);

	nv3_gdi_rectangle_text_ptr = (cmd_nv3_gdi_rectangle_text*)
		&(regs[(si->engine.fifo.ch_ptr[NV4_GDI_RECTANGLE_TEXT]) >> 2]);
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nv_acc_setup_blit()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_image_pattern_ptr->FifoFree) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	nv_image_pattern_ptr->SetShape = 0x00000000; /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_image_pattern_ptr->SetColor0 = 0xffffffff;
	nv_image_pattern_ptr->SetColor1 = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[0] = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[1] = 0xffffffff;

	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_rop5_solid_ptr->FifoFree) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10);
	}
	/* now setup ROP (writing 1 32bit word) */
	nv_rop5_solid_ptr->SetRop5 = 0xcc;

	return B_OK;
}

status_t nv_acc_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* Note: blit-copy direction is determined inside riva hardware: no setup needed */

	/* instruct engine what to blit:
	 * wait for room in fifo for blit cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_image_blit_ptr->FifoFree) >> 2) < 3)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup blit (writing 3 32bit words) */
	nv_image_blit_ptr->SourceOrg = ((ys << 16) | xs);
	nv_image_blit_ptr->DestOrg = ((yd << 16) | xd);
	nv_image_blit_ptr->HeightWidth = (((h + 1) << 16) | (w + 1));

	return B_OK;
}

/* rectangle fill - i.e. workspace and window background color */
/* span fill - i.e. (selected) menuitem background color (Dano) */
status_t nv_acc_setup_rectangle(uint32 color)
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_image_pattern_ptr->FifoFree) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	nv_image_pattern_ptr->SetShape = 0x00000000; /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_image_pattern_ptr->SetColor0 = 0xffffffff;
	nv_image_pattern_ptr->SetColor1 = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[0] = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[1] = 0xffffffff;

	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_rop5_solid_ptr->FifoFree) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup ROP (writing 1 32bit word) for GXcopy */
	nv_rop5_solid_ptr->SetRop5 = 0xcc;

	/* setup fill color:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv3_gdi_rectangle_text_ptr->FifoFree) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10);
	}
	/* now setup color (writing 1 32bit word) */
	nv3_gdi_rectangle_text_ptr->Color1A = color;

	return B_OK;
}

status_t nv_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to fill:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv3_gdi_rectangle_text_ptr->FifoFree) >> 2) < 2)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup fill (writing 2 32bit words) */
	nv3_gdi_rectangle_text_ptr->UnclippedRectangle[0].LeftTop =
		((xs << 16) | (ys & 0x0000ffff));
	nv3_gdi_rectangle_text_ptr->UnclippedRectangle[0].WidthHeight =
		(((xe - xs) << 16) | (yl & 0x0000ffff));

	return B_OK;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t nv_acc_setup_rect_invert()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_image_pattern_ptr->FifoFree) >> 2) < 5)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup pattern (writing 5 32bit words) */
	nv_image_pattern_ptr->SetShape = 0x00000000; /* 0 = 8x8, 1 = 64x1, 2 = 1x64 */
	nv_image_pattern_ptr->SetColor0 = 0xffffffff;
	nv_image_pattern_ptr->SetColor1 = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[0] = 0xffffffff;
	nv_image_pattern_ptr->SetPattern[1] = 0xffffffff;

	/* ROP registers (Raster OPeration):
	 * wait for room in fifo for ROP cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv_rop5_solid_ptr->FifoFree) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup ROP (writing 1 32bit word) for GXinvert */
	nv_rop5_solid_ptr->SetRop5 = 0x55;

	/* reset fill color:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv3_gdi_rectangle_text_ptr->FifoFree) >> 2) < 1)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now reset color (writing 1 32bit word) */
	nv3_gdi_rectangle_text_ptr->Color1A = 0x00000000;

	return B_OK;
}

status_t nv_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	/* instruct engine what to invert:
	 * wait for room in fifo for bitmap cmd if needed.
	 * (fifo holds 256 32bit words: count those, not bytes) */
	while (((nv3_gdi_rectangle_text_ptr->FifoFree) >> 2) < 2)
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (10); 
	}
	/* now setup invert (writing 2 32bit words) */
	nv3_gdi_rectangle_text_ptr->UnclippedRectangle[0].LeftTop =
		((xs << 16) | (ys & 0x0000ffff));
	nv3_gdi_rectangle_text_ptr->UnclippedRectangle[0].WidthHeight =
		(((xe - xs) << 16) | (yl & 0x0000ffff));

	return B_OK;
}

/* screen to screen tranparent blit */
status_t nv_acc_transparent_blit(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h,uint32 colour)
{
	//fixme: implement.

	return B_ERROR;
}

/* screen to screen scaled filtered blit - i.e. scale video in memory */
status_t nv_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd)
{
	//fixme: implement.

	return B_ERROR;
}
