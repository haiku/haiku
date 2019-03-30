/*
 * Copyright (c) 1998, 1999, Be Incorporated.  All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

/*! Ethernet driver: handles PCI NE2000 cards */

#include "etherpci_private.h"
#include <ether_driver.h>

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define kDevName "etherpci"
#define kDevDir "net/" kDevName "/"
#define DEVNAME_LENGTH		64
#define MAX_CARDS 			 4			/* maximum number of driver instances */

int32 api_version = B_CUR_DRIVER_API_VERSION;

/* debug flags */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define PCI_IO    0x0040		/* pci reads and writes */
#define SEQ		  0x0080		/* trasnmit & receive TCP/IP sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off on final release */

/* diagnostic debug flags - compile in here or set while running with debugger "AcmeRoadRunner" command */
#define DEFAULT_DEBUG_FLAGS ( ERR | INFO | WARN | FUNCTION )

//#define TRACE_ETHERPCI
#ifdef TRACE_ETHERPCI
#	define ETHER_DEBUG(mask, enabled, format, args...) \
		do { if (mask & enabled) \
			dprintf(format , ##args); } while (0)
#else
#	define ETHER_DEBUG(mask, enabled, format, args...) ;
#endif


static pci_module_info		*gPCIModInfo;
static char 				*gDevNameList[MAX_CARDS+1];
static pci_info 			*gDevList[MAX_CARDS+1];
static int32 				gOpenMask = 0;


/* Driver Entry Points */
status_t init_hardware(void);
status_t init_driver(void);
void uninit_driver(void);
const char** publish_devices(void);
device_hooks *find_device(const char *name);



/*
 * Define STAY_ON_PAGE_0 if you want the driver to stay on page 0 all the
 * time.  This is faster than if it has to switch to page 1 to get the
 * current register to see if there are any more packets.  Reason: the
 * interrupt handler does not have to do any locking if it knows it is
 * always on page 0.  I'm not sure why the speed difference is so dramatic,
 * so perhaps there is actually a bug in page-changing version.
 *
 * STAY_ON_PAGE_0 uses a rather dubious technique to avoid changing the
 * register page and it may not be 100% reliable.  The technique used is to
 * make sure all ring headers are zeroed out before packets are received
 * into them.  Then, if you detect a non-zero ring header, you can be pretty
 * sure that it is another packet.
 *
 * We never read the "might-be-a-packet" immediately.  Instead, we just
 * release a semaphore so that the next read will occur later on enough
 * so that the ring header information should be completely filled in.
 *
 */
#define STAY_ON_PAGE_0	0


/*
 * We only care about these interrupts in our driver
 */
#define INTS_WE_CARE_ABOUT (ISR_RECEIVE | ISR_RECEIVE_ERROR |\
							ISR_TRANSMIT | ISR_TRANSMIT_ERROR | ISR_COUNTER)

typedef struct etherpci_private {
	int32			devID; 		/* device identifier: 0-n */
	pci_info		*pciInfo;
	uint16			irq;		/* IRQ line */
	uint32			reg_base;	/* hardware register base address */
	area_id			ioarea; 	/* Area used for MMIO of hostRegs */
	int boundary;			/* boundary register value (mirrored) */
	ether_address_t myaddr;	/* my ethernet address */
	unsigned nmulti;		/* number of multicast addresses */
	ether_address_t multi[MAX_MULTI];	/* multicast addresses */
	sem_id iolock;			/* ethercard io, except interrupt handler */
	int nonblocking;		/* non-blocking mode */
#if !STAY_ON_PAGE_0
	spinlock intrlock;		/* ethercard io, including interrupt handler */
#endif /* !STAY_ON_PAGE_0 */
	volatile int interrupted;   /* interrupted system call */
	sem_id inrw;			/* in read or write function */
	sem_id ilock;			/* waiting for input */
	sem_id olock;	 		/* waiting to output */

	/*
	 * Various statistics
	 */
	volatile int ints;	/* total number of interrupts */
	volatile int rints;	/* read interrupts */
	volatile int wints;	/* write interrupts */
	volatile int reads;	/* reads */
	volatile int writes;	/* writes */
	volatile int resets;	/* resets */
	volatile int rerrs;	/* read errors */
	volatile int werrs;	/* write errors */
	volatile int interrs;/* unknown interrupts */
	volatile int frame_errs;	/* frame alignment errors */
	volatile int crc_errs;	/* crc errors */
	volatile int frames_lost;/* frames lost due to buffer problems */

	/*
	 * Most recent values of the error statistics to detect any changes in them
	 */
	int rerrs_last;
	int werrs_last;
	int interrs_last;
	int frame_errs_last;
	int crc_errs_last;
	int frames_lost_last;

	/* stats from the hardware */
	int chip_rx_frame_errors;
	int chip_rx_crc_errors;
	int chip_rx_missed_errors;
	/*
	 * These values are set once and never looked at again.  They are
	 * almost as good as constants, but they differ for the various
	 * cards, so we can't set them now.
	 */
	int ETHER_BUF_START;
	int ETHER_BUF_SIZE;
	int EC_VMEM_PAGE;
	int EC_VMEM_NPAGES;
	int EC_RXBUF_START_PAGE;
	int EC_RXBUF_END_PAGE;
	int EC_RINGSTART;
	int EC_RINGSIZE;

	uint32 debug;

} etherpci_private_t;


#if __POWERPC__

#define ether_inb(device, offset)  			(*((volatile uint8*)(device->reg_base + (offset)))); __eieio()
#define ether_inw(device, offset)  			(*((volatile uint16*)(device->reg_base + (offset)))); __eieio()
#define ether_outb(device, offset, value)  	(*((volatile uint8 *)(device->reg_base + (offset))) = (value)); __eieio()
#define ether_outw(device, offset, value) 	(*((volatile uint16*)(device->reg_base + (offset))) = (value)); __eieio()

#else /* !PPC */

#define ether_outb(device, offset, value)	(*gPCIModInfo->write_io_8)((device->reg_base + (offset)), (value))
#define ether_outw(device, offset, value)	(*gPCIModInfo->write_io_16)((device->reg_base + (offset)), (value))
#define ether_inb(device, offset)			((*gPCIModInfo->read_io_8)(device->reg_base + (offset)))
#define ether_inw(device, offset)			((*gPCIModInfo->read_io_16)(device->reg_base + (offset)))

#endif

#if 0
#if __i386__

uint8  ether_inb(etherpci_private_t *device, uint32 offset) {
  	uint8 result;
	result = ((*gPCIModInfo->read_io_8)(device->reg_base + (offset)));
	ETHER_DEBUG(PCI_IO, device->debug, " inb(%x) %x \n", offset, result);
	return result;
};
uint16  ether_inw(etherpci_private_t *device, uint32 offset) {
  	uint16 result;
	result = ((*gPCIModInfo->read_io_16)(device->reg_base + (offset)));
	ETHER_DEBUG(PCI_IO, device->debug, " inw(%x) %x \n", offset, result);
	return result;
};
void  ether_outb(etherpci_private_t *device, uint32 offset, uint8 value) {
	(*gPCIModInfo->write_io_8)((device->reg_base + (offset)), (value));
	ETHER_DEBUG(PCI_IO, device->debug, " outb(%x) %x \n", offset, value);
};
void  ether_outw(etherpci_private_t *device, uint32 offset, uint16 value) {
	(*gPCIModInfo->write_io_16)((device->reg_base + (offset)), (value));
	ETHER_DEBUG(PCI_IO, device->debug, " outb(%x) %x \n", offset, value);
};


#else /* PPC */

uint8  ether_inb(etherpci_private_t *device, uint32 offset) {
  	uint8 result;
	result = (*((volatile uint8*) (device->reg_base + (offset)))); __eieio();
	ETHER_DEBUG(PCI_IO, device->debug, " inb(%x) %x \n", offset, result);
	return result;
};

uint16  ether_inw(etherpci_private_t *device, uint32 offset) {
  	uint16 result;
	result = (*((volatile uint16*) (device->reg_base + (offset)))); __eieio();
	ETHER_DEBUG(PCI_IO, device->debug, " inw(%x) %x \n", offset, result);
	return result;
};

void  ether_outb(etherpci_private_t *device, uint32 offset, uint8 value) {
	(*((volatile uint8 *)(device->reg_base + (offset))) = (value)); __eieio();
	ETHER_DEBUG(PCI_IO, device->debug, " outb(%x) %x \n", offset, value);
};
void  ether_outw(etherpci_private_t *device, uint32 offset, uint16 value) {
	(*((volatile uint16 *)(device->reg_base + (offset))) = (value)); __eieio();
	ETHER_DEBUG(PCI_IO, device->debug, " outb(%x) %x \n", offset, value);
};

#endif
#endif





/* for serial debug command*/
#define DEBUGGER_COMMAND true
#if DEBUGGER_COMMAND
etherpci_private_t * gdev;
static int etherpci(int argc, char **argv);
	/* serial debug command */
#endif

/*
 * io_lock gets you exclusive access to the card, except that
 * the interrupt handler can still run.
 * There is probably no need to io_lock() a 3com card, so look into
 * removing it for that case.
 */
#define io_lock(data)	acquire_sem(data->iolock)
#define io_unlock(data)	release_sem_etc(data->iolock, 1, B_DO_NOT_RESCHEDULE)

/*
 * output_wait wakes up when the card is ready to transmit another packet
 */
#define output_wait(data, t) acquire_sem_etc(data->olock, 1, B_TIMEOUT, t)
#define output_unwait(data, c)	release_sem_etc(data->olock, c, B_DO_NOT_RESCHEDULE)

/*
 * input_wait wakes up when the card has at least one packet on it
 */
#define input_wait(data)		acquire_sem_etc(data->ilock ,1, B_CAN_INTERRUPT, 0)
#define input_unwait(data, c)	release_sem_etc(data->ilock, c, B_DO_NOT_RESCHEDULE)


/* prototypes */
static status_t open_hook(const char *name, uint32 flags, void **cookie);
static status_t close_hook(void *);
static status_t free_hook(void *);
static status_t control_hook(void * cookie,uint32 msg,void *buf,size_t len);
static status_t read_hook(void *data, off_t pos, void *buf, size_t *len);
static status_t write_hook(void *data, off_t pos, const void *buf, size_t *len);
//static int32    etherpci_interrupt(void *data);                	   /* interrupt handler */
//static int32    get_pci_list(pci_info *info[], int32 maxEntries);  /* Get pci_info for each device */
//static status_t free_pci_list(pci_info *info[]);                   /* Free storage used by pci_info list */
//static void 	dump_packet(const char * msg, unsigned char * buf, uint16 size); /* diagnostic packet trace */
//static status_t enable_addressing(etherpci_private_t  *data);             /* enable pci io address space for device */
//static int domulti(etherpci_private_t *data,char *addr);

static device_hooks gDeviceHooks =  {
	open_hook, 			/* -> open entry point */
	close_hook,         /* -> close entry point */
	free_hook,          /* -> free entry point */
	control_hook,       /* -> control entry point */
	read_hook,          /* -> read entry point */
	write_hook,         /* -> write entry point */
	NULL,               /* -> select entry point */
	NULL,                /* -> deselect entry point */
	NULL,               /* -> readv */
	NULL                /* -> writev */
};


static int32
get_pci_list(pci_info *info[], int32 maxEntries)
{
	int32 i, entries;
	pci_info *item;

	item = (pci_info *)malloc(sizeof(pci_info));
	if (item == NULL)
		return 0;

	for (i = 0, entries = 0; entries < maxEntries; i++) {
		if (gPCIModInfo->get_nth_pci_info(i, item) != B_OK)
			break;

		if ((item->vendor_id == 0x10ec && item->device_id == 0x8029)		// RealTek 8029
			|| (item->vendor_id == 0x1106 && item->device_id == 0x0926)		// VIA
			|| (item->vendor_id == 0x4a14 && item->device_id == 0x5000)		// NetVin 5000
			|| (item->vendor_id == 0x1050 && item->device_id == 0x0940)		// ProLAN
			|| (item->vendor_id == 0x11f6 && item->device_id == 0x1401)		// Compex
			|| (item->vendor_id == 0x8e2e && item->device_id == 0x3000)) {	// KTI
			/* check if the device really has an IRQ */
			if (item->u.h0.interrupt_line == 0 || item->u.h0.interrupt_line == 0xFF) {
				dprintf(kDevName " found with invalid IRQ - check IRQ assignement");
				continue;
			}

			dprintf(kDevName " found at IRQ %x ", item->u.h0.interrupt_line);
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
			if (item == NULL)
				break;
		}
	}
	info[entries] = NULL;
	free(item);
	return entries;
}


static status_t
free_pci_list(pci_info *info[])
{
	pci_info *item;
	int32 i;

	for (i = 0; (item = info[i]) != NULL; i++) {
		free(item);
	}
	return B_OK;
}


#if 0
/*! How many waiting for io? */
static long
io_count(etherpci_private_t *data)
{
	long count;

	get_sem_count(data->iolock, &count);
	return (count);
}


/*! How many waiting for output? */
static long
output_count(etherpci_private_t *data)
{
	long count;

	get_sem_count(data->olock, &count);
	return (count);
}
#endif

/*
 * How many waiting for input?
 */
static int32
input_count(etherpci_private_t *data)
{
	int32 count;

	get_sem_count(data->ilock, &count);
	return (count);
}

#if STAY_ON_PAGE_0

#define INTR_LOCK(data, expression) (expression)

#else /* STAY_ON_PAGE_0 */

/*
 * Spinlock for negotiating access to card with interrupt handler
 */
#define intr_lock(data)			acquire_spinlock(&data->intrlock)
#define intr_unlock(data)		release_spinlock(&data->intrlock)

/*
 * The interrupt handler must lock all calls to the card
 * This macro is useful for that purpose.
 */
#define INTR_LOCK(data, expression) (intr_lock(data), (expression), intr_unlock(data))

#endif /* STAY_ON_PAGE_0 */


/*
 * Calculate various constants
 * These must be done at runtime, since 3com and ne2000 cards have different
 * values.
 */
static void
calc_constants(etherpci_private_t *data)
{
	data->EC_VMEM_PAGE = (data->ETHER_BUF_START >> EC_PAGE_SHIFT);
	data->EC_VMEM_NPAGES = (data->ETHER_BUF_SIZE >> EC_PAGE_SHIFT);

	data->EC_RXBUF_START_PAGE = (data->EC_VMEM_PAGE + 6);
	data->EC_RXBUF_END_PAGE = (data->EC_VMEM_PAGE + data->EC_VMEM_NPAGES);

	data->EC_RINGSTART = (data->EC_RXBUF_START_PAGE << EC_PAGE_SHIFT);
	data->EC_RINGSIZE = ((data->EC_VMEM_NPAGES - 6) << EC_PAGE_SHIFT);
}


/*! Print an ethernet address */
static void
print_address(ether_address_t *addr)
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	dprintf("%s\n", buf);
}


