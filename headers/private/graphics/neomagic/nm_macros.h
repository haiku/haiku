/* NM registers definitions and macros for access to */

/* PCI_config_space */
#define NMCFG_DEVID		0x00
#define NMCFG_DEVCTRL	0x04
#define NMCFG_CLASS		0x08
#define NMCFG_HEADER	0x0c
#define NMCFG_BASE1FB	0x10
#define NMCFG_BASE2REG1	0x14
#define NMCFG_BASE3REG2	0x18
#define NMCFG_BASE4		0x1c //unknown if used
#define NMCFG_BASE5		0x20 //unknown if used
#define NMCFG_BASE6		0x24 //unknown if used
#define NMCFG_BASE7		0x28 //unknown if used
#define NMCFG_SUBSYSID1	0x2c
#define NMCFG_ROMBASE	0x30
#define NMCFG_CAPPTR	0x34
#define NMCFG_CFG_1		0x38 //unknown if used
#define NMCFG_INTERRUPT	0x3c
#define NMCFG_CFG_3		0x40 //unknown if used
#define NMCFG_CFG_4		0x44 //unknown if used
#define NMCFG_CFG_5		0x48 //unknown if used
#define NMCFG_CFG_6		0x4c //unknown if used
#define NMCFG_CFG_7		0x50 //unknown if used
#define NMCFG_CFG_8		0x54 //unknown if used
#define NMCFG_CFG_9		0x58 //unknown if used
#define NMCFG_CFG_10	0x5c //unknown if used
#define NMCFG_CFG_11	0x60 //unknown if used
#define NMCFG_CFG_12	0x64 //unknown if used
#define NMCFG_CFG_13	0x68 //unknown if used
#define NMCFG_CFG_14	0x6c //unknown if used
#define NMCFG_CFG_15	0x70 //unknown if used
#define NMCFG_CFG_16	0x74 //unknown if used
#define NMCFG_CFG_17	0x78 //unknown if used
#define NMCFG_CFG_18	0x7c //unknown if used
#define NMCFG_CFG_19	0x80 //unknown if used
#define NMCFG_CFG_20	0x84 //unknown if used
#define NMCFG_CFG_21	0x88 //unknown if used
#define NMCFG_CFG_22	0x8c //unknown if used
#define NMCFG_CFG_23	0x90 //unknown if used
#define NMCFG_CFG_24	0x94 //unknown if used
#define NMCFG_CFG_25	0x98 //unknown if used
#define NMCFG_CFG_26	0x9c //unknown if used
#define NMCFG_CFG_27	0xa0 //unknown if used
#define NMCFG_CFG_28	0xa4 //unknown if used
#define NMCFG_CFG_29	0xa8 //unknown if used
#define NMCFG_CFG_30	0xac //unknown if used
#define NMCFG_CFG_31	0xb0 //unknown if used
#define NMCFG_CFG_32	0xb4 //unknown if used
#define NMCFG_CFG_33	0xb8 //unknown if used
#define NMCFG_CFG_34	0xbc //unknown if used
#define NMCFG_CFG_35	0xc0 //unknown if used
#define NMCFG_CFG_36	0xc4 //unknown if used
#define NMCFG_CFG_37	0xc8 //unknown if used
#define NMCFG_CFG_38	0xcc //unknown if used
#define NMCFG_CFG_39	0xd0 //unknown if used
#define NMCFG_CFG_40	0xd4 //unknown if used
#define NMCFG_CFG_41	0xd8 //unknown if used
#define NMCFG_CFG_42	0xdc //unknown if used
#define NMCFG_CFG_43	0xe0 //unknown if used
#define NMCFG_CFG_44	0xe4 //unknown if used
#define NMCFG_CFG_45	0xe8 //unknown if used
#define NMCFG_CFG_46	0xec //unknown if used
#define NMCFG_CFG_47	0xf0 //unknown if used
#define NMCFG_CFG_48	0xf4 //unknown if used
#define NMCFG_CFG_49	0xf8 //unknown if used
#define NMCFG_CFG_50	0xfc //unknown if used

