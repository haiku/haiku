//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Niels S. Reedijk
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.

// Parts of the code: Copyright (c) 1998 Be, Inc. All Rights Reserved

/* ++++++++++
	driver.c
	Implementation of the Realtek 8139 Chipset
+++++ */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <PCI.h>
#include <malloc.h>

#include "ether_driver.h"
#include "util.h"


//#define RTL_NODEBUG
#ifdef RTL_NODEBUG
#define dprintf no_printf
void no_printf( const char *useless , ... ) {};
#endif

/* ----------
	global data
----- */
static pci_info *m_device = 0;
static pci_module_info *m_pcimodule = 0;	//To call methods of pci
static int16 m_isopen = 0;					//Is the thing already opened?

int32 api_version = B_CUR_DRIVER_API_VERSION; //Procedure

//Forward declaration
static int32 rtl8139_interrupt(void *data);    /* interrupt handler */

enum registers {
	IDR0 = 0x0 ,
	Command = 0x37 ,
	TxConfig = 0x40 ,
	RxConfig = 0x44 ,
	Config1 = 0x52 , 
	BMCR = 0x62 , 		//BAsic Mode Configuration Register
	BMSR = 0x64 ,
	RBSTART = 0x30 ,
	IMR = 0x3C ,		//Interrupt mask registers
	ISR = 0x3E ,
	_9346CR = 0x50 , 	// 9346 Configuration register
	Config4 = 0x5a ,
	TSD0 = 0x10 ,
	TSD1 = 0x14 ,
	MULINT = 0x5c ,		//Multiple interrupt 
	TSD2 = 0x18 ,
	TSD3 = 0x1C ,
	ESRS = 0x36 ,
	TSAD0 = 0x20 ,
	TSAD1 = 0x24 ,
	TSAD2 = 0x28 ,
	TSAD3 = 0x2C ,
	TSAD = 0x60 ,		//Transmit Status of All Descriptors
	CAPR = 0x38 ,		//Tail pointer for buffer thingie
	CBR = 0x3A ,		//Offset in rxbuffer of next packet
	MPC = 0x4C ,
	MAR0 = 0x8			//Multicast register
	
};


enum Command_actions {
	Reset = 0x10 ,				// 5th bit 
	EnableReceive = 0x08 ,		// 4th bit
	EnableTransmit = 0x04 ,		// 3rd bit
	BUFE = 0x01
};

enum Transmitter_actions {
	MXDMA_2 = 0x400 ,			// 11th bit
	MXDMA_1 = 0x200 ,			// 10th bit
	MXDMA_0 = 0x100	,			// 9th bit
	IFG_1 = 0x2000000 ,			// 26th bit
	IFG_0 = 0x1000000 			// 25th bit
};

enum Receiver_actions {
	RXFTH2 = 0x8000, 			// 16th bit
	RXFTH1 = 0x4000, 			// 15th bit
	RXFTH0 = 0x2000 ,           // 14th bit
	RBLEN_1 = 0x1000 ,			// 13rd bit
	RBLEN_0 = 0x800	,			// 12th bit
	// MXDMA equal to transmitter
	WRAP = 0x100 ,				// 8th bit
	AB = 0x8 ,					// 4rd bit
	AM = 0x4 , 					// 3rd bit
	APM = 0x2					// 2nd bit
};

enum TransmitDescription {
	OWN = 0x2000 ,
	TOK = 0x8000
};
	

enum InterruptStatusBits {
	ReceiveOk = 0x01 ,
	ReceiveError = 0x02 ,
	TransmitOk = 0x04 ,
	TransmitError = 0x08 ,
	ReceiveOverflow = 0x10 ,
	ReceiveUnderflow = 0x20 ,
	ReceiveFIFOOverrun = 0x40 ,
	TimeOut = 0x4000 ,
	SystemError = 0x8000
};

enum BMCR_Commands {
	ANE = 0x2000 , 				// Enable auto config
	Duplex_Mode = 0x100 ,
	Restart_Auto_Negotiation = 0x400
};

typedef enum chiptype { RTL_8139_C , RTL_8139_D } chiptype;

/* ----------
	structure that stores internal data
---- */