/*! Get the isr register */
static unsigned char
getisr(etherpci_private_t *data)
{
	return ether_inb(data, EN0_ISR);
}


/*! Set the isr register */
static void
setisr(etherpci_private_t *data, unsigned char isr)
{
	ether_outb(data, EN0_ISR, isr);
}


/*! Wait for the DMA to complete */
static int
wait_for_dma_complete(etherpci_private_t *data, unsigned short addr,
	unsigned short size)
{
	unsigned short hi, low;
	unsigned short where;
	int bogus;

#define MAXBOGUS 20

	/*
	 * This is the expected way to wait for DMA completion, which
	 * is in fact incorrect. I think ISR_DMADONE gets set too early.
	 */
	bogus = 0;
	while (!(getisr(data) & ISR_DMADONE) && ++bogus < MAXBOGUS) {
		/* keep waiting */
	}
	if (bogus >= MAXBOGUS)
		dprintf("Bogus alert: waiting for ISR\n");

	/*
	 * This is the workaround
	 */
	bogus = 0;
	do {
		hi = ether_inb(data, EN0_RADDRHI);
		low = ether_inb(data, EN0_RADDRLO);
		where = (hi << 8) | low;
	} while (where < addr + size && ++bogus < MAXBOGUS);

	if (bogus >= MAXBOGUS * 2) {
		/*
		 * On some cards, the counters will never clear.
		 * So only print this message when debugging.
		 */
		dprintf("Bogus alert: waiting for counters to zero\n");
		return -1;
	}

	setisr(data, ISR_DMADONE);
	ether_outb(data, EN_CCMD, ENC_NODMA);
	return 0;
}