/* neomagic ISA direct registers */
/* VGA standard registers: */
#define NMISA8_ATTRINDW		0x03c0
#define NMISA8_ATTRINDR		0x03c1
#define NMISA8_ATTRDATW		0x03c0
#define NMISA8_ATTRDATR		0x03c1
#define NMISA8_SEQIND		0x03c4
#define NMISA8_SEQDAT		0x03c5
#define NMISA16_SEQIND		0x03c4
#define NMISA8_CRTCIND		0x03d4
#define NMISA8_CRTCDAT		0x03d5
#define NMISA16_CRTCIND		0x03d4
#define NMISA8_GRPHIND		0x03ce
#define NMISA8_GRPHDAT		0x03cf
#define NMISA16_GRPHIND		0x03ce

/* neomagic PCI direct registers */
#define NM2PCI8_SEQIND		0x03c4
#define NM2PCI8_SEQDAT		0x03c5
#define NM2PCI16_SEQIND		0x03c4
#define NM2PCI8_GRPHIND		0x03ce
#define NM2PCI8_GRPHDAT		0x03cf
#define NM2PCI16_GRPHIND	0x03ce

/* neomagic ISA GENERAL direct registers */
/* VGA standard registers: */
#define NMISA8_MISCW 		0x03c2
#define NMISA8_MISCR 		0x03cc
#define NMISA8_INSTAT1 		0x03da

/* neomagic ISA (DAC) COLOR direct registers (VGA palette RAM) */
/* VGA standard registers: */
#define NMISA8_PALMASK		0x03c6
#define NMISA8_PALINDR		0x03c7
#define NMISA8_PALINDW		0x03c8
#define NMISA8_PALDATA		0x03c9

/* neomagic ISA CRTC indexed registers */
/* VGA standard registers: */
#define NMCRTCX_HTOTAL		0x00
#define NMCRTCX_HDISPE		0x01
#define NMCRTCX_HBLANKS		0x02
#define NMCRTCX_HBLANKE		0x03
#define NMCRTCX_HSYNCS		0x04
#define NMCRTCX_HSYNCE		0x05
#define NMCRTCX_VTOTAL		0x06
#define NMCRTCX_OVERFLOW	0x07
#define NMCRTCX_PRROWSCN	0x08
#define NMCRTCX_MAXSCLIN	0x09
#define NMCRTCX_VGACURCTRL	0x0a
#define NMCRTCX_FBSTADDH	0x0c
#define NMCRTCX_FBSTADDL	0x0d
#define NMCRTCX_VSYNCS		0x10
#define NMCRTCX_VSYNCE		0x11
#define NMCRTCX_VDISPE		0x12
#define NMCRTCX_PITCHL		0x13
#define NMCRTCX_VBLANKS		0x15
#define NMCRTCX_VBLANKE		0x16
#define NMCRTCX_MODECTL		0x17
#define NMCRTCX_LINECOMP	0x18
/* NeoMagic specific registers: */
#define NMCRTCX_PANEL_0x40	0x40
#define NMCRTCX_PANEL_0x41	0x41
#define NMCRTCX_PANEL_0x42	0x42
#define NMCRTCX_PANEL_0x43	0x43
#define NMCRTCX_PANEL_0x44	0x44
#define NMCRTCX_PANEL_0x45	0x45
#define NMCRTCX_PANEL_0x46	0x46
#define NMCRTCX_PANEL_0x47	0x47
#define NMCRTCX_PANEL_0x48	0x48
#define NMCRTCX_PANEL_0x49	0x49
#define NMCRTCX_PANEL_0x4a	0x4a
#define NMCRTCX_PANEL_0x4b	0x4b
#define NMCRTCX_PANEL_0x4c	0x4c
#define NMCRTCX_PANEL_0x4d	0x4d
#define NMCRTCX_PANEL_0x4e	0x4e
#define NMCRTCX_PANEL_0x4f	0x4f
#define NMCRTCX_PANEL_0x50	0x50 /* >= NM2090 */
#define NMCRTCX_PANEL_0x51	0x51 /* >= NM2090 */
#define NMCRTCX_PANEL_0x52	0x52 /* >= NM2090 */
#define NMCRTCX_PANEL_0x53	0x53 /* >= NM2090 */
#define NMCRTCX_PANEL_0x54	0x54 /* >= NM2090 */
#define NMCRTCX_PANEL_0x55	0x55 /* >= NM2090 */
#define NMCRTCX_PANEL_0x56	0x56 /* >= NM2090 */
#define NMCRTCX_PANEL_0x57	0x57 /* >= NM2090 */
#define NMCRTCX_PANEL_0x58	0x58 /* >= NM2090 */
#define NMCRTCX_PANEL_0x59	0x59 /* >= NM2090 */
#define NMCRTCX_PANEL_0x60	0x60 /* >= NM2097(?) */
#define NMCRTCX_PANEL_0x61	0x61 /* >= NM2097(?) */
#define NMCRTCX_PANEL_0x62	0x62 /* >= NM2097(?) */
#define NMCRTCX_PANEL_0x63	0x63 /* >= NM2097(?) */
#define NMCRTCX_PANEL_0x64	0x64 /* >= NM2097(?) */
#define NMCRTCX_VEXT		0x70 /* >= NM2200 */

