/*
* Copyright 2003-2006, Haiku.
* Distributed under the terms of the MIT License.
*
* A module driver for the generic mpu401 midi interface.
*
* Author:
*		Greg Crain (gsc70@comcast.net)
*
*  mpu401.c
*/

#include <ISA.h>
#include <KernelExport.h>
#include <OS.h>
#include <midi_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "mpu401_priv.h"

/* ----------
	midi_create_device -  
----- */
/*-----------------------------*/
/*  Version 1 of mpu401 module */
/*-----------------------------*/

static status_t 
create_device(int port, void ** out_storage, uint32 workarounds, void (*interrupt_op)(int32 op, void * card), void * card)
{
	mpu401device mpu_device;
	mpu401device *mpuptr;

	/* fill the structure with specific info from caller */
	mpu_device.addrport = port;
	mpu_device.workarounds = workarounds;
	mpu_device.V2 = FALSE;
	mpu_device.count = 1;
	mpu_device.interrupt_op = interrupt_op;
	mpu_device.card = card; 

	LOG(("create_device count= %ld, addrport 0x%x, workarounds: %d",mpu_device.count, mpu_device.addrport, mpu_device.workarounds));

	// basically, each call to create device allocates memory for 
	//a structure with the specific device info. The pointer to this is 
	// returned back to calling driver
	mpuptr = (mpu401device*)malloc (sizeof(mpu401device));
	memcpy(mpuptr, &mpu_device, sizeof(mpu_device));
	*out_storage = (void *)mpuptr;
	
	return B_OK;
}
/*-----------------------------*/
/*  Version 2 of mpu401 module */
/*-----------------------------*/

static status_t 
create_device_v2(int port, void ** out_storage, uint32 workarounds, void (*interrupt_op)(int32 op, void * card), void * card)
{
	mpu401device *mpuptr;
	mpu401device mpu_device;

	LOG(("v2: create_device, **out_storage %p ,workarounds %ld ,void *card %p\n",out_storage, workarounds, card));

	// not sure exactly how v2 of the module works.  I think that two ports are
	// created.  One for midi in data, and another for midi out data.
	// Instead of transfering data using a buffer and pointer, the midi
	// data is transfered via the global ports.
	// If the ports are created in the midi server, then the port id's 
	// should be known in this hook call.
	// If the ports are created in this hook call, the the port id's 
	// should be returned to the midi server.

	// One version of read/write hook functions are used for both v1, v2.  Therefore, in 
	// those calls, it needs to be known whether the mididata is to be read/written 
	// to a buffer, or to the port.

 	mpu_device.addrport = port;
	mpu_device.workarounds = workarounds;
	mpu_device.V2 = TRUE; 
	mpu_device.count =1; 
	mpu_device.card = card; 


	mpuptr = (mpu401device*)malloc(sizeof(mpu401device));
	memcpy(mpuptr, &mpu_device, sizeof(mpu_device));
	*out_storage = (void *)mpuptr;

	return B_OK;
}


/* ----------
	midi_delete_device 
----- */
static status_t 
delete_device(void * storage)
{	
	mpu401device * mpu_device = (mpu401device *)storage;

	LOG(("device->addrport= 0x%x count= %ld\n",mpu_device->addrport, mpu_device->count));
	LOG(("delete_device: *storage:%p\n",storage));

	free(storage);   // free the memory allocated in create_device 
		
	return B_OK;
}

/* ----------
	midi_open - handle open() calls
----- */

