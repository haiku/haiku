/*
 * Copyright 2006, Hideyuki Abe. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//! Ethernet Driver for VMware PCnet/PCI virtual network controller

#include "vlance.h"

#include <ether_driver.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* debug flag definitions */
#define ERR       0x0001
#define INFO      0x0002
#define RX        0x0004		/* dump received frames */
#define TX        0x0008		/* dump transmitted frames */
#define INTERRUPT 0x0010		/* interrupt calls */
#define FUNCTION  0x0020		/* function calls */
#define PCI_IO    0x0040		/* pci reads and writes */
#define SEQ		  0x0080		/* trasnmit & receive TCP/IP sequence sequence numbers */
#define WARN	  0x0100		/* Warnings - off on final release */

/* debug flag */
//#define DEBUG_FLG	( ERR | INFO | WARN )
//#define DEBUG_FLG	( ERR | INFO | INTERRUPT | FUNCTION | WARN )
#define DEBUG_FLG	( ERR | WARN )

#define	DEBUG(x)	(DEBUG_FLG & (x))


/* PCI vendor and device ID's */
#define VENDOR_ID        0x1022		/* AMD */
#define DEVICE_ID        0x2000		/* PCnet/PCI */


#define DEVICE_NAME "vlance"
#define DEVICE_NAME_LEN		64

#define MAX_CARDS			4		/* maximum number of driver instances */

#define BUFFER_SIZE			2048L	/* B_PAGE_SIZE divided into even amounts that will hold a 1518 frame */
#define MAX_FRAME_SIZE		1514	/* 1514 + 4 bytes checksum */

/* ring buffer sizes  */
#define	TX_BUFF_IDX			(4)
#define	RX_BUFF_IDX			(5)
#define TX_BUFFERS			(1 << TX_BUFF_IDX)
#define RX_BUFFERS			(1 << RX_BUFF_IDX)		/* Must be a power of 2 */

/* max number of multicast address */
#define	MAX_MULTI			(4)


/*
 * 6-octet MAC address
 */
typedef struct {
	uint8 ch[6];
	uint8 rsv[2];
} mac_addr_t;


/* driver intance definition */
typedef struct {
	mac_addr_t		mac_addr;				/* MAC address */
	int32			devID; 					/* device identifier */
	pci_info		*devInfo;				/* device information */
	uint16			irq;					/* our IRQ line */
	sem_id			ilock, olock;			/* I/O semaphores */
	int32			readLock, writeLock;	/* reentrant read/write lock */
	int32			blockFlg;				/* for blocking (0) or nonblocking (!=0) read */
	init_block_t	init_blk;				/* Initialization Block */
	uint32			phys_init_blk;			/* Initialization Block physical address */
	area_id			tx_desc_area; 			/* transmit descriptor area */
	area_id			tx_buf_area; 			/* transmit buffer area */
	area_id			rx_desc_area; 			/* receive descriptor area */
	area_id			rx_buf_area; 			/* receive buffer area */
	uchar			*tx_buf[TX_BUFFERS];	/* tx buffers */
	uchar			*rx_buf[RX_BUFFERS];	/* rx buffers */
	trns_desc_t		*tx_desc[TX_BUFFERS];	/* tx frame descriptors */
	recv_desc_t		*rx_desc[RX_BUFFERS];	/* rx frame descriptors */
	uint32			phys_tx_buf;			/* tx buffer physical address */
	uint32			phys_rx_buf;			/* rx buffer physical address */
	uint32			phys_tx_desc;			/* tx descriptor physical address */
	uint32			phys_rx_desc;			/* rx descriptor physical address */
	int16			tx_sent, tx_acked;		/* in & out index to tx buffers */
	int16			rx_received, rx_acked;	/* in & out index to rx buffers */
	int32			nmulti;					/* number of multicast address */
	mac_addr_t		multi[MAX_MULTI];		/* multicast address */
	uint32			reg_base; 				/* base address of PCI regs */
} dev_info_t;


/* function prototypes */
static inline uint32	csr_read(dev_info_t *device, uint32 reg_num);
static inline void		csr_write(dev_info_t *device, uint32 reg_num, uint32 data);
static inline uint32	bcr_read(dev_info_t *device, uint32 reg_num);
static inline void		bcr_write(dev_info_t *device, uint32 reg_num, uint32 data);
static status_t vlance_open(const char *name, uint32 flags, void **_cookie);
static status_t vlance_close(void *_device);
static status_t vlance_free(void *_device);
static status_t vlance_control(void *cookie, uint32 msg, void *buf, size_t len);
static status_t vlance_read(void *_device, off_t pos, void *buf, size_t *len);
static status_t vlance_write(void *_device, off_t pos, const void *buf, size_t *len);
static int32    vlance_interrupt(void *_device);			/* interrupt handler */
static int32	get_card_info(pci_info *info[]);			/* get pci_info for each device */
static status_t	free_card_info(pci_info *info[]);			/* free pci_info list */
static status_t	map_pci_addr(dev_info_t *device);			/* enable PCI I/O address space */
static status_t	alloc_buffers(dev_info_t *device);			/* allocate frame buffers */
static status_t	init_buffers(dev_info_t *device);			/* initialize frame buffers */
static void		free_buffers(dev_info_t *device);			/* free frame buffers */
static void		get_mac_addr(dev_info_t *device);			/* get MAC address */
static status_t	setpromisc(dev_info_t *device);				/* set promisc mode */
static status_t	domulti(dev_info_t *device, uint8 *addr);	/* add multicast address */
static void		reset_device(dev_info_t *device);			/* reset LAN controller hardware */
static status_t	alloc_resources(dev_info_t *device);		/* allocate semaphores */
static void		free_resources(dev_info_t *device);			/* deallocate semaphores */


