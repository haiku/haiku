/* NV registers definitions and macros for access to */

//new:
/* PCI_config_space */
#define NVCFG_DEVID		0x00
#define NVCFG_DEVCTRL	0x04
#define NVCFG_CLASS		0x08
#define NVCFG_HEADER	0x0c
#define NVCFG_BASE1REGS	0x10
#define NVCFG_BASE2FB	0x14
#define NVCFG_BASE3		0x18
#define NVCFG_BASE4		0x1c //unknown if used
#define NVCFG_BASE5		0x20 //unknown if used
#define NVCFG_BASE6		0x24 //unknown if used
#define NVCFG_BASE7		0x28 //unknown if used
#define NVCFG_SUBSYSID1	0x2c
#define NVCFG_ROMBASE	0x30
#define NVCFG_CFG_0		0x34
#define NVCFG_CFG_1		0x38 //unknown if used
#define NVCFG_INTERRUPT	0x3c
#define NVCFG_SUBSYSID2	0x40
#define NVCFG_AGPREF	0x44
#define NVCFG_AGPSTAT	0x48
#define NVCFG_AGPCMD	0x4c
#define NVCFG_ROMSHADOW	0x50
#define NVCFG_VGA		0x54
#define NVCFG_SCHRATCH	0x58
#define NVCFG_CFG_10	0x5c
#define NVCFG_CFG_11	0x60
#define NVCFG_CFG_12	0x64
#define NVCFG_CFG_13	0x68 //unknown if used
#define NVCFG_CFG_14	0x6c //unknown if used
#define NVCFG_CFG_15	0x70 //unknown if used
#define NVCFG_CFG_16	0x74 //unknown if used
#define NVCFG_CFG_17	0x78 //unknown if used
#define NVCFG_GF2IGPU	0x7c
#define NVCFG_CFG_19	0x80 //unknown if used
#define NVCFG_GF4MXIGPU	0x84
#define NVCFG_CFG_21	0x88 //unknown if used
#define NVCFG_CFG_22	0x8c //unknown if used
#define NVCFG_CFG_23	0x90 //unknown if used
#define NVCFG_CFG_24	0x94 //unknown if used
#define NVCFG_CFG_25	0x98 //unknown if used
#define NVCFG_CFG_26	0x9c //unknown if used
#define NVCFG_CFG_27	0xa0 //unknown if used
#define NVCFG_CFG_28	0xa4 //unknown if used
#define NVCFG_CFG_29	0xa8 //unknown if used
#define NVCFG_CFG_30	0xac //unknown if used
#define NVCFG_CFG_31	0xb0 //unknown if used
#define NVCFG_CFG_32	0xb4 //unknown if used
#define NVCFG_CFG_33	0xb8 //unknown if used
#define NVCFG_CFG_34	0xbc //unknown if used
#define NVCFG_CFG_35	0xc0 //unknown if used
#define NVCFG_CFG_36	0xc4 //unknown if used
#define NVCFG_CFG_37	0xc8 //unknown if used
#define NVCFG_CFG_38	0xcc //unknown if used
#define NVCFG_CFG_39	0xd0 //unknown if used
#define NVCFG_CFG_40	0xd4 //unknown if used
#define NVCFG_CFG_41	0xd8 //unknown if used
#define NVCFG_CFG_42	0xdc //unknown if used
#define NVCFG_CFG_43	0xe0 //unknown if used
#define NVCFG_CFG_44	0xe4 //unknown if used
#define NVCFG_CFG_45	0xe8 //unknown if used
#define NVCFG_CFG_46	0xec //unknown if used
#define NVCFG_CFG_47	0xf0 //unknown if used
#define NVCFG_CFG_48	0xf4 //unknown if used
#define NVCFG_CFG_49	0xf8 //unknown if used
#define NVCFG_CFG_50	0xfc //unknown if used

/*    if(pNv->SecondCRTC) {
       pNv->riva.PCIO = pNv->riva.PCIO0 + 0x2000;
       pNv->riva.PCRTC = pNv->riva.PCRTC0 + 0x800;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0 + 0x800;
       pNv->riva.PDIO = pNv->riva.PDIO0 + 0x2000;
    } else {
       pNv->riva.PCIO = pNv->riva.PCIO0;
       pNv->riva.PCRTC = pNv->riva.PCRTC0;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0;
       pNv->riva.PDIO = pNv->riva.PDIO0;
    }
*/
    /*
     * These registers are read/write as 8 bit values.  Probably have to map
     * sparse on alpha.
     */
