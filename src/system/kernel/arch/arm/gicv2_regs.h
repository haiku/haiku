#ifndef ARCH_ARM_GIC_REGS_H
#define ARCH_ARM_GIC_REGS_H

#define GICD_REG_START		0x08000000
#define GICD_REG_SIZE		0x00010000

#define GICD_REG_CTLR		0
#define GICD_REG_TYPER		1
#define GICD_REG_IIDR		2

#define GICD_REG_IGROUP		32
#define GICD_REG_ISENABLER	64
#define GICD_REG_ICENABLER	96
#define GICD_REG_ISPENDR	128
#define GICD_REG_ICPENDR	160
#define GICD_REG_ISACTIVER	192
#define GICD_REG_ICACTIVER	224
#define GICD_REG_IPRIORITYR	256
#define GICD_REG_ITARGETSR	512

#define GICD_REG_ICPIDR0	1016
#define GICD_REG_ICPIDR1	1017
#define GICD_REG_ICPIDR2	1018

#define GICD_REG_SGIR		960

#define GICC_REG_START		0x08010000
#define GICC_REG_SIZE		0x00010000

#define GICC_REG_CTLR		0
#define GICC_REG_PMR		1
#define GICC_REG_BPR		2
#define GICC_REG_IAR		3
#define GICC_REG_EOIR		4
#define GICC_REG_RPR		5
#define GICC_REG_HPPIR		6

#define GICC_REG_IIDR		63
#define GICC_REG_DIR		1024

#endif