static status_t 
midi_open(void * storage, uint32 flags, void ** out_cookie)
{
	char semname[25];
	int ack_byte; 
	mpu401device * mpu_device = (mpu401device *)storage;

	LOG(("open()  flags: %ld, * storage: %p, **out_cookie: %p\n",flags, storage,out_cookie));
	LOG(("open:  device->addrport 0x%x ,workarounds 0x%x\n",mpu_device->addrport, mpu_device->workarounds));

	// the undocumented V2 module is not complete
	// we will allow the device to be created since some drivers depend on it
	// but will return an error if the actual midi device is opened:
	if ( mpu_device->V2 == TRUE)
		return B_ERROR;
    
	switch (mpu_device->workarounds){
		case 0x11020004:   // This is still required for Creative Audigy, Audidy2
		case 0x11020005:
		case 0:
			// don't know the current mpu state
			PRINT(("reset MPU401\n"));
			Write_MPU401(mpu_device->addrport, UARTCMD, mpu_device->workarounds, MPU401_RESET);         
			snooze(30000);
			Write_MPU401(mpu_device->addrport, UARTCMD, mpu_device->workarounds, MPU401_RESET);         
			snooze(30000);
			ack_byte = Read_MPU401(mpu_device->addrport, UARTDATA, mpu_device->workarounds);
			PRINT(("enable UART mode\n"));
			Write_MPU401(mpu_device->addrport, UARTCMD, mpu_device->workarounds, MPU401_UART);   
			snooze(30000);
			ack_byte = Read_MPU401(mpu_device->addrport, UARTDATA, mpu_device->workarounds );  
			PRINT(("port cmd ack is 0x%x\n", ack_byte));
			*out_cookie = mpu_device;
			break;
		case 1:
			//  Some devices are always in UART mode 
			PRINT(("already in UART mode\n"));
  			break;
		default:
			PRINT(("Unknown workaround value: %d\n", mpu_device->workarounds));
			break;
	}  //end switch

	// Create Read semaphore for midi-in data
	sprintf(semname, "mpu401:%04x:read_sem", mpu_device->addrport);
	mpu_device->readsemaphore = create_sem(0, semname);
  	
	// Create Write semaphore for midi-out data
	sprintf(semname,"mpu401:%04x:write_sem", mpu_device->addrport);
	mpu_device->writesemaphore = create_sem(1, semname);
	    
	// clear midi-in buffer
	mbuf_bytes=0;   
	mbuf_current=0;
	mbuf_start=0;

  	//Enable midi interrupts
	mpu_device->interrupt_op(B_MPU_401_ENABLE_CARD_INT, mpu_device->card );
         
	if ((mpu_device->readsemaphore > B_OK)
		&& (mpu_device->writesemaphore > B_OK)) {
		atomic_add(&mpu_device->count, 1);
		PRINT(("midi_open() done (count = %x)\n", open_count));
		return B_OK;
	} 
	return B_ERROR;
}

/* ----------
	midi_close - handle close() calls
----- */
static status_t 
midi_close(void * cookie)
{
	mpu401device * mpu_device = (mpu401device *)cookie;

	if (mpu_device->count <= 0)
 	   return B_ERROR;

	//Disable soundcard midi interrupts
	mpu_device->interrupt_op(B_MPU_401_DISABLE_CARD_INT, mpu_device->card );

	// Delete the semaphores
	delete_sem(mpu_device->readsemaphore);
	delete_sem(mpu_device->writesemaphore);

  	atomic_add(&mpu_device->count, -1); 
  	PRINT(("midi_close() done (count = %lu)\n", mpu_device->count));

	return B_OK;
}

/* ----------
	midi_free - free up allocated memory
----- */
static status_t 
midi_free(void * cookie)
{
	LOG(("midi_free()\n"));
	return B_OK;
}

/* ----------
	midi_control - handle control() calls
----- */
static status_t 
midi_control(void * cookie, uint32 op, void * data, size_t len)
{
	//mpu401device *mpu_device = (mpu401device *)cookie;

	/* I don't think this is ever called ...*/
	LOG(("midi_control()\n"));
	return B_OK;
}