/*    pNv->riva.PCIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00601000,
                                           0x00003000);
    pNv->riva.PDIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00681000,
                                           0x00003000);
    pNv->riva.PVIO = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x000C0000,
                                           0x00001000);
    pNv->riva.PRAMDAC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00680000, 0x00003000);
    pNv->riva.PCRTC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00003000);
*/

/* Nvidia PCI direct registers */
#define NV8_MISCW 			0x000c03c2
#define NV8_MISCR 			0x000c03cc
#define NV8_SEQIND			0x000c03c4
#define NV16_SEQIND			0x000c03c4
#define NV8_SEQDAT			0x000c03c5
#define NV8_GRPHIND			0x000c03ce
#define NV16_GRPHIND		0x000c03ce
#define NV8_GRPHDAT			0x000c03cf

/* bootstrap info registers */
#define NV32_NV4STRAPINFO	0x00100000
#define NV32_NV10STRAPINFO	0x0010020c
#define NV32_NVSTRAPINFO2	0x00101000

/* primary head */
#define NV8_ATTRINDW		0x006013c0
#define NV8_ATTRDATW		0x006013c0
#define NV8_ATTRDATR		0x006013c1
#define NV8_CRTCIND			0x006013d4
#define NV16_CRTCIND		0x006013d4
#define NV8_CRTCDAT			0x006013d5
#define NV8_INSTAT1			0x006013da
#define NV32_NV10FBSTADD32	0x00600800
#define NV32_CONFIG         0x00600804//not yet used (coldstart)...
#define NV32_RASTER			0x00600808
#define NV32_NV10CURADD32	0x0060080c
#define NV32_CURCONF		0x00600810

/* secondary head */
#define NV8_ATTR2INDW		0x006033c0
#define NV8_ATTR2DATW		0x006033c0
#define NV8_ATTR2DATR		0x006033c1
#define NV8_CRTC2IND		0x006033d4
#define NV16_CRTC2IND		0x006033d4
#define NV8_CRTC2DAT		0x006033d5
#define NV8_2INSTAT1		0x006033da//verify!!!
#define NV32_NV10FB2STADD32	0x00602800//verify!!!
#define NV32_RASTER2		0x00602808//verify!!!
#define NV32_NV10CUR2ADD32	0x0060280c//verify!!!
#define NV32_2CURCONF		0x00602810//verify!!!

/* Nvidia DAC direct registers (standard VGA palette RAM registers) */
/* primary head */
#define NV8_PALMASK			0x006813c6
#define NV8_PALINDR			0x006813c7
#define NV8_PALINDW			0x006813c8
#define NV8_PALDATA			0x006813c9
/* secondary head */
#define NV8_PAL2MASK		0x006833c6//verify!!!
#define NV8_PAL2INDR		0x006833c7//verify!!!
#define NV8_PAL2INDW		0x006833c8//verify!!!
#define NV8_PAL2DATA		0x006833c9//verify!!!

/* Nvidia PCI direct DAC registers (32bit) */
/* primary head */
#define NVDAC_CURPOS		0x00680300
#define NVDAC_PIXPLLC		0x00680508
#define NVDAC_PLLSEL		0x0068050c
#define NVDAC_GENCTRL		0x00680600
/* secondary head */
#define NVDAC2_CURPOS		0x00680b00
#define NVDAC2_PIXPLLC		0x00680d20//verify!!!
#define NVDAC2_PLLSEL		0x00680d0c//verify!!!
#define NVDAC2_GENCTRL		0x00680e00//verify!!!

