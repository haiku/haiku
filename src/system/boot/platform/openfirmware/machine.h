/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MACHINE_H
#define MACHINE_H


#include <SupportDefs.h>


// Possible gMachine OpenFirmware platforms
#define MACHINE_UNKNOWN	0x0000

#define MACHINE_CHRP	0x0001
#define MACHINE_MAC		0x0002

#define MACHINE_PEGASOS	0x0100
#define MACHINE_QEMU	0x0200
#define MACHINE_SPARC	0x0300


extern uint32 gMachine;
	// stores the machine type

#endif	/* MACHINE_H */