typedef struct rtl8139_properties
{
	pci_info 	*pcii;				/* Pointer to PCI Info for the device */
	uint32 		reg_base; 			/* Base address for registers */
	chiptype	chip_type;			/* Storage for the chip type */
	
	area_id		receivebuffer;		/* Memoryaddress for the receive buffer */
	void 		*receivebufferlog;	/* Logical address */
	void		*receivebufferphy;	/* Physical address */
	uint16		receivebufferoffset;/* Offset for the next package */
	
	uint8 		writes;				/* Number of writes (0, maximum 4) */
	area_id		transmitbuffer[4];	/* Transmitbuffers */
	void		*transmitbufferlog[4]; /* Logical addresses of the transmit buffer */
	void		*transmitbufferphy[4]; /* Physical addresses of the transmit buffer */
	uint8		transmitstatus[4];	/* Transmitstatus: 0 means empty and 1 means in use */
	uint8		nexttransmitstatus; 
	
	uint8		multiset;			/* determines if multicast address is set */
	ether_address_t address;		/* holds the MAC address */
	sem_id 		lock;				/* lock this structure: still interrupt */
	sem_id		input_wait;			/* locked until there is a packet to be read */
	uint8		nonblocking;			/* determines if the card blocks on read requests */
} rtl8139_properties_t;

typedef struct packetheader
{
	volatile uint16		bits;				/* Status bits of the packet header */
	volatile uint16		length;				/* Length of the packet including header + CRC */
	volatile uint8		data[1];
} packetheader_t;

static status_t close_hook( void * );

/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *rtl8139_name[] = {
	"net/rtl8139/0",
	NULL
};

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	// Nielx: no special requirements here...
	dprintf( "rtl8139_nielx: init_hardware\n" );
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	status_t status; 		//Storage for statuses
	pci_info *item;			//Storage used while looking through pci
	int32 i;				//Counter
	
	m_device = 0; //...
	
	/*
	Nielx: Some notes
	- I will implement multiple cards later
	- For now this thing just searches for the card and use it.
	*/
	dprintf( "rtl8139_nielx: init_driver()\n" );
	
	// Try if the PCI module is loaded (it would be weird if it wouldn't, but alas)
	if( ( status = get_module( B_PCI_MODULE_NAME, (module_info **)&m_pcimodule )) != B_OK) 
	{
		dprintf( "rtl8139_nielx init_driver(): Get PCI module failed! %lu \n", status);
		return status;
	}
	
	// 
	i = 0;
	item = (pci_info *)malloc(sizeof(pci_info));
	while( true )
	{
		if ((status = m_pcimodule->get_nth_pci_info(i, item)) != B_OK)
			break;
		// Vendorid = 0x10ec and device_id = 0x8139
		if ( ( item->vendor_id == 0x10ec ) && ( item->device_id == 0x8139 ) )
		{
			//Also done in etherpci sample code
			if ((item->u.h0.interrupt_line == 0) || (item->u.h0.interrupt_line == 0xFF)) {
				dprintf( "rtl8139_nielx init_driver(): found with invalid IRQ - check IRQ assignement\n");
				i++; //next
				continue;
			}
			dprintf("rtl8139_nielx init_driver(): found at IRQ %u \n", item->u.h0.interrupt_line);
			m_device = item;
			break;
		}
		i++; //Look for the next one
	}
	
	//Check if we have found any devices:
	if ( m_device == 0 )
	{
		dprintf( "rtl8139_nielx init_driver(): no device found\n" );
		put_module(B_PCI_MODULE_NAME ); //dereference module
		return ENODEV;
	}
			
	return B_OK;
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
	free ( m_device );
	put_module( B_PCI_MODULE_NAME );
}

	
/* ----------
	open_hook - handle open() calls
----- */