static device_hooks DeviceHooks = {
	vlance_open,		/* open entry point */
	vlance_close,		/* close entry point */
	vlance_free,		/* free entry point */
	vlance_control,		/* control entry point */
	vlance_read,		/* read entry point */
	vlance_write,		/* write entry point */
	NULL,				/* select entry point */
	NULL,				/* deselect entry point */
	NULL,				/* readv */
	NULL				/* writev */
};


static pci_module_info		*PCIInfo;
static uint32				NumOfCards;
static char 				*DevName[MAX_CARDS + 1];	/* NULL-terminated */
static pci_info 			*CardInfo[MAX_CARDS];
static int32				OpenLock[MAX_CARDS];


#define write8(addr, val)			(*PCIInfo->write_io_8)((addr), (val))
#define write16(addr, val)			(*PCIInfo->write_io_16)((addr), (val))
#define write32(addr, val)			(*PCIInfo->write_io_32)((addr), (val))
#define read8(addr)					((*PCIInfo->read_io_8)(addr))
#define read16(addr)				((*PCIInfo->read_io_16)(addr))
#define read32(addr)				((*PCIInfo->read_io_32)(addr))


/* export Driver Entry Points */
__declspec(dllexport) status_t init_hardware(void);
__declspec(dllexport) status_t init_driver(void);
__declspec(dllexport) void uninit_driver(void);
__declspec(dllexport) const char **publish_devices(void);
__declspec(dllexport) device_hooks *find_device(const char *name);


/*
 * Driver Entry Points
 */
status_t init_hardware(void) {

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": init hardware\n");
#endif
	return B_OK;
}


status_t init_driver() {
	status_t 	status;
	char		devName[DEVICE_NAME_LEN];
	int32		i;
	uint32		mgc_num;

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": init_driver\n");
#endif

	/* check VMware */
	asm volatile (
		"movl	$0x564d5868, %%eax; "
		"movl	$0, %%ebx; "
		"movw	$0x000a, %%cx; "
		"movw	$0x5658, %%dx; "
		"inl	%%dx, %%eax"
		: "=b"(mgc_num)
		:
		: "%eax", "%ecx", "%edx"
	);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": VMware magic number %x\n", mgc_num);
#endif
	if(!(mgc_num == 0x564d5868)) return B_ERROR;


	status = get_module(B_PCI_MODULE_NAME, (module_info **)&PCIInfo);
	if(status != B_OK) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " Get module failed! %s\n", strerror(status ));
#endif
		return status;
	}

	/* find LAN cards */
	NumOfCards = get_card_info(CardInfo);
	if(NumOfCards == 0) {
#if DEBUG(ERR)
		dprintf("init_driver: " DEVICE_NAME " not found\n");
#endif
		free_card_info(CardInfo);
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	/* create device name list*/
	for(i = 0; i < NumOfCards; i++) {
		sprintf(devName, "net/%s/%ld", DEVICE_NAME, i);
		DevName[i] = (char *)malloc(DEVICE_NAME_LEN);
		strcpy(DevName[i], devName);
	}
	DevName[NumOfCards] = NULL;

	/* init open locks */
	for(i = 0; i < NumOfCards; i++) OpenLock[i] = 0;

	return B_OK;
}

void uninit_driver(void) {
	int32 	i;

	/* free device name list*/
	for(i = 0; i < NumOfCards; i++) {
		free(DevName[i]);
	}

	/* free device list*/
	free_card_info(CardInfo);
	put_module(B_PCI_MODULE_NAME);
}


const char **publish_devices(void) {
#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": publish_devices()\n" );
#endif
	return (const char **)DevName;
}


device_hooks *find_device(const char *name) {
	int32 	i;

	/* find device name */
	for(i = 0; i < NumOfCards; i++) {
		if(strcmp(name, DevName[i]) == 0) {
			return &DeviceHooks;
		}
	}
	return NULL;	/* not found */
}


static inline uint32 csr_read(dev_info_t *device, uint32 reg_num) {
	write32(device->reg_base + PCNET_RAP_OFFSET, reg_num);
	return read32(device->reg_base + PCNET_RDP_OFFSET);
}


