/* device.c - device hooks for SiS900 networking
**
** Copyright © 2001-2003 pinc Software. All Rights Reserved.
*/

#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>
#include <image.h>

#include <stdlib.h>
#include <string.h>

#include "ether_driver.h"
#include "driver.h"
#include "device.h"
#include "sis900.h"


/* device hooks prototypes */

status_t device_open(const char *, uint32, void **);
status_t device_close(void *);
status_t device_free(void *);
status_t device_ioctl(void *, uint32, void *, size_t);
status_t device_read(void *, off_t, void *, size_t *);
status_t device_write(void *, off_t, const void *, size_t *);


int32 gDeviceOpenMask = 0;

device_hooks gDeviceHooks = {
	device_open,
	device_close,
	device_free,
	device_ioctl,
	device_read,
	device_write,
	NULL,
	NULL,
	NULL,
	NULL
};

#include <stdarg.h>

//#define EXCESSIVE_DEBUG
//#define THE_BUSY_WAITING_WAY

#ifdef EXCESSIVE_DEBUG
//#	include <time.h>

sem_id gIOLock;

int
bug(const char *format, ...)
{
	va_list vl;
	char    c[2048];
	int     i;
	int     file;
	
	if ((file = open("/boot/home/sis900-driver.log", O_RDWR | O_APPEND | O_CREAT)) >= 0) {
//		time_t timer = time(NULL);
//		strftime(c, 2048, "%H:%M:S: ", localtime(&timer));
		
		va_start(vl, format);
		i = vsprintf(c, format, vl);
		va_end(vl);

		write(file, c, strlen(c));
		close(file);
	}

	return i;
}

#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const char *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;)
	{
		int start = i;

		bug(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++)
		{
			if (!(i % 4))
				bug(" ");

			if (i >= size)
				bug("  ");
			else
				bug("%02x", *(unsigned char *)(buffer + i));
		}
		bug("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++)
		{
			if (i < size)
			{
				char c = buffer[i];

				if (c < 30)
					bug(".");
				else
					bug("%c", c);
			}
			else
				break;
		}
		bug("\n");
	}
}

#endif	/* EXCESSIVE_DEBUG */
//	#pragma mark -


static status_t
checkDeviceInfo(struct sis_info *info)
{
	if (!info || info->cookieMagic != SiS_COOKIE_MAGIC)
		return EINVAL;

	return B_OK;
}


static void
deleteSemaphores(struct sis_info *info)
{
}


static status_t
createSemaphores(struct sis_info *info)
{
	if ((info->rxSem = create_sem(0, "sis900 receive")) < B_OK)
		return info->rxSem;
	set_sem_owner(info->rxSem, B_SYSTEM_TEAM);

	if ((info->txSem = create_sem(NUM_Tx_DESCR, "sis900 transmit")) < B_OK)
	{
		delete_sem(info->rxSem);
		return info->txSem;
	}
	set_sem_owner(info->txSem, B_SYSTEM_TEAM);

#ifdef EXCESSIVE_DEBUG
	if ((gIOLock = create_sem(1, "sis900 debug i/o lock")) < B_OK)
		return gIOLock;
	set_sem_owner(gIOLock, B_SYSTEM_TEAM);
#endif

	info->rxLock = info->txLock = 0;
	
	return B_OK;
}


//--------------------------------------------------------------------------
//	#pragma mark -
//	the device will be accessed through the following functions (a.k.a. device hooks)