/*! Check the status of the last packet transmitted */
static void
check_transmit_status(etherpci_private_t *data)
{
	unsigned char status;

	status = ether_inb(data, EN0_TPSR);
	if (status & (TSR_ABORTED | TSR_UNDERRUN)) {
		dprintf("transmit error: %02x\n", status);
	}
#if 0
	if (data->wints + data->werrs != data->writes) {
		dprintf("Write interrupts %d, errors %d, transmits %d\n",
				data->wints, data->werrs, data->writes);
	}
#endif
}


static long
inrw(etherpci_private_t *data)
{
	return data->inrw;
}


static status_t
create_sems(etherpci_private_t *data)
{
	data->iolock = create_sem(1, "ethercard io");
	if (data->iolock < B_OK)
		return data->iolock;

	data->olock = create_sem(1, "ethercard output");
	if (data->olock < B_OK) {
		delete_sem(data->iolock);
		return data->olock;
	}

	data->ilock = create_sem(0, "ethercard input");
	if (data->ilock < B_OK) {
		delete_sem(data->iolock);
		delete_sem(data->olock);
		return data->ilock;
	}

	return B_OK;
}


/*! Get data from the ne2000 card */
static void
etherpci_min(etherpci_private_t *data, unsigned char *dst,
	unsigned src, unsigned len)
{
	int i;

	if (len & 1)
		len++;

	ether_outb(data, EN0_RCNTLO, len & 0xff);
	ether_outb(data, EN0_RCNTHI, len >> 8);
	ether_outb(data, EN0_RADDRLO, src & 0xff);
	ether_outb(data, EN0_RADDRHI, src >> 8);
	ether_outb(data, EN_CCMD, ENC_DMAREAD);

	for (i = 0; i < len; i += 2) {
		unsigned short word;

		word = ether_inw(data, NE_DATA);
#if __i386__
		dst[i + 1] = word >> 8;
		dst[i + 0] = word & 0xff;
#else
		dst[i] = word >> 8;
		dst[i + 1] = word & 0xff;
#endif
	}
	wait_for_dma_complete(data, src, len);
}