static status_t
open_hook(const char *name, uint32 flags, void** cookie)
{

	rtl8139_properties_t *data;
	uint8 temp8;
	uint16 temp16;
	uint32 temp32;
	unsigned char cmd;

	dprintf( "rtl8139_nielx open_hook()\n" );
	// Check if the device name is ours
	if ( strcmp( name , rtl8139_name[0] ) != 0 )
		return EINVAL;
		
	if ( m_isopen == 1 )
	{
		dprintf( "rtl8139_nielx open_hook(): Device already in use\n" );
		return B_BUSY;
	}
	
	//We are now officially opening (don't know if it is possible that there
	//are multiple calls, however, this could prevent some ugly bugs)
	m_isopen = 1;
	
	//Create a structure that contains the internals
	if (!(*cookie = data = (rtl8139_properties_t *)malloc(sizeof(rtl8139_properties_t)))) 
	{
		dprintf( "rtl8139_nielx open_hook(): Out of memory\n" );
		m_isopen = 0;
		return B_NO_MEMORY;
	}
	
	//Clear memory
	memset( data , 0 , sizeof( rtl8139_properties_t ) );
	
	// Create lock
	data->lock = create_sem( 1 , "rtl8139_nielx data protect" );
	set_sem_owner( data->lock , B_SYSTEM_TEAM );
	data->input_wait = create_sem( 0 , "rtl8139_nielx read wait" );
	set_sem_owner( data->input_wait , B_SYSTEM_TEAM );
	
	//Set up the cookie
	data->pcii = m_device;
	
	//Enable the registers
	data->reg_base = data->pcii->u.h0.base_registers[0];
	
	/* enable pci address access */	
	cmd = m_pcimodule->read_pci_config(data->pcii->bus, data->pcii->device, data->pcii->function, PCI_command, 2);
	cmd = cmd | PCI_command_io | PCI_command_master | PCI_command_memory;
	m_pcimodule->write_pci_config(data->pcii->bus, data->pcii->device, data->pcii->function, PCI_command, 2, cmd );
	
	// Check for the chipversion -- The version bits are bits 31-27 and 24-23
	temp32 = m_pcimodule->read_io_32( data->reg_base + TxConfig );
	
	if ( temp32 == 0xFFFFFF )
	{
		dprintf( "rtl8139_nielx open_hook(): Faulty chip\n" );
		m_isopen = 0;
		put_module( B_PCI_MODULE_NAME );
		return EIO;
	}
	
	temp32 = temp32 & 0x7cc00000;

	switch( temp32 )
	{
	case 0x74000000:
		dprintf( "rtl8139_nielx open_hook(): Chip is the 8139 C\n" );
		data->chip_type = RTL_8139_C;
		break;
	
	case 0x74400000:
		dprintf( "rtl8139_niels open_hook(): Chip is the 8139 D\n" );
		data->chip_type = RTL_8139_D;
		break;
	
	default:
		dprintf( "rtl8139_nielx open_hook(): Unknown chip, assuming 8139 C\n" );
		data->chip_type = RTL_8139_C;
	}

	/* TODO: Linux driver does power management here... */

	/* Reset the chip -- command register;*/
	m_pcimodule->write_io_8( data->reg_base + Command , Reset );
	temp16 = 10000;
	while ( ( m_pcimodule->read_io_8( data->reg_base + Command ) & Reset ) && temp16 > 0 )
		temp16--;
	
	if ( temp16 == 0 )
	{
		dprintf( "rtl8139_nielx open_hook(): Reset failed... Bailing out\n" );
		m_isopen = 0;
		free( data );
		return EIO;
	}
	dprintf( "rtl8139_nielx open_hook(): Chip reset: %u \n" , temp16 );
	
	/* Enable writing to the configuration registers */
	m_pcimodule->write_io_8( data->reg_base + _9346CR , 0xc0 );

	/* Since the reset was succesful, we can immediately open the transmit and receive registers */
	m_pcimodule->write_io_8( data->reg_base + Command , EnableReceive | EnableTransmit );
			
	/* Reset Config1 register */
	m_pcimodule->write_io_8( data->reg_base + Config1 , 0 );
	
	// Turn off lan-wake and set the driver-loaded bit
	m_pcimodule->write_io_8( data->reg_base + Config1, (m_pcimodule->read_io_8(data->reg_base + Config1 )& ~0x30) | 0x20);

	// Enable FIFO auto-clear
	m_pcimodule->write_io_8( data->reg_base + Config4, m_pcimodule->read_io_8( data->reg_base + Config4) | 0x80);

	// Go to normal operation
	m_pcimodule->write_io_8( data->reg_base + _9346CR , 0 );
	
	/* Reset Rx Missed counter*/
	m_pcimodule->write_io_16( data->reg_base + MPC , 0 );
	
	/* Configure the Transmit Register */
	//settings: Max DMA burst size per Tx DMA burst is 1024 ( = 110 )
	//settings: Interframe GAP time according to IEEE standard ( = 11 )
	m_pcimodule->write_io_32( data->reg_base + TxConfig , 
			(m_pcimodule->read_io_32( data->reg_base + TxConfig )) /*| IFG_1 | 
			IFG_0*/ | MXDMA_2 | MXDMA_1 );
	
	/* Configure the Receive Register */
	//settings: Early Rx Treshold is 1024 kB ( = 110 ) DISABLED
	//settings: Max DMA burst size per Rx DMA burst is 1024 ( = 110 )
	//settings: The Rx Buffer length is 64k + 16 bytes ( = 11 )
	//settings: continue last packet in memory if it exceeds buffer length.
	m_pcimodule->write_io_32( data->reg_base + RxConfig , /*RXFTH2 | RXFTH1 | */
		RBLEN_1 | RBLEN_0 | WRAP | MXDMA_2 | MXDMA_1 | APM | AB);
	
	//Disable blocking
	data->nonblocking = 0;
	//Allocate the ring buffer for the receiver. 
	// Size is set above: as 16k + 16 bytes + 1.5 kB--- 16 bytes for last CRC (a
	data->receivebuffer = alloc_mem( &(data->receivebufferlog) , &(data->receivebufferphy) , 1024 * 64 + 16 , "rx buffer" );
	if( data->receivebuffer == B_ERROR )
	{
		dprintf( "rtl8139_nielx open_hook(): memory allocation for ringbuffer failed\n" );
		return B_ERROR;
	}
	m_pcimodule->write_io_32( data->reg_base + RBSTART , (int32) data->receivebufferphy );
	data->receivebufferoffset = 0;  //First packet starts at 0
	
	//Disable all multi-interrupts
	m_pcimodule->write_io_16( data->reg_base + MULINT , 0 );

	//Allocate buffers for transmit (There can be two buffers in one page)
	data->transmitbuffer[0] = alloc_mem( &(data->transmitbufferlog[0]) , &(data->transmitbufferphy[0]) , 4096 , "txbuffer01" );
	m_pcimodule->write_io_32( data->reg_base + TSAD0 , (int32)data->transmitbufferphy[0] );
	data->transmitbuffer[1] = data->transmitbuffer[0];
	data->transmitbufferlog[1] = data->transmitbufferlog[0] + 2048;
	data->transmitbufferphy[1] = data->transmitbufferphy[0] + 2048;
	m_pcimodule->write_io_32( data->reg_base + TSAD1 , (int32)data->transmitbufferphy[1] );
	
	data->transmitbuffer[2] = alloc_mem( &(data->transmitbufferlog[2]) , &(data->transmitbufferphy[2]) , 4096 , "txbuffer23" );
	m_pcimodule->write_io_32( data->reg_base + TSAD2 , (int32)data->transmitbufferphy[2] );
	data->transmitbuffer[3] = data->transmitbuffer[2];
	data->transmitbufferlog[3] = data->transmitbufferlog[2] + 2048;
	data->transmitbufferphy[3] = data->transmitbufferphy[2] + 2048;
	m_pcimodule->write_io_32( data->reg_base + TSAD3 , (int32)data->transmitbufferphy[3] );
	
	if( data->transmitbuffer[0] == B_ERROR || data->transmitbuffer[2] == B_ERROR )
	{
		dprintf( "rtl8139_nielx open_hook(): memory allocation for transmitbuffer failed\n" );
		return B_ERROR;
	}

	data->nexttransmitstatus = 0;
		
	// Receive hardware MAC address
	// Suggestion by Marcus Overhagen: Make it a nice for loop...
	temp8 = 0;
	do 
	{
		data->address.ebyte[ temp8 ] = m_pcimodule->read_io_8( data->reg_base + IDR0 + temp8  );
		temp8++;
	} while ( temp8 < 6 );
	
	dprintf( "rlt8139_nielx open_hook(): MAC address: %x:%x:%x:%x:%x:%x\n",
	data->address.ebyte[0] , data->address.ebyte[1] , data->address.ebyte[2] ,
	data->address.ebyte[3] , data->address.ebyte[4] , data->address.ebyte[5] );
	
	/* Receive physical match packets and broadcast packets */
	m_pcimodule->write_io_32( data->reg_base + RxConfig ,
			(m_pcimodule->read_io_32( data->reg_base + RxConfig )) | APM | AB );
	
	//Clear multicast mask
	m_pcimodule->write_io_32( data->reg_base + MAR0 , 0 );
	m_pcimodule->write_io_32( data->reg_base + MAR0 + 4 , 0 );
	

	/* We want interrupts! */
	if ( install_io_interrupt_handler( data->pcii->u.h0.interrupt_line , rtl8139_interrupt , data , 0 ) != B_OK )
	{
		dprintf( "rtl8139_nielx open_hook(): Error installing interrupt handler\n" );
		return B_ERROR;
	}

	m_pcimodule->write_io_16( data->reg_base + IMR , 
		ReceiveOk | ReceiveError | TransmitOk | TransmitError | 
		ReceiveOverflow | ReceiveUnderflow | ReceiveFIFOOverrun |
		TimeOut | SystemError );
	      
	/* Enable once more */
	m_pcimodule->write_io_8( data->reg_base + _9346CR , 0 );
	m_pcimodule->write_io_8( data->reg_base + Command , EnableReceive | EnableTransmit );
	
	//Check if Tx and Rx are enabled
	if( !( m_pcimodule->read_io_8( data->reg_base + Command ) & EnableReceive ) || !( m_pcimodule->read_io_8( data->reg_base + Command ) & EnableTransmit ) )
		dprintf( "TRANSMIT AND RECEIVE NOT ENABLED!!!\n" );
	else
		dprintf( "TRANSMIT AND RECEIVE ENABLED!!!\n" );
		
	dprintf( "rtl8139_nielx open_hook(): Basic Mode Status Register: 0x%x ESRS: 0x%x\n" ,
		m_pcimodule->read_io_16( data->reg_base + BMSR ) , 
		m_pcimodule->read_io_8( data->reg_base + ESRS ) );
		
	return B_OK;
}