status_t
device_open(const char *name, uint32 flags, void **cookie)
{
	struct sis_info *info;
	area_id area;
	int id;

	// verify device access
	{
		char *thisName;
		int32 mask;

		// search for device name
		for (id = 0; (thisName = gDeviceNames[id]) != NULL; id++) {
			if (!strcmp(name, thisName))
				break;
		}
		if (!thisName)
			return EINVAL;
	
		// check if device is already open
		mask = 1L << id;
		if (atomic_or(&gDeviceOpenMask, mask) & mask)
			return B_BUSY;
	}

	// allocate internal device structure
	if ((area = create_area(DEVICE_NAME " private data", cookie,
							B_ANY_KERNEL_ADDRESS,
							ROUND_TO_PAGE_SIZE(sizeof(struct sis_info)),
							B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA)) < B_OK) {
		gDeviceOpenMask &= ~(1L << id);
		return B_NO_MEMORY;
	}
	info = (struct sis_info *)*cookie;
	memset(info, 0, sizeof(struct sis_info));

#ifdef DEBUG
	load_driver_symbols("sis900");
#endif

	info->cookieMagic = SiS_COOKIE_MAGIC;
	info->thisArea = area;
	info->id = id;
	info->pciInfo = pciInfo[id];
	info->registers = (char *)pciInfo[id]->u.h0.base_registers[0];

	if (sis900_getMACAddress(info))
		dprintf(DEVICE_NAME ": MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
			  info->address.ebyte[0], info->address.ebyte[1], info->address.ebyte[2],
			  info->address.ebyte[3], info->address.ebyte[4], info->address.ebyte[5]);
	else
		dprintf(DEVICE_NAME ": could not get MAC address\n");

	if (createSemaphores(info) == B_OK) {
		status_t status = sis900_initPHYs(info);
		if (status == B_OK) {
			TRACE((DEVICE_NAME ": MII status = %d\n", mdio_read(info, MII_STATUS)));	

			//sis900_configFIFOs(info);
			sis900_reset(info);

			// install & enable interrupts
			install_io_interrupt_handler(info->pciInfo->u.h0.interrupt_line,
				sis900_interrupt, info, 0);
			sis900_enableInterrupts(info);

			sis900_setRxFilter(info);
			sis900_createRings(info);

			// enable receiver's state machine
			write32((uint32)info->registers + SiS900_MAC_COMMAND, SiS900_MAC_CMD_Rx_ENABLE);

			// check link mode & add timer
			sis900_checkMode(info);
			add_timer(&info->timer, sis900_timer, 1000000LL, B_PERIODIC_TIMER);	

			return B_OK;
		}
		dprintf(DEVICE_NAME ": could not initialize MII PHY: %s\n", strerror(status));
		deleteSemaphores(info);
	} else
		dprintf(DEVICE_NAME ": could not create semaphores.\n");

	gDeviceOpenMask &= ~(1L << id);
	delete_area(area);

	return B_ERROR;
}


status_t
device_close(void *data)
{
	struct sis_info *info;

	if (checkDeviceInfo(info = data) != B_OK)
		return EINVAL;

	info->cookieMagic = SiS_FREE_COOKIE_MAGIC;

	// cancel timer
	cancel_timer(&info->timer);

	// disable the transmitter's and receiver's state machine
	write32((uint32)info->registers + SiS900_MAC_COMMAND,
		SiS900_MAC_CMD_Rx_DISABLE | SiS900_MAC_CMD_Tx_DISABLE);

	// remove & disable interrupt
	sis900_disableInterrupts(info);
	remove_io_interrupt_handler(info->pciInfo->u.h0.interrupt_line, sis900_interrupt, info);

	delete_sem(info->rxSem);
	delete_sem(info->txSem);

#ifdef EXCESSIVE_DEBUG
	delete_sem(gIOLock);
#endif

	return B_OK;
}


status_t
device_free(void *data)
{
	struct sis_info *info = data;
	status_t retval = B_NO_ERROR;

	if (info == NULL || info->cookieMagic != SiS_FREE_COOKIE_MAGIC)
		retval = EINVAL;

	gDeviceOpenMask &= ~(1L << info->id);

	sis900_deleteRings(info);
	delete_area(info->thisArea);

	return retval;
}