static inline void csr_write(dev_info_t *device, uint32 reg_num, uint32 data) {
	write32(device->reg_base + PCNET_RAP_OFFSET, reg_num);
	write32(device->reg_base + PCNET_RDP_OFFSET, data);
}


static inline uint32 bcr_read(dev_info_t *device, uint32 reg_num) {
	write32(device->reg_base + PCNET_RAP_OFFSET, reg_num);
	return read32(device->reg_base + PCNET_BDP_OFFSET);
}


static inline void bcr_write(dev_info_t *device, uint32 reg_num, uint32 data) {
	write32(device->reg_base + PCNET_RAP_OFFSET, reg_num);
	write32(device->reg_base + PCNET_BDP_OFFSET, data);
}


static status_t vlance_open(const char *name, uint32 flags, void **cookie) {
	int32				devID;
	status_t			status;
	dev_info_t			*device;

	/*	find device name */
	for(devID = 0; devID < NumOfCards; devID++) {
		if(strcmp(name, DevName[devID]) == 0) break;
	}
	if(devID >= NumOfCards) return EINVAL;

	/* check if the device is busy and set in-use flag if not */
	if(atomic_or(&(OpenLock[devID]), 1)) {
		return B_BUSY;
	}

	/* allocate storage for the cookie */
	*cookie = device = (dev_info_t *)malloc(sizeof(dev_info_t));
	if(device == NULL) {
		status = B_NO_MEMORY;
		goto err0;
	}
	memset(device, 0, sizeof(dev_info_t));

	/* setup the cookie */
	device->devInfo = CardInfo[devID];
	device->devID = devID;

#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ": open %s device=%x\n", name, device);
#endif

	/* enable access to the cards address space */
	status = map_pci_addr(device);
	if(status != B_OK) goto err1;

	status = alloc_resources(device);
	if(status != B_OK) goto err1;

	/* init device */
	reset_device(device);

	/* allocate and initialize frame buffer rings & descriptors */
	status = alloc_buffers(device);
	if(status != B_OK) goto err2;
	status = init_buffers(device);
	if(status != B_OK) goto err2;

	/* setup interrupts */
	install_io_interrupt_handler(device->devInfo->u.h0.interrupt_line, vlance_interrupt, *cookie, 0);
	/* init hardware */
#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": hardware specific init\n");
#endif

	write32(device->reg_base + PCNET_RDP_OFFSET, 0UL);	/* DWIO mode */
	bcr_write(device, PCNET_BCR_SWS, 0x0002UL);	/* 32bit mode */

	get_mac_addr(device);

	device->init_blk.s.mode = ((TX_BUFF_IDX & 0x0f) << 28) | ((RX_BUFF_IDX & 0x0f) << 20) | 0x0000UL;	/* TLEN, RLEN */
	memcpy(device->init_blk.s.padr, &(device->mac_addr), sizeof(mac_addr_t));
	device->init_blk.s.padr[6] = 0;
	device->init_blk.s.padr[7] = 0;
	device->init_blk.s.ladr[0] = 0UL;
	device->init_blk.s.ladr[1] = 0UL;
	device->init_blk.s.rdra = device->phys_rx_desc;
	device->init_blk.s.tdra = device->phys_tx_desc;
	{
		int i;
		for(i = 0; i < sizeof(init_block_t); i++) {
#if DEBUG(INFO)
			dprintf(" %02X", *(((unsigned char *)&(device->init_blk)) + i));
#endif
		}
#if DEBUG(INFO)
		dprintf("\n");
#endif
	}
	csr_write(device, PCNET_CSR_IADDR0, (device->phys_init_blk) & 0xffffUL);	/* set init block address L */
	csr_write(device, PCNET_CSR_IADDR1, (device->phys_init_blk) >> 16);			/* set init block address H */
	csr_write(device, PCNET_CSR_STATUS, 0x0001UL);				/* INIT */
	while(!(csr_read(device, PCNET_CSR_STATUS) & 0x0100UL));	/* check IDON */
	csr_write(device, PCNET_CSR_STATUS, 0x0004UL);				/* STOP */
#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": STATUS = %04X\n", csr_read(device, PCNET_CSR_STATUS));
	dprintf(DEVICE_NAME ": IADDR0 = %04X\n", csr_read(device, PCNET_CSR_IADDR0));
	dprintf(DEVICE_NAME ": IADDR1 = %04X\n", csr_read(device, PCNET_CSR_IADDR1));
	dprintf(DEVICE_NAME ": MODE = %04X\n", csr_read(device, PCNET_CSR_MODE));
	dprintf(DEVICE_NAME ": PADR0 = %04X\n", csr_read(device, PCNET_CSR_PADR0));
	dprintf(DEVICE_NAME ": PADR1 = %04X\n", csr_read(device, PCNET_CSR_PADR1));
	dprintf(DEVICE_NAME ": PADR2 = %04X\n", csr_read(device, PCNET_CSR_PADR2));
	dprintf(DEVICE_NAME ": LADRF0 = %04X\n", csr_read(device, PCNET_CSR_LADRF0));
	dprintf(DEVICE_NAME ": LADRF1 = %04X\n", csr_read(device, PCNET_CSR_LADRF1));
	dprintf(DEVICE_NAME ": LADRF2 = %04X\n", csr_read(device, PCNET_CSR_LADRF2));
	dprintf(DEVICE_NAME ": LADRF3 = %04X\n", csr_read(device, PCNET_CSR_LADRF3));
	dprintf(DEVICE_NAME ": BADRL = %04X\n", csr_read(device, PCNET_CSR_BADRL));
	dprintf(DEVICE_NAME ": BADRH = %04X\n", csr_read(device, PCNET_CSR_BADRH));
	dprintf(DEVICE_NAME ": BADXL = %04X\n", csr_read(device, PCNET_CSR_BADXL));
	dprintf(DEVICE_NAME ": BADXH = %04X\n", csr_read(device, PCNET_CSR_BADXH));
	dprintf(DEVICE_NAME ": BCR18 = %04X\n", bcr_read(device, PCNET_BCR_BSBC));
	dprintf(DEVICE_NAME ": BCR20 = %04X\n", bcr_read(device, PCNET_BCR_SWS));
#endif
	csr_write(device, PCNET_CSR_STATUS, 0x7f00);	/* clear int source */
	csr_write(device, PCNET_CSR_STATUS, 0x0042);	/* IENA, STRT */

	return B_NO_ERROR;

	err2:
		free_buffers(device);

	err1:
		free_resources(device);
		free(device);

	err0:
		atomic_and(&(OpenLock[devID]), 0);
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": open failed!\n");
#endif
		return status;

}


