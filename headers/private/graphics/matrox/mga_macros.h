/* MGA registers definitions and macros for access to */

/* apsed : merged mga_macro.h and mga_regs.c with #define for speed */

/* PCI_config_space */
#define MGACFG_DEVID        0x00
#define MGACFG_DEVCTRL      0x04
#define MGACFG_CLASS        0x08
#define MGACFG_HEADER       0x0c
#define MGACFG_MGABASE2     0x10
#define MGACFG_MGABASE1     0x14
#define MGACFG_MGABASE3     0x18 // >= MYST
#define MGACFG_SUBSYSIDR    0x2c // >= MYST
#define MGACFG_ROMBASE      0x30
#define MGACFG_CAP_PTR      0x34 // >= MIL2
#define MGACFG_INTCTRL      0x3c
#define MGACFG_OPTION       0x40
#define MGACFG_MGA_INDEX    0x44
#define MGACFG_MGA_DATA     0x48
#define MGACFG_SUBSYSIDW    0x4c // >= MYST
#define MGACFG_OPTION2      0x50 // >= G100
#define MGACFG_OPTION3      0x54 // >= G400
#define MGACFG_OPTION4      0x58 // >= G450
#define MGACFG_PM_IDENT     0xdc // >= G100
#define MGACFG_PM_CSR       0xe0 // >= G100
#define MGACFG_AGP_IDENT    0xf0 // >= MIL2
#define MGACFG_AGP_STS      0xf4 // >= MIL2
#define MGACFG_AGP_CMD      0xf8 // >= MIL2

/* VGA registers - these are byte wide */
#define MGAVGA_ATTR_I       0x1FC0 // apsed as SEQ
#define MGAVGA_ATTR_D       0x1FC1 // apsed as SEQ
#define MGAVGA_MISCW        0x1FC2
#define MGAVGA_SEQ_I        0x1FC4
#define MGAVGA_SEQ_D        0x1FC5
#define MGAVGA_DACSTAT      0x1FC7
#define MGAVGA_FEATR        0x1FCA
#define MGAVGA_MISCR        0x1FCC
#define MGAVGA_GCTL_I       0x1FCE
#define MGAVGA_GCTL_D       0x1FCF
#define MGAVGA_CRTC_I       0x1FD4
#define MGAVGA_CRTC_D       0x1FD5
#define MGAVGA_INSTS1       0x1FDA
#define MGAVGA_FEATW        0x1FDA
#define MGAVGA_CRTCEXT_I    0x1FDE
#define MGAVGA_CRTCEXT_D    0x1FDF

/* TVP3026 'non-std' DAC registers (>= MIL1) */
#define MGADAC_TVP_CUROVRWTADD  0x3c04
#define MGADAC_TVP_CUROVRDATA   0x3c05
#define MGADAC_TVP_CUROVRRDADD  0x3c07
#define MGADAC_TVP_DIRCURCTRL   0x3c09
#define MGADAC_TVP_CURRAMDATA   0x3c0b

/* TVP3026 'non'std' (D)AC (X) (I)ndexed registers (>= MIL1) */
#define MGADXI_TVP_SILICONREV      0x01
#define MGADXI_TVP_LATCHCTRL       0x0f
#define MGADXI_TVP_TCOLCTRL        0x18
#define MGADXI_TVP_CLOCKSEL        0x1a
#define MGADXI_TVP_PALPAGE         0x1c
#define MGADXI_TVP_PLLADDR         0x2c
#define MGADXI_TVP_PIXPLLDATA      0x2d
#define MGADXI_TVP_MEMPLLDATA      0x2e
#define MGADXI_TVP_LOOPLLDATA      0x2f
#define MGADXI_TVP_COLKEYOL        0x30
#define MGADXI_TVP_COLKEYOH        0x31
#define MGADXI_TVP_COLKEYRL        0x32
#define MGADXI_TVP_COLKEYRH        0x33
#define MGADXI_TVP_COLKEYGL        0x34
#define MGADXI_TVP_COLKEYGH        0x35
#define MGADXI_TVP_COLKEYBL        0x36
#define MGADXI_TVP_COLKEYBH        0x37
#define MGADXI_TVP_COLKEYCTRL      0x38
#define MGADXI_TVP_MEMCLKCTRL      0x39
#define MGADXI_TVP_TESTMODEDATA    0x3b
#define MGADXI_TVP_ID              0x3f
#define MGADXI_TVP_RESET           0xff

/* DAC registers (>= G100) */
#define MGADAC_PALWTADD     0x3C00
#define MGADAC_PALDATA      0x3C01
#define MGADAC_PIXRDMSK     0x3C02
#define MGADAC_PALRDADD     0x3C03
#define MGADAC_X_DATAREG    0x3C0A
#define MGADAC_CURSPOSXL    0x3C0C
#define MGADAC_CURSPOSXH    0x3C0D
#define MGADAC_CURSPOSYL    0x3C0E
#define MGADAC_CURSPOSYH    0x3C0F

