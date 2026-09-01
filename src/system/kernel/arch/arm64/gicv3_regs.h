/*
 * Copyright 2017 The Fuchsia Authors
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ARCH_ARM64_GICV3_REGS_H
#define ARCH_ARM64_GICV3_REGS_H

#include <arch/arm64/arch_cpu.h>

#define BIT_32(bit) (1u << (bit))
#define BIT_64(bit) (1ul << (bit))

#define GICD_REG_SIZE (0x10000)

// CPU interface system registers.

#define ICC_CTLR_EL1 S3_0_C12_C12_4
#define ICC_PMR_EL1 S3_0_C4_C6_0
#define ICC_IAR1_EL1 S3_0_C12_C12_0
#define ICC_SRE_EL1 S3_0_C12_C12_5
#define ICC_BPR1_EL1 S3_0_C12_C12_3
#define ICC_IGRPEN1_EL1 S3_0_C12_C12_7
#define ICC_EOIR1_EL1 S3_0_C12_C12_1
#define ICC_DIR_EL1 S3_0_C12_C11_1
#define ICC_SGI1R_EL1 S3_0_C12_C11_5

// ICC_SGI1R bit definitions
#define ICC_SGI1R_IRM BIT_64(40)

// Distributor registers.

#define GICD_CTLR _GicdReg32(0x0000)
#define GICD_TYPER _GicdReg32(0x0004)
#define GICD_IIDR _GicdReg32(0x0008)
#define GICD_IGROUPR(n) _GicdReg32(0x0080 + (n) * 4)
#define GICD_ISENABLER(n) _GicdReg32(0x0100 + (n) * 4)
#define GICD_ICENABLER(n) _GicdReg32(0x0180 + (n) * 4)
#define GICD_ISPENDR(n) _GicdReg32(0x0200 + (n) * 4)
#define GICD_ICPENDR(n) _GicdReg32(0x0280 + (n) * 4)
#define GICD_ISACTIVER(n) _GicdReg32(0x0300 + (n) * 4)
#define GICD_ICACTIVER(n) _GicdReg32(0x0380 + (n) * 4)
#define GICD_IPRIORITYR(n) _GicdReg32(0x0400 + (n) * 4)
#define GICD_ITARGETSR(n) _GicdReg32(0x0800 + (n) * 4)
#define GICD_ICFGR(n) _GicdReg32(0x0c00 + (n) * 4)
#define GICD_IGRPMODR(n) _GicdReg32(0x0d00 + (n) * 4)
#define GICD_NSACR(n) _GicdReg32(0x0e00 + (n) * 4)
#define GICD_SGIR _GicdReg32(0x0f00)
#define GICD_CPENDSGIR(n) _GicdReg32(0x0f10 + (n) * 4)
#define GICD_SPENDSGIR(n) _GicdReg32(0x0f20 + (n) * 4)
#define GICD_IROUTER(n) _GicdReg64(0x6000 + (n) * 8)

// GICD_CTLR bit definitions.

#define CTLR_ENABLE_G0 BIT_32(0)
#define CTLR_ENABLE_G1NS BIT_32(1)
#define CTLR_ENABLE_G1S BIT_32(2)
#define CTLR_RES0 BIT_32(3)
#define CTLR_ARE_S BIT_32(4)
#define CTLR_ARE_NS BIT_32(5)
#define CTLR_DS BIT_32(6)
#define CTLR_E1NWF BIT_32(7)
#define GICD_CTLR_RWP BIT_32(31)

// GICR_CTLR bit definitions.

#define GICR_CTLR_RWP BIT_32(3)

// Peripheral identification registers.

#define GICD_CIDR0 _GicdReg32(0xfff0)
#define GICD_CIDR1 _GicdReg32(0xfff4)
#define GICD_CIDR2 _GicdReg32(0xfff8)
#define GICD_CIDR3 _GicdReg32(0xfffc)
#define GICD_PIDR0 _GicdReg32(0xffe0)
#define GICD_PIDR1 _GicdReg32(0xffe4)
#define GICD_PIDR2 _GicdReg32(0xffe8)
#define GICD_PIDR3 _GicdReg32(0xffec)

// GICD_PIDR bit definitions and masks.

#define GICD_PIDR2_ARCHREV_SHIFT 4
#define GICD_PIDR2_ARCHREV_MASK 0xf

// GICR_WAKER bit definitions and masks.

#define WAKER_CHILDREN_ASLEEP BIT_32(2)
#define WAKER_PROCESSOR_SLEEP BIT_32(1)

// Redistributor registers.
// Arranged as 2 or 3 banks (+1 empty bank) of 64KiB depending if it's
// GICv3 or GICv4.
#define GICR_SGI_BASE (0x10000)
#define GICR_VLPI_BASE (0x20000) // GICv4 only

#define GICR_CTLR(i) _GicrReg32(fGicrStride*(i) + 0x0000)
#define GICR_IIDR(i) _GicrReg32(fGicrStride*(i) + 0x0004)
#define GICR_TYPER(i) _GicrReg64(fGicrStride*(i) + 0x0008)
#define GICR_STATUSR(i) _GicrReg32(fGicrStride*(i) + 0x0010)
#define GICR_WAKER(i) _GicrReg32(fGicrStride*(i) + 0x0014)
#define GICR_IGROUPR0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0080)
#define GICR_IGRPMOD0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0d00)
#define GICR_ISENABLER0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0100)
#define GICR_ICENABLER0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0180)
#define GICR_ISPENDR0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0200)
#define GICR_ICPENDR0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0280)
#define GICR_ISACTIVER0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0300)
#define GICR_ICACTIVER0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0380)
#define GICR_IPRIORITYR(i, n) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0400 + (n) * 4)
#define GICR_ICFGR0(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0c00)
#define GICR_ICFGR1(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0c04)
#define GICR_NSACR(i) _GicrReg32(GICR_SGI_BASE + fGicrStride * (i) + 0x0e00)


static void
gic_write_pmr(uint32_t val)
{
	WRITE_SPECIALREG(ICC_PMR_EL1, val);
	arm64_isb();
	memory_full_barrier();
}


static void
gic_write_igrpen(uint32_t val)
{
	WRITE_SPECIALREG(ICC_IGRPEN1_EL1, val);
	arm64_isb();
}


static uint32_t
gic_read_sre()
{
	uint64_t val = READ_SPECIALREG(ICC_SRE_EL1);
	return static_cast<uint32_t>(val);
}


static void
gic_write_sre(uint32_t val)
{
	WRITE_SPECIALREG(ICC_SRE_EL1, val);
	arm64_isb();
}


static void
gic_write_eoir(uint32_t val)
{
	WRITE_SPECIALREG(ICC_EOIR1_EL1, val);
	arm64_isb();
}


static uint32_t
gic_read_iar()
{
	uint64_t val = READ_SPECIALREG(ICC_IAR1_EL1);
	memory_full_barrier();
	return static_cast<uint32_t>(val);
}


static void
gic_write_sgi1r(uint64_t val)
{
	WRITE_SPECIALREG(ICC_SGI1R_EL1, val);
	arm64_isb();
	memory_full_barrier();
}

#endif // ARCH_ARM64_GICV3_REGS_H