/* Nvidia CRTC indexed registers */
/* VGA standard registers: */
#define NVCRTCX_HTOTAL		0x00
#define NVCRTCX_HDISPE		0x01
#define NVCRTCX_HBLANKS		0x02
#define NVCRTCX_HBLANKE		0x03
#define NVCRTCX_HSYNCS		0x04
#define NVCRTCX_HSYNCE		0x05
#define NVCRTCX_VTOTAL		0x06
#define NVCRTCX_OVERFLOW	0x07
#define NVCRTCX_PRROWSCN	0x08
#define NVCRTCX_MAXSCLIN	0x09
#define NVCRTCX_VGACURCTRL	0x0a
#define NVCRTCX_FBSTADDH	0x0c //confirmed
#define NVCRTCX_FBSTADDL	0x0d //confirmed
#define NVCRTCX_VSYNCS		0x10
#define NVCRTCX_VSYNCE		0x11
#define NVCRTCX_VDISPE		0x12
#define NVCRTCX_PITCHL		0x13 //confirmed
#define NVCRTCX_VBLANKS		0x15
#define NVCRTCX_VBLANKE		0x16
#define NVCRTCX_MODECTL		0x17
#define NVCRTCX_LINECOMP	0x18
/* Nvidia specific registers: */
#define NVCRTCX_REPAINT0	0x19
#define NVCRTCX_REPAINT1	0x1a
#define NVCRTCX_LOCK		0x1f
#define NVCRTCX_LSR			0x25
#define NVCRTCX_PIXEL		0x28
#define NVCRTCX_HEB			0x2d
#define NVCRTCX_CURCTL2		0x2f
#define NVCRTCX_CURCTL1		0x30
#define NVCRTCX_CURCTL0		0x31

/* Nvidia ATTRIBUTE indexed registers */
/* VGA standard registers: */
#define NVATBX_MODECTL		0x10
#define NVATBX_OSCANCOLOR	0x11
#define NVATBX_COLPLANE_EN	0x12
#define NVATBX_HORPIXPAN	0x13 //confirmed
#define NVATBX_COLSEL		0x14

/* Nvidia SEQUENCER indexed registers */
/* VGA standard registers: */
#define NVSEQX_RESET		0x00
#define NVSEQX_CLKMODE		0x01
#define NVSEQX_MEMMODE		0x04

/* Nvidia GRAPHICS indexed registers */
/* VGA standard registers: */
#define NVGRPHX_ENSETRESET	0x01
#define NVGRPHX_DATAROTATE	0x03
#define NVGRPHX_READMAPSEL	0x04
#define NVGRPHX_MODE		0x05
#define NVGRPHX_MISC		0x06
#define NVGRPHX_BITMASK		0x08
//end new.

/* (D)AC (X) (I)ndexed registers (>= G100) */
#define NVDXI_VREFCTRL        0x18
#define NVDXI_MULCTRL         0x19
#define NVDXI_PIXCLKCTRL      0x1A
#define NVDXI_GENCTRL         0x1D
#define NVDXI_MISCCTRL        0x1E
#define NVDXI_PANELMODE       0x1F
#define NVDXI_MAFCDEL         0x20
#define NVDXI_GENIOCTRL       0x2A
#define NVDXI_GENIODATA       0x2B
#define NVDXI_SYSPLLM         0x2C
#define NVDXI_SYSPLLN         0x2D
#define NVDXI_SYSPLLP         0x2E
#define NVDXI_SYSPLLSTAT      0x2F
#define NVDXI_ZOOMCTRL        0x38
#define NVDXI_SENSETEST       0x3A
#define NVDXI_CRCREML         0x3C
#define NVDXI_CRCREMH         0x3D
#define NVDXI_CRCBITSEL       0x3E
#define NVDXI_COLMSK          0x40
#define NVDXI_COLKEY          0x42
#define NVDXI_PIXPLLAM        0x44
#define NVDXI_PIXPLLAN        0x45
#define NVDXI_PIXPLLAP        0x46
#define NVDXI_PIXPLLBM        0x48
#define NVDXI_PIXPLLBN        0x49
#define NVDXI_PIXPLLBP        0x4A
#define NVDXI_PIXPLLCM        0x4C
#define NVDXI_PIXPLLCN        0x4D
#define NVDXI_PIXPLLCP        0x4E
#define NVDXI_PIXPLLSTAT      0x4F
#define NVDXI_CURCOLEXT       0x60   /*sequential from CURCOL3->15, RGB*/

