/*  National Semiconductor dp83815 driver
 *	Copyright (c) 2006 Urias McCullough (umccullough@gmail.com)
 *	Portions of code Copyright (c) 2003-2004, Niels Sascha Reedijk
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a
 *	copy of this software and associated documentation files (the "Software"),
 *	to deal in the Software without restriction, including without limitation
 *	the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *	and/or sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *	DEALINGS IN THE SOFTWARE.
 */
/*
 *	Portions of code based on dp83815 driver by: Antonio Carpio (BolivianTONE@nc.rr.com)
 *	Portions of code may be: Copyright (c) 1998, 1999 Be, Inc. All Rights Reserved under terms of Be Sample Code License.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <PCI.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "ether_driver.h"
#include "util.h"
#include "dp83815_regs.h"

#define kDevName "dp83815"
#define kDevDir "net/" kDevName "/"
#define MAX_CARDS 4
#define MAX_DESC			128			/* eventually going into config file	*/
#define	BUFFER_SIZE			2048		/* make it easy to do math and whatnot	*/
#define MAX_PACKET_SIZE		1518		/* can be 2046 if RXCFG has ALP set		*/
#define	NONBLOCK_WAIT		1000

//#define DEBUG
#ifdef DEBUG
#	define TRACE(x)  dprintf x
#else
#	define TRACE(x) ;
#endif

typedef struct supported_device {
	uint16 vendor_id;
	uint16 device_id;
	char * name;
} supported_device_t;

static supported_device_t m_supported_devices[] = {
	{ 0x100B, 0x0020, "National Semiconductor dp83815 (Netgear 311/312)" },
	{ 0, 0, NULL } /* End of list */
};

int32 api_version = B_CUR_DRIVER_API_VERSION; //Procedure

typedef struct descriptor {
	uint32				link;			/*	Physical address of next descriptor	*/
	volatile uint32		cmd;			/*	info to/from NIC					*/
	volatile uint32		ptr;			/*	Physical address of the buffer		*/
	struct descriptor	*virt_next;		/*	virtual address of next descriptor	*/
	void				*virt_buff;		/*	virtual address of the buffer		*/
} descriptor_t;

typedef struct desc_ring {
	descriptor_t	*Curr;				/*	Current descriptor to be accessed	*/
	sem_id			Sem;				/*	Queue for outstanding reads/writes	*/
	spinlock		Lock;				/*	Spinlock used to grab a descriptor	*/
	descriptor_t	*CurrInt;			/*	Interrupt Hook's Current Descriptor	*/
} desc_ring_t;

typedef	struct stats {
	uint32	rx_ok,
			tx_ok,
			tx_err,
			rx_err,
			rx_overrun,
			tx_underrun,
			collisions,
			rx_att,
			tx_att;
} stats_t;

typedef struct dp83815_properties
{
	pci_info 		*pcii;				/* Pointer to PCI Info for the device */
	uint32 			reg_base; 			/* Base address for registers */
	area_id			ioarea;				/* PPC: Area where the mmaped registers are */
	area_id			reg_area,
					mem_area;
	uint8			device_id;			/* Which device id this is... */

	ether_address_t address;		/* holds the MAC address */
	sem_id 			lock;				/* lock this structure: still interrupt */
	int32			blockFlag;			/* for blocking or nonblocking reads */

	stats_t			stats;
	desc_ring_t		Rx, Tx;
} dp83815_properties_t;

static pci_info *m_devices[MAX_CARDS];
static pci_module_info *m_pcimodule = 0;	//To call methods of pci
static char *dp83815_names[MAX_CARDS + 1];
static int32 m_openmask = 0;					//Is the thing already opened?
static uint32 pages_needed(uint32 mem_size);
static int32 dp83815_interrupt_hook(void *data);    /* interrupt handler */

static status_t allocate_resources(dp83815_properties_t *data);     	   /* allocate semaphores & spinlocks */
static void     free_resources(dp83815_properties_t *data);                /* deallocate semaphores & spinlocks */
static status_t init_ring_buffers(dp83815_properties_t *data);             /* allocate and initialize frame buffer rings */
static status_t domulti(dp83815_properties_t *data, uint8 *addr);		   /* add multicast address to hardware filter list */
static status_t free_hook(void *cookie);
static status_t close_hook(void *);