/*! Put data on the ne2000 card */
static void
etherpci_mout(etherpci_private_t *data, unsigned dst,
	const unsigned char *src, unsigned len)
{
	int i;
	int tries = 1;

	// This loop is for a bug that showed up with the old ISA 3com cards
	// that were in the original BeBox.  Sometimes the dma status would just
	// stop on some part of the buffer, never finishing.
	// If we notice this error, we redo the dma transfer.
again:
#define MAXTRIES 2
	if (tries > MAXTRIES) {
		dprintf("ether_mout : tried %d times, stopping\n", tries);
		return;
	}

	if (len & 1)
		len++;

	ether_outb(data, EN0_RCNTHI, len >> 8);
	ether_outb(data, EN0_RADDRLO, dst & 0xff);
	ether_outb(data, EN0_RADDRHI, dst >> 8);

	/*
	 * The 8390 hardware has documented bugs in it related to DMA write.
	 * So, we follow the documentation on how to work around them.
	 */

	/*
	 * Step 1: You must put a non-zero value in this register. We use 2.
	 */
	if ((len & 0xff) == 0) {
		ether_outb(data, EN0_RCNTLO, 2);
	} else {
		ether_outb(data, EN0_RCNTLO, len & 0xff);
	}

#if you_want_to_follow_step2_even_though_it_hangs
	ether_outb(data, EN_CCMD, ENC_DMAREAD);				/* Step 2 */
#endif

	ether_outb(data, EN_CCMD, ENC_DMAWRITE);

	for (i = 0; i < len; i += 2) {
		unsigned short word;

#if __i386__
		word = (src[i + 1] << 8) | src[i + 0];
#else
		word = (src[i] << 8) | src[i + 1];
#endif
		ether_outw(data, NE_DATA, word);
	}

	if ((len & 0xff) == 0) {
		/*
		 * Write out the two extra bytes
		 */
		ether_outw(data, NE_DATA, 0);
		len += 2;
	}
	if (wait_for_dma_complete(data, dst, len) != 0) {
		tries++;
		goto again;
	}
	//if (tries != 1) { dprintf("wait_for_dma worked after %d tries\n", tries); }
}


#if STAY_ON_PAGE_0
/*! Zero out the headers in the ring buffer */
static void
ringzero(etherpci_private_t *data, unsigned boundary,
	unsigned next_boundary)
{
	ring_header ring;
	int i;
	int pages;
	unsigned offset;

	ring.count = 0;
	ring.next_packet = 0;
	ring.status = 0;

	if (data->boundary < next_boundary) {
		pages = next_boundary - data->boundary;
	} else {
		pages = (data->EC_RINGSIZE >> EC_PAGE_SHIFT) - (data->boundary - next_boundary);
	}

	for (i = 0; i < pages; i++) {
		offset = data->boundary << EC_PAGE_SHIFT;
		etherpci_mout(data, offset, (unsigned char *)&ring, sizeof(ring));
		data->boundary++;
		if (data->boundary >= data->EC_RXBUF_END_PAGE) {
			data->boundary = data->EC_RXBUF_START_PAGE;
		}
	}
}
#endif /* STAY_ON_PAGE_0 */


/*! Determine if we have an ne2000 PCI card */
static int
probe(etherpci_private_t *data)
{
	int i;
	int reg;
	unsigned char test[EC_PAGE_SIZE];
	short waddr[ETHER_ADDR_LEN];
	uint8 reg_val;

	data->ETHER_BUF_START = ETHER_BUF_START_NE2000;
	data->ETHER_BUF_SIZE = ETHER_BUF_SIZE_NE2000;
	calc_constants(data);

	reg = ether_inb(data, NE_RESET);
	snooze(2000);
	ether_outb(data, NE_RESET, reg);
	snooze(2000);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP | ENC_PAGE0);

	i = 10000;
	while ( i-- > 0) {
		reg_val = ether_inb(data, EN0_ISR);
		if (reg_val & ISR_RESET) break;
	}
	if (i < 0)
		dprintf("reset failed -- ignoring\n");

	ether_outb(data, EN0_ISR, 0xff);
	ether_outb(data, EN0_DCFG, DCFG_BM16);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP | ENC_PAGE0);

	snooze(2000);

	reg = ether_inb(data, EN_CCMD);
	if (reg != (ENC_NODMA|ENC_STOP|ENC_PAGE0)) {
		dprintf("command register failed: %02x != %02x\n", reg, ENC_NODMA|ENC_STOP);
		return 0;
	}

	ether_outb(data, EN0_TXCR, 0);
	ether_outb(data, EN0_RXCR, ENRXCR_MON);
	ether_outb(data, EN0_STARTPG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN0_STOPPG, data->EC_RXBUF_END_PAGE);
	ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE);
	ether_outb(data, EN0_IMR, 0);
	ether_outb(data, EN0_ISR, 0);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE1 | ENC_STOP);
	ether_outb(data, EN1_CURPAG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE0 | ENC_STOP);

	/* stop chip */
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE0);
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_STOP);

	memset(&waddr[0], 0, sizeof(waddr));
	etherpci_min(data, (unsigned char *)&waddr[0], 0, sizeof(waddr));

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		data->myaddr.ebyte[i] = ((unsigned char *)&waddr[0])[2*i];
	}

	/* test memory */
	for (i = 0; i < sizeof(test); i++) {
		test[i] = i;
	}

	etherpci_mout(data, data->ETHER_BUF_START, (unsigned char *)&test[0], sizeof(test));
	memset(&test, 0, sizeof(test));
	etherpci_min(data, (unsigned char *)&test[0], data->ETHER_BUF_START, sizeof(test));

	for (i = 0; i < sizeof(test); i++) {
		if (test[i] != i) {
			dprintf("memory test failed: %02x %02x\n", i, test[i]);
			return 0;
		}
	}

	dprintf("ne2000 pci ethernet card found - ");
	print_address(&data->myaddr);
	return 1;
}


/*! Initialize the ethernet card */
static void
init(etherpci_private_t *data)
{
	int i;

#if STAY_ON_PAGE_0
	/*
	 * Set all the ring headers to zero
	 */
	ringzero(data, data->EC_RXBUF_START_PAGE, data->EC_RXBUF_END_PAGE);
#endif /* STAY_ON_PAGE_0 */

	/* initialize data configuration register */
	ether_outb(data, EN0_DCFG, DCFG_BM16);

	/* clear remote byte count registers */
	ether_outb(data, EN0_RCNTLO, 0x0);
	ether_outb(data, EN0_RCNTHI, 0x0);

	/* initialize receive configuration register */
	ether_outb(data, EN0_RXCR, ENRXCR_BCST);

	/* get into loopback mode */
	ether_outb(data, EN0_TXCR, TXCR_LOOPBACK);

	/* set boundary, page start and page stop */
	ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE);
	data->boundary = data->EC_RXBUF_START_PAGE;
	ether_outb(data, EN0_STARTPG, data->EC_RXBUF_START_PAGE);
	ether_outb(data, EN0_STOPPG, data->EC_RXBUF_END_PAGE);

	/* set transmit page start register */
	ether_outb(data, EN0_TPSR, data->EC_VMEM_PAGE);

	/* clear interrupt status register */
	ether_outb(data, EN0_ISR, 0xff);

	/* initialize interrupt mask register */
	ether_outb(data, EN0_IMR, INTS_WE_CARE_ABOUT);

	/* set page 1 */
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_PAGE1);

	/* set physical address */
	for (i = 0; i < 6; i++) {
		ether_outb(data, EN1_PHYS + i, data->myaddr.ebyte[i]);
	}

	/* set multicast address */
	for (i = 0; i < 8; i++) {
		ether_outb(data, EN1_MULT+i, 0xff);
	}
	data->nmulti = 0;

	/* set current pointer */
	ether_outb(data, EN1_CURPAG, data->EC_RXBUF_START_PAGE);

	/* start chip */
	ether_outb(data, EN_CCMD, ENC_START | ENC_PAGE0 | ENC_NODMA);

	/* initialize transmit configuration register */
	ether_outb(data, EN0_TXCR, 0x00);
}


