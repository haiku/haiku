#ifndef __VOODOO3_IO_H__
#define __VOODOO3_IO_H__

#include <ByteOrder.h>

// voodoo3 register locations in pci memory space
/* membase0 register offsets */
#define STATUS		0x00
#define PCIINIT0	0x04
#define SIPMONITOR	0x08
#define LFBMEMORYCONFIG	0x0c
#define MISCINIT0	0x10
#define MISCINIT1	0x14
#define DRAMINIT0	0x18
#define DRAMINIT1	0x1c
#define AGPINIT		0x20
#define TMUGBEINIT	0x24
#define VGAINIT0	0x28
#define VGAINIT1	0x2c
#define DRAMCOMMAND	0x30
#define DRAMDATA	0x34
/* reserved             0x38 */
/* reserved             0x3c */
#define PLLCTRL0	0x40
#define PLLCTRL1	0x44
#define PLLCTRL2	0x48
#define DACMODE		0x4c
#define DACADDR		0x50
#define DACDATA		0x54
#define RGBMAXDELTA	0x58
#define VIDPROCCFG	0x5c
#define HWCURPATADDR	0x60
#define HWCURLOC	0x64
#define HWCURC0		0x68
#define HWCURC1		0x6c
#define VIDINFORMAT	0x70
#define VIDINSTATUS	0x74
#define VIDSERPARPORT	0x78
#define VIDINXDELTA	0x7c
#define VIDININITERR	0x80
#define VIDINYDELTA	0x84
#define VIDPIXBUFTHOLD	0x88
#define VIDCHRMIN	0x8c
#define VIDCHRMAX	0x90
#define VIDCURLIN	0x94
#define VIDSCREENSIZE	0x98
#define VIDOVRSTARTCRD	0x9c
#define VIDOVRENDCRD	0xa0
#define VIDOVRDUDX	0xa4
#define VIDOVRDUDXOFF	0xa8
#define VIDOVRDVDY	0xac
/*  ... */

#define VIDOVRDVDYOFF	0xe0
#define VIDDESKSTART	0xe4
#define VIDDESKSTRIDE	0xe8
#define VIDINADDR0	0xec
#define VIDINADDR1	0xf0
#define VIDINADDR2	0xf4
#define VIDINSTRIDE	0xf8
#define VIDCUROVRSTART	0xfc
#define VIDOVERLAYSTARTCOORDS 0x9c
#define VIDOVERLAYENDSCREENCOORDS 0xa0
#define VIDOVERLAYDUDX 0xa4
#define VIDOVERLAYDUDXOFFSETSRCWIDTH 0xa8
#define VIDOVERLAYDVDY 0xac
#define VIDOVERLAYDVDYOFFSET 0xe0

#define SST_3D_OFFSET           	0x200000
#define SST_3D_LEFTOVERLAYBUF		SST_3D_OFFSET+0x250

#define INTCTRL		(0x00100000 + 0x04)
#define CLIP0MIN	(0x00100000 + 0x08)
#define CLIP0MAX	(0x00100000 + 0x0c)
#define DSTBASE		(0x00100000 + 0x10)
#define DSTFORMAT	(0x00100000 + 0x14)
#define SRCBASE		(0x00100000 + 0x34)
#define COMMANDEXTRA_2D	(0x00100000 + 0x38)
#define CLIP1MIN	(0x00100000 + 0x4c)
#define CLIP1MAX	(0x00100000 + 0x50)
#define SRCFORMAT	(0x00100000 + 0x54)
#define SRCSIZE		(0x00100000 + 0x58)
#define SRCXY		(0x00100000 + 0x5c)
#define COLORBACK	(0x00100000 + 0x60)
#define COLORFORE	(0x00100000 + 0x64)
#define DSTSIZE		(0x00100000 + 0x68)
#define DSTXY		(0x00100000 + 0x6c)
#define COMMAND_2D	(0x00100000 + 0x70)
#define LAUNCH_2D	(0x00100000 + 0x80)

#define COMMAND_3D	(0x00200000 + 0x120)

/* register bitfields (not all, only as needed) */

#define BIT(x) (1UL << (x))

/* COMMAND_2D reg. values */
#define ROP_COPY	0xcc     // src
#define ROP_INVERT      0x55     // NOT dst
#define ROP_XOR         0x66     // src XOR dst

#define AUTOINC_DSTX                    BIT(10)
#define AUTOINC_DSTY                    BIT(11)
#define COMMAND_2D_FILLRECT		0x05
#define COMMAND_2D_S2S_BITBLT		0x01      // screen to screen
#define COMMAND_2D_H2S_BITBLT           0x03       // host to screen