/* (D)AC (X) (I)ndexed registers (>= G200) */
#define	NVDXI_KEYOPMODE	   0x51
#define	NVDXI_COLMSK0RED 	   0x52
#define	NVDXI_COLMSK0GREEN    0x53
#define	NVDXI_COLMSK0BLUE 	   0x54
#define	NVDXI_COLKEY0RED	   0x55
#define	NVDXI_COLKEY0GREEN    0x56
#define	NVDXI_COLKEY0BLUE 	   0x57

/* (D)AC (X) (I)ndexed registers (>= G450) */
#define NVDXI_TVO_IDX         0x87
#define NVDXI_TVO_DATA        0x88
#define NVDXI_OUTPUTCONN      0x8A
#define NVDXI_SYNCCTRL        0x8B
#define NVDXI_VIDPLLSTAT      0x8C
#define NVDXI_VIDPLLP         0x8D
#define NVDXI_VIDPLLM         0x8E
#define NVDXI_VIDPLLN         0x8F
#define NVDXI_PWRCTRL         0xA0
#define NVDXI_PANMODE         0xA2

/* NV 1st CRTC registers */
#define NVCR1_VCOUNT        0x1E20

/* NV 2nd CRTC registers (>= G400) */
#define NVCR2_CTL           0x3C10
#define NVCR2_HPARAM        0x3C14
#define NVCR2_HSYNC         0x3C18
#define NVCR2_VPARAM        0x3C1C
#define NVCR2_VSYNC         0x3C20
#define NVCR2_PRELOAD       0x3C24
#define NVCR2_STARTADD0     0x3C28
#define NVCR2_STARTADD1     0x3C2C
#define NVCR2_OFFSET        0x3C40
#define NVCR2_MISC          0x3C44
#define NVCR2_VCOUNT        0x3C48
#define NVCR2_DATACTL       0x3C4C

/* NV ACCeleration registers */
#define NVACC_DWGCTL          0x1C00
#define NVACC_MACCESS         0x1C04
#define NVACC_MCTLWTST        0x1C08
#define NVACC_ZORG            0x1C0C
#define NVACC_PLNWT           0x1C1C
#define NVACC_BCOL            0x1C20
#define NVACC_FCOL            0x1C24
#define NVACC_XYSTRT          0x1C40
#define NVACC_XYEND           0x1C44
#define NVACC_SGN             0x1C58
#define NVACC_LEN             0x1C5C
#define NVACC_AR0             0x1C60
#define NVACC_AR3             0x1C6C
#define NVACC_AR5             0x1C74
#define NVACC_CXBNDRY         0x1C80
#define NVACC_FXBNDRY         0x1C84
#define NVACC_YDSTLEN         0x1C88
#define NVACC_PITCH           0x1C8C
#define NVACC_YDST            0x1C90
#define NVACC_YDSTORG         0x1C94
#define NVACC_YTOP            0x1C98
#define NVACC_YBOT            0x1C9C
#define NVACC_CXLEFT          0x1CA0
#define NVACC_CXRIGHT         0x1CA4
#define NVACC_FXLEFT          0x1CA8
#define NVACC_FXRIGHT         0x1CAC
#define NVACC_STATUS          0x1E14
#define NVACC_ICLEAR          0x1E18 /* required for interrupt stuff */
#define NVACC_IEN             0x1E1C /* required for interrupt stuff */
#define NVACC_RST             0x1E40
#define NVACC_MEMRDBK         0x1E44
#define NVACC_OPMODE          0x1E54
#define NVACC_PRIMADDRESS     0x1E58
#define NVACC_PRIMEND         0x1E5C
#define NVACC_TEXORG          0x2C24 // >= G100 
#define NVACC_DWGSYNC         0x2C4C // >= G200
#define NVACC_TEXORG1         0x2CA4 // >= G200
#define NVACC_TEXORG2         0x2CA8 // >= G200 
#define NVACC_TEXORG3         0x2CAC // >= G200 
#define NVACC_TEXORG4         0x2CB0 // >= G200 
#define NVACC_SRCORG          0x2CB4 // >= G200
#define NVACC_DSTORG          0x2CB8 // >= G200