#ifdef __INTEL__
	#define write8(  offset , value)	(m_pcimodule->write_io_8 ((data->reg_base + (offset)), (value) ) )
	#define write16( offset , value)	(m_pcimodule->write_io_16((data->reg_base + (offset)), (value) ) )
	#define write32( offset , value)	(m_pcimodule->write_io_32((data->reg_base + (offset)), (value) ) )

	#define read8(  offset )			(m_pcimodule->read_io_8 ((data->reg_base + offset)))
	#define read16( offset )			(m_pcimodule->read_io_16((data->reg_base + offset)))
	#define read32( offset )			(m_pcimodule->read_io_32((data->reg_base + offset)))

	static void
	dp83815_init_registers(dp83815_properties_t *data)
	{
		data->reg_base = data->pcii->u.h0.base_registers[0];
	}
#else /* PPC */
	#include <ByteOrder.h>
	#define write8(  offset , value)	(*((volatile uint8 *)(data->reg_base + (offset))) = (value))
	#define write16( offset , value)	(*((volatile uint8 *)(data->reg_base + (offset))) = B_HOST_TO_LENDIAN_INT16(value))
	#define write32( offset , value)	(*((volatile uint8 *)(data->reg_base + (offset))) = B_HOST_TO_LENDIAN_INT32(value))

	#define read8(  offset )			(*((volatile uint8*)(data->reg_base + (offset))))
	#define read16( offset )			B_LENDIAN_TO_HOST_INT16(*((volatile uint16*)(data->reg_base + (offset))))
	#define read32( offset )			B_LENDIAN_TO_HOST_INT32(*((volatile uint32*)(data->reg_base + (offset))))

	static void
	dp83815_init_registers(rtl8139_properties_t *data)
	{
		int32 base, size, offset;
		base = data->pcii->u.h0.base_registers[0];
		size = data->pcii->u.h0.base_register_sizes[0];

		/* Round down to nearest page boundary */
		base = base & ~(B_PAGE_SIZE - 1);

		/* Adjust the size */
		offset = data->pcii->u.h0.base_registers[0] - base;
		size += offset;
		size = (size + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1);

		TRACE(( kDevName " _open_hook(): PCI base=%lx size=%lx offset=%lx\n",
			base, size, offset));

		data->ioarea = map_physical_memory(kDevName " Regs", base, size,
			B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
			(void **)&data->reg_base);

		data->reg_base = data->reg_base + offset;
	}
#endif


uint32
pages_needed(uint32 mem_size)
{
	uint32 pages = mem_size / B_PAGE_SIZE;
	if (pages % B_PAGE_SIZE != 0)
		pages += 1;
	return pages;
}


status_t
init_hardware(void)
{
	// Nielx: no special requirements here...
	TRACE(( kDevName ": init_hardware\n" ));
	return B_OK;
}


