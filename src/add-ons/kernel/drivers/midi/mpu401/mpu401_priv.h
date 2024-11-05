/*
* Copyright 2003-2006, Haiku.
* Distributed under the terms of the MIT License.
*
* A module driver for the generic mpu401 midi interface.
*
* Author:
*		Greg Crain (gsc70@comcast.net)
*
* mpu401_priv.h
*/

#ifndef mpu401_priv_H
#define mpu401_priv_H

#include <module.h>
#include <PCI.h>

/*--------------------------------*/
/* UART Definitions               */

#define MPU401_RESET    0xff
#define MPU401_UART     0x3f
#define MPU401_CMDOK    0xfe
#define MPU401_OK2WR    0x40
#define MPU401_OK2RD    0x80
// Typically,
//  UART data port = addr
//  UART cmd port = addr +1
#define UARTDATA		0
#define UARTCMD			1

#define MBUF_ELEMENTS    1000  //Input buffer size

/*--------------------------------*/
/* Private definitions            */

typedef struct _mpu401device {
	unsigned int addrport;
	unsigned int  workarounds;
	int32 count;
	bool V2;
	sem_id readsemaphore;
	sem_id writesemaphore;
	void (*interrupt_op)(int32 op, void * card);
	void *card;
} mpu401device;

int mbuf_current;
int mbuf_start;
int mbuf_bytes;
unsigned char mpubuffer[MBUF_ELEMENTS];
static int open_count;
static isa_module_info *gISA;
static pci_module_info *gPCI;


/*--------------------------------*/
/* Function prototypes            */

unsigned char Read_MPU401(unsigned int, const char, unsigned int);
status_t Write_MPU401(unsigned int, const char, unsigned int, uchar);

cpu_status lock(void);
void unlock (cpu_status);

/********************/
/*   Workarounds    */
/********************/
// This is a list of workarounds in order to  
// get a specific manufacturer, card vendor, or UART type  
// to work nicely with the generic module.
// The defined workaround value must match the value that
// is used in the driver 'create_device' call.


/****************************/
/* Creative Audigy, Audigy2 */ 
/****************************/
/* 0x11020004   Gameport Midi connector       */
/* 0x11020005   LiveDrive Expansion connector */
/* IN the create_device hook, use the PCI base*/
/* address, instead of midi address port      */
#define PTR_ADDRESS_MASK 0x0fff0000
#define D_DATA 0x04
#define D_PTR  0x00
#define I_MPU1 0x70
#define I_MPU2 0x72


#endif // mpu401_priv_H