/* neomagic ISA SEQUENCER indexed registers */
/* VGA standard registers: */
#define NMSEQX_RESET		0x00
#define NMSEQX_CLKMODE		0x01
#define NMSEQX_MAPMASK		0x02
#define NMSEQX_MEMMODE		0x04
/* NeoMagic BES registers: (> NM2070) (accessible via mapped I/O: >= NM2097) */
#define NMSEQX_BESCTRL2		0x08
#define NMSEQX_0x09			0x09 //??
#define NMSEQX_ZVCAP_DSCAL	0x0a
#define NMSEQX_BUF2ORGL		0x0c
#define NMSEQX_BUF2ORGM		0x0d
#define NMSEQX_BUF2ORGH		0x0e
#define NMSEQX_VD2COORD1L	0x14 /* >= NM2200(?) */
#define NMSEQX_VD2COORD2L	0x15 /* >= NM2200(?) */
#define NMSEQX_VD2COORD21H	0x16 /* >= NM2200(?) */
#define NMSEQX_HD2COORD1L	0x17 /* >= NM2200(?) */
#define NMSEQX_HD2COORD2L	0x18 /* >= NM2200(?) */
#define NMSEQX_HD2COORD21H	0x19 /* >= NM2200(?) */
#define NMSEQX_BUF2PITCHL	0x1a
#define NMSEQX_BUF2PITCHH	0x1b
#define NMSEQX_0x1c			0x1c //??
#define NMSEQX_0x1d			0x1d //??
#define NMSEQX_0x1e			0x1e //??
#define NMSEQX_0x1f			0x1f //??

/* neomagic ISA ATTRIBUTE indexed registers */
/* VGA standard registers: */
#define NMATBX_MODECTL		0x10
#define NMATBX_OSCANCOLOR	0x11
#define NMATBX_COLPLANE_EN	0x12
#define NMATBX_HORPIXPAN	0x13
#define NMATBX_COLSEL		0x14
#define NMATBX_0x16			0x16