static status_t vlance_close(void *_device) {
	dev_info_t *device = (dev_info_t *) _device;

#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ": vlance_close\n");
#endif

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": hardware specific code to halt NIC and DMA activity\n");
#endif
	csr_write(device, PCNET_CSR_STATUS, 0x0004);	/* STOP */
#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ": STATUS = %04X\n", csr_read(device,PCNET_CSR_STATUS));
#endif

	/* release resources */
	free_resources(device);

	return (B_NO_ERROR);
}


static status_t vlance_free(void *cookie) {
	dev_info_t *device = (dev_info_t *)cookie;

#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME": free %x\n",device);
#endif

    /* remove Interrupt Handler */
	remove_io_interrupt_handler(device->devInfo->u.h0.interrupt_line, vlance_interrupt, cookie);

	free_buffers(device);

	/* device is now available again */
	atomic_and(&(OpenLock[device->devID]), 0);

	free(device);
	return 0;
}


/* driver control function */
static status_t vlance_control(void *cookie, uint32 msg, void *buf, size_t len) {
	dev_info_t *device = (dev_info_t *)cookie;

	switch(msg) {
	case ETHER_GETADDR: {
		uint8 i;
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x ether_getaddr\n",msg);
#endif
		for(i = 0; i < 6; i++) {
			((uint8 *)buf)[i] = device->mac_addr.ch[i];
		}
		return B_NO_ERROR;
	}
	case ETHER_INIT:
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x init\n",msg);
#endif
		return B_NO_ERROR;

	case ETHER_GETFRAMESIZE:
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x get_framesize\n",msg);
#endif
		*(uint32 *)buf = MAX_FRAME_SIZE;
		return B_NO_ERROR;

	case ETHER_ADDMULTI:
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x add multi\n",msg);
#endif
		return (domulti(device, (unsigned char *)buf));

	case ETHER_SETPROMISC:
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x set promiscuous\n",msg);
#endif
		return setpromisc(device);
//		return B_ERROR;

	case ETHER_NONBLOCK:
#if DEBUG(FUNCTION)
		dprintf(DEVICE_NAME ": control %x blocking %d\n",msg,*((int32*)buf));
#endif

		if(*((int32 *)buf))
			device->blockFlg = 1;	// set non-blocking
		else
			device->blockFlg = 0;	// set blocking
		return B_NO_ERROR;

	default:
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": unknown iocontrol %x\n",msg);
#endif
		return B_ERROR;
	}

}


/*
 * read system call
 */
static status_t vlance_read(void *_device, off_t pos, void *buf, size_t *len) {
	dev_info_t  *device = (dev_info_t *) _device;
	int16		frame_size;
	uint32		flags;
	status_t    status;

//	*len = 0;

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": read buf %p, len %d\n", buf, *len);
#endif

	/* block until data is available (default) */
	flags = B_CAN_INTERRUPT;
	if(device->blockFlg) flags |= B_RELATIVE_TIMEOUT;	// non-blocking (0-timeout)
	status = acquire_sem_etc(device->ilock, 1, flags, 0);
	if(status != B_NO_ERROR) {
#if DEBUG(INFO)
		dprintf(DEVICE_NAME ": cannot acquire rx semaphore\n");
#endif
		*len = 0;
		return status;
	}
