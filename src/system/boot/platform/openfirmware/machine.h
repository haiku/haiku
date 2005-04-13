/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef MACHINE_H
#define MACHINE_H


#include <SupportDefs.h>


#define MACHINE_UNKNOWN	0x0000

#define MACHINE_CHRP	0x0001
#define MACHINE_MAC		0x0002

#define MACHINE_PEGASOS	0x0100


extern uint32 gMachine;
	// stores the machine type

#endif	/* MACHINE_H */