/* ----------
	read_hook - handle read() calls
----- */

static status_t
read_hook (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	rtl8139_properties_t *data =/* (rtl8139_properties_t *)*/cookie;
	packetheader_t *packet_header;
	
	dprintf( "rtl8139_nielx: read_hook()\n" );

	//if( !data->nonblocking )
		acquire_sem_etc( data->input_wait , 1 , B_CAN_INTERRUPT , 0 );
	
	//Next: check in command register if there's actually anything to be read
	if ( m_pcimodule->read_io_8( data->reg_base + Command ) & BUFE  )
	{
		dprintf( "rtl8139_nielx read_hook: Nothing to read!!!\n" );
		return B_IO_ERROR;
	}
	
	// Retrieve the packet header
	packet_header = (packetheader_t *) ( ( uint8 *)data->receivebufferlog + data->receivebufferoffset );
	
	// Check if the transfer is already done: EarlyRX
	if ( packet_header->length == 0xfff0 )
	{
		dprintf( "rtl8139_nielx read_hook: The transfer is not yet finished!!!\n" );
		return B_IO_ERROR;
	}
	
	//Check for an error: if needed: resetrx, length may not be bigger than 1514 + 4 CRC
	if ( !( packet_header->bits & 0x1 ) || packet_header->length > 1518 )
	{
		dprintf( "rtl8139_nielx read_hook: Error in package reception: bits: %u length %u!!!\n" , packet_header->bits , packet_header->length);
		return B_IO_ERROR;
	}
	
	dprintf( "rtl8139_nielx read_hook(): Packet size: %u Receiveheader: %u Buffer size: %lu\n" , packet_header->length , packet_header->bits , *num_bytes );
	
	//Copy the packet
	*num_bytes = packet_header->length - 4;
	if ( data->receivebufferoffset + *num_bytes > 65536 )
	{
		//Packet wraps around , copy last bits except header ( = +4 )
		memcpy( buf , data->receivebufferlog + data->receivebufferoffset + 4 , 0x10000 - ( data->receivebufferoffset + 4 ) ); 
		//copy remaining bytes from the beginning
		memcpy( buf + 0x10000 - ( data->receivebufferoffset + 4 ) , data->receivebufferlog , *num_bytes - (0x10000 - ( data->receivebufferoffset + 4 ) ) );
		dprintf( "rtl8139_nielx read_hook: Wrapping around end of buffer\n" );
	}
	else
		memcpy( buf , data->receivebufferlog + data->receivebufferoffset + 4 , packet_header->length - 4);  //length-4 because we don't want to copy the 4 bytes CRC
	
	//Update the buffer -- 4 for the header length, plus 3 for the dword allignment
	data->receivebufferoffset = ( data->receivebufferoffset + packet_header->length + 4 + 3 ) & ~3;
			
	m_pcimodule->write_io_16( data->reg_base + CAPR , data->receivebufferoffset - 16 ); //-16, avoid overflow
	
	dprintf( "rtl8139_nielx read_hook(): CBP %u CAPR %u \n" , m_pcimodule->read_io_16( data->reg_base + CBR ) , m_pcimodule->read_io_16( data->reg_base + CAPR ) );
	
	// Re-enable interrupts
	m_pcimodule->write_io_16( data->reg_base + ISR , ReceiveOk );
	m_pcimodule->write_io_16( data->reg_base + IMR , 
		ReceiveOk | ReceiveError | TransmitOk | TransmitError | 
		ReceiveOverflow | ReceiveUnderflow | ReceiveFIFOOverrun |
		TimeOut | SystemError );
	return packet_header->length - 4;
}