/* neomagic ISA GRAPHICS indexed registers */
/* VGA standard registers: */
#define NMGRPHX_ENSETRESET	0x01
#define NMGRPHX_DATAROTATE	0x03
#define NMGRPHX_READMAPSEL	0x04
#define NMGRPHX_MODE		0x05
#define NMGRPHX_MISC		0x06
#define NMGRPHX_BITMASK		0x08
/* NeoMagic specific registers: */
#define NMGRPHX_GRPHXLOCK	0x09
#define NMGRPHX_GENLOCK		0x0a
#define NMGRPHX_FBSTADDE	0x0e
#define NMGRPHX_CRTC_PITCHE	0x0f /* > NM2070 */
#define NMGRPHX_IFACECTRL1	0x10
#define NMGRPHX_IFACECTRL2	0x11
#define NMGRPHX_0x15		0x15
#define NMGRPHX_ACT_CLK_SAV	0x19 /* >= NM2200? auto-pwr-save.. (b2-0) */
#define NMGRPHX_PANELCTRL1	0x20
#define NMGRPHX_PANELTYPE	0x21
#define NMGRPHX_PANELCTRL2	0x25
#define NMGRPHX_PANELVCENT1	0x28
#define NMGRPHX_PANELVCENT2	0x29
#define NMGRPHX_PANELVCENT3	0x2a
#define NMGRPHX_PANELCTRL3	0x30 /* > NM2070 */
#define NMGRPHX_PANELVCENT4	0x32 /* > NM2070 */
#define NMGRPHX_PANELHCENT1	0x33 /* > NM2070 */
#define NMGRPHX_PANELHCENT2	0x34 /* > NM2070 */
#define NMGRPHX_PANELHCENT3	0x35 /* > NM2070 */
#define NMGRPHX_PANELHCENT4	0x36 /* >= NM2160 */
#define NMGRPHX_PANELVCENT5	0x37 /* >= NM2200 */
#define NMGRPHX_PANELHCENT5	0x38 /* >= NM2200 */
#define NMGRPHX_CURCTRL		0x82
#define NMGRPHX_COLDEPTH	0x90
/* mem or core PLL register??? */
#define NMGRPHX_SPEED		0x93
/* (NeoMagic pixelPLL set C registers) */
#define NMGRPHX_PLLC_NH		0x8f /* >= NM2200 */
#define NMGRPHX_PLLC_NL		0x9b
#define NMGRPHX_PLLC_M		0x9f
/* NeoMagic BES registers: (> NM2070) (accessible via mapped I/O: >= NM2097) */
#define NMGRPHX_BESCTRL1	0xb0
#define NMGRPHX_HD1COORD21H	0xb1
#define NMGRPHX_HD1COORD1L	0xb2
#define NMGRPHX_HD1COORD2L	0xb3
#define NMGRPHX_VD1COORD21H	0xb4
#define NMGRPHX_VD1COORD1L	0xb5
#define NMGRPHX_VD1COORD2L	0xb6
#define NMGRPHX_BUF1ORGH	0xb7
#define NMGRPHX_BUF1ORGM	0xb8
#define NMGRPHX_BUF1ORGL	0xb9
#define NMGRPHX_BUF1PITCHH	0xba
#define NMGRPHX_BUF1PITCHL	0xbb
#define NMGRPHX_0xbc		0xbc //??
#define NMGRPHX_0xbd		0xbd //??
#define NMGRPHX_0xbe		0xbe //??
#define NMGRPHX_0xbf		0xbf //??
#define NMGRPHX_XSCALEH		0xc0
#define NMGRPHX_XSCALEL		0xc1
#define NMGRPHX_YSCALEH		0xc2
#define NMGRPHX_YSCALEL		0xc3
#define NMGRPHX_BRIGHTNESS	0xc4
#define NMGRPHX_COLKEY_R	0xc5
#define NMGRPHX_COLKEY_G	0xc6
#define NMGRPHX_COLKEY_B	0xc7

/* NeoMagic specific PCI cursor registers < NM2200 */
#define NMCR1_CURCTRL    		0x0100
#define NMCR1_CURX       		0x0104
#define NMCR1_CURY       		0x0108
#define NMCR1_CURBGCOLOR		0x010c
#define NMCR1_CURFGCOLOR 		0x0110
#define NMCR1_CURADDRESS		0x0114
/* NeoMagic specific PCI cursor registers >= NM2200 */
#define NMCR1_22CURCTRL   		0x1000
#define NMCR1_22CURX      		0x1004
#define NMCR1_22CURY      		0x1008
#define NMCR1_22CURBGCOLOR		0x100c
#define NMCR1_22CURFGCOLOR   	0x1010
#define NMCR1_22CURADDRESS		0x1014

/* NeoMagic PCI acceleration registers */
/* all cards, but some registers only on 2090 and later; and some on 2200 and later */
#define NMACC_STATUS			0x0000
#define NMACC_CONTROL			0x0004
#define NMACC_FGCOLOR			0x000c
#define NMACC_2200_SRC_PITCH	0x0014
#define NMACC_2090_CLIPLT		0x0018
#define NMACC_2090_CLIPRB		0x001c
#define NMACC_SRCSTARTOFF		0x0024
#define NMACC_2090_DSTSTARTOFF	0x002c
#define NMACC_2090_XYEXT		0x0030
/* NM2070 only */
#define NMACC_2070_PLANEMASK	0x0014
#define NMACC_2070_XYEXT		0x0018
#define NMACC_2070_SRCPITCH		0x001c
#define NMACC_2070_SRCBITOFF	0x0020
#define NMACC_2070_DSTPITCH		0x0028
#define NMACC_2070_DSTBITOFF	0x002c
#define NMACC_2070_DSTSTARTOFF	0x0030


/* Macros for convenient accesses to the NM chips */

/* primary PCI register area */
#define NM_REG8(r_)  ((vuint8  *)regs)[(r_)]
#define NM_REG16(r_) ((vuint16 *)regs)[(r_) >> 1]
#define NM_REG32(r_) ((vuint32 *)regs)[(r_) >> 2]
/* secondary PCI register area */
#define NM_2REG8(r_)  ((vuint8  *)regs2)[(r_)]
#define NM_2REG16(r_) ((vuint16 *)regs2)[(r_) >> 1]
#define NM_2REG32(r_) ((vuint32 *)regs2)[(r_) >> 2]

