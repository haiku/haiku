/* NM registers definitions and macros for access to */

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

//new:
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
//end new.

/* NM ACCeleration registers */
#define NMACC_DWGCTL          0x1C00
#define NMACC_MACCESS         0x1C04
#define NMACC_MCTLWTST        0x1C08
#define NMACC_ZORG            0x1C0C
#define NMACC_PLNWT           0x1C1C
#define NMACC_BCOL            0x1C20
#define NMACC_FCOL            0x1C24
#define NMACC_XYSTRT          0x1C40
#define NMACC_XYEND           0x1C44
#define NMACC_SGN             0x1C58
#define NMACC_LEN             0x1C5C
#define NMACC_AR0             0x1C60
#define NMACC_AR3             0x1C6C
#define NMACC_AR5             0x1C74
#define NMACC_CXBNDRY         0x1C80
#define NMACC_FXBNDRY         0x1C84
#define NMACC_YDSTLEN         0x1C88
#define NMACC_PITCH           0x1C8C
#define NMACC_YDST            0x1C90
#define NMACC_YDSTORG         0x1C94
#define NMACC_YTOP            0x1C98
#define NMACC_YBOT            0x1C9C
#define NMACC_CXLEFT          0x1CA0
#define NMACC_CXRIGHT         0x1CA4
#define NMACC_FXLEFT          0x1CA8
#define NMACC_FXRIGHT         0x1CAC
#define NMACC_STATUS          0x1E14
#define NMACC_ICLEAR          0x1E18 /* required for interrupt stuff */
#define NMACC_IEN             0x1E1C /* required for interrupt stuff */
#define NMACC_RST             0x1E40
#define NMACC_MEMRDBK         0x1E44
#define NMACC_OPMODE          0x1E54
#define NMACC_PRIMADDRESS     0x1E58
#define NMACC_PRIMEND         0x1E5C
#define NMACC_TEXORG          0x2C24 // >= G100 
#define NMACC_DWGSYNC         0x2C4C // >= G200
#define NMACC_TEXORG1         0x2CA4 // >= G200
#define NMACC_TEXORG2         0x2CA8 // >= G200 
#define NMACC_TEXORG3         0x2CAC // >= G200 
#define NMACC_TEXORG4         0x2CB0 // >= G200 
#define NMACC_SRCORG          0x2CB4 // >= G200
#define NMACC_DSTORG          0x2CB8 // >= G200

/*NM BES (Back End Scaler) registers (>= G200) */
#define NMBES_A1ORG           0x3D00
#define NMBES_A2ORG           0x3D04
#define NMBES_B1ORG           0x3D08
#define NMBES_B2ORG           0x3D0C
#define NMBES_A1CORG          0x3D10
#define NMBES_A2CORG          0x3D14
#define NMBES_B1CORG          0x3D18
#define NMBES_B2CORG          0x3D1C
#define NMBES_CTL             0x3D20
#define NMBES_PITCH           0x3D24
#define NMBES_HCOORD          0x3D28
#define NMBES_VCOORD          0x3D2C
#define NMBES_HISCAL          0x3D30
#define NMBES_VISCAL          0x3D34
#define NMBES_HSRCST          0x3D38
#define NMBES_HSRCEND         0x3D3C
#define NMBES_LUMACTL         0x3D40
#define NMBES_V1WGHT          0x3D48
#define NMBES_V2WGHT          0x3D4C
#define NMBES_HSRCLST         0x3D50
#define NMBES_V1SRCLST        0x3D54
#define NMBES_V2SRCLST        0x3D58
#define NMBES_A1C3ORG         0x3D60
#define NMBES_A2C3ORG         0x3D64
#define NMBES_B1C3ORG         0x3D68
#define NMBES_B2C3ORG         0x3D6C
#define NMBES_GLOBCTL         0x3DC0
#define NMBES_STATUS          0x3DC4

/* Macros for convenient accesses to the NM chips */

#define NM_REG8(r_)  ((vuint8  *)regs)[(r_)]
#define NM_REG32(r_) ((vuint32 *)regs)[(r_) >> 2]

/* read and write to PCI config space */
#define CFGR(A)   (mn_pci_access.offset=NMCFG_##A, ioctl(fd,MN_GET_PCI, &mn_pci_access,sizeof(mn_pci_access)), mn_pci_access.value)
#define CFGW(A,B) (mn_pci_access.offset=NMCFG_##A, mn_pci_access.value = B, ioctl(fd,MN_SET_PCI,&mn_pci_access,sizeof(mn_pci_access)))

/* read and write from the powergraphics registers */
#define ACCR(A)    (NM_REG32(NMACC_##A))
#define ACCW(A,B)  (NM_REG32(NMACC_##A)=B)
#define ACCGO(A,B) (NM_REG32(NMACC_##A + 0x0100)=B)

/* read and write from the backend scaler registers */
#define BESR(A)   (NM_REG32(NMBES_##A))
#define BESW(A,B) (NM_REG32(NMBES_##A)=B)

//new:
/* read and write from first CRTC (mapped) */
#define CR1R(A)   (NM_REG32(NMCR1_##A))
#define CR1W(A,B) (NM_REG32(NMCR1_##A) = (B))

/* read and write from ISA I/O space */
#define ISAWB(A,B)(mn_isa_access.adress=NMISA8_##A, mn_isa_access.data = (uint8)B, mn_isa_access.size = 1, ioctl(fd,MN_ISA_OUT, &mn_isa_access,sizeof(mn_isa_access)))
#define ISAWW(A,B)(mn_isa_access.adress=NMISA16_##A, mn_isa_access.data = B, mn_isa_access.size = 2, ioctl(fd,MN_ISA_OUT, &mn_isa_access,sizeof(mn_isa_access)))
#define ISARB(A)  (mn_isa_access.adress=NMISA8_##A, ioctl(fd,MN_ISA_IN, &mn_isa_access,sizeof(mn_isa_access)), (uint8)mn_isa_access.data)
#define ISARW(A)  (mn_isa_access.adress=NMISA16_##A, ioctl(fd,MN_ISA_IN, &mn_isa_access,sizeof(mn_isa_access)), mn_isa_access.data)

/* read and write from ISA CRTC indexed registers */
#define ISACRTCW(A,B)(ISAWW(CRTCIND, ((NMCRTCX_##A) | ((B) << 8))))
#define ISACRTCR(A)  (ISAWB(CRTCIND, (NMCRTCX_##A)), ISARB(CRTCDAT))

/* read and write from ISA GRAPHICS indexed registers */
#define ISAGRPHW(A,B)(ISAWW(GRPHIND, ((NMGRPHX_##A) | ((B) << 8))))
#define ISAGRPHR(A)  (ISAWB(GRPHIND, (NMGRPHX_##A)), ISARB(GRPHDAT))

/* read and write from ISA SEQUENCER indexed registers */
#define ISASEQW(A,B)(ISAWW(SEQIND, ((NMSEQX_##A) | ((B) << 8))))
#define ISASEQR(A)  (ISAWB(SEQIND, (NMSEQX_##A)), ISARB(SEQDAT))

/* read and write from ISA ATTRIBUTE indexed registers */
/* define DUMMY to prevent compiler warnings */
#define static uint8 DUMMY;
#define ISAATBW(A,B)(DUMMY = ISARB(INSTAT1), ISAWB(ATTRINDW, ((NMATBX_##A) | 0x20)), ISAWB(ATTRDATW, (B)))
#define ISAATBR(A)  (DUMMY = ISARB(INSTAT1), ISAWB(ATTRINDW, ((NMATBX_##A) | 0x20)), ISARB(ATTRDATR))