/* ----------
	midi_read - handle read() calls
----- */
static status_t 
midi_read(void *cookie, off_t pos, void *buffer, size_t *num_bytes)
{
	/* The actual midi data is read from the device in the interrupt handler;
	   this reads and returns the data from a buffer */

	unsigned char *data;	   
	unsigned int i;
	size_t count;
	cpu_status status;
	status_t bestat;
	mpu401device *mpu_device = (mpu401device *)cookie;

 	data = (unsigned char*)buffer;
 	count = *(num_bytes);

	i=0;
	*num_bytes=0;
	bestat = acquire_sem_etc ( mpu_device->readsemaphore, 1, B_CAN_INTERRUPT, 0);
	if (bestat == B_INTERRUPTED) {
		//PRINT(("acquire_sem B_INTERRUPTED!\n"));
		return B_INTERRUPTED;   
	}
	if (bestat != B_OK) {
		TRACE(("acquire_sem not B_OK %d\n",(int)bestat));
		*num_bytes = 1;
		return B_INTERRUPTED;   
	}
	if (bestat == B_OK) {
		status = lock();
		*(data+i) = mpubuffer[mbuf_start];
		i++;
		mbuf_start++;	// pointer to data in ringbuffer
		if (mbuf_start >= (MBUF_ELEMENTS-1))
			mbuf_start = 0; 		//wraparound of ringbuffer
		*num_bytes =1;	// tell caller how many bytes are being returned in buffer
		if (mbuf_bytes>0)
			mbuf_bytes--;	// bytes read from buffer, so decrement buffer count
   		unlock(status);
   		//PRINT(("bytes in buffer: %d\n",mbuf_bytes));
	}	

	return B_OK;
}

/* ----------
	midi_write - handle write() calls
----- */
static status_t 
midi_write(void * cookie, off_t pos, const void * data, size_t * num_bytes)
{
	unsigned char *bufdata;
	uint32 i;
	size_t count;

	mpu401device *mpu_device = (mpu401device *)cookie;
	bufdata = (unsigned char*)data;	/* Pointer to midi data buffer */ 
	count = *num_bytes;
	
	/* Only for deep debugging..will slow things down */
	//PRINT(("write %d bytes,  addrport 0x%x, workarounds 0x%x\n",(int)count,mpu_device->addrport, mpu_device->workarounds));

	acquire_sem(mpu_device->writesemaphore);
	for (i=0; i<count; i++) {
		// wait until device is ready
		while ((Read_MPU401(mpu_device->addrport, UARTCMD, mpu_device->workarounds) & MPU401_OK2WR));
			Write_MPU401(mpu_device->addrport, UARTDATA, mpu_device->workarounds, *(bufdata+i));	
	}  	  

	*num_bytes = 0;	
	release_sem(mpu_device->writesemaphore);

	return B_OK;
}
/* ----------
	interrupt_hook - handle interrupts for mpu401 data 
----- */
static bool 
interrupt_hook(void * cookie)
{
	status_t bestat;
	mpu401device *mpu_device = (mpu401device *)cookie;
	
	//PRINT(("irq! port: 0x%x\n",mpu_device->addrport));  /* Only for deep debugging..will slow things down */

	/* Input data is available when bit 7 of the Status port is zero. Conversely, when bit 7 is
	  is a one, no MIDI data is available.  Reading from the data port will often clear the
	  interrupt signal depending on the sound card. */ 
	
	if ((Read_MPU401(mpu_device->addrport, UARTCMD, mpu_device->workarounds) & MPU401_OK2RD) == 0) {
		/* Okay, midi data waiting to be read from device */
		if (mbuf_current >= (MBUF_ELEMENTS-1))
			mbuf_current = 0;
			    
		mpubuffer[mbuf_current] = Read_MPU401(mpu_device->addrport, 
			UARTDATA, mpu_device->workarounds);	/* store midi data byte into buffer  */
		mbuf_current++;					/* pointer to next blank byte */
		mbuf_bytes++;					/* increment count of midi data bytes */
			
		bestat = release_sem_etc(mpu_device->readsemaphore, 1, B_DO_NOT_RESCHEDULE);
				       
		return TRUE;   //B_INVOKE_SCHEDULER
	} 
	/* No midi data from this interrupt */
	return FALSE;  //B_UNHANDLED_INTERRUPT 
}

/*-----------------------------------------------------------------*/

