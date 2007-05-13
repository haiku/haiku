#ifndef HDAC_REGS_H
#define HDAC_REGS_H

#include <SupportDefs.h>

/* Accessors for HDA controller registers */
#define REG32(ctrlr,reg)	(*(vuint32*)((ctrlr)->regs + HDAC_##reg))
#define REG16(ctrlr,reg)	(*(vuint16*)((ctrlr)->regs + HDAC_##reg))
#define REG8(ctrlr,reg)		(*((ctrlr)->regs + HDAC_##reg))

#define OREG32(ctrlr,s,reg)	(*(vuint32*)((ctrlr)->regs + HDAC_SDBASE + (s) + HDAC_SD_##reg))
#define OREG16(ctrlr,s,reg)	(*(vuint16*)((ctrlr)->regs + HDAC_SDBASE + (s) + HDAC_SD_##reg))
#define OREG8(ctrlr,s,reg)	(*((ctrlr)->regs           + HDAC_SDBASE + (s) + HDAC_SD_##reg))

/* Register definitions */
#define HDAC_GCAP			0x00		/* 16bits */
#define GCAP_OSS(gcap)		(((gcap) >> 12) & 15)
#define GCAP_ISS(gcap)		(((gcap) >> 8) & 15)
#define GCAP_BSS(gcap)		(((gcap) >> 3) & 15)
#define GCAP_NSDO(gcap)		(((gcap) >> 1) & 3)
#define GCAP_64OK			((gcap) & 1)

#define HDAC_VMIN			0x02		/* 8bits */
#define HDAC_VMAJ			0x03		/* 8bits */

#define HDAC_GCTL			0x08		/* 32bits */
#define GCTL_UNSOL			(1 << 8)	/* Accept Unsolicited responses */
#define GCTL_FCNTRL			(1 << 1)	/* Flush Control */
#define GCTL_CRST			(1 << 0)	/* Controller Reset */

#define HDAC_WAKEEN			0x0c		/* 16bits */
#define HDAC_STATESTS		0x0e		/* 16bits */

#define HDAC_INTCTL			0x20		/* 32bits */
#define INTCTL_GIE			(1 << 31)	/* Global Interrupt Enable */
#define INTCTL_CIE			(1 << 30)	/* Controller Interrupt Enable */

#define HDAC_INTSTS			0x24		/* 32bits */
#define INTSTS_GIS			(1 << 31)	/* Global Interrupt Status */
#define INTSTS_CIS			(1 << 30)	/* Controller Interrupt Status */

#define HDAC_CORBLBASE		0x40		/* 32bits */
#define HDAC_CORBUBASE		0x44		/* 32bits */
#define HDAC_CORBWP			0x48		/* 16bits */

#define HDAC_CORBRP			0x4a		/* 16bits */
#define CORBRP_RST			(1 << 15)

#define HDAC_CORBCTL		0x4c		/* 8bits */
#define CORBCTL_RUN			(1 << 1)
#define CORBCTL_MEIE		(1 << 0)

#define HDAC_CORBSTS		0x4d		/* 8bits */
#define CORBSTS_MEI			(1 << 0)

#define HDAC_CORBSIZE		0x4e		/* 8bits */
#define CORBSIZE_CAP_2E		(1 << 4)
#define CORBSIZE_CAP_16E	(1 << 5)
#define CORBSIZE_CAP_256E	(1 << 6)
#define CORBSIZE_SZ_2E		0
#define CORBSIZE_SZ_16E		(1 << 0)
#define CORBSIZE_SZ_256E	(1 << 1)

#define HDAC_RIRBLBASE		0x50		/* 32bits */
#define HDAC_RIRBUBASE		0x54		/* 32bits */

#define HDAC_RIRBWP			0x58		/* 16bits */
#define RIRBWP_RST			(1 << 15)

#define HDAC_RINTCNT		0x5a		/* 16bits */

#define HDAC_RIRBCTL		0x5c		/* 8bits */
#define RIRBCTL_OIC			(1 << 2)
#define RIRBCTL_DMAEN		(1 << 1)
#define RIRBCTL_RINTCTL		(1 << 0)

#define HDAC_RIRBSTS		0x5d
#define RIRBSTS_OIS			(1 << 2)
#define RIRBSTS_RINTFL		(1 << 0)

#define HDAC_RIRBSIZE		0x5e		/* 8bits */
#define RIRBSIZE_CAP_2E		(1 << 4)
#define RIRBSIZE_CAP_16E	(1 << 5)
#define RIRBSIZE_CAP_256E	(1 << 6)
#define RIRBSIZE_SZ_2E		0
#define RIRBSIZE_SZ_16E		(1 << 0)
#define RIRBSIZE_SZ_256E	(1 << 1)

#define HDAC_SDBASE			0x80
#define HDAC_SDSIZE			0x20

#define HDAC_SD_CTL0		0x00		/* 8bits */
#define CTL0_SRST			(1 << 0)
#define CTL0_RUN			(1 << 1)
#define CTL0_IOCE			(1 << 2)
#define CTL0_FEIE			(1 << 3)
#define CTL0_DEIE			(1 << 4)
#define HDAC_SD_CTL1		0x01		/* 8bits */
#define HDAC_SD_CTL2		0x02		/* 8bits */
#define CTL2_DIR			(1 << 3)
#define CTL2_TP				(1 << 2)
#define HDAC_SD_STS			0x03		/* 8bits */
#define STS_BCIS			(1 << 2)
#define STS_FIFOE			(1 << 3)
#define STS_DESE			(1 << 4)
#define STS_FIFORDY			(1 << 5)
#define HDAC_SD_LPIB		0x04		/* 32bits */
#define HDAC_SD_CBL			0x08		/* 32bits */
#define HDAC_SD_LVI			0x0C		/* 16bits */
#define HDAC_SD_FIFOS		0x10		/* 16bits */
#define HDAC_SD_FMT			0x12		/* 16bits */
#define HDAC_SD_BDPL		0x18		/* 32bits */
#define HDAC_SD_BDPU		0x1C		/* 32bits */

typedef uint32 corb_t;
typedef struct {
	uint32 response;
	uint32 resp_ex;
#define RESP_EX_UNSOL	(1 << 4)
} rirb_t;

typedef struct {
	uint64	address;
	uint32	length;
	uint32	ioc;
} bdl_entry_t;

#endif /* HDAC_REGS_H */