/* (D)AC (X) (I)ndexed registers (>= G100) */
#define MGADXI_CURADDL         0x04
#define MGADXI_CURADDH         0x05
#define MGADXI_CURCTRL         0x06
#define MGADXI_CURCOL0RED      0x08
#define MGADXI_CURCOL0GREEN    0x09
#define MGADXI_CURCOL0BLUE     0x0A
#define MGADXI_CURCOL1RED      0x0C
#define MGADXI_CURCOL1GREEN    0x0D
#define MGADXI_CURCOL1BLUE     0x0E
#define MGADXI_CURCOL2RED      0x10
#define MGADXI_CURCOL2GREEN    0x11
#define MGADXI_CURCOL2BLUE     0x12
#define MGADXI_VREFCTRL        0x18
#define MGADXI_MULCTRL         0x19
#define MGADXI_PIXCLKCTRL      0x1A
#define MGADXI_GENCTRL         0x1D
#define MGADXI_MISCCTRL        0x1E
#define MGADXI_PANELMODE       0x1F
#define MGADXI_MAFCDEL         0x20
#define MGADXI_GENIOCTRL       0x2A
#define MGADXI_GENIODATA       0x2B
#define MGADXI_SYSPLLM         0x2C
#define MGADXI_SYSPLLN         0x2D
#define MGADXI_SYSPLLP         0x2E
#define MGADXI_SYSPLLSTAT      0x2F
#define MGADXI_ZOOMCTRL        0x38
#define MGADXI_SENSETEST       0x3A
#define MGADXI_CRCREML         0x3C
#define MGADXI_CRCREMH         0x3D
#define MGADXI_CRCBITSEL       0x3E
#define MGADXI_COLMSK          0x40
#define MGADXI_COLKEY          0x42
#define MGADXI_PIXPLLAM        0x44
#define MGADXI_PIXPLLAN        0x45
#define MGADXI_PIXPLLAP        0x46
#define MGADXI_PIXPLLBM        0x48
#define MGADXI_PIXPLLBN        0x49
#define MGADXI_PIXPLLBP        0x4A
#define MGADXI_PIXPLLCM        0x4C
#define MGADXI_PIXPLLCN        0x4D
#define MGADXI_PIXPLLCP        0x4E
#define MGADXI_PIXPLLSTAT      0x4F
#define MGADXI_CURCOLEXT       0x60   /*sequential from CURCOL3->15, RGB*/

/* (D)AC (X) (I)ndexed registers (>= G200) */
#define	MGADXI_KEYOPMODE	   0x51
#define	MGADXI_COLMSK0RED 	   0x52
#define	MGADXI_COLMSK0GREEN    0x53
#define	MGADXI_COLMSK0BLUE 	   0x54
#define	MGADXI_COLKEY0RED	   0x55
#define	MGADXI_COLKEY0GREEN    0x56
#define	MGADXI_COLKEY0BLUE 	   0x57

/* (D)AC (X) (I)ndexed registers (>= G450) */
#define MGADXI_TVO_IDX         0x87
#define MGADXI_TVO_DATA        0x88
#define MGADXI_OUTPUTCONN      0x8A
#define MGADXI_SYNCCTRL        0x8B
#define MGADXI_VIDPLLSTAT      0x8C
#define MGADXI_VIDPLLP         0x8D
#define MGADXI_VIDPLLM         0x8E
#define MGADXI_VIDPLLN         0x8F
#define MGADXI_PWRCTRL         0xA0
#define MGADXI_PANMODE         0xA2

/* MGA 1st CRTC registers */
#define MGACR1_VCOUNT        0x1E20

/* MGA 2nd CRTC registers (>= G400) */
#define MGACR2_CTL           0x3C10
#define MGACR2_HPARAM        0x3C14
#define MGACR2_HSYNC         0x3C18
#define MGACR2_VPARAM        0x3C1C
#define MGACR2_VSYNC         0x3C20
#define MGACR2_PRELOAD       0x3C24
#define MGACR2_STARTADD0     0x3C28
#define MGACR2_STARTADD1     0x3C2C
#define MGACR2_OFFSET        0x3C40
#define MGACR2_MISC          0x3C44
#define MGACR2_VCOUNT        0x3C48
#define MGACR2_DATACTL       0x3C4C