#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": try to atomic_or readLock\n");
#endif
	/* prevent reentrant read */
	if(atomic_or(&device->readLock, 1)) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": cannot atomic_or readLock\n");
#endif
		release_sem_etc(device->ilock, 1, 0);
		*len = 0;
		return B_ERROR;
	}

	/* hardware specific code to copy data from the NIC into buf */
	if((device->rx_desc[device->rx_acked]->s.status) & 0x8000) {	/* owned by controller */
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": rx desc owned by controller\n");
#endif
		*len = 0;
		status = B_ERROR;
	} else {
#if DEBUG(INFO)
		dprintf(DEVICE_NAME ": rx desc owned by host\n");
#endif
		frame_size = 0;
		status = B_ERROR;
		if(!((device->rx_desc[device->rx_acked]->s.status) & 0x4000)) {	/* not receive error */
			frame_size = (device->rx_desc[device->rx_acked]->s.mcnt) & 0xfff;
			if(frame_size > *len) frame_size = *len;
			memcpy(buf, device->rx_buf[device->rx_acked], frame_size);
			status = B_OK;
		}
		if(frame_size < *len) *len = frame_size;
		device->rx_desc[device->rx_acked]->s.mcnt = 0;
		device->rx_desc[device->rx_acked]->s.status = 0x8000;	/* OWN */
		device->rx_acked = (device->rx_acked + 1) & (RX_BUFFERS - 1);
	}

	/* release reentrant lock */
	atomic_and(&device->readLock, 0);

	return status;
}


/*
 * write system call
 */
static status_t vlance_write(void *_device, off_t pos, const void *buf, size_t *len) {
	dev_info_t	*device = (dev_info_t *)_device;
	int16		frame_size;
	status_t	status;

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": write buf %p len %d\n", buf, *len);
#endif

	if(*len > MAX_FRAME_SIZE) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": write %d > 1514 tooo long\n", *len);
#endif
		*len = MAX_FRAME_SIZE;
	}
	frame_size = *len;

	status = acquire_sem_etc(device->olock, 1, B_CAN_INTERRUPT, 0);
	if(status != B_NO_ERROR) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": cannot acquire tx semaphore\n");
#endif
		*len = 0;
		return status;
	}

	/* prevent reentrant write */
	if(atomic_or(&device->writeLock, 1)) {
		release_sem_etc(device->olock, 1, 0);
		*len = 0;
		return B_ERROR;
	}

	/* hardware specific code to transmit buff */
	if((device->tx_desc[device->tx_sent]->s.status) & 0x8000) {	/* owned by controller */
#if DEBUG(ERR)
		dprintf(DEVICE_NAME ": tx desc owned by controller\n");
#endif
	} else {
#if DEBUG(INFO)
		dprintf(DEVICE_NAME ": tx desc owned by host\n");
#endif
		memcpy(device->tx_buf[device->tx_sent], buf, frame_size);
		device->tx_desc[device->tx_sent]->s.bcnt = -frame_size;
//		(device->tx_desc[device->tx_sent]->s.status) |= 0x8300;	/* OWN, STP, ENP */
		(device->tx_desc[device->tx_sent]->s.status) |= 0x9300;	/* OWN, LTINT, STP, ENP */
		device->tx_sent = (device->tx_sent + 1) & (TX_BUFFERS - 1);
		csr_write(device, PCNET_CSR_STATUS, 0x0048UL);	/* IENA, TDMD */
	}

	/* release reentrant lock */
	atomic_and(&device->writeLock, 0);

	return B_OK;
}


