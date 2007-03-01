/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Bus Control registers
*/

#ifndef _BUSCNTRL_REGS_H
#define _BUSCNTRL_REGS_H

#define RADEON_BUS_CNTL                     0x0030
#       define RADEON_BUS_MASTER_DIS         (1 << 6)
#       define RADEON_BUS_RD_DISCARD_EN      (1 << 24)
#       define RADEON_BUS_RD_ABORT_EN        (1 << 25)
#       define RADEON_BUS_MSTR_DISCONNECT_EN (1 << 28)
#       define RADEON_BUS_WRT_BURST          (1 << 29)
#       define RADEON_BUS_READ_BURST         (1 << 30)
#define RADEON_BUS_CNTL1                    0x0034
#       define RADEON_BUS_WAIT_ON_LOCK_EN    (1 << 4)

#define RADEON_AGP_CNTL                     0x0174
#       define RADEON_AGP_APER_SIZE_256MB   (0x00 << 0)
#       define RADEON_AGP_APER_SIZE_128MB   (0x20 << 0)
#       define RADEON_AGP_APER_SIZE_64MB    (0x30 << 0)
#       define RADEON_AGP_APER_SIZE_32MB    (0x38 << 0)
#       define RADEON_AGP_APER_SIZE_16MB    (0x3c << 0)
#       define RADEON_AGP_APER_SIZE_8MB     (0x3e << 0)
#       define RADEON_AGP_APER_SIZE_4MB     (0x3f << 0)
#       define RADEON_AGP_APER_SIZE_MASK    (0x3f << 0)
#define RADEON_AGP_COMMAND                  0x0f60 /* PCI */
#define RADEON_AGP_PLL_CNTL                 0x000b /* PLL */
#define RADEON_AGP_STATUS                   0x0f5c /* PCI */
#       define RADEON_AGP_1X_MODE           0x01
#       define RADEON_AGP_2X_MODE           0x02
#       define RADEON_AGP_4X_MODE           0x04
#       define RADEON_AGP_MODE_MASK         0x07

#define RADEON_MM_DATA                      0x0004

#define RADEON_AIC_CNTL                     0x01d0
#       define RADEON_PCIGART_TRANSLATE_EN  (1 << 0)

// the limit is taken from XFree86; actually, I haven't
// found any restrictions in the specs
#define ATI_MAX_PCIGART_PAGES		8192	// 32 MB aperture, 4K pages
#define ATI_PCIGART_PAGE_SIZE		4096	// PCI GART page size

#define RADEON_AIC_STAT			0x01d4
#define RADEON_AIC_PT_BASE		0x01d8
#define RADEON_AIC_LO_ADDR		0x01dc
#define RADEON_AIC_HI_ADDR		0x01e0
#define RADEON_AIC_TLB_ADDR		0x01e4
#define RADEON_AIC_TLB_DATA		0x01e8

#define RADEON_HOST_PATH_CNTL               0x0130
#       define RADEON_HDP_SOFT_RESET        (1 << 26)
#       define RADEON_HDP_APER_CNTL         (1 << 23)

#endif