/* ----------
	rtl8139_write - handle write() calls
----- */

static status_t
write_hook (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	rtl8139_properties_t *data =/* (rtl8139_properties_t *)*/cookie;
	int buflen = *num_bytes;
	int transmitid = 0;
	uint32 transmitdescription = 0;

	dprintf( "rtl8139_nielx write_hook()\n" );
	
	acquire_sem_etc( data->lock , 1 , B_CAN_INTERRUPT, 0);
	
	if ( data->writes == 4 )
	{
		dprintf( "rtl8139_nielx write_hook(): already doing four writes\n" );
		release_sem_etc( data->lock , 1 , B_DO_NOT_RESCHEDULE );
		return B_INTERRUPTED;
	}

	if ( buflen > 1792 ) //Maximum of 1792 bytes
	{
		dprintf( "rtl8139_nielx write_hook(): packet is too long\n" );
		release_sem_etc( data->lock , 1 , B_DO_NOT_RESCHEDULE );
		return B_IO_ERROR;
	}
		
	// We need to determine a free transmit descriptor
	if ( data->transmitstatus[ data->nexttransmitstatus ] == 1 )
	{
		//No free descriptor]
		release_sem_etc( data->lock , 1 , B_DO_NOT_RESCHEDULE );
		return B_IO_ERROR;
	}
	
	//Update our current transmit id
	transmitid = data->nexttransmitstatus;
	if ( data->nexttransmitstatus == 3 )
		data->nexttransmitstatus = 0;
	else
		data->nexttransmitstatus++;
	
	dprintf( "rtl8139_nielx write_hook(): TransmitID: %u Packagelen: %u Register: %lx\n" , transmitid , buflen , TSD0 + (sizeof(uint32) * transmitid ) );
	
	if ( transmitid == -1 )
	{
		dprintf( "rtl8139_nielx_write_hook(): no free buffer ?!?\n" );
		return B_IO_ERROR;
	}
	
	data->writes++;
	// Set the buffer as used
	data->transmitstatus[transmitid] = 1;
	
	release_sem_etc( data->lock , 1 , B_DO_NOT_RESCHEDULE );
	
	//Copy the packet into the buffer
	memcpy( data->transmitbufferlog[transmitid] , buffer , buflen );
	
	if ( buflen < 60 )
		buflen = 60;
	
	//Clear OWN and start transfer Create transmit description with early Tx FIFO, size
	transmitdescription = ( buflen | 0x80000 | transmitdescription ) ^OWN; //0x80000 = early tx treshold
	dprintf( "rtl8139_nielx write: transmitdescription = %lu\n" , transmitdescription );
	m_pcimodule->write_io_32( data->reg_base + TSD0 + (sizeof(uint32) * transmitid ) , transmitdescription );

	dprintf( "rtl8139_nielx write: TSAD: %u\n" , m_pcimodule->read_io_16( data->reg_base + TSAD ) );

	//Done
	return B_OK;
}