/* LAN controller interrupt handler */
static int32 vlance_interrupt(void *_device) {
	dev_info_t	*device = (dev_info_t *)_device;
	int32		handled = B_UNHANDLED_INTERRUPT;
	int32		InterruptStatus;
	cpu_status	IntStat_sav;

#if DEBUG(INTERRUPT)
	dprintf(DEVICE_NAME ": ISR_ENTRY\n");
#endif
	IntStat_sav = disable_interrupts();	/* disable int state */

	InterruptStatus = csr_read(device, PCNET_CSR_STATUS);
	if(InterruptStatus & 0x0080) {
#if DEBUG(INTERRUPT)
		dprintf(DEVICE_NAME ": status %04X\n", InterruptStatus);
#endif

		/* clear interrupts */
		csr_write(device, PCNET_CSR_STATUS, 0x7f00);

		if(InterruptStatus & 0x8000) {	/* Error */
#if DEBUG(ERR)
			dprintf(DEVICE_NAME ": int error status %04X\n", InterruptStatus);
#endif
			csr_write(device, PCNET_CSR_STATUS, 0x0004);	/* STOP */
			init_buffers(device);
			csr_write(device, PCNET_CSR_STATUS, 0x0001UL);	/* INIT */
			while(!(csr_read(device, PCNET_CSR_STATUS) & 0x0100UL));	/* check IDON */
			csr_write(device, PCNET_CSR_STATUS, 0x0042);	/* IENA, STRT */
			/* init semaphore ??? */
			release_sem_etc(device->olock, TX_BUFFERS, B_DO_NOT_RESCHEDULE);
			acquire_sem_etc(device->ilock, RX_BUFFERS, B_RELATIVE_TIMEOUT, 0);
			/* not count TINT & RINT ??? */
			InterruptStatus &= ~0x0600;
		}
		if(InterruptStatus & 0x0200) {	/* TINT */
			while(!((device->tx_desc[device->tx_acked]->s.status) & 0x8000)) {
#if DEBUG(INFO)
				dprintf(DEVICE_NAME ": rel tx sem\n");
#endif
				release_sem_etc(device->olock, 1, B_DO_NOT_RESCHEDULE);
				device->tx_acked = (device->tx_acked + 1) & (TX_BUFFERS - 1);
				if(device->tx_acked == device->tx_sent) break;
			}
		}
		if(InterruptStatus & 0x0400) {	/* RINT */
			while(!((device->rx_desc[device->rx_received]->s.status) & 0x8000)) {
#if DEBUG(INFO)
				dprintf(DEVICE_NAME ": rel rx sem\n");
#endif
				release_sem_etc(device->ilock, 1, B_DO_NOT_RESCHEDULE);
				device->rx_received = (device->rx_received + 1) & (RX_BUFFERS - 1);
			}
		}

		handled = B_INVOKE_SCHEDULER;   /* set because the interrupt was from the NIC, not some other device sharing the interrupt line */

#if DEBUG(INTERRUPT)
		dprintf(DEVICE_NAME ": ISR - its ours\n");
#endif
	}

	csr_write(device, PCNET_CSR_STATUS, 0x0042);

	restore_interrupts(IntStat_sav);	/* restore int state */

	return handled;
}


static int32 get_card_info(pci_info *info[]) {
	status_t	status;
	int32		i, entries;
	pci_info	*item;

	item = (pci_info *)malloc(sizeof(pci_info));

	for(i = 0, entries = 0; entries < MAX_CARDS; i++) {
		status = PCIInfo->get_nth_pci_info(i, item);
		if(status != B_OK) break;
#if DEBUG(INFO)
		dprintf(DEVICE_NAME ": Check Vendor and Device ID\n");
#endif
		if((item->vendor_id == VENDOR_ID) && (item->device_id == DEVICE_ID)) {
			/* check if the device really has an IRQ */
			if((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
#if DEBUG(INFO)
				dprintf(DEVICE_NAME " found with invalid IRQ - check IRQ assignement\n");
#endif
				continue;
			}
#if DEBUG(INFO)
			dprintf(DEVICE_NAME " found at IRQ %x\n", item->u.h0.interrupt_line);
#endif
			info[entries++] = item;
			item = (pci_info *)malloc(sizeof(pci_info));
		}
	}
	free(item);
	return entries;
}

static status_t free_card_info(pci_info *info[]) {
	int32 		i;

	for(i = 0; i < NumOfCards; i++) {
		free(info[i]);
	}

	return B_OK;
}


static status_t map_pci_addr(dev_info_t *device) {
	pci_info *dev_info = device->devInfo;
	int32 pci_cmd;

	pci_cmd = PCIInfo->read_pci_config(
		dev_info->bus, dev_info->device,
		dev_info->function, PCI_command, 2
	);
	/* turn on I/O port decode, Memory Address Decode, and Bus Mastering */
	PCIInfo->write_pci_config(
		dev_info->bus, dev_info->device,
		dev_info->function, PCI_command, 2,
		pci_cmd | PCI_command_io | PCI_command_memory | PCI_command_master
	);

	device->reg_base = dev_info->u.h0.base_registers[0];

#if DEBUG(PCI_IO)
	dprintf(DEVICE_NAME ": reg_base=%x\n", device->reg_base);
#endif

	return B_OK;
}


#define RNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))

static status_t alloc_buffers(dev_info_t *device) {
	uint32			size;
	uint16			i;
	physical_entry	entry;


	/* get physical address of Initialization Block */
	size = RNDUP( sizeof(dev_info_t), B_PAGE_SIZE);
	get_memory_map(&(device->init_blk), size, &entry, 1);
	device->phys_init_blk = (uint32)(entry.address);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME " init block va=%p pa=%p\n",
		&(device->init_blk), device->phys_init_blk, size);
#endif


	/* create tx descriptor area */
	size = RNDUP( sizeof(trns_desc_t) * TX_BUFFERS, B_PAGE_SIZE);
	device->tx_desc_area = create_area(
		DEVICE_NAME " tx descriptors", (void **)device->tx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA
	);
	if(device->tx_desc_area < 0) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create tx descriptor area failed %x \n", device->tx_desc_area);
#endif
		return device->tx_desc_area;
	}
	for(i = 1; i < TX_BUFFERS; i++) {
		device->tx_desc[i] = (device->tx_desc[i-1]) + 1;
	}
	/* get physical address of tx descriptor */
	get_memory_map(device->tx_desc[0], size, &entry, 1);
	device->phys_tx_desc = (uint32)(entry.address);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME " create tx desc area va=%p pa=%p sz=%d\n",
		device->tx_desc[0], device->phys_tx_desc, size);