/*! Copy data from the card's ring buffer */
static void
ringcopy(etherpci_private_t *data, unsigned char *ether_buf,
	int offset, int len)
{
	int roffset;
	int rem;

	roffset = offset - data->EC_RINGSTART;
	rem = data->EC_RINGSIZE - roffset;
	if (len > rem) {
		etherpci_min(data, &ether_buf[0], offset, rem);
		etherpci_min(data, &ether_buf[rem], data->EC_RINGSTART, len - rem);
	} else {
		etherpci_min(data, &ether_buf[0], offset, len);
	}
}


/*!
	Set the boundary register, both on the card and internally
	NOTE: you cannot make the boundary = current register on the card,
	so we actually set it one behind.
*/
static void
setboundary(etherpci_private_t *data, unsigned char nextboundary)
{
	if (nextboundary != data->EC_RXBUF_START_PAGE) {
		ether_outb(data, EN0_BOUNDARY, nextboundary - 1);
	} else {
		/* since it's a ring buffer */
		ether_outb(data, EN0_BOUNDARY, data->EC_RXBUF_END_PAGE - 1);
	}
	data->boundary = nextboundary;
}


/*! Start resetting the chip, because of ring overflow */
static int
reset(etherpci_private_t *data)
{
	unsigned char cmd;
	int resend = false;

	cmd = ether_inb(data, EN_CCMD);
	ether_outb(data, EN_CCMD, ENC_STOP | ENC_NODMA);
	snooze(10 * 1000);
	ether_outb(data, EN0_RCNTLO, 0x0);
	ether_outb(data, EN0_RCNTHI, 0x0);
	if (cmd & ENC_TRANS) {
		if(!(getisr(data) & (ISR_TRANSMIT | ISR_TRANSMIT_ERROR)))
			resend = true;  // xmit command issued but ISR shows its not completed
	}
	/* get into loopback mode */
	ether_outb(data, EN0_TXCR, TXCR_LOOPBACK);
	ether_outb(data, EN_CCMD, ENC_START | ENC_PAGE0 | ENC_NODMA);
	return (resend);
}


/*! finish the reset */
static void
finish_reset(etherpci_private_t *data, int resend)
{
	setisr(data, ISR_OVERWRITE);
	ether_outb(data, EN0_TXCR, 0x00);

	if (resend) {
//		dprintf("Xmit CMD resent\n");
		ether_outb(data, EN_CCMD, ENC_START | ENC_PAGE0 | ENC_NODMA | ENC_TRANS);
	}
}


