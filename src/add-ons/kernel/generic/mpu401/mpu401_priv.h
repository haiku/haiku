/* 
 * OpenBeOS
 * A module driver for the generic mpu401 midi interface
 *
 * Copyright (c) 2003 Greg Crain 
 * All rights reserved. 
 * This software is released under the MIT/OpenBeOS license.  
 *
 * mpu401_priv.h
 */

#ifndef mpu401_H
#define mpu401_H

#include <module.h>

//Private header info

 typedef struct _mpu401device
{
	unsigned int dataport;
	unsigned int cmdport;
	unsigned int  workarounds;
	int32 count;
	bool V2;
	sem_id readsemaphore;
	sem_id writesemaphore;
	void (*interrupt_op)(int32 op, void * card);
} mpu401device;

#define MPU401_RESET    0xff
#define MPU401_UART     0x3f
#define MPU401_CMDOK    0xfe
#define MPU401_OK2WR    0x40
#define MPU401_OK2RD    0x80

#define MPUDEBUG 1 
#define MBUF_ELEMENTS    1000
/*--------------------------------*/
unsigned char Read_MPU401( unsigned int );
status_t Write_MPU401( int, unsigned char );
cpu_status lock(void);
void unlock (cpu_status);
/*--------------------------------*/
short open_count=0;
int mbuf_current=0;
int mbuf_start=0;
int mbuf_bytes=0;
unsigned char mpubuffer[MBUF_ELEMENTS];
static isa_module_info *gISA;

#endif // mpu401.h