/* ----------
	rtl8139_control - handle ioctl calls
----- */

static status_t
control_hook (void* cookie, uint32 op, void* arg, size_t len)
{
	rtl8139_properties_t *data = (rtl8139_properties_t *)cookie;
	ether_address_t address;
	dprintf( "rtl8139_nielx control_hook()\n" );
	
	
	switch ( op )
	{	
	case ETHER_INIT:
		dprintf( "rtl8139_nielx control_hook(): Wants us to init... ;-)\n" );
		return B_NO_ERROR;
		
	case ETHER_GETADDR:
		if ( data == NULL )
			return B_ERROR;
			
		dprintf( "rtl8139_nielx control_hook(): Wants our address...\n" );
		memcpy( arg , (void *) &(data->address) , sizeof( ether_address_t ) );
		return B_OK;
	
	case ETHER_ADDMULTI:
		if (data == NULL )
			return B_ERROR;
		//Check if the maximum of multicast addresses isn't reached
		if ( data->multiset == 8 )
			return B_ERROR;
			
		dprintf( "rtl8139_nielx control_hook(): Add multicast...\n" );
		memcpy( &address , arg , sizeof( address ) );
		dprintf( "Multicast address: %i %i %i %i %i %i \n" , address.ebyte[0] ,
		address.ebyte[1] , address.ebyte[2] , address.ebyte[3] , address.ebyte[4] ,
		address.ebyte[5] );
		return B_OK;

		/*data->multiset;
		m_pcimodule->write_io_32( MAR0 , address[0] ); //First 32 bits
		m_pcimodule->write_io_32( MAR0 + 0x4 , address
		*/
	
	case ETHER_NONBLOCK:
		if ( data == NULL )
			return B_ERROR;
		
		dprintf( "rtl8139_nielx control_hook(): Wants to set block/nonblock\n" );
		memcpy( &data->nonblocking , arg , sizeof( data->nonblocking ) );
		return B_NO_ERROR;
		
	case ETHER_REMMULTI:
		dprintf( "rtl8139_nielx control_hook(): Wants REMMULTI\n" );
		return B_OK;
		
	case ETHER_SETPROMISC:
		dprintf("rtl8139_nielx control_hook(): Wants PROMISC\n" );
		return B_OK;
	
	case ETHER_GETFRAMESIZE:
		dprintf("rtl8139_nielx control_hook(): Wants GETFRAMESIZE\n" ) ;
		*( (unsigned int *)arg ) = 1514;
		return B_OK;
	}
	return B_BAD_VALUE;
}