#endif


	/* create tx buffer area */
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	device->tx_buf_area = create_area(
		DEVICE_NAME " tx buffers", (void **)device->tx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA
	);
	if(device->tx_buf_area < 0) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create tx buffer area failed %x \n", device->tx_buf_area);
#endif
		delete_area(device->tx_desc_area);	// sensitive to alloc ordering
		return device->tx_buf_area;
	}
	for(i = 1; i < TX_BUFFERS; i++) {
		device->tx_buf[i] = (device->tx_buf[i-1]) + BUFFER_SIZE;
	}
	/* get physical address of tx buffer */
	get_memory_map(device->tx_buf[0], size, &entry, 1);
	device->phys_tx_buf = (uint32)(entry.address);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME " create tx buf area va=%p pa=%08x sz=%d\n",
		device->tx_buf[0], device->tx_desc[0]->s.tbadr, size);
#endif


	/* create rx descriptor area */
	size = RNDUP( sizeof(recv_desc_t) * RX_BUFFERS, B_PAGE_SIZE);
	device->rx_desc_area = create_area(
		DEVICE_NAME " rx descriptors", (void **)device->rx_desc,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA
	);
	if(device->rx_desc_area < 0) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create rx descriptor area failed %x \n", device->rx_desc_area);
#endif
		delete_area(device->tx_desc_area);
		delete_area(device->tx_buf_area);	// sensitive to alloc ordering
		return device->rx_desc_area;
	}
	for(i = 1; i < RX_BUFFERS; i++) {
		device->rx_desc[i] = (device->rx_desc[i-1]) + 1;
	}
	/* get physical address of rx descriptor */
	get_memory_map(device->rx_desc[0], size, &entry, 1);
	device->phys_rx_desc = (uint32)(entry.address);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME " create rx desc area va=%p pa=%p sz=%d\n",
		device->rx_desc[0], device->phys_rx_desc, size);
#endif

	/* create rx buffer area */
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	device->rx_buf_area = create_area(
		DEVICE_NAME " rx buffers", (void **) device->rx_buf,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA
	);
	if(device->rx_buf_area < 0) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create rx buffer area failed %x \n", device->rx_buf_area);
#endif
		delete_area(device->tx_desc_area);
		delete_area(device->tx_buf_area);
		delete_area(device->rx_desc_area);	// sensitive to alloc ordering
		return device->rx_buf_area;
	}
	for(i = 1; i < RX_BUFFERS; i++) {
		device->rx_buf[i] = (device->rx_buf[i-1]) + BUFFER_SIZE;
	}
	/* get physical address of rx buffer */
	get_memory_map(device->rx_buf[0], size, &entry, 1);
	device->phys_rx_buf = (uint32)(entry.address);
#if DEBUG(INFO)
	dprintf(DEVICE_NAME " create rx buf area va=%p pa=%08x sz=%d\n",
		device->rx_buf[0], device->rx_desc[0]->s.rbadr, size);
#endif


	return B_OK;
}


static status_t init_buffers(dev_info_t *device) {
	int i;

	/* initilize tx descriptors */
	for(i = 0; i < TX_BUFFERS; i++) {
		device->tx_desc[i]->s.tbadr = device->phys_tx_buf + BUFFER_SIZE * i;
		device->tx_desc[i]->s.bcnt = -BUFFER_SIZE;
		device->tx_desc[i]->s.status = 0;
		device->tx_desc[i]->s.misc = 0UL;
		device->tx_desc[i]->s.rsvd = 0UL;
	}


	/* initialize rx descriptors */
	for(i = 0; i < RX_BUFFERS; i++) {
		device->rx_desc[i]->s.rbadr = device->phys_rx_buf + BUFFER_SIZE * i;
		device->rx_desc[i]->s.bcnt = -BUFFER_SIZE;
//		device->rx_desc[i]->s.status = 0;
		device->rx_desc[i]->s.status = 0x8000;	/* OWN */
		device->rx_desc[i]->s.mcnt = 0UL;
		device->rx_desc[i]->s.rsvd = 0UL;
	}


	/* initialize frame indexes */
	device->tx_sent = device->tx_acked = device->rx_received = device->rx_acked = 0;


	return B_OK;
}


static void free_buffers(dev_info_t *device) {

		delete_area(device->tx_desc_area);
		delete_area(device->tx_buf_area);
		delete_area(device->rx_desc_area);
		delete_area(device->rx_buf_area);

}