/*! Handle ethernet interrupts */
static int32
etherpci_interrupt(void *_data)
{
	etherpci_private_t *data = (etherpci_private_t *) _data;
	unsigned char isr;
	int wakeup_reader = 0;
	int wakeup_writer = 0;
	int32 handled = B_UNHANDLED_INTERRUPT;
	data->ints++;

	ETHER_DEBUG(INTERRUPT, data->debug, "ENTR isr=%x & %x?\n",getisr(data), INTS_WE_CARE_ABOUT);
	for (INTR_LOCK(data, isr = getisr(data));
			isr & INTS_WE_CARE_ABOUT;
			INTR_LOCK(data, isr = getisr(data))) {
		if (isr & ISR_RECEIVE) {
			data->rints++;
			wakeup_reader++;
			INTR_LOCK(data, setisr(data, ISR_RECEIVE));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_TRANSMIT_ERROR) {
			data->werrs++;
			INTR_LOCK(data, setisr(data, ISR_TRANSMIT_ERROR));
			wakeup_writer++;
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_TRANSMIT) {
			data->wints++;
			INTR_LOCK(data, setisr(data, ISR_TRANSMIT));
			wakeup_writer++;
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (isr & ISR_RECEIVE_ERROR) {
			uint32 err_count;
			err_count = ether_inb(data, EN0_CNTR0);  data->frame_errs += err_count;
			err_count = ether_inb(data, EN0_CNTR1);  data->crc_errs += err_count;
			err_count = ether_inb(data, EN0_CNTR2);  data->frames_lost += err_count;

			isr &= ~ISR_RECEIVE_ERROR;
			INTR_LOCK(data, setisr(data, ISR_RECEIVE_ERROR));
			handled = B_HANDLED_INTERRUPT;
		}
		if (isr & ISR_DMADONE) {
			isr &= ~ISR_DMADONE;	/* handled elsewhere */
			handled = B_HANDLED_INTERRUPT;
		}
		if (isr & ISR_OVERWRITE) {
			isr &= ~ISR_OVERWRITE; /* handled elsewhere */
			handled = B_HANDLED_INTERRUPT;
		}

		if (isr & ISR_COUNTER) {
			isr &= ~ISR_COUNTER; /* handled here */
//			dprintf("Clearing Stats Cntr\n");
			INTR_LOCK(data, setisr(data, ISR_COUNTER));
			handled = B_HANDLED_INTERRUPT;
		}

		if (isr) {
			/*
			 * If any other interrupts, just clear them (hmmm....)
			 * ??? This doesn't seem right - HB
			 */
//			ddprintf("ISR=%x rdr=%x wtr=%x io=%x inrw=%x nonblk=%x\n",
//				isr,input_count(data), output_count(data),io_count(data),
//				data->inrw,data->nonblocking);
			INTR_LOCK(data, setisr(data, isr));
			data->interrs++;
		}
	}
	if (wakeup_reader) {
		input_unwait(data, 1);
	}
	if (wakeup_writer) {
		output_unwait(data, 1);
	}

	return handled;
}


/*! Check to see if there are any new errors */
static void
check_errors(etherpci_private_t *data)
{
#define DOIT(stat, message) \
	if (stat > stat##_last) { \
		stat##_last = stat; \
		dprintf(message, stat##_last); \
	}

	DOIT(data->rerrs, "Receive errors now %d\n");
	DOIT(data->werrs, "Transmit errors now %d\n");
	DOIT(data->interrs, "Interrupt errors now %d\n");
	DOIT(data->frames_lost, "Frames lost now %d\n");
#undef DOIT
#if 0
	/*
	 * these are normal errors because collisions are normal
	 * so don't make a big deal about them.
	 */
	DOIT(data->frame_errs, "Frame alignment errors now %d\n");
	DOIT(data->crc_errs, "CRC errors now %d\n");
#endif
}


/*! Find out if there are any more packets on the card */
static int
more_packets(etherpci_private_t *data, int didreset)
{
#if STAY_ON_PAGE_0
	unsigned offset;
	ring_header ring;

	offset = data->boundary << EC_PAGE_SHIFT;
	etherpci_min(data, (unsigned char *)&ring, offset, sizeof(ring));
	return ring.status && ring.next_packet && ring.count;

#else /* STAY_ON_PAGE_0 */

	cpu_status ps;
	unsigned char cur;

	/*
	 * First, get the current registe
	 */
	ps = disable_interrupts();
	intr_lock(data);
	ether_outb(data, EN_CCMD, ENC_PAGE1);
	cur = ether_inb(data, EN1_CURPAG);
	ether_outb(data, EN_CCMD, ENC_PAGE0);
	intr_unlock(data);
	restore_interrupts(ps);

	/*
	 * Then return the result
	 * Must use didreset since cur == boundary in
	 * an overflow situation.
	 */
	return didreset || cur != data->boundary;
#endif	/* STAY_ON_PAGE_0 */
}


/*! Copy a packet from the ethernet card */
static int
copy_packet(etherpci_private_t *data, unsigned char *ether_buf,
	int buflen)
{
	ring_header ring;
	unsigned offset;
	int len;
	int rlen;
	int ether_len = 0;
	int didreset = 0;
	int resend = 0;

	io_lock(data);
	check_errors(data);

	/*
	 * Check for overwrite error first
	 */
	if (getisr(data) & ISR_OVERWRITE) {
//		dprintf("starting ether reset!\n");
		data->resets++;
		resend = reset(data);
		didreset++;
	}

	if (more_packets(data, didreset)) do {
		/*
		 * Read packet ring header
		 */
		offset = data->boundary << EC_PAGE_SHIFT;
		etherpci_min(data, (unsigned char *)&ring, offset, sizeof(ring));

		len = swapshort(ring.count);

		if (!(ring.status & RSR_INTACT)) {
			dprintf("packet not intact! (%02x,%u,%02x) (%d)\n",
				ring.status, ring.next_packet, ring.count, data->boundary);

			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}

		if (ring.next_packet < data->EC_RXBUF_START_PAGE
			|| ring.next_packet >= data->EC_RXBUF_END_PAGE) {
			dprintf("etherpci_read: bad next packet! (%02x,%u,%02x) (%d)\n",
				ring.status, ring.next_packet, ring.count, data->boundary);

			data->rerrs++;
			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}

		len = swapshort(ring.count);
		rlen = len - 4;
		if (rlen < ETHER_MIN_SIZE || rlen > ETHER_MAX_SIZE) {
			dprintf("etherpci_read: bad length! (%02x,%u,%02x) (%d)\n",
				ring.status, ring.next_packet, ring.count, data->boundary);

			data->rerrs++;
			/* discard bad packet */
			ether_len = 0; setboundary(data, ring.next_packet);
			break;
		}

		if (rlen > buflen)
			rlen = buflen;

		ringcopy(data, ether_buf, offset + 4, rlen);

#if STAY_ON_PAGE_0
		ringzero(data, data->boundary, ring.next_packet);
#endif /* STAY_ON_PAGE_0 */

		ether_len = rlen;
		setboundary(data, ring.next_packet);
		data->reads++;
	} while (0);

	if (didreset) {
		dprintf("finishing reset!\n");
		finish_reset(data, resend);
	}

	if (input_count(data) <= 0 && more_packets(data, didreset)) {
		/*
		 * Looks like there is another packet
		 * So, make sure they get woken up
		 */
		input_unwait(data, 1);
	}

	io_unlock(data);
	return ether_len;
}


/*! Checks if the received packet is really for us */
static int
my_packet(etherpci_private_t *data, char *addr)
{
	int i;
	const char broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (memcmp(addr, &data->myaddr, sizeof(data->myaddr)) == 0
		|| memcmp(addr, broadcast, sizeof(broadcast)) == 0)
		return 1;

	for (i = 0; i < data->nmulti; i++) {
		if (memcmp(addr, &data->multi[i], sizeof(data->multi[i])) == 0)
			return 1;
	}

	return 0;
}


static status_t
enable_addressing(etherpci_private_t *data)
{
	unsigned char cmd;

#if __i386__
	data->reg_base = data->pciInfo->u.h0.base_registers[0];
#else
	uint32 base, size, offset;
	base = data->pciInfo->u.h0.base_registers[0];
	size = data->pciInfo->u.h0.base_register_sizes[0];

	/* Round down to nearest page boundary */
	base = base & ~(B_PAGE_SIZE-1);

	/* Adjust the size */
	offset = data->pciInfo->u.h0.base_registers[0] - base;
	size += offset;
	size = (size +(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);

	dprintf(kDevName ": PCI base=%x size=%x offset=%x\n", base, size, offset);

	if ((data->ioarea = map_physical_memory(kDevName "_regs", base, size,
			B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
			(void **)&data->reg_base)) < 0) {
		return B_ERROR;
	}

	data->reg_base = data->reg_base + offset;

#endif
	dprintf(kDevName ": reg_base=%" B_PRIx32 "\n", data->reg_base);

	/* enable pci address access */
	cmd = (gPCIModInfo->read_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, PCI_command, 2);
	(gPCIModInfo->write_pci_config)(data->pciInfo->bus, data->pciInfo->device, data->pciInfo->function, PCI_command, 2, cmd | PCI_command_io);

	return B_OK;
}


static int
domulti(etherpci_private_t *data, char *addr)
{
	int i;
	int nmulti = data->nmulti;

	if (nmulti == MAX_MULTI)
		return B_ERROR;

	for (i = 0; i < nmulti; i++) {
		if (memcmp(&data->multi[i], addr, sizeof(data->multi[i])) == 0) {
			break;
		}
	}

	if (i == nmulti) {
		/*
		 * Only copy if it isn't there already
		 */
		memcpy(&data->multi[i], addr, sizeof(data->multi[i]));
		data->nmulti++;
	}
	if (data->nmulti == 1) {
		dprintf("Enabling multicast\n");
		ether_outb(data, EN0_RXCR, ENRXCR_BCST | ENRXCR_MCST);
	}

	return B_NO_ERROR;
}


/*!
	Serial Debugger command
	Connect a terminal emulator to the serial port at 19.2 8-1-None
	Press the keys ( alt-sysreq on Intel) or (Clover-leaf Power on Mac ) to enter the debugger
	At the kdebug> prompt enter "etherpci arg...",
	for example "etherpci R" to enable a received packet trace.
*/
#if DEBUGGER_COMMAND
static int
etherpci(int argc, char **argv) {
	uint16 i,j;
	const char * usage = "usage: etherpci { Function_calls | PCI_IO | Stats | Rx_trace | Tx_trace }\n";


	if (argc < 2) {
		kprintf("%s",usage);	return 0;
	}

	for (i= argc, j= 1; i > 1; i--, j++) {
		switch (*argv[j]) {
		case 'F':
		case 'f':
			gdev->debug ^= FUNCTION;
			if (gdev->debug & FUNCTION)
				kprintf("Function() call trace Enabled\n");
			else
				kprintf("Function() call trace Disabled\n");
			break;
		case 'N':
		case 'n':
			gdev->debug ^= SEQ;
			if (gdev->debug & SEQ)
				kprintf("Sequence numbers packet trace Enabled\n");
			else
				kprintf("Sequence numbers packet trace Disabled\n");
			break;
		case 'R':
		case 'r':
			gdev->debug ^= RX;
			if (gdev->debug & RX)
				kprintf("Receive packet trace Enabled\n");
			else
				kprintf("Receive packet trace Disabled\n");
			break;
		case 'T':
		case 't':
			gdev->debug ^= TX;
			if (gdev->debug & TX)
				kprintf("Transmit packet trace Enabled\n");
			else
				kprintf("Transmit packet trace Disabled\n");
			break;
		case 'S':
		case 's':
			kprintf(kDevName " statistics\n");
			kprintf("rx_ints %d,  tx_ints %d\n", gdev->rints, gdev->wints);
			kprintf("resets %d \n", gdev->resets);
			kprintf("crc_errs %d, frame_errs %d, frames_lost %d\n", gdev->crc_errs, gdev->frame_errs, gdev->frames_lost);
			break;
		case 'P':
		case 'p':
			gdev->debug ^= PCI_IO;
			if (gdev->debug & PCI_IO)
				kprintf("PCI IO trace Enabled\n");
			else
				kprintf("PCI IO trace Disabled\n");
			break;
		default:
			kprintf("%s",usage);
			return 0;
		}
	}

	return 0;
}
#endif /* DEBUGGER_COMMAND */


static void
dump_packet(const char * msg, unsigned char * buf, uint16 size)
{
	uint16 j;

	dprintf("%s dumping %p size %u \n", msg, buf, size);
	for (j = 0; j < size; j++) {
		if ((j & 0xF) == 0)
			dprintf("\n");
		dprintf("%2.2x ", buf[j]);
	}
}


//	#pragma mark - Driver Entry Points


status_t
init_hardware(void)
{
	return B_NO_ERROR;
}


status_t
init_driver(void)
{
	status_t status;
	int32 entries;
	char devName[64];
	int32 i;

	dprintf(kDevName ": init_driver ");

	if ((status = get_module( B_PCI_MODULE_NAME, (module_info **)&gPCIModInfo )) != B_OK) {
		dprintf(kDevName " Get module failed! %s\n", strerror(status ));
		return status;
	}

	/* Find Lan cards*/
	if ((entries = get_pci_list(gDevList, MAX_CARDS )) == 0) {
		dprintf("init_driver: " kDevName " not found\n");
		free_pci_list(gDevList);
		put_module(B_PCI_MODULE_NAME );
		return B_ERROR;
	}
	dprintf("\n");

	/* Create device name list*/
	for (i=0; i<entries; i++ )
	{
		sprintf(devName, "%s%" B_PRId32, kDevDir, i );
		gDevNameList[i] = (char *)malloc(strlen(devName)+1);
		strcpy(gDevNameList[i], devName);
	}
	gDevNameList[i] = NULL;

	return B_OK;
}


void
uninit_driver(void)
{
	void *item;
	int32 i;

	/*Free device name list*/
	for (i = 0; (item = gDevNameList[i]) != NULL; i++) {
		free(item);
	}

	/*Free device list*/
	free_pci_list(gDevList);
	put_module(B_PCI_MODULE_NAME);
}


device_hooks *
find_device(const char *name)
{
	int32 i;
	char *item;

	/* Find device name */
	for (i = 0; (item = gDevNameList[i]) != NULL; i++) {
		if (!strcmp(name, item))
			return &gDeviceHooks;
	}

	return NULL;
}


const char**
publish_devices(void)
{
	dprintf(kDevName ": publish_devices()\n");
	return (const char **)gDevNameList;
}


//	#pragma mark Device Hooks


/*! Implements the read() system call to the ethernet driver */
static status_t
read_hook(void *_data, off_t pos, void *buf, size_t *len)
{
	etherpci_private_t *data = (etherpci_private_t *) _data;
	ulong buflen;
	int packet_len;

	buflen = *len;
	atomic_add(&data->inrw, 1);
	if (data->interrupted) {
		atomic_add(&data->inrw, -1);
		return B_INTERRUPTED;
	}

	do {
		if (!data->nonblocking) {
			input_wait(data);
		}
		if (data->interrupted) {
			atomic_add(&data->inrw, -1);
			return B_INTERRUPTED;
		}
		packet_len = copy_packet(data, (unsigned char *)buf, buflen);
		if ((packet_len) && (data->debug & RX)) {
			dump_packet("RX:" ,buf, packet_len);
		}
	} while (!data->nonblocking && packet_len == 0 && !my_packet(data, buf));

	atomic_add(&data->inrw, -1);
	*len = packet_len;
	return 0;
}


static status_t
open_hook(const char *name, uint32 flags, void **cookie)
{
	int32 devID;
	int32 mask;
	status_t status;
	char *devName;
	etherpci_private_t *data;

	/*	Find device name*/
	for (devID = 0; (devName = gDevNameList[devID]); devID++) {
		if (strcmp(name, devName) == 0)
			break;
	}
	if (!devName)
		return EINVAL;

	/* Check if the device is busy and set in-use flag if not */
	mask = 1 << devID;
	if (atomic_or(&gOpenMask, mask) &mask)
		return B_BUSY;

	/*	Allocate storage for the cookie*/
	if (!(*cookie = data = (etherpci_private_t *)malloc(sizeof(etherpci_private_t)))) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(data, 0, sizeof(etherpci_private_t));

	/* Setup the cookie */
	data->pciInfo = gDevList[devID];
	data->devID = devID;
	data->interrupted = 0;
	data->inrw = 0;
	data->nonblocking = 0;

	data->debug = DEFAULT_DEBUG_FLAGS;
	ETHER_DEBUG(FUNCTION, data->debug, kDevName ": open %s dev=%p\n", name, data);

#if DEBUGGER_COMMAND
	gdev = data;
	add_debugger_command (kDevName, etherpci, "Ethernet driver Info");
#endif

	/* enable access to the cards address space */
	if ((status = enable_addressing(data)) != B_OK)
		goto err1;

	if (!probe(data)) {
		dprintf(kDevName ": probe failed\n");
		goto err1;
	}

	if (create_sems(data) != B_OK)
		goto err2;

	/* Setup interrupts */
	install_io_interrupt_handler( data->pciInfo->u.h0.interrupt_line, etherpci_interrupt, *cookie, 0 );

	dprintf("Interrupts installed at %x\n", data->pciInfo->u.h0.interrupt_line);
	/* Init Device */
	init(data);

	return B_NO_ERROR;

err2:
#if !__i386__
	delete_area(data->ioarea);
#endif
err1:
#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, etherpci);
#endif
	free(data);

err0:
	atomic_and(&gOpenMask, ~mask);
	dprintf(kDevName ": open failed!\n");
	return B_ERROR;
}


static status_t
close_hook(void *_data)
{
	etherpci_private_t *data = (etherpci_private_t *)_data;
	ETHER_DEBUG(FUNCTION, data->debug, kDevName ": close dev=%p\n", data);

	/*
	 * Force pending reads and writes to terminate
	 */
	io_lock(data);
	data->interrupted = 1;
	input_unwait(data, 1);
	output_unwait(data, 1);
	io_unlock(data);
	while (inrw(data)) {
		snooze(1000000);
		dprintf("ether: still waiting for read/write to finish\n");
	}

	/*
	 * Stop the chip
	 */
	ether_outb(data, EN_CCMD, ENC_STOP);
	snooze(2000);

	/*
	 * And clean up
	 */
	remove_io_interrupt_handler(data->pciInfo->u.h0.interrupt_line, etherpci_interrupt, data);
	delete_sem(data->iolock);
	delete_sem(data->ilock);
	delete_sem(data->olock);

	/*
	 * Reset all the statistics
	 */
	data->ints = 0;
	data->rints = 0;
	data->rerrs = 0;
	data->wints = 0;
	data->werrs = 0;
	data->reads = 0;
	data->writes = 0;
	data->interrs = 0;
	data->resets = 0;
	data->frame_errs = 0;
	data->crc_errs = 0;
	data->frames_lost = 0;

	data->rerrs_last = 0;
	data->werrs_last = 0;
	data->interrs_last = 0;
	data->frame_errs_last = 0;
	data->crc_errs_last = 0;
	data->frames_lost_last = 0;

	data->chip_rx_frame_errors = 0;
	data->chip_rx_crc_errors = 0;
	data->chip_rx_missed_errors = 0;

#if DEBUGGER_COMMAND
	remove_debugger_command (kDevName, etherpci);
#endif

	return B_OK;
}


static status_t
free_hook(void *_data)
{
	etherpci_private_t *data = (etherpci_private_t *)_data;
	int32 mask;

	ETHER_DEBUG(FUNCTION, data->debug, kDevName ": free dev=%p\n", data);

#if !__i386__
	delete_area(data->ioarea);
#endif

	// make sure the device can be reopened again
	mask = 1L << data->devID;
	atomic_and(&gOpenMask, ~mask);

	free(data);
	return B_OK;
}


static status_t
write_hook(void *_data, off_t pos, const void *buf, size_t *len)
{
	etherpci_private_t *data = (etherpci_private_t *) _data;
	ulong buflen;
	int status;

	buflen = *len;
	atomic_add(&data->inrw, 1);
	if (data->interrupted) {
		atomic_add(&data->inrw, -1);
		return B_INTERRUPTED;
	}
	/*
	 * Wait for somebody else (if any) to finish transmitting
	 */
	status = output_wait(data, ETHER_TRANSMIT_TIMEOUT);
	if (status < B_NO_ERROR || data->interrupted) {
		atomic_add(&data->inrw, -1);
		return status;
	}

	io_lock(data);
	check_errors(data);

	if (data->writes > 0)
		check_transmit_status(data);

	etherpci_mout(data, data->ETHER_BUF_START, (const unsigned char *)buf, buflen);
	if (buflen < ETHER_MIN_SIZE) {
		/*
		 * Round up to ETHER_MIN_SIZE
		 */
		buflen = ETHER_MIN_SIZE;
	}
	ether_outb(data, EN0_TCNTLO, (char)(buflen & 0xff));
	ether_outb(data, EN0_TCNTHI, (char)(buflen >> 8));
	ether_outb(data, EN_CCMD, ENC_NODMA | ENC_TRANS);
	data->writes++;
	io_unlock(data);
	atomic_add(&data->inrw, -1);
	*len = buflen;

	if (data->debug & TX)
		dump_packet("TX:",(unsigned char *) buf, buflen);

	return 0;
}


/*! Standard driver control function */
static status_t
control_hook(void *_data, uint32 msg, void *buf, size_t len)
{
	etherpci_private_t *data = (etherpci_private_t *) _data;

	switch (msg) {
		case ETHER_INIT:
			ETHER_DEBUG(FUNCTION, data->debug, kDevName ": control: ETHER_INIT \n");
			return B_OK;

		case ETHER_GETADDR:
			if (data == NULL)
				return B_ERROR;

			ETHER_DEBUG(FUNCTION, data->debug, kDevName ": control: GET_ADDR \n");
			memcpy(buf, &data->myaddr, sizeof(data->myaddr));
			return B_OK;

		case ETHER_NONBLOCK:
			if (data == NULL)
				return B_ERROR;

			memcpy(&data->nonblocking, buf, sizeof(data->nonblocking));
			ETHER_DEBUG(FUNCTION, data->debug, kDevName ": control: NON_BLOCK %x\n", data->nonblocking);
			return B_OK;

		case ETHER_ADDMULTI:
			ETHER_DEBUG(FUNCTION, data->debug, kDevName ": control: DO_MULTI\n");
			return domulti(data, (char *)buf);
	}

	return B_ERROR;
}