/* read and write to PCI config space */
#define CFGR(A)   (nm_pci_access.offset=NMCFG_##A, ioctl(fd,NM_GET_PCI, &nm_pci_access,sizeof(nm_pci_access)), nm_pci_access.value)
#define CFGW(A,B) (nm_pci_access.offset=NMCFG_##A, nm_pci_access.value = B, ioctl(fd,NM_SET_PCI,&nm_pci_access,sizeof(nm_pci_access)))

/* read and write from acceleration engine */
#define ACCR(A)   (NM_REG32(NMACC_##A))
#define ACCW(A,B) (NM_REG32(NMACC_##A) = (B))

/* read and write from first CRTC (mapped) */
#define CR1R(A)   (NM_REG32(NMCR1_##A))
#define CR1W(A,B) (NM_REG32(NMCR1_##A) = (B))

/* read and write from ISA I/O space */
#define ISAWB(A,B)(nm_isa_access.adress=NMISA8_##A, nm_isa_access.data = (uint8)B, nm_isa_access.size = 1, ioctl(fd,NM_ISA_OUT, &nm_isa_access,sizeof(nm_isa_access)))
#define ISAWW(A,B)(nm_isa_access.adress=NMISA16_##A, nm_isa_access.data = B, nm_isa_access.size = 2, ioctl(fd,NM_ISA_OUT, &nm_isa_access,sizeof(nm_isa_access)))
#define ISARB(A)  (nm_isa_access.adress=NMISA8_##A, ioctl(fd,NM_ISA_IN, &nm_isa_access,sizeof(nm_isa_access)), (uint8)nm_isa_access.data)
#define ISARW(A)  (nm_isa_access.adress=NMISA16_##A, ioctl(fd,NM_ISA_IN, &nm_isa_access,sizeof(nm_isa_access)), nm_isa_access.data)

/* read and write from ISA CRTC indexed registers */
#define ISACRTCW(A,B)(ISAWW(CRTCIND, ((NMCRTCX_##A) | ((B) << 8))))
#define ISACRTCR(A)  (ISAWB(CRTCIND, (NMCRTCX_##A)), ISARB(CRTCDAT))

/* read and write from ISA GRAPHICS indexed registers */
#define ISAGRPHW(A,B)(ISAWW(GRPHIND, ((NMGRPHX_##A) | ((B) << 8))))
#define ISAGRPHR(A)  (ISAWB(GRPHIND, (NMGRPHX_##A)), ISARB(GRPHDAT))

/* read and write from PCI GRAPHICS indexed registers (>= NM2097) */
#define PCIGRPHW(A,B)(NM_2REG16(NM2PCI16_GRPHIND) = ((NMGRPHX_##A) | ((B) << 8)))
#define PCIGRPHR(A)  (NM_2REG8(NM2PCI8_GRPHIND) = (NMGRPHX_##A), NM_2REG8(NM2PCI8_GRPHDAT))

/* read and write from ISA SEQUENCER indexed registers */
#define ISASEQW(A,B)(ISAWW(SEQIND, ((NMSEQX_##A) | ((B) << 8))))
#define ISASEQR(A)  (ISAWB(SEQIND, (NMSEQX_##A)), ISARB(SEQDAT))

/* read and write from PCI SEQUENCER indexed registers (>= NM2097) */
#define PCISEQW(A,B)(NM_2REG16(NM2PCI16_SEQIND) = ((NMSEQX_##A) | ((B) << 8)))
#define PCISEQR(A)  (NM_2REG8(NM2PCI8_SEQIND) = (NMSEQX_##A), NM_2REG8(NM2PCI8_SEQDAT))

/* read and write from ISA ATTRIBUTE indexed registers */
#define ISAATBW(A,B)((void)ISARB(INSTAT1), ISAWB(ATTRINDW, ((NMATBX_##A) | 0x20)), ISAWB(ATTRDATW, (B)))
#define ISAATBR(A)  ((void)ISARB(INSTAT1), ISAWB(ATTRINDW, ((NMATBX_##A) | 0x20)), ISARB(ATTRDATR))