/*NV BES (Back End Scaler) registers (>= G200) */
#define NVBES_A1ORG           0x3D00
#define NVBES_A2ORG           0x3D04
#define NVBES_B1ORG           0x3D08
#define NVBES_B2ORG           0x3D0C
#define NVBES_A1CORG          0x3D10
#define NVBES_A2CORG          0x3D14
#define NVBES_B1CORG          0x3D18
#define NVBES_B2CORG          0x3D1C
#define NVBES_CTL             0x3D20
#define NVBES_PITCH           0x3D24
#define NVBES_HCOORD          0x3D28
#define NVBES_VCOORD          0x3D2C
#define NVBES_HISCAL          0x3D30
#define NVBES_VISCAL          0x3D34
#define NVBES_HSRCST          0x3D38
#define NVBES_HSRCEND         0x3D3C
#define NVBES_LUMACTL         0x3D40
#define NVBES_V1WGHT          0x3D48
#define NVBES_V2WGHT          0x3D4C
#define NVBES_HSRCLST         0x3D50
#define NVBES_V1SRCLST        0x3D54
#define NVBES_V2SRCLST        0x3D58
#define NVBES_A1C3ORG         0x3D60
#define NVBES_A2C3ORG         0x3D64
#define NVBES_B1C3ORG         0x3D68
#define NVBES_B2C3ORG         0x3D6C
#define NVBES_GLOBCTL         0x3DC0
#define NVBES_STATUS          0x3DC4

/*MAVEN registers (<= G400) */
#define NVMAV_PGM            0x3E
#define NVMAV_PIXPLLM        0x80
#define NVMAV_PIXPLLN        0x81
#define NVMAV_PIXPLLP        0x82
#define NVMAV_GAMMA1         0x83
#define NVMAV_GAMMA2         0x84
#define NVMAV_GAMMA3         0x85
#define NVMAV_GAMMA4         0x86
#define NVMAV_GAMMA5         0x87
#define NVMAV_GAMMA6         0x88
#define NVMAV_GAMMA7         0x89
#define NVMAV_GAMMA8         0x8A
#define NVMAV_GAMMA9         0x8B
#define NVMAV_MONSET         0x8C
#define NVMAV_TEST           0x8D
#define NVMAV_WREG_0X8E_L    0x8E
#define NVMAV_WREG_0X8E_H    0x8F
#define NVMAV_HSCALETV       0x90
#define NVMAV_TSCALETVL      0x91
#define NVMAV_TSCALETVH      0x92
#define NVMAV_FFILTER        0x93
#define NVMAV_MONEN          0x94
#define NVMAV_RESYNC         0x95
#define NVMAV_LASTLINEL      0x96
#define NVMAV_LASTLINEH      0x97
#define NVMAV_WREG_0X98_L    0x98
#define NVMAV_WREG_0X98_H    0x99
#define NVMAV_HSYNCLENL      0x9A
#define NVMAV_HSYNCLENH      0x9B
#define NVMAV_HSYNCSTRL      0x9C
#define NVMAV_HSYNCSTRH      0x9D
#define NVMAV_HDISPLAYL      0x9E
#define NVMAV_HDISPLAYH      0x9F
#define NVMAV_HTOTALL        0xA0
#define NVMAV_HTOTALH        0xA1
#define NVMAV_VSYNCLENL      0xA2
#define NVMAV_VSYNCLENH      0xA3
#define NVMAV_VSYNCSTRL      0xA4
#define NVMAV_VSYNCSTRH      0xA5
#define NVMAV_VDISPLAYL      0xA6
#define NVMAV_VDISPLAYH      0xA7
#define NVMAV_VTOTALL        0xA8
#define NVMAV_VTOTALH        0xA9
#define NVMAV_HVIDRSTL       0xAA
#define NVMAV_HVIDRSTH       0xAB
#define NVMAV_VVIDRSTL       0xAC
#define NVMAV_VVIDRSTH       0xAD
#define NVMAV_VSOMETHINGL    0xAE
#define NVMAV_VSOMETHINGH    0xAF
#define NVMAV_OUTMODE        0xB0
#define NVMAV_LOCK           0xB3
#define NVMAV_LUMA           0xB9
#define NVMAV_VDISPLAYTV     0xBE
#define NVMAV_STABLE         0xBF
#define NVMAV_HDISPLAYTV     0xC2
#define NVMAV_BREG_0XC6      0xC6

//new:
/* Macros for convenient accesses to the NV chips */
#define NV_REG8(r_)  ((vuint8  *)regs)[(r_)]
#define NV_REG16(r_) ((vuint16 *)regs)[(r_) >> 1]
#define NV_REG32(r_) ((vuint32 *)regs)[(r_) >> 2]