status_t
init_driver(void)
{
	status_t status; 		//Storage for statuses
	pci_info *item;			//Storage used while looking through pci
	int32 i, found;			//Counter

	TRACE(( kDevName ": init_driver()\n" ));

	// Try if the PCI module is loaded (it would be weird if it wouldn't,
	// but alas)
	if ((status = get_module(B_PCI_MODULE_NAME, (module_info **)&m_pcimodule))
		!= B_OK) {
		TRACE((kDevName " init_driver(): Get PCI module failed! %lu \n",
			status));
		return status;
	}

	i = 0;
	item = (pci_info *)malloc(sizeof(pci_info));
	for (i = found = 0; m_pcimodule->get_nth_pci_info(i, item) == B_OK; i++) {
		supported_device_t *supported;

		for (supported = m_supported_devices; supported->name; supported++) {
			if ((item->vendor_id == supported->vendor_id)
				&& (item->device_id == supported->device_id)) {
				//Also done in etherpci sample code
				if ((item->u.h0.interrupt_line == 0)
					|| (item->u.h0.interrupt_line == 0xFF)) {
					TRACE(( kDevName " init_driver(): found %s with invalid IRQ"
						" - check IRQ assignement\n", supported->name));
					continue;
				}

				TRACE(( kDevName " init_driver(): found %s at IRQ %u \n",
					supported->name, item->u.h0.interrupt_line));
				m_devices[found] = item;
				item = (pci_info *)malloc(sizeof(pci_info));
				
				found++;
			}
		}
	}

	free(item);

	//Check if we have found any devices:
	if (found == 0) {
		TRACE(( kDevName " init_driver(): no device found\n" ));
		put_module(B_PCI_MODULE_NAME); //dereference module
		return ENODEV;
	}

	//Create the devices list
	{
		char name[32];

		for (i = 0; i < found; i++) {
			snprintf(name, 32, "%s%ld", kDevDir, i);
			dp83815_names[i] = strdup(name);
		}
		dp83815_names[i] = NULL;
	}
	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver(void)
{
	int index;
	void *item;
	TRACE(( kDevName ": uninit_driver()\n" ));

	for (index = 0; (item = dp83815_names[index]) != NULL; index++)
	{
		free(item);
		free(m_devices[index]);
	}

	put_module(B_PCI_MODULE_NAME);
}


static status_t
open_hook(const char *name, uint32 flags, void** cookie)
{

	dp83815_properties_t *data;
	uint8 temp8;
//	uint16 temp16;
	uint32 temp32;
	unsigned char cmd;

	TRACE(( kDevName " open_hook()\n" ));

	// verify device access
	{
		char *thisName;
		int32 mask;

		// search for device name
		for (temp8 = 0; (thisName = dp83815_names[temp8]) != NULL; temp8++) {
			if (!strcmp(name, thisName))
				break;
		}
		if (!thisName)
			return EINVAL;

		// check if device is already open
		mask = 1L << temp8;
		if (atomic_or(&m_openmask, mask) & mask)
			return B_BUSY;
	}

	//Create a structure that contains the internals
	if (!(*cookie = data =
		(dp83815_properties_t *)malloc(sizeof(dp83815_properties_t)))) {
		TRACE(( kDevName " open_hook(): Out of memory\n" ));
		return B_NO_MEMORY;
	}

	//Set status to open:
	m_openmask &= ~(1L << temp8);

	//Clear memory
	memset(data, 0, sizeof(dp83815_properties_t));

	//Set the ID
	data->device_id = temp8;

	// Create lock
	data->lock = create_sem(1, kDevName " data protect");
	set_sem_owner(data->lock, B_SYSTEM_TEAM);
	data->Rx.Sem = create_sem(0, kDevName " read wait");
	set_sem_owner(data->Rx.Sem, B_SYSTEM_TEAM);
	data->Tx.Sem = create_sem(1, kDevName " write wait");
	set_sem_owner(data->Tx.Sem, B_SYSTEM_TEAM);

	//Set up the cookie
	data->pcii = m_devices[data->device_id];

	//Enable the registers
	dp83815_init_registers(data);

	/* enable pci address access */
	cmd = m_pcimodule->read_pci_config(data->pcii->bus, data->pcii->device,
		data->pcii->function, PCI_command, 2);
	cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
	m_pcimodule->write_pci_config(data->pcii->bus, data->pcii->device,
		data->pcii->function, PCI_command, 2, cmd);

	if (allocate_resources(data) != B_OK)
		goto err1;

	/* We want interrupts! */
	if (install_io_interrupt_handler(data->pcii->u.h0.interrupt_line,
		dp83815_interrupt_hook, data, 0) != B_OK) {
		TRACE((kDevName " open_hook(): Error installing interrupt handler\n"));
		return B_ERROR;
	}

	{
		temp32 = read32(REG_SRR);
		TRACE(( "SRR: %x\n", temp32));
	}

	write32(REG_CR, CR_RXR|CR_TXR);			/*	Reset Tx & Rx		*/

	if (init_ring_buffers(data) != B_OK)		/*	Init ring buffers	*/
		goto err1;

	write32(REG_RFCR, RFCR_RFEN | RFCR_AAB | RFCR_AAM | RFCR_AAU);

	write32(REG_RXCFG, RXCFG_ATP | RXCFG_DRTH(31));	/*	Set the drth		*/

	write32(REG_TXCFG, TXCFG_CSI |
						TXCFG_HBI |
						TXCFG_ATP |
						TXCFG_MXDMA_256 |
						TXCFG_FLTH(16) |
						TXCFG_DRTH(16));

	write32(REG_IMR, ISR_RXIDLE | ISR_TXOK | ISR_RXOK );

	write32(REG_CR, CR_RXE);			/*	Enable Rx				*/
	write32(REG_IER, 1);				/*	Enable interrupts		*/

	return B_OK;

	err1:
		free_resources(data);
		free(data);
		return B_ERROR;
}


static status_t
read_hook (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	dp83815_properties_t *data = cookie;
	cpu_status former;
	descriptor_t*	desc;
	size_t			length = 0;

	TRACE(( kDevName ": read_hook()\n" ));

	//if( !data->nonblocking )
		acquire_sem_etc(data->Rx.Sem, 1, B_CAN_INTERRUPT | data->blockFlag,
			NONBLOCK_WAIT);

	{
		former = disable_interrupts();
		acquire_spinlock(&data->Rx.Lock);
		desc = data->Rx.Curr;
		data->Rx.Curr = desc->virt_next;
		release_spinlock(&data->Rx.Lock);
		restore_interrupts(former);
	}

	length = DESC_LENGTH&desc->cmd;

	if (desc->cmd & (DESC_RXA | DESC_RXO | DESC_LONG | DESC_RUNT | DESC_ISE
			| DESC_CRCE | DESC_FAE | DESC_LBP | DESC_COL))
		TRACE(( "desc cmd: %x\n", desc->cmd ));

	if (length < 64) {
		*num_bytes = 0;
		return B_ERROR;
	}

	if (*num_bytes < length)
		length = *num_bytes;

	memcpy(buf, desc->virt_buff, length);
	desc->cmd = DESC_LENGTH&MAX_PACKET_SIZE;
	*num_bytes = length;

	atomic_add(&data->stats.rx_att, 1);

	if (length == 0)
		return B_ERROR;

	return B_OK;
}


static status_t
write_hook(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	dp83815_properties_t *data = cookie;
	cpu_status former;
	descriptor_t*	desc;

	TRACE(( kDevName " write_hook()\n" ));

	acquire_sem(data->lock);
	acquire_sem_etc(data->Tx.Sem, 1, B_CAN_INTERRUPT | data->blockFlag,
		NONBLOCK_WAIT);

	{
		former = disable_interrupts();
		acquire_spinlock(&data->Tx.Lock);
		desc = data->Tx.Curr;
		data->Tx.Curr = desc->virt_next;
		release_spinlock(&data->Tx.Lock);
		restore_interrupts(former);
	}

	if (*num_bytes > MAX_PACKET_SIZE) {			/* if needed				*/
		TRACE(( "Had to truncate the packet from %d to %d\n", *num_bytes,
			MAX_PACKET_SIZE));
		*num_bytes = MAX_PACKET_SIZE;			/*   truncate the packet	*/
	}


	while (desc->cmd & DESC_OWN) {		/* make sure a buffer is available	*/
		TRACE(( "spinning in the write hook\n"));
		spin(1000);						/* wait a while if not				*/
	}

	memcpy(desc->virt_buff, buffer, *num_bytes);	/* now copy the data	*/
	desc->cmd = DESC_OWN | *num_bytes;				/* update the cmd bits	*/

	write32(REG_CR, CR_TXE);				/* tell the card to start tx	*/
	atomic_add(&data->stats.tx_att, 1);

	release_sem_etc(data->lock, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


static status_t
control_hook (void* cookie, uint32 op, void* arg, size_t len)
{
	dp83815_properties_t *data = (dp83815_properties_t *)cookie;
	TRACE(( kDevName " control_hook()\n" ));

	switch (op) {
		case ETHER_INIT:
			TRACE((kDevName " control_hook(): Wants us to init... ;-)\n"));
			return B_NO_ERROR;

		case ETHER_GETADDR:
			if (data == NULL)
				return B_ERROR;

			TRACE((kDevName " control_hook(): Wants our address...\n"));
			memcpy(arg, (void *) &(data->address), sizeof(ether_address_t));
			return B_OK;

		case ETHER_ADDMULTI:
			return domulti(data, (unsigned char *)arg);

		case ETHER_NONBLOCK:
			if (data == NULL)
				return B_ERROR;

			TRACE((kDevName " control_hook(): Wants to set block/nonblock\n"));

			if (*((int32 *)arg))
				data->blockFlag = B_RELATIVE_TIMEOUT;
			else
				data->blockFlag = 0;

			return B_NO_ERROR;

		case ETHER_REMMULTI:
			TRACE((kDevName " control_hook(): Wants REMMULTI\n"));
			return B_OK;

		case ETHER_SETPROMISC:
			TRACE((kDevName " control_hook(): Wants PROMISC\n"));
			return B_OK;

		case ETHER_GETFRAMESIZE:
			TRACE((kDevName " control_hook(): Wants GETFRAMESIZE\n"));
			*((unsigned int *)arg) = 1514;
			return B_OK;
	}

	return B_BAD_VALUE;
}


static int32
dp83815_interrupt_hook(void *cookie)
{
	dp83815_properties_t *data = (dp83815_properties_t *) cookie;
	uint32		isr;
	isr = read32(REG_ISR);

	if (isr == 0)
		return B_UNHANDLED_INTERRUPT;

	if (isr & ISR_RXOK) {
		int				num_packets = 0;
		descriptor_t	*curr = data->Rx.CurrInt;

		while (curr->cmd & DESC_OWN) {
			curr = curr->virt_next;
			num_packets++;
		}

		data->Rx.CurrInt = curr;
		data->stats.rx_ok += num_packets;
		if (num_packets > 1)
			TRACE(( "received %d descriptors\n", num_packets));
		if (num_packets)
			release_sem_etc(data->Rx.Sem, num_packets, B_DO_NOT_RESCHEDULE);
	}

	if (isr & ISR_TXOK) {
		data->stats.tx_ok++;
		release_sem_etc(data->Tx.Sem, 1, B_DO_NOT_RESCHEDULE);
	}

	if (isr & ISR_RXIDLE)
		TRACE(("RX IS IDLE!\n"));

	if (isr & ~(ISR_TXIDLE | ISR_TXOK | ISR_RXOK | ISR_RXIDLE | ISR_RXEARLY))
		TRACE(("ISR: %x\n", isr));

	return B_INVOKE_SCHEDULER;
}


static status_t
close_hook(void* cookie)
{
	dp83815_properties_t * data = (dp83815_properties_t *) cookie;

	write32(REG_IER, 0);					/*	Disable interrupts		*/
	write32(REG_CR, CR_RXD | CR_TXD);		/*	Disable Rx & Tx			*/

	return B_OK;
}


static status_t
free_hook(void* cookie)
{
	dp83815_properties_t *data = (dp83815_properties_t *) cookie;

	TRACE(( kDevName " free_hook()\n" ));

	while (data->Tx.Lock);		/*	wait for any current writes to finish	*/
	while (data->Rx.Lock);		/*	wait for any current reads to finish	*/

	//Remove interrupt handler
	remove_io_interrupt_handler(data->pcii->u.h0.interrupt_line,
		dp83815_interrupt_hook, cookie);

	m_openmask &= ~(1L << data->device_id);

	free_resources(data);					/*	unblock waiting threads		*/

	//Finally, free the cookie
	free(data);

	//Put the pci module
	put_module(B_PCI_MODULE_NAME);

	return B_OK;
}


device_hooks dp83815_hooks = {
	open_hook,	 			/* -> open entry point */
	close_hook, 			/* -> close entry point */
	free_hook,			/* -> free cookie */
	control_hook, 			/* -> control entry point */
	read_hook,			/* -> read entry point */
	write_hook				/* -> write entry point */
};


const char**
publish_devices()
{
	return (const char **)dp83815_names;
}


device_hooks*
find_device(const char* name)
{
	return &dp83815_hooks;
}


static status_t
init_ring_buffers(dp83815_properties_t *data)
{
	uint32			i;
	area_info		info;
	physical_entry	map[2];
	uint32 pages;

	descriptor_t	*RxDescRing = NULL;
	descriptor_t	*TxDescRing = NULL;

	descriptor_t	*desc_base_virt_addr;
	uint32			desc_base_phys_addr;

	void			*buff_base_virt_addr;
	uint32			buff_base_phys_addr;


	data->mem_area = 0;

#define NUM_BUFFS	2*MAX_DESC

	pages = pages_needed(2 * MAX_DESC * sizeof(descriptor_t)
		+ NUM_BUFFS * BUFFER_SIZE);

	data->mem_area = create_area(kDevName " desc buffer", (void**)&RxDescRing,
		B_ANY_KERNEL_ADDRESS, pages * B_PAGE_SIZE, B_32_BIT_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);
	if (data->mem_area < 0)
		return B_ERROR;

	get_area_info(data->mem_area, &info);
	get_memory_map(info.address, info.size, map, 4);

	desc_base_phys_addr = map[0].address + NUM_BUFFS * BUFFER_SIZE;
	desc_base_virt_addr = info.address + NUM_BUFFS * BUFFER_SIZE;

	buff_base_phys_addr = map[0].address;
	buff_base_virt_addr = info.address;

	RxDescRing = desc_base_virt_addr;
	for (i = 0; i < MAX_DESC; i++) {
		RxDescRing[i].link = desc_base_phys_addr
			+ ((i + 1) % MAX_DESC) * sizeof(descriptor_t);
		RxDescRing[i].cmd = MAX_PACKET_SIZE;
		RxDescRing[i].ptr = buff_base_phys_addr + i * BUFFER_SIZE;
		RxDescRing[i].virt_next = &RxDescRing[(i + 1) % MAX_DESC];
		RxDescRing[i].virt_buff = buff_base_virt_addr + i * BUFFER_SIZE;
	}

	TxDescRing = desc_base_virt_addr + MAX_DESC;
	for (i = 0; i < MAX_DESC; i++) {
		TxDescRing[i].link = desc_base_phys_addr
			+ MAX_DESC * sizeof(descriptor_t)
			+ ((i + 1) % MAX_DESC) * sizeof(descriptor_t);
		TxDescRing[i].cmd = MAX_PACKET_SIZE;
		TxDescRing[i].ptr = buff_base_phys_addr
			+ ((i + MAX_DESC) * BUFFER_SIZE);
		TxDescRing[i].virt_next = &TxDescRing[(i + 1) % MAX_DESC];
		TxDescRing[i].virt_buff = buff_base_virt_addr
			+ ((i + MAX_DESC) * BUFFER_SIZE);
	}

	data->Rx.Curr = RxDescRing;
	data->Tx.Curr = TxDescRing;

	data->Rx.CurrInt = RxDescRing;
	data->Tx.CurrInt = TxDescRing;


	write32(REG_RXDP, desc_base_phys_addr);	/* set the initial rx descriptor */

	i = desc_base_phys_addr + MAX_DESC * sizeof(descriptor_t);
	write32(REG_TXDP, i);				/* set the initial tx descriptor	*/

	return B_OK;
}


static status_t
allocate_resources(dp83815_properties_t *data)
{
	/* intialize rx semaphore with zero received packets		*/
	if ((data->Rx.Sem = create_sem(0, kDevName " rx")) < 0) {
		TRACE(( kDevName " create rx sem failed %x \n", data->Rx.Sem));
		return (data->Rx.Sem);
	}
	set_sem_owner(data->Rx.Sem, B_SYSTEM_TEAM);


	/* intialize tx semaphore with the number of free tx buffers */
	if ((data->Tx.Sem = create_sem(MAX_DESC, kDevName " tx")) < 0) {
		delete_sem(data->Rx.Sem);
		TRACE(( kDevName " create read sem failed %x \n", data->Tx.Sem));
		return (data->Tx.Sem);
	}

	set_sem_owner(data->Tx.Sem, B_SYSTEM_TEAM);

	data->blockFlag = 0;

	return (B_OK);
}


static void free_resources(dp83815_properties_t *data)
{
		delete_sem(data->Rx.Sem);
		delete_sem(data->Tx.Sem);
}


static status_t domulti(dp83815_properties_t *data, uint8 *addr)
{
	TRACE(( "Set up multicast filter here\n"));
	return (B_ERROR);
}
