/* NV Acceleration functions */

/* Author:
   Rudolf Cornelissen 8/2003-1/2005.

   This code was possible thanks to:
    - the Linux XFree86 NV driver,
    - the Linux UtahGLX 3D driver.
*/

/*
	note:
	attempting DMA on NV40 and higher because without it I can't get it going ATM.
	Later on this can become a nv.settings switch, and maybe later we can even
	forget about non-DMA completely (depends on 3D acceleration attempts).
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
static void nv_acc_assert_fifo_dma(void);

status_t nv_acc_wait_idle_dma()
{
	/* wait until engine completely idle */

	/* wait until all upcoming commands are in execution at least */
	while (NV_REG32(NVACC_FIFO + NV_GENERAL_DMAGET +
			si->engine.fifo.handle[(si->engine.fifo.ch_ptr[NV_ROP5_SOLID])])
			!= (si->engine.dma.put << 2))
	{
		/* snooze a bit so I do not hammer the bus */
		snooze (100);
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
	//fixme: move to shared info:
	uint32 max;

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

		ACCW(HT_HANDL_02, (0x80000000 | NV3_GDI_RECTANGLE_TEXT)); /* 32bit handle */
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
		ACCW(HT_HANDL_00, (0x80000000 | NV3_SURFACE_1)); /* 32bit handle (not used) */
		ACCW(HT_VALUE_00, 0x8001114c); /* instance $114c, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_01, (0x80000000 | NV_IMAGE_BLIT)); /* 32bit handle */
		ACCW(HT_VALUE_01, 0x80011148); /* instance $1146, engine = acc engine, CHID = $00 */

		ACCW(HT_HANDL_02, (0x80000000 | NV3_GDI_RECTANGLE_TEXT)); /* 32bit handle */
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
	 * CTX determines which HT handles point to what engine commands.
	 * (CTX registers are actually a sort of RAM space.) */
	if (si->ps.card_arch >= NV40A)
	{
		/* setup a DMA define for use by command defines below. */
		ACCW(PR_CTX0_R, 0x00003000); /* DMA page table present and of linear type;
									  * DMA target node is NVM (non-volatile memory?)
									  * (instead of doing PCI or AGP transfers) */
		ACCW(PR_CTX1_R, (si->ps.memory_size - 1)); /* DMA limit */
		ACCW(PR_CTX2_R, 0x00000002); /* DMA access type is READ_AND_WRITE */
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
		/* setup set '5' for cmd NV3_GDI_RECTANGLE_TEXT */
		ACCW(PR_CTX0_8, 0x0208004b); /* NVclass $04b, patchcfg ROP_AND, nv10+: little endian */
		ACCW(PR_CTX1_8, 0x02000000); /* colorspace not set, notify instance is $0200 (b16-31) */
		ACCW(PR_CTX2_8, 0x00000000); /* DMA0 and DMA1 instance invalid */
		ACCW(PR_CTX3_8, 0x00000000); /* method traps disabled */
		ACCW(PR_CTX0_9, 0x00000000); /* extra */
		ACCW(PR_CTX1_9, 0x00000000); /* extra */
		/* setup set '6' for cmd NV10_CONTEXT_SURFACES_2D */
		ACCW(PR_CTX0_A, 0x02080062); /* NVclass $062, nv10+: little endian */
		ACCW(PR_CTX1_A, 0x00000000); /* colorspace not set, notify instance invalid (b16-31) */
		ACCW(PR_CTX2_6, 0x00001140); /* DMA0 instance is $1140, DMA1 instance invalid */
		ACCW(PR_CTX3_6, 0x00001140); /* method trap 0 is $1140, trap 1 disabled */
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
		//fixme: setup...
	}

	if (si->ps.card_arch == NV04A)
	{
/*
       if((pNv->Chipset & 0x0fff) == 0x0020)
       {
           pNv->PRAMIN[0x0824] |= 0x00020000;
           pNv->PRAMIN[0x0826] += pNv->FbAddress;
       }
       pNv->PGRAPH[0x0080/4] = 0x000001FF;//acc DEBUG0
       pNv->PGRAPH[0x0080/4] = 0x1230C000;
       pNv->PGRAPH[0x0084/4] = 0x72111101;
       pNv->PGRAPH[0x0088/4] = 0x11D5F071;
       pNv->PGRAPH[0x008C/4] = 0x0004FF31;
       pNv->PGRAPH[0x008C/4] = 0x4004FF31;

       pNv->PGRAPH[0x0140/4] = 0x00000000;
       pNv->PGRAPH[0x0100/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0170/4] = 0x10010100;
       pNv->PGRAPH[0x0710/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0720/4] = 0x00000001;

       pNv->PGRAPH[0x0810/4] = 0x00000000;
       pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF; 
*/
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
/*
           pNv->PGRAPH[0x0084/4] = 0x00118700;//acc DEBUG1
           pNv->PGRAPH[0x0088/4] = 0x24E00810;//acc DEBUG2
           pNv->PGRAPH[0x008C/4] = 0x55DE0030;//acc DEBUG3

//tile spul NV10:
           for(i = 0; i < 32; i++)
             pNv->PGRAPH[(0x0B00/4) + i] = pNv->PFB[(0x0240/4) + i];//NV10_TIL0AD etc

           pNv->PGRAPH[0x640/4] = 0;//OFFSET0
           pNv->PGRAPH[0x644/4] = 0;//OFFSET1
           pNv->PGRAPH[0x684/4] = pNv->FbMapSize - 1;//BLIMIT0
           pNv->PGRAPH[0x688/4] = pNv->FbMapSize - 1;//BLIMIT1

           pNv->PGRAPH[0x0810/4] = 0x00000000;//PAT_SHP
           pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF;//<<<<<<<<<<<< nvx: NV_PGRAPH_BETA_AND
*/
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
			case NV45: //fixme, checkout: this is cardID 0x016x at least!
				ACCW(NV40P_WHAT0, 0x83280eff);
				ACCW(NV40P_WHAT1, 0x000000a0);

				NV_REG32(NV32_NV45_WHAT10) = NV_REG32(NV32_NV10STRAPINFO);
				NV_REG32(NV32_NV45_WHAT11) = 0x00000000;
				NV_REG32(NV32_NV45_WHAT12) = 0x00000000;
				NV_REG32(NV32_NV45_WHAT13) = NV_REG32(NV32_NV10STRAPINFO);

				ACCW(NV45_WHAT2, 0x00000000);
				ACCW(NV45_WHAT3, 0x00000000);
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
			ACCW(NV4X_WHAT0, 0x00001200);//1200 DMA instance????(test shutoff in PIO mod)
//test if trouble:
//			ACCW(NV4X_WHAT0, 0x00001140);
			break;
		case NV30A:
/*
              pNv->PGRAPH[0x0084/4] = 0x40108700;//acc DEBUG1
              pNv->PGRAPH[0x0890/4] = 0x00140000;//0x00400890 nieuw: unknown!!<<<<<<<<(NV25)
              pNv->PGRAPH[0x008C/4] = 0xf00e0431;//acc DEBUG3
              pNv->PGRAPH[0x0090/4] = 0x00008000;//acc NV10_DEBUG4
              pNv->PGRAPH[0x0610/4] = 0xf04b1f36;//NVACC_NV4X_WHAT2 nw, dus ook op NV30!<<<
              pNv->PGRAPH[0x0B80/4] = 0x1002d888;//0x00400b80 nieuw: unknown!!<<<<<<<
              pNv->PGRAPH[0x0B88/4] = 0x62ff007f;//0x00400b88 nieuw: unknown!!<<<<<<<
*/
			break;
		case NV20A:
/*
              pNv->PGRAPH[0x0084/4] = 0x00118700;//acc DEBUG1
              pNv->PGRAPH[0x008C/4] = 0xF20E0431;//acc DEBUG3
              pNv->PGRAPH[0x0090/4] = 0x00000000;//acc NV10_DEBUG4
              pNv->PGRAPH[0x009C/4] = 0x00000040;//0x0040009c nieuw: unknown!!<<<<<<<<

              if((pNv->Chipset & 0x0ff0) >= 0x0250)
              {
                 pNv->PGRAPH[0x0890/4] = 0x00080000;//0x00400890 nieuw: unknown!!<<<<<<<<
                 pNv->PGRAPH[0x0610/4] = 0x304B1FB6;//NVACC_NV4X_WHAT2 nw,ook op NV25 en+!<<< 
                 pNv->PGRAPH[0x0B80/4] = 0x18B82880;//0x00400b80 nieuw: unknown!!<<<<<<< 
                 pNv->PGRAPH[0x0B84/4] = 0x44000000;//0x00400b84 nieuw: unknown!!<<<<<<< 
                 pNv->PGRAPH[0x0098/4] = 0x40000080;//0x00400098 nieuw: unknown!!<<<<<<< 
                 pNv->PGRAPH[0x0B88/4] = 0x000000ff;//0x00400b88 nieuw: unknown!!<<<<<<< 
              }
              else
              {
                 pNv->PGRAPH[0x0880/4] = 0x00080000;
                 pNv->PGRAPH[0x0094/4] = 0x00000005;
                 pNv->PGRAPH[0x0B80/4] = 0x45CAA208; 
                 pNv->PGRAPH[0x0B84/4] = 0x24000000;
                 pNv->PGRAPH[0x0098/4] = 0x00000040;
                 pNv->PGRAPH[0x0750/4] = 0x00E00038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
                 pNv->PGRAPH[0x0750/4] = 0x00E10038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
              }
*/
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
			if (si->ps.card_type == NV40)
			{
				/* copy unknown tile setup stuff from 'source' to acc engine(?) */
 				ACCW(NV20_WHAT_T0, ACCR(NV20_FBWHAT0));
				ACCW(NV20_WHAT_T1, ACCR(NV20_FBWHAT1));
				ACCW(NV40_WHAT_T2, ACCR(NV20_FBWHAT0));
				ACCW(NV40_WHAT_T3, ACCR(NV20_FBWHAT1));

				/* setup accesible card memory range for acc engine */
				//fixme: should these two be zero after all??!!
				ACCW(NV20_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				ACCW(NV20_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				//end fixme.
				ACCW(NV20_BLIMIT6, (si->ps.memory_size - 1));
				ACCW(NV20_BLIMIT7, (si->ps.memory_size - 1));
			}
			else
			{
				/* copy unknown tile setup stuff from 'source' to acc engine(?) */
				ACCW(NV40P_WHAT_T0, ACCR(NV20_FBWHAT0));
				ACCW(NV40P_WHAT_T1, ACCR(NV20_FBWHAT1));
				ACCW(NV40P_WHAT_T2, ACCR(NV20_FBWHAT0));
				ACCW(NV40P_WHAT_T3, ACCR(NV20_FBWHAT1));

				/* setup accesible card memory range for acc engine */
				//fixme: should these two be zero after all??!!
				ACCW(NV40P_OFFSET0, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				ACCW(NV40P_OFFSET1, ((uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer));
				//end fixme.
				ACCW(NV40P_BLIMIT6, (si->ps.memory_size - 1));
				ACCW(NV40P_BLIMIT7, (si->ps.memory_size - 1));
			}
		}
		else /* NV20A and NV30A: */
		{
/*
   			//NVACC_NV20_WHAT0 from NV20_FBWHAT0
              pNv->PGRAPH[0x09A4/4] = pNv->PFB[0x0200/4];
     		//NVACC_NV20_WHAT1 from NV20_FBWHAT1
              pNv->PGRAPH[0x09A8/4] = pNv->PFB[0x0204/4];

              pNv->PGRAPH[0x0750/4] = 0x00EA0000;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0200/4];
              pNv->PGRAPH[0x0750/4] = 0x00EA0004;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0204/4];

              pNv->PGRAPH[0x0820/4] = 0;//NV20_OFFSET0
              pNv->PGRAPH[0x0824/4] = 0;//NV20_OFFSET1
              pNv->PGRAPH[0x0864/4] = pNv->FbMapSize - 1;//NV20_BLIMIT6
              pNv->PGRAPH[0x0868/4] = pNv->FbMapSize - 1;//NV20_BLIMIT7
*/
		}

		/* NV20A, NV30A and NV40A: */
		/* setup some acc engine tile stuff */
		ACCW(NV10_TIL2AD, 0x00000000);
		ACCW(NV10_TIL0ED, 0xffffffff);
	}

	/* all cards: */
	/* setup some clipping: rect size is 32768 x 32768, probably max. setting */
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
	 * This define tells the engine where the DMA cmd buffer is and what it's size is;
	 * inside that cmd buffer you'll find the engine handles for the FIFO channels,
	 * followed by actual issued engine commands. */
	//fixme: if needed...
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
	si->engine.fifo.handle[5] = NV3_GDI_RECTANGLE_TEXT;
	si->engine.fifo.handle[6] = NV1_RENDER_SOLID_LIN;
	si->engine.fifo.handle[7] = NV4_DX5_TEXTURE_TRIANGLE;
	/* preset no FIFO channels assigned to cmd's */
	for (cnt = 0; cnt < 0x20; cnt++)
	{
		si->engine.fifo.ch_ptr[cnt] = 0;
	}
	/* set handle's pointers to their assigned FIFO channels */
	for (cnt = 0; cnt < 0x08; cnt++)
		si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] = (cnt * 0x00002000);

	/* init DMA command buffer pointer */
	si->engine.dma.cmdbuffer = (uint32 *)((char *)si->framebuffer +
		((si->ps.memory_size - 1) & 0xffff8000));
	LOG(4,("ACC_DMA: command buffer is at adress $%08x\n",
		((uint32)(si->engine.dma.cmdbuffer))));

	/* init FIFO via DMA command buffer. */
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
	 * to set new adresses if jumps are needed. */
	si->engine.dma.cmdbuffer[0x00] = (1 << 18) | 0x00000;
	/* send actual cmd word */
	si->engine.dma.cmdbuffer[0x01] =
		(0x80000000 | si->engine.fifo.handle[0]); /* Raster OPeration */

	/* etc.. */
	si->engine.dma.cmdbuffer[0x02] = (1 << 18) | 0x02000;
	si->engine.dma.cmdbuffer[0x03] =
		(0x80000000 | si->engine.fifo.handle[1]); /* Clip */

	si->engine.dma.cmdbuffer[0x04] = (1 << 18) | 0x04000;
	si->engine.dma.cmdbuffer[0x05] =
		(0x80000000 | si->engine.fifo.handle[2]); /* Pattern */

	si->engine.dma.cmdbuffer[0x06] = (1 << 18) | 0x06000;
	si->engine.dma.cmdbuffer[0x07] =
		(0x80000000 | si->engine.fifo.handle[3]); /* 2D Surface */

	si->engine.dma.cmdbuffer[0x08] = (1 << 18) | 0x08000;
	si->engine.dma.cmdbuffer[0x09] =
		(0x80000000 | si->engine.fifo.handle[4]); /* Blit */

	si->engine.dma.cmdbuffer[0x0a] = (1 << 18) | 0x0a000;
	si->engine.dma.cmdbuffer[0x0b] =
		(0x80000000 | si->engine.fifo.handle[5]); /* Bitmap */

	si->engine.dma.cmdbuffer[0x0c] = (1 << 18) | 0x0c000;
//	si->engine.dma.cmdbuffer[0x0d] =
//		(0x80000000 | si->engine.fifo.handle[6]); /* Line (not used or 3D only?) */
//fixme: temporary so there's something valid here.. (maybe needed, don't yet know)
	si->engine.dma.cmdbuffer[0x0d] =
		(0x80000000 | si->engine.fifo.handle[0]);

	si->engine.dma.cmdbuffer[0x0e] = (1 << 18) | 0x0e000;
//	si->engine.dma.cmdbuffer[0x0f] =
//		(0x80000000 | si->engine.fifo.handle[7]); /* Textured Triangle (3D only) */
//fixme: temporary so there's something valid here.. (maybe needed, don't yet know)
	si->engine.dma.cmdbuffer[0x0f] =
		(0x80000000 | si->engine.fifo.handle[0]);

	/* initialize our local pointers */
	nv_acc_assert_fifo_dma();

	/* we have issued no DMA cmd's to the engine yet: the above ones are still
	 * awaiting execution start. */
	si->engine.dma.put = 0;
	/* the current first free adress in the DMA buffer is at offset 16 */
	si->engine.dma.current = 16;
	/* the DMA buffer can hold 8k 32-bit words (it's 32kb in size) */
	/* note:
	 * one word is reserved at the end of the DMA buffer to be able to instruct the
	 * engine to do a buffer wrap-around!
	 * (DMA opcode 'noninc method': issue word $20000000.) */
	/*si->dma.*/max = 8191;
	/* note the current free space we have left in the DMA buffer */
	si->engine.dma.free = /*si->dma.*/max - si->engine.dma.current /*+ 1*/;

	//fixme: add colorspace and buffer config cmd's or predefine in the non-DMA way.
	//fixme: overlay should stay outside the DMA buffer, also add a failsafe
	//       space in between both functions as errors might hang the engine!

	/* tell the engine to fetch the commands in the DMA buffer that where not
	 * executed before. */
	nv_start_dma();

	return B_OK;
}

static void nv_start_dma(void)
{
	uint8 dummy;

	if (si->engine.dma.current != si->engine.dma.put)
	{
		si->engine.dma.put = si->engine.dma.current;
		/* fixme: is this actually needed? (force some flush somewhere) */
		ISAWB(0x03d0, 0x00);
		/* dummy read the first adress of the framebuffer: flushes MTRR-WC buffers so
		 * we know for sure the DMA command buffer received all data. */
		dummy = *((char *)(si->framebuffer));
		/* actually start DMA to execute all commands now in buffer */
		/* note:
		 * the actual FIFO channel that gets activated does not really matter:
		 * all FIFO fill-level info actually points at the same registers. */
		NV_REG32(NVACC_FIFO + NV_GENERAL_DMAPUT +
			si->engine.fifo.handle[(si->engine.fifo.ch_ptr[NV_ROP5_SOLID])]) =
			(si->engine.dma.put << 2);
	}
//test:
for (dummy = 0; dummy < 2; dummy++)
{
	LOG(4,("ACC_DMA: get $%08x\n", NV_REG32(NVACC_FIFO + NV_GENERAL_DMAGET +
		si->engine.fifo.handle[(si->engine.fifo.ch_ptr[NV_ROP5_SOLID])])));
	LOG(4,("ACC_DMA: put $%08x\n", NV_REG32(NVACC_FIFO + NV_GENERAL_DMAPUT +
		si->engine.fifo.handle[(si->engine.fifo.ch_ptr[NV_ROP5_SOLID])])));
}
}

static status_t nv_acc_fifofree_dma(uint16 cmd_size)
{
	//fixme: implement buffer wraparounds.. (buffer resets when full)
	//for now only executing the first commands issued and drop exec after;
	//this offers testing options already (depending on how much windows are visible,
	//exec remains going say 30 seconds to a few minutes if only blits are enabled.
//	if (si->dma.free >= cmd_size) return B_OK;
	if ((si->engine.dma.current + cmd_size) < 8191) return B_OK;

	return B_ERROR;
}

static void nv_acc_cmd_dma(uint32 cmd, uint16 offset, uint16 size)
{
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = ((size << 18) |
		(si->engine.fifo.ch_ptr[cmd] + offset));
}

/* fixme? (check this out..)
 * Looks like this stuff can be very much simplified and speed-up, as it seems it's not
 * nessesary to wait for the engine to become idle before re-assigning channels.
 * Because the cmd handles are actually programmed _inside_ the fifo channels, it might
 * well be that the assignment is buffered along with the commands that still have to 
 * be executed!
 * (sounds very plausible to me :) */
static void nv_acc_assert_fifo_dma(void)
{
	/* does every engine cmd this accelerant needs have a FIFO channel? */
	//fixme: can probably be optimized for both speed and channel selection...
	if (!si->engine.fifo.ch_ptr[NV_ROP5_SOLID] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLACK_RECTANGLE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_PATTERN] ||
		!si->engine.fifo.ch_ptr[NV4_SURFACE] ||
		!si->engine.fifo.ch_ptr[NV_IMAGE_BLIT] ||
		!si->engine.fifo.ch_ptr[NV3_GDI_RECTANGLE_TEXT])
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
		si->engine.fifo.handle[5] = NV3_GDI_RECTANGLE_TEXT;

		/* set handle's pointers to their assigned FIFO channels */
		for (cnt = 0; cnt < 0x08; cnt++)
		{
			si->engine.fifo.ch_ptr[(si->engine.fifo.handle[cnt])] =
				(cnt * 0x00002000);
		}

		/* program new FIFO assignments */
		//fixme: should be done via DMA cmd buffer...
		//ACCW(FIFO_CH0, (0x80000000 | si->engine.fifo.handle[0])); /* Raster OPeration */
		//ACCW(FIFO_CH1, (0x80000000 | si->engine.fifo.handle[1])); /* Clip */
		//ACCW(FIFO_CH2, (0x80000000 | si->engine.fifo.handle[2])); /* Pattern */
		//ACCW(FIFO_CH3, (0x80000000 | si->engine.fifo.handle[3])); /* 2D Surface */
		//ACCW(FIFO_CH4, (0x80000000 | si->engine.fifo.handle[4])); /* Blit */
		//ACCW(FIFO_CH5, (0x80000000 | si->engine.fifo.handle[5])); /* Bitmap */
	}
}