/* read and write to PCI config space */
#define CFGR(A)   (nv_pci_access.offset=NVCFG_##A, ioctl(fd,NV_GET_PCI, &nv_pci_access,sizeof(nv_pci_access)), nv_pci_access.value)
#define CFGW(A,B) (nv_pci_access.offset=NVCFG_##A, nv_pci_access.value = B, ioctl(fd,NV_SET_PCI,&nv_pci_access,sizeof(nv_pci_access)))

/* read and write from the dac registers */
#define DACR(A)   (NV_REG32(NVDAC_##A))
#define DACW(A,B) (NV_REG32(NVDAC_##A)=B)
#define DAC2R(A)   (NV_REG32(NVDAC2_##A))
#define DAC2W(A,B) (NV_REG32(NVDAC2_##A)=B)
//end new.

/* read and write from the dac index register */
#define DXIR(A)   (DACW(PALWTADD,NVDXI_##A),DACR(X_DATAREG))
#define DXIW(A,B) (DACW(PALWTADD,NVDXI_##A),DACW(X_DATAREG,B))

/* read and write from the vga registers */
#define VGAR(A)   (NV_REG8(NVVGA_##A))
#define VGAW(A,B) (NV_REG8(NVVGA_##A)=B)

/* read and write from the indexed vga registers */
#define VGAR_I(A,B)   (VGAW(A##_I,B),VGAR(A##_D))
#define VGAW_I(A,B,C) (VGAW(A##_I,B),VGAW(A##_D,C))

/* read and write from the powergraphics registers */
#define ACCR(A)    (NV_REG32(NVACC_##A))
#define ACCW(A,B)  (NV_REG32(NVACC_##A)=B)
#define ACCGO(A,B) (NV_REG32(NVACC_##A + 0x0100)=B)

/* read and write from the backend scaler registers */
#define BESR(A)   (NV_REG32(NVBES_##A))
#define BESW(A,B) (NV_REG32(NVBES_##A)=B)

/* read and write from first CRTC */
#define CR1R(A)   (NV_REG32(NVCR1_##A))
#define CR1W(A,B) (NV_REG32(NVCR1_##A)=B)

/* read and write from second CRTC */
#define CR2R(A)   (NV_REG32(NVCR2_##A))
#define CR2W(A,B) (NV_REG32(NVCR2_##A)=B)

//new:
/* read and write from CRTC indexed registers */
#define CRTCW(A,B)(NV_REG16(NV16_CRTCIND) = ((NVCRTCX_##A) | ((B) << 8)))
#define CRTCR(A)  (NV_REG8(NV8_CRTCIND) = (NVCRTCX_##A), NV_REG8(NV8_CRTCDAT))

/* read and write from ATTRIBUTE indexed registers */
#define ATBW(A,B)(NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTRINDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTRDATW) = (B))
#define ATBR(A)  (NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTRINDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTRDATR))

/* read and write from SEQUENCER indexed registers */
#define SEQW(A,B)(NV_REG16(NV16_SEQIND) = ((NVSEQX_##A) | ((B) << 8)))
#define SEQR(A)  (NV_REG8(NV8_SEQIND) = (NVSEQX_##A), NV_REG8(NV8_SEQDAT))

/* read and write from PCI GRAPHICS indexed registers (>= NM2097) */
#define GRPHW(A,B)(NV_REG16(NV16_GRPHIND) = ((NVGRPHX_##A) | ((B) << 8)))
#define GRPHR(A)  (NV_REG8(NV8_GRPHIND) = (NVGRPHX_##A), NV_REG8(NV8_GRPHDAT))
//end new.

/* read and write from maven (<= G400) */
#define MAVR(A)     (i2c_maven_read (NVMAV_##A ))
#define MAVW(A,B)   (i2c_maven_write(NVMAV_##A ,B))
#define MAVRW(A)    (i2c_maven_read (NVMAV_##A )|(i2c_maven_read(NVMAV_##A +1)<<8))
#define MAVWW(A,B)  (i2c_maven_write(NVMAV_##A ,B &0xFF),i2c_maven_write(NVMAV_##A +1,B >>8))