/* ----------
	interrupt_handler - handle issued interrupts
----- */

static int32
rtl8139_interrupt( void *cookie )
{
	rtl8139_properties_t *data = (rtl8139_properties_t *)cookie;
	uint8 temp8;
	uint16 isr_contents;
	uint16 isr_write = 0;
	uint32 txstatus;
	int32 retval = B_UNHANDLED_INTERRUPT; 
	cpu_status status;
	
	status = lock();
	
	isr_contents = m_pcimodule->read_io_16( data->reg_base + ISR );
	dprintf( "NIELX INTERRUPT: %u \n" , isr_contents );
	if( isr_contents & ReceiveOk )
	{
		dprintf( "rtl8139_nielx interrupt ReceiveOk\n" );
		release_sem_etc( data->input_wait , 1 , B_DO_NOT_RESCHEDULE );
		// First, disable all interrupts until the read hook is finished. It will re-enable them.
		m_pcimodule->write_io_16( data->reg_base + IMR , 0 );
		retval = B_INVOKE_SCHEDULER;
	}
	
	if (isr_contents & 	ReceiveError )
	{
		//Do something
		;
	}
	
	if (isr_contents & TransmitOk )
	{
		// Check each status descriptor
		for (temp8 = 0 ; temp8 < 4 ; temp8++)
		{
			// If a register isn't used, continue next run
			if ( data->transmitstatus[temp8] != 1 )
				continue;
			txstatus = m_pcimodule->read_io_32( data->reg_base + TSD0 + temp8 * sizeof( int32 ) );
			dprintf( "run: %u txstatus: %lu Register: %lx\n" , temp8 , txstatus , TSD0 + temp8 * sizeof( int32 ) );
			
			//m_pcimodule->write_io_32( data->reg_base + TSAD0 + temp8 * sizeof( int32) , (m_pcimodule->read_io_32( data->reg_base + TSAD0 + temp8 * sizeof( int32 ) ) ) | OWN ) ;
			
			if ( ( txstatus & TOK ) )
			{
				//this one is the one!
				dprintf( "NIELX INTERRUPT: TXOK, clearing register %u\n" , temp8 );
				data->transmitstatus[temp8] = 0; //That's all there is to it
				data->writes--;
				//update next transmitid
				continue;
			}

		}
		isr_write |= TransmitOk;
		retval = B_HANDLED_INTERRUPT;
	}
	
	if( isr_contents & TransmitError )
	{
		//
		;
	}
	
	if( isr_contents & ReceiveOverflow )
	{
		// Discard all the current packages to be processed -- newos driver
		m_pcimodule->write_io_16( data->reg_base + CAPR , ( m_pcimodule->read_io_16( CBR ) + 16 ) % 0x1000 );
		isr_write |= ReceiveOverflow;
		retval = B_HANDLED_INTERRUPT;
	}
	
	if( isr_contents & ReceiveUnderflow )
	{
		// Most probably a link change -> TODO CHECK!
		isr_write |= ReceiveUnderflow;
		dprintf( "rtl8139_nielx interrupt(): BMCR: 0x%x BMSR: 0x%x\n" ,
			m_pcimodule->read_io_16( data->reg_base + BMCR ) ,
			m_pcimodule->read_io_16( data->reg_base + BMSR ) );
		retval = B_HANDLED_INTERRUPT;
	}
	
	if ( isr_contents & ReceiveFIFOOverrun )
	{
		//
		;
	}
	
	if ( isr_contents & TimeOut )
	{
		//
		;
	}
	
	if ( isr_contents & SystemError )
	{
		//
		;
	}
	
	m_pcimodule->write_io_16( data->reg_base + ISR , isr_write );

	unlock( status );
	
	return retval;	
}