status_t
device_ioctl(void *data, uint32 msg, void *buffer, size_t bufferLength)
{
	struct sis_info *info;

	if (checkDeviceInfo(info = data) != B_OK)
		return EINVAL;

	switch (msg) {
		case ETHER_GETADDR:
			TRACE(("ioctl: get MAC address\n"));
			memcpy(buffer, &info->address, 6);
			return B_NO_ERROR;

		case ETHER_INIT:
			TRACE(("ioctl: init\n"));
			return B_NO_ERROR;
	
		case ETHER_GETFRAMESIZE:
			TRACE(("ioctl: get frame size\n"));
			*(uint32*)buffer = MAX_FRAME_SIZE;
			return B_NO_ERROR;
	
		case ETHER_SETPROMISC:
			TRACE(("ioctl: set promisc\n"));
			sis900_setPromiscuous(info, *(uint32 *)buffer != 0);
			return B_OK;
	
		case ETHER_NONBLOCK:
			TRACE(("ioctl: non blocking ? %s\n", info->blockFlag ? "yes" : "no"));
			info->blockFlag = *(int32 *)buffer ? B_TIMEOUT : 0;
			return B_NO_ERROR;
	
		case ETHER_ADDMULTI:
			TRACE(("ioctl: add multicast\n"));
			/* not yet implemented */
			break;
		
		default:
			TRACE(("ioctl: unknown message %d (length = %ld)\n", msg, bufferLength));
			break;
	}
	return B_ERROR;
}


#ifdef DEBUG
// sis900.c
extern int32 intrCounter;
extern int32 lastIntr[100];
uint32 counter = 0;
#endif


status_t
device_read(void *data, off_t pos, void *buffer, size_t *_length)
{
	struct sis_info *info;
	status_t status;
	size_t size;
	int32 blockFlag;
	uint32 check;
	int16 current;

	if (checkDeviceInfo(info = data) != B_OK) {
		*_length = 0;
		return EINVAL;
	}

	//dprintf("read: rx: isr = %d, free = %d, current = %d\n",
	//		info->rxInterruptIndex, info->rxFree, info->rxCurrent);

	blockFlag = info->blockFlag;

	// read is not reentrant
	if (atomic_or(&info->rxLock, 1)) {
		*_length = 0;
		return B_ERROR;
	}

	//TRACE(("current rx descr: %08x (last = %ld)\n", rxp = read32((uint32)info->registers + SiS900_MAC_Rx_DESCR), (info->rxLast+1) % NUM_Rx_DESCR));

	// block until data is available (if blocking is allowed)
	if ((status = acquire_sem_etc(info->rxSem, 1, B_CAN_INTERRUPT | blockFlag, 0)) != B_NO_ERROR) {
		TRACE(("cannot acquire read sem: %x, %s\n", status, strerror(status)));
		atomic_and(&info->rxLock, 0);
		*_length = 0;
		return status;
	}

	/* three cases, frame is good, bad, or we don't own the descriptor */
	current = info->rxCurrent;
	check = info->rxDescriptor[current].status;

	if (!(check & SiS900_DESCR_OWN)) {	// the buffer is still in use!
		TRACE(("ERROR: read: buffer %d still in use: %x\n", current, status));
		atomic_and(&info->rxLock, 0);
		*_length = 0;
		return B_BUSY;
	}

	if (check & (SiS900_DESCR_Rx_ABORT | SiS900_DESCR_Rx_OVERRUN |
				 SiS900_DESCR_Rx_LONG_PACKET | SiS900_DESCR_Rx_RUNT_PACKET |
				 SiS900_DESCR_Rx_INVALID_SYM | SiS900_DESCR_Rx_FRAME_ALIGN |
				 SiS900_DESCR_Rx_CRC_ERROR)) {
		TRACE(("ERROR read: packet with errors: %ld\n", check));
		*_length = 0;
	} else {
		// copy buffer
		size = (check & SiS900_DESCR_SIZE_MASK) - CRC_SIZE;
		if (size > MAX_FRAME_SIZE || size > *_length) {
			TRACE(("ERROR read: bad frame size %d\n", size));
			size = *_length;
		}
		memcpy(buffer, (void *)info->rxBuffer[current], size);
	}
	info->rxCurrent = (current + 1) & NUM_Rx_MASK;

	/* update indexes and buffer ownership */
	{
		cpu_status former;
		former = disable_interrupts();
		acquire_spinlock(&info->rxSpinlock);

		// release buffer to ring
		info->rxDescriptor[current].status = MAX_FRAME_SIZE + CRC_SIZE;
		info->rxFree++;

		release_spinlock(&info->rxSpinlock);
   		restore_interrupts(former);
	}

	atomic_and(&info->rxLock, 0);
	return B_OK;
}