/* MGA ACCeleration registers */
#define MGAACC_DWGCTL          0x1C00
#define MGAACC_MACCESS         0x1C04
#define MGAACC_MCTLWTST        0x1C08
#define MGAACC_ZORG            0x1C0C
#define MGAACC_PLNWT           0x1C1C
#define MGAACC_BCOL            0x1C20
#define MGAACC_FCOL            0x1C24
#define MGAACC_XYSTRT          0x1C40
#define MGAACC_XYEND           0x1C44
#define MGAACC_SGN             0x1C58
#define MGAACC_LEN             0x1C5C
#define MGAACC_AR0             0x1C60
#define MGAACC_AR3             0x1C6C
#define MGAACC_AR5             0x1C74
#define MGAACC_CXBNDRY         0x1C80
#define MGAACC_FXBNDRY         0x1C84
#define MGAACC_YDSTLEN         0x1C88
#define MGAACC_PITCH           0x1C8C
#define MGAACC_YDST            0x1C90
#define MGAACC_YDSTORG         0x1C94
#define MGAACC_YTOP            0x1C98
#define MGAACC_YBOT            0x1C9C
#define MGAACC_CXLEFT          0x1CA0
#define MGAACC_CXRIGHT         0x1CA4
#define MGAACC_FXLEFT          0x1CA8
#define MGAACC_FXRIGHT         0x1CAC
#define MGAACC_STATUS          0x1E14
#define MGAACC_ICLEAR          0x1E18 /* required for interrupt stuff */
#define MGAACC_IEN             0x1E1C /* required for interrupt stuff */
#define MGAACC_RST             0x1E40
#define MGAACC_MEMRDBK         0x1E44
#define MGAACC_OPMODE          0x1E54
#define MGAACC_PRIMADDRESS     0x1E58
#define MGAACC_PRIMEND         0x1E5C
#define MGAACC_TEXORG          0x2C24 // >= G100 
#define MGAACC_DWGSYNC         0x2C4C // >= G200
#define MGAACC_TEXORG1         0x2CA4 // >= G200
#define MGAACC_TEXORG2         0x2CA8 // >= G200 
#define MGAACC_TEXORG3         0x2CAC // >= G200 
#define MGAACC_TEXORG4         0x2CB0 // >= G200 
#define MGAACC_SRCORG          0x2CB4 // >= G200
#define MGAACC_DSTORG          0x2CB8 // >= G200

/*MGA BES (Back End Scaler) registers (>= G200) */
#define MGABES_A1ORG           0x3D00
#define MGABES_A2ORG           0x3D04
#define MGABES_B1ORG           0x3D08
#define MGABES_B2ORG           0x3D0C
#define MGABES_A1CORG          0x3D10
#define MGABES_A2CORG          0x3D14
#define MGABES_B1CORG          0x3D18
#define MGABES_B2CORG          0x3D1C
#define MGABES_CTL             0x3D20
#define MGABES_PITCH           0x3D24
#define MGABES_HCOORD          0x3D28
#define MGABES_VCOORD          0x3D2C
#define MGABES_HISCAL          0x3D30
#define MGABES_VISCAL          0x3D34
#define MGABES_HSRCST          0x3D38
#define MGABES_HSRCEND         0x3D3C
#define MGABES_LUMACTL         0x3D40
#define MGABES_V1WGHT          0x3D48
#define MGABES_V2WGHT          0x3D4C
#define MGABES_HSRCLST         0x3D50
#define MGABES_V1SRCLST        0x3D54
#define MGABES_V2SRCLST        0x3D58
#define MGABES_A1C3ORG         0x3D60
#define MGABES_A2C3ORG         0x3D64
#define MGABES_B1C3ORG         0x3D68
#define MGABES_B2C3ORG         0x3D6C
#define MGABES_GLOBCTL         0x3DC0
#define MGABES_STATUS          0x3DC4