/* ----------
	close_hook - handle close() calls
----- */

static status_t
close_hook (void* cookie)
{
	rtl8139_properties_t * data = (rtl8139_properties_t *) cookie;
	//Stop Rx and Tx process
	m_pcimodule->write_io_8( data->reg_base + Command , 0 );
	m_pcimodule->write_io_16( data->reg_base + IMR , 0 );
	return B_OK;
}


/* -----
	rtl8139_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
rtl8139_free (void* cookie)
{
	rtl8139_properties_t *data = (rtl8139_properties_t *) cookie;

	dprintf( "rtl8139_nielx free_hook()\n" );
	
	//Remove interrupt handler
	remove_io_interrupt_handler( data->pcii->u.h0.interrupt_line , 
	                             rtl8139_interrupt , cookie );
	
	//Free Rx and Tx buffers
	delete_area( data->receivebuffer );
	delete_area( data->transmitbuffer[0] );
	delete_area( data->transmitbuffer[2] );
	
	//Finally, free the cookie
	free( data );
	
	//Put the pci module
	put_module( B_PCI_MODULE_NAME );
	
	//To conclude: set the status of the device to closed
	m_isopen = 0;
	
	return B_OK;
}


/* -----
	function pointers for the device hooks entry points
----- */

device_hooks rtl8139_hooks = {
	open_hook,	 			/* -> open entry point */
	close_hook, 			/* -> close entry point */
	rtl8139_free,			/* -> free cookie */
	control_hook, 			/* -> control entry point */
	read_hook,			/* -> read entry point */
	write_hook				/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	return rtl8139_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &rtl8139_hooks;
}