status_t
device_write(void *data, off_t pos, const void *buffer, size_t *_length)
{
	struct sis_info *info;
	status_t status;
	uint16 frameSize;
	int16 current;
	uint32 check;

	if (checkDeviceInfo(info = data) != B_OK)
		return EINVAL;

	//TRACE(("****\t%5ld: write... %lx, %ld (%d) thread: %ld, counter = %ld\n", counter++, buf, *len, info->txLock, threadID, intrCounter));
	atomic_add(&info->txLock, 1);

	if (*_length > MAX_FRAME_SIZE)
		*_length = MAX_FRAME_SIZE;

	frameSize = *_length;
	current = info->txCurrent;

	//dprintf("\t%5ld: \twrite: tx: isr = %d, sent = %d, current = %d\n",counter++,
	//		info->txInterruptIndex,info->txSent,info->txCurrent);

	// block until a free tx descriptor is available
	if ((status = acquire_sem_etc(info->txSem, 1, B_TIMEOUT, ETHER_TRANSMIT_TIMEOUT)) < B_NO_ERROR) {
		write32((uint32)info->registers + SiS900_MAC_COMMAND, SiS900_MAC_CMD_Tx_ENABLE);
		TRACE(("write: acquiring sem failed: %x, %s\n", status, strerror(status)));
		atomic_add(&info->txLock, -1);
		*_length = 0;
		return status;
	}

	check = info->txDescriptor[current].status;
	if (check & SiS900_DESCR_OWN) {
		// descriptor is still in use
		dprintf(DEVICE_NAME ": card owns buffer %d\n", current);
		atomic_add(&info->txLock, -1);
		*_length = 0;
		return B_ERROR;
	}

	/* Copy data to tx buffer */
	memcpy((void *)info->txBuffer[current], buffer, frameSize);
	info->txCurrent = (current + 1) & NUM_Tx_MASK;

	{
		cpu_status former;
		former = disable_interrupts();
		acquire_spinlock(&info->txSpinlock);

		info->txDescriptor[current].status = SiS900_DESCR_OWN | frameSize;
		info->txSent++;

#ifdef DEBUG
		{
			struct buffer_desc *b = (void *)read32((uint32)info->registers + SiS900_MAC_Tx_DESCR);
			int16 that;

			dprintf("\twrite: status %d = %lx, sent = %d\n", current, info->txDescriptor[current].status,info->txSent);
			dprintf("write: %d: mem = %lx : hardware = %lx\n", current, physicalAddress(&info->txDescriptor[current],sizeof(struct buffer_desc)),read32((uint32)info->registers + SiS900_MAC_Tx_DESCR));

			for (that = 0;that < NUM_Tx_DESCR && (void *)physicalAddress(&info->txDescriptor[that],sizeof(struct buffer_desc)) != b;that++);

			if (that == NUM_Tx_DESCR)
			{
				//dprintf("not in ring!\n");
				that = 0;
			}
			dprintf("(hardware status %d = %lx)!\n", that, info->txDescriptor[that].status);
		}
#endif
		release_spinlock(&info->txSpinlock);
   		restore_interrupts(former);
	}

	// enable transmit state machine
	write32((uint32)info->registers + SiS900_MAC_COMMAND, SiS900_MAC_CMD_Tx_ENABLE);

#ifdef EXCESSIVE_DEBUG
	acquire_sem(gIOLock);
	bug("\t\twrite last interrupts:\n");
	{
		int ii;
		for (ii = (intrCounter-2) % 100; ii < intrCounter; ii = (ii + 1) % 100)
			bug("\t\t\t%ld: %08lx\n", ii, lastIntr[ii % 100]);
	}
	bug("\t\twrite block (%ld bytes) thread = %ld:\n", frameSize, threadID);
	dumpBlock(buf,frameSize, "\t\t\t");
	release_sem(gIOLock);
#endif

	atomic_add(&info->txLock, -1);
	return B_OK;
}