/* screen to screen blit - i.e. move windows around and scroll within them. */
status_t nv_acc_setup_blit_dma()
{
	/* setup solid pattern:
	 * wait for room in fifo for pattern cmd if needed. */
	//fixme: testing..
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
	//fixme: testing..
	if (nv_acc_fifofree_dma(2) != B_OK) return B_ERROR;

	/* now setup ROP (writing 2 32bit words) */
	nv_acc_cmd_dma(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
	si->engine.dma.cmdbuffer[si->engine.dma.current++] = 0xcc; /* SetRop5 */

	return B_OK;
}

status_t nv_acc_blit_dma(uint16 xs,uint16 ys,uint16 xd,uint16 yd,uint16 w,uint16 h)
{
	/* Note: blit-copy direction is determined inside riva hardware: no setup needed */

	/* instruct engine what to blit:
	 * wait for room in fifo for blit cmd if needed. */
	//fixme: testing..
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
	//fixme: implement.

	return B_ERROR;
}

status_t nv_acc_rectangle_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	//fixme: implement.

	return B_ERROR;
}

/* rectangle invert - i.e. text cursor and text selection */
status_t nv_acc_setup_rect_invert_dma()
{
	//fixme: implement.

	return B_ERROR;
}

status_t nv_acc_rectangle_invert_dma(uint32 xs,uint32 xe,uint32 ys,uint32 yl)
{
	//fixme: implement.

	return B_ERROR;
}
