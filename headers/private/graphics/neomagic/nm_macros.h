/* NM registers definitions and macros for access to */

//old:
/* PCI_config_space */
#define NMCFG_DEVID        0x00
#define NMCFG_DEVCTRL      0x04
#define NMCFG_CLASS        0x08
#define NMCFG_HEADER       0x0c
#define NMCFG_NMBASE2     0x10
#define NMCFG_NMBASE1     0x14
#define NMCFG_NMBASE3     0x18 // >= MYST
#define NMCFG_SUBSYSIDR    0x2c // >= MYST
#define NMCFG_ROMBASE      0x30
#define NMCFG_CAP_PTR      0x34 // >= MIL2
#define NMCFG_INTCTRL      0x3c
#define NMCFG_OPTION       0x40
#define NMCFG_NM_INDEX    0x44
#define NMCFG_NM_DATA     0x48
#define NMCFG_SUBSYSIDW    0x4c // >= MYST
#define NMCFG_OPTION2      0x50 // >= G100
#define NMCFG_OPTION3      0x54 // >= G400
#define NMCFG_OPTION4      0x58 // >= G450
#define NMCFG_PM_IDENT     0xdc // >= G100
#define NMCFG_PM_CSR       0xe0 // >= G100
#define NMCFG_AGP_IDENT    0xf0 // >= MIL2
#define NMCFG_AGP_STS      0xf4 // >= MIL2
#define NMCFG_AGP_CMD      0xf8 // >= MIL2
//end old.

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
#define NMCRTCX_VEXT		0x70 /* >= NM2200 */

/* neomagic ISA SEQUENCER indexed registers */
/* VGA standard registers: */
#define NMSEQX_RESET		0x00
#define NMSEQX_CLKMODE		0x01
#define NMSEQX_MEMMODE		0x04
/* NeoMagic BES registers: (> NM2070) (accessible via mapped I/O: >= NM2097) */
#define NMSEQX_BESCTRL2		0x08
#define NMSEQX_0x09			0x09 //??
#define NMSEQX_0x0a			0x0a //??
#define NMSEQX_BUF2ORGL		0x0c
#define NMSEQX_BUF2ORGM		0x0d
#define NMSEQX_BUF2ORGH		0x0e
#define NMSEQX_VSCOORD1L	0x14 /* >= NM2200(?), so clipping done via buffer startadress instead */
#define NMSEQX_VSCOORD2L	0x15 /* >= NM2200(?), so clipping done via buffer startadress instead */
#define NMSEQX_VSCOORD21H	0x16 /* >= NM2200(?), so clipping done via buffer startadress instead */
#define NMSEQX_HSCOORD1L	0x17 /* >= NM2200(?), so clipping done via buffer startadress instead */
#define NMSEQX_HSCOORD2L	0x18 /* >= NM2200(?), so clipping done via buffer startadress instead */
#define NMSEQX_HSCOORD21H	0x19 /* >= NM2200(?), so clipping done via buffer startadress instead */
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
#define NMGRPHX_CRTC_PITCHE	0x0f
#define NMGRPHX_IFACECTRL	0x11
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
/* (NeoMagic pixelPLL set C registers) */
#define NMGRPHX_PLLC_NL		0x9b
#define NMGRPHX_PLLC_NH		0x8f /* >= NM2200 */
#define NMGRPHX_PLLC_M		0x9f
/* NeoMagic BES registers: (> NM2070) (accessible via mapped I/O: >= NM2097) */
#define NMGRPHX_BESCTRL1	0xb0
#define NMGRPHX_HDCOORD21H	0xb1
#define NMGRPHX_HDCOORD1L	0xb2
#define NMGRPHX_HDCOORD2L	0xb3
#define NMGRPHX_VDCOORD21H	0xb4
#define NMGRPHX_VDCOORD1L	0xb5
#define NMGRPHX_VDCOORD2L	0xb6
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
#define NMACC_STATUS			0x0000
#define NMACC_CONTROL			0x0004
#define NMACC_FGCOLOR			0x000c
#define NMACC_CLIPLT			0x0018
#define NMACC_CLIPRB			0x001c
#define NMACC_SRCSTARTOFF		0x0024
#define NMACC_DSTSTARTOFF		0x002c
#define NMACC_XYEXT				0x0030


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