static void get_mac_addr(dev_info_t *device) {
	int i;

#if DEBUG(INFO)
	dprintf(DEVICE_NAME ": Mac address: ");
#endif
	for(i = 0; i < 6; i++) {
		device->mac_addr.ch[i] = read8(device->reg_base + PCNET_APROM_OFFSET + i);
#if DEBUG(INFO)
		dprintf(" %02x", device->mac_addr.ch[i]);
#endif
	}
#if DEBUG(INFO)
	dprintf("\n");
#endif
}


/* set hardware so all packets are received. */
static status_t setpromisc(dev_info_t *device) {
#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ":setpormisc\n");
#endif
	csr_write(device, PCNET_CSR_STATUS, 0x0004UL);
	csr_write(device, PCNET_CSR_MODE, 0x8000UL);	/* promiscous mode */
	csr_write(device, PCNET_CSR_STATUS, 0x0042UL);
	return(B_NO_ERROR);
}


#define CRC_POLYNOMIAL_LE 0xedb88320UL  /* Ethernet CRC, little endian */

static status_t domulti(dev_info_t *device, uint8 *addr) {
	uint16 mcast_table[4];
	uint8 *addrs;
	int i, j, bit, byte;
	uint32 crc, poly = CRC_POLYNOMIAL_LE;

	{
		if(device->nmulti == MAX_MULTI) return (B_ERROR);
		for(i = 0; i < device->nmulti; i++) {
			if(memcmp(&device->multi[i], addr, sizeof(device->multi[i])) == 0) {
				break;
			}
		}
		if(i == device->nmulti) {
			/*
			 * only copy if it isn't there already
			 */
			memcpy(&device->multi[i], addr, sizeof(device->multi[i]));
			device->nmulti++;
		} else {
			return (B_ERROR);
		}
	}

#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ": domulti %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
		addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
#endif
	/* clear the multicast filter */
	mcast_table[0] = 0;	mcast_table[1] = 0;
	mcast_table[2] = 0;	mcast_table[3] = 0;
	/* set addresses */
	for(i = 0; i < device->nmulti; i++) {
		addrs = (uint8 *)(&(device->multi[i]));
		/* multicast address? */
		if(!(*addrs & 1)) break;
		crc = 0xffffffff;
		for(byte = 0; byte < 6; byte++) {
			for(bit = *addrs++, j = 0; j < 8; j++, bit >>= 1) {
				int test;

				test = ((bit ^ crc) & 0x01);
				crc >>= 1;
				if(test) {
					crc = crc ^ poly;
				}
			}
		}
		crc = crc >> 26;
		mcast_table[crc >> 4] |= 1 << (crc & 0xf);
	}
	csr_write(device, PCNET_CSR_STATUS, 0x0004UL);
	csr_write(device, PCNET_CSR_MODE, 0x0000UL);	/* portsel ?? */
	csr_write(device, PCNET_CSR_LADRF0, mcast_table[0]);
	csr_write(device, PCNET_CSR_LADRF1, mcast_table[1]);
	csr_write(device, PCNET_CSR_LADRF2, mcast_table[2]);
	csr_write(device, PCNET_CSR_LADRF3, mcast_table[3]);
	csr_write(device, PCNET_CSR_STATUS, 0x0042UL);
	return (B_NO_ERROR);
}


static void reset_device(dev_info_t *device) {
#if DEBUG(FUNCTION)
	dprintf(DEVICE_NAME ": reset_device reset the NIC hardware\n");
#endif
	read32(device->reg_base + PCNET_RST_OFFSET);
	write32(device->reg_base + PCNET_RST_OFFSET, 0UL);
	snooze(2);	/* wait >1us */
};


/*
 * allocate and initialize semaphores.
 */
static status_t alloc_resources(dev_info_t *device) {

	/* rx semaphores */
	device->ilock = create_sem(0, DEVICE_NAME " rx");
	if(device->ilock < 0) {
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create rx sem failed %x \n", device->ilock);
#endif
		return (device->ilock);
	}
	set_sem_owner(device->ilock, B_SYSTEM_TEAM);

	/* tx semaphores */
	device->olock = create_sem(TX_BUFFERS, DEVICE_NAME " tx");
	if(device->olock < 0) {
		delete_sem(device->ilock);
#if DEBUG(ERR)
		dprintf(DEVICE_NAME " create tx sem failed %x \n", device->olock);
#endif
		return (device->olock);
	}
	set_sem_owner(device->olock, B_SYSTEM_TEAM);

	device->readLock = device->writeLock = 0;

	device->blockFlg = 0;	// set blocking

	return (B_OK);
}


static void free_resources(dev_info_t *device) {
	delete_sem(device->ilock);
	delete_sem(device->olock);
}


int32 api_version = B_CUR_DRIVER_API_VERSION;


/* End of vlance.c */
