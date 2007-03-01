/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Config registers (most are in PCI configuration space)
*/

#ifndef _CONFIG_REGS_H
#define _CONFIG_REGS_H

// mmio registers
#define RADEON_CONFIG_APER_0_BASE           0x0100
#define RADEON_CONFIG_APER_1_BASE           0x0104
#define RADEON_CONFIG_APER_SIZE             0x0108

#define RADEON_CONFIG_CNTL                  0x00e0
#       define RADEON_CFG_ATI_REV_A11       (0   << 16)
#       define RADEON_CFG_ATI_REV_A12       (1   << 16)
#       define RADEON_CFG_ATI_REV_A13       (2   << 16)
#       define RADEON_CFG_ATI_REV_ID_MASK   (0xf << 16)
#define	RADEON_CFG_ATI_REV_ID_SHIFT		16
#define RADEON_CONFIG_MEMSIZE               0x00f8
#       define RADEON_CONFIG_MEMSIZE_MASK   0x1ff00000

// following registers can be accessed via PCI configuration space too
// (PCI-configuration-space-add + 0xf00 = MMIO-address)
#define RADEON_VENDOR_ID                    0x0f00
#define RADEON_DEVICE_ID                    0x0f02
#define RADEON_COMMAND                      0x0f04
#define RADEON_STATUS                       0x0f06
#define RADEON_REVISION_ID                  0x0f08
#define RADEON_REGPROG_INF                  0x0f09
#define RADEON_SUB_CLASS                    0x0f0a
#define RADEON_BASE_CODE                    0x0f0b
#define RADEON_CACHE_LINE                   0x0f0c
#define RADEON_LATENCY                      0x0f0d
#define RADEON_HEADER                       0x0f0e
#define RADEON_BIST                         0x0f0f
#define RADEON_MEM_BASE                     0x0f10
#define RADEON_IO_BASE                      0x0f14
#define RADEON_REG_BASE                     0x0f18
#define RADEON_ADAPTER_ID                   0x0f2c //mirror of AADPER_ID_W
#define RADEON_BIOS_ROM                     0x0f30
#define RADEON_CAPABILITIES_PTR             0x0f34
#define RADEON_INTERRUPT_LINE               0x0f3c
#define RADEON_INTERRUPT_PIN                0x0f3d
#define RADEON_MIN_GRANT                    0x0f3e
#define RADEON_MAX_LATENCY                  0x0f3f
#define RADEON_ADAPTER_ID_W                 0x0f4c
#define RADEON_CAPABILITIES_ID              0x0f50

#define RADEON_PMI_CAP_ID                   0x0f50
#define RADEON_PMI_NXT_CAP_PTR              0x0f51
#define RADEON_PMI_PMC_REG                  0x0f52
#define RADEON_PMI_STATUS                   0x0f54
#define RADEON_PMI_DATA                     0x0f57

#define RADEON_AGP_CAP_ID                   0x0f58
#define RADEON_AGP_STATUS                   0x0f5c
#define RADEON_AGP_COMMAND                  0x0f60


#endif