#define COMMAND_3D_NOP			0x00
#define STATUS_RETRACE			BIT(6)
#define STATUS_BUSY			BIT(9)
#define MISCINIT1_CLUT_INV		BIT(0)
#define MISCINIT1_2DBLOCK_DIS		BIT(15)
#define DRAMINIT0_SGRAM_NUM		BIT(26)
#define DRAMINIT0_SGRAM_TYPE		BIT(27)
#define DRAMINIT1_MEM_SDRAM		BIT(30)
#define VGAINIT0_VGA_DISABLE		BIT(0)
#define VGAINIT0_EXT_TIMING		BIT(1)
#define VGAINIT0_8BIT_DAC		BIT(2)
#define VGAINIT0_EXT_ENABLE		BIT(6)
#define VGAINIT0_WAKEUP_3C3		BIT(8)
#define VGAINIT0_LEGACY_DISABLE		BIT(9)
#define VGAINIT0_ALT_READBACK		BIT(10)
#define VGAINIT0_FAST_BLINK		BIT(11)
#define VGAINIT0_EXTSHIFTOUT		BIT(12)
#define VGAINIT0_DECODE_3C6		BIT(13)
#define VGAINIT0_SGRAM_HBLANK_DISABLE	BIT(22)
#define VGAINIT1_MASK			0x1fffff
#define VIDCFG_VIDPROC_ENABLE		BIT(0)
#define VIDCFG_CURS_X11			BIT(1)
#define VIDCFG_HALF_MODE		BIT(4)
#define VIDCFG_CHROMA_KEY		BIT(5)
#define VIDCFG_CHROMA_KEY_INVERSION	BIT(6)
#define VIDCFG_DESK_ENABLE		BIT(7)
#define VIDCFG_OVL_ENABLE		BIT(8)
#define VIDCFG_OVL_NOT_VIDEO_IN	BIT(9)
#define VIDCFG_CLUT_BYPASS		BIT(10)
#define VIDCFG_OVL_CLUT_BYPASS	BIT(11)
#define VIDCFG_OVL_HSCALE		BIT(14)
#define VIDCFG_OVL_VSCALE		BIT(15)
#define VIDCFG_OVL_FILTER_SHIFT	16
#define VIDCFG_OVL_FILTER_POINT	0
#define VIDCFG_OVL_FILTER_2X2	1
#define VIDCFG_OVL_FILTER_4X4	2
#define VIDCFG_OVL_FILTER_BILIN	3
#define VIDCFG_OVL_FMT_SHIFT	21
#define VIDCFG_OVL_FMT_RGB565	1
#define VIDCFG_OVL_FMT_YUV411	4
#define VIDCFG_OVL_FMT_YUYV422	5
#define VIDCFG_OVL_FMT_UYVY422	6
#define VIDCFG_OVL_FMT_RGB565_DITHER 7

#define VIDCFG_2X			BIT(26)
#define VIDCFG_HWCURSOR_ENABLE BIT(27)
#define VIDCFG_PIXFMT_SHIFT		18
#define DACMODE_2X			BIT(0)
#define VIDPROCCFGMASK          0xa2e3eb6c
#define VIDPROCDEFAULT			134481025

#define VIDCHROMAMIN 0x8c
#define VIDCHROMAMAX 0x90
#define VIDDESKTOPOVERLAYSTRIDE 0xe8

#define CRTC_INDEX  0x3d4
#define CRTC_DATA   0x3d5
#define SEQ_INDEX   0x3c4
#define SEQ_DATA    0x3c5
#define MISC_W		0x3c2
#define GRA_INDEX	0x3ce
#define ATT_IW		0x3c0
#define IS1_R		0x3da

// pci read from memory
#define PCI_MEM_RD_8(address) (*(volatile uint8 *) (address))
#define PCI_MEM_RD_16(address) B_LENDIAN_TO_HOST_INT16((*(volatile uint16 *) (address)))
#define PCI_MEM_RD_32(address) B_LENDIAN_TO_HOST_INT32((*(volatile uint32 *) (address)))
// pci write to memory
#define PCI_MEM_WR_8(address, value) ((*(volatile uint8 *) (address)) = (uint8) (value))
#define PCI_MEM_WR_16(address, value) ((*(volatile uint16 *) (address)) = (uint16) B_HOST_TO_LENDIAN_INT16((value))
#define PCI_MEM_WR_32(address, value) ((*(volatile uint32 *) (address)) = (uint32) B_HOST_TO_LENDIAN_INT32(value))

#endif
