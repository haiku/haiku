/*
	Mach64.h
	Mach64 Definitions and things
	
	Rene MacKinney
	22.4.99
*/

/* ATI PCI constants */
#define ATI_VENDOR_ID           0x1002
#define PCI_MACH64_GX_ID        0x4758
#define PCI_MACH64_CX_ID        0x4358
#define PCI_MACH64_CT_ID        0x4354
#define PCI_MACH64_ET_ID        0x4554
#define PCI_MACH64_VT_ID        0x5654
#define PCI_MACH64_VU_ID        0x5655
#define PCI_MACH64_GT_ID        0x4754
#define PCI_MACH64_GU_ID        0x4755
#define PCI_MACH64_GB_ID        0x4742
#define PCI_MACH64_GD_ID        0x4744
#define PCI_MACH64_GI_ID        0x4749
#define PCI_MACH64_GP_ID        0x4750
#define PCI_MACH64_GQ_ID        0x4751
#define PCI_MACH64_VV_ID        0x5656
#define PCI_MACH64_GV_ID        0x4756
#define PCI_MACH64_GW_ID        0x4757
#define PCI_MACH64_GZ_ID        0x475A
#define PCI_MACH64_LD_ID        0x4C44
#define PCI_MACH64_LG_ID        0x4C47
#define PCI_MACH64_LB_ID        0x4C42
#define PCI_MACH64_LI_ID        0x4C49
#define PCI_MACH64_LP_ID        0x4C50    

/* 
	Define a structure to hold all the necessary information 
*/

	
typedef struct card_info {
    pci_info	pcii;
    vuchar	*base0;
#if !defined(__INTEL__)
    vuchar	*isa_io;
#endif
    int     theMem;
    int	    scrnRowByte;
    int	    scrnWidth;
    int	    scrnHeight;
    int	    offscrnWidth;
    int	    offscrnHeight;
    int	    scrnPosH;
    int	    scrnPosV;
    int	    scrnColors;
    void    *scrnBase;
    float   scrnRate;
    short   crtPosH;
    short   crtSizeH;
    short   crtPosV;
    short   crtSizeV;
    ulong   scrnResCode;
    int     scrnResNum;
    uchar   *scrnBufBase;
    long	scrnRes;
    ulong   available_spaces;
    int     hotpt_h;
    int     hotpt_v;
    short   lastCrtHT;
    short   lastCrtVT;
    int     CursorMode;
    ulong	dot_clock;
} card_info;