/*MAVEN registers (<= G400) */
#define MGAMAV_PGM            0x3E
#define MGAMAV_PIXPLLM        0x80
#define MGAMAV_PIXPLLN        0x81
#define MGAMAV_PIXPLLP        0x82
#define MGAMAV_GAMMA1         0x83
#define MGAMAV_GAMMA2         0x84
#define MGAMAV_GAMMA3         0x85
#define MGAMAV_GAMMA4         0x86
#define MGAMAV_GAMMA5         0x87
#define MGAMAV_GAMMA6         0x88
#define MGAMAV_GAMMA7         0x89
#define MGAMAV_GAMMA8         0x8A
#define MGAMAV_GAMMA9         0x8B
#define MGAMAV_MONSET         0x8C
#define MGAMAV_TEST           0x8D
#define MGAMAV_WREG_0X8E_L    0x8E
#define MGAMAV_WREG_0X8E_H    0x8F
#define MGAMAV_HSCALETV       0x90
#define MGAMAV_TSCALETVL      0x91
#define MGAMAV_TSCALETVH      0x92
#define MGAMAV_FFILTER        0x93
#define MGAMAV_MONEN          0x94
#define MGAMAV_RESYNC         0x95
#define MGAMAV_LASTLINEL      0x96
#define MGAMAV_LASTLINEH      0x97
#define MGAMAV_WREG_0X98_L    0x98
#define MGAMAV_WREG_0X98_H    0x99
#define MGAMAV_HSYNCLENL      0x9A
#define MGAMAV_HSYNCLENH      0x9B
#define MGAMAV_HSYNCSTRL      0x9C
#define MGAMAV_HSYNCSTRH      0x9D
#define MGAMAV_HDISPLAYL      0x9E
#define MGAMAV_HDISPLAYH      0x9F
#define MGAMAV_HTOTALL        0xA0
#define MGAMAV_HTOTALH        0xA1
#define MGAMAV_VSYNCLENL      0xA2
#define MGAMAV_VSYNCLENH      0xA3
#define MGAMAV_VSYNCSTRL      0xA4
#define MGAMAV_VSYNCSTRH      0xA5
#define MGAMAV_VDISPLAYL      0xA6
#define MGAMAV_VDISPLAYH      0xA7
#define MGAMAV_VTOTALL        0xA8
#define MGAMAV_VTOTALH        0xA9
#define MGAMAV_HVIDRSTL       0xAA
#define MGAMAV_HVIDRSTH       0xAB
#define MGAMAV_VVIDRSTL       0xAC
#define MGAMAV_VVIDRSTH       0xAD
#define MGAMAV_VSOMETHINGL    0xAE
#define MGAMAV_VSOMETHINGH    0xAF
#define MGAMAV_OUTMODE        0xB0
#define MGAMAV_VERSION        0xB2
#define MGAMAV_LOCK           0xB3
#define MGAMAV_LUMA           0xB9
#define MGAMAV_VDISPLAYTV     0xBE
#define MGAMAV_STABLE         0xBF
#define MGAMAV_HDISPLAYTV     0xC2
#define MGAMAV_BREG_0XC6      0xC6

/* Macros for convenient accesses to the MGA chips */

#define MGA_REG8(r_)  ((vuint8  *)regs)[(r_)]
#define MGA_REG32(r_) ((vuint32 *)regs)[(r_) >> 2]

/* read and write to PCI config space */
#define CFGR(A)   (gx00_pci_access.offset=MGACFG_##A, ioctl(fd,GX00_GET_PCI, &gx00_pci_access,sizeof(gx00_pci_access)), gx00_pci_access.value)
#define CFGW(A,B) (gx00_pci_access.offset=MGACFG_##A, gx00_pci_access.value = B, ioctl(fd,GX00_SET_PCI,&gx00_pci_access,sizeof(gx00_pci_access)))

/* read and write from the dac registers */
#define DACR(A)   (MGA_REG8(MGADAC_##A))
#define DACW(A,B) (MGA_REG8(MGADAC_##A)=B)

/* read and write from the dac index register */
#define DXIR(A)   (DACW(PALWTADD,MGADXI_##A),DACR(X_DATAREG))
#define DXIW(A,B) (DACW(PALWTADD,MGADXI_##A),DACW(X_DATAREG,B))

/* read and write from the vga registers */
#define VGAR(A)   (MGA_REG8(MGAVGA_##A))
#define VGAW(A,B) (MGA_REG8(MGAVGA_##A)=B)

/* read and write from the indexed vga registers */
#define VGAR_I(A,B)   (VGAW(A##_I,B),VGAR(A##_D))
#define VGAW_I(A,B,C) (VGAW(A##_I,B),VGAW(A##_D,C))

/* read and write from the powergraphics registers */
#define ACCR(A)    (MGA_REG32(MGAACC_##A))
#define ACCW(A,B)  (MGA_REG32(MGAACC_##A)=B)
#define ACCGO(A,B) (MGA_REG32(MGAACC_##A + 0x0100)=B)

/* read and write from the backend scaler registers */
#define BESR(A)   (MGA_REG32(MGABES_##A))
#define BESW(A,B) (MGA_REG32(MGABES_##A)=B)

/* read and write from first CRTC */
#define CR1R(A)   (MGA_REG32(MGACR1_##A))
#define CR1W(A,B) (MGA_REG32(MGACR1_##A)=B)

/* read and write from second CRTC */
#define CR2R(A)   (MGA_REG32(MGACR2_##A))
#define CR2W(A,B) (MGA_REG32(MGACR2_##A)=B)

/* read and write from maven (<= G400) */
#define MAVR(A)     (i2c_maven_read (MGAMAV_##A ))
#define MAVW(A,B)   (i2c_maven_write(MGAMAV_##A ,B))
#define MAVRW(A)    (i2c_maven_read (MGAMAV_##A )|(i2c_maven_read(MGAMAV_##A +1)<<8))
#define MAVWW(A,B)  (i2c_maven_write(MGAMAV_##A ,B &0xFF),i2c_maven_write(MGAMAV_##A +1,B >>8))