uchar 
Read_MPU401(unsigned int addrport, const char cmdtype, unsigned int workarounds)
{
	uchar mpudatabyte;
	cpu_status status;
	unsigned int regptr;
	
	//PRINT(("read workaround 0x%x\n",workarounds));  /* Only for deep debugging..will slow things down */
	switch (workarounds) {
		case 0x11020004: /* Creative Audigy Gameport */
			regptr = (((I_MPU1 + cmdtype) << 16) & PTR_ADDRESS_MASK);
			status = lock(); 
			gPCI->write_io_32(addrport + D_PTR, regptr);  /*DATA or CMD */
			mpudatabyte = gPCI->read_io_32(addrport + D_DATA);
			unlock(status);
			break;

		case 0x11020005:  /* Creative Audigy LiveDrive */
			regptr = (((I_MPU2 + cmdtype) << 16) & PTR_ADDRESS_MASK);
			status = lock(); 
			gPCI->write_io_32(addrport + D_PTR, regptr);  /*DATA2 or CMD2 */
			mpudatabyte = gPCI->read_io_32(addrport + D_DATA);
			unlock(status);
			break;

		default:
			mpudatabyte = gISA->read_io_8(addrport + cmdtype );
			break;
	}
	return mpudatabyte;	
}


status_t 
Write_MPU401(unsigned int addrport, const char cmdtype, unsigned int workarounds, uchar mpudatabyte )
{
	cpu_status status;
	unsigned int regptr;
	
	/* Only for deep debugging..will slow things down */
	//PRINT(("write workaround 0x%x at addr: 0x%x\n",workarounds,addrport));  
	switch (workarounds)  {
		case 0x11020004:  /* Creative Audigy Gameport */
			regptr = (((I_MPU1 + cmdtype ) << 16) & PTR_ADDRESS_MASK);
			status = lock(); 
			gPCI->write_io_32(addrport + D_PTR, regptr);  /*DATA or CMD */
			gPCI->write_io_32(addrport + D_DATA, mpudatabyte );  
			unlock(status);
			break;

		case 0x11020005:  /* Creative Audigy LiveDrive */
			regptr = (((I_MPU2 + cmdtype ) << 16) & PTR_ADDRESS_MASK);
			status = lock(); 
			gPCI->write_io_32(addrport + D_PTR, regptr);  /*DATA2 or CMD2 */
			gPCI->write_io_32(addrport + D_DATA, mpudatabyte );  
			unlock(status);
			break;

		default:
			gISA->write_io_8(addrport + cmdtype, mpudatabyte);
			break;
	}
	return B_OK;	
}


/*-----------------------------------------------------------------*/

static status_t
std_ops(int32 op, ...)
{
	switch(op) {
	
	case B_MODULE_INIT:

		LOG_CREATE();
		PRINT(("B_MODULE_INIT\n"));

		if (get_module(B_ISA_MODULE_NAME, (module_info **)&gISA) < B_OK)
			return B_ERROR;
 		if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI) < B_OK)
			return B_ERROR;
		return B_OK;
	
	case B_MODULE_UNINIT:
		put_module(B_ISA_MODULE_NAME);
		put_module(B_PCI_MODULE_NAME);
		PRINT(("B_MODULE_UNINIT\n"));
		return B_OK;
	
	default:
		return B_ERROR;
	}
}

static generic_mpu401_module mpu401_module = 
{
	{
		B_MPU_401_MODULE_NAME,
		B_KEEP_LOADED /*0*/ ,
		std_ops
	},
	create_device,
	delete_device,
	midi_open,
	midi_close,
	midi_free,
	midi_control,
	midi_read,
	midi_write,
	interrupt_hook
};

// Module v2 seems to be undocumented 
static generic_mpu401_module mpu401_module2 = 
{
	{
		"generic/mpu401/v2",
		0,
		std_ops
	},
	create_device_v2,
	delete_device,
	midi_open,
	midi_close,
	midi_free,
	midi_control,
	midi_read,
	midi_write,
	interrupt_hook
};

_EXPORT generic_mpu401_module *modules[] =
{
	&mpu401_module,
	&mpu401_module2,
	NULL
};

spinlock locked = B_SPINLOCK_INITIALIZER;
cpu_status 
lock(void)
{
	cpu_status status = disable_interrupts();
	acquire_spinlock(&locked);
	return status;
}

void 
unlock(cpu_status status)
{
	 release_spinlock(&locked);
	 restore_interrupts(status);
}

