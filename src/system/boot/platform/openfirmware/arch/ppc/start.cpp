/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Alexander von Gluck, kallisti5@unixzen.com
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */

#include "start.h"

#include <string.h>
#include <OS.h>
#include <platform/openfirmware/openfirmware.h>

#include "machine.h"

extern "C" void _start(uint32 _unused1, uint32 _unused2,
	void *openFirmwareEntry);

// XCOFF "entry-point" is actually a pointer to the real code
extern "C" void *_coff_start;
void *_coff_start = (void *)&_start;

// GCC defined globals
extern uint8 __bss_start;
extern uint8 _end;


static void
clear_bss(void)
{
	memset(&__bss_start, 0, &_end - &__bss_start);
}


void
determine_machine(void)
{
	gMachine = MACHINE_UNKNOWN;

	intptr_t root = of_finddevice("/");
	char buffer[64];
	int length;

	// TODO : Probe other OpenFirmware platforms and set gMachine as needed

	if ((length = of_getprop(root, "device_type", buffer, sizeof(buffer) - 1))
		!= OF_FAILED) {
		buffer[length] = '\0';
		if (!strcasecmp("chrp", buffer))
			gMachine = MACHINE_CHRP;
		else if (!strcasecmp("bootrom", buffer))
			gMachine = MACHINE_MAC;
	} else
		gMachine = MACHINE_MAC;

	if ((length = of_getprop(root, "model", buffer, sizeof(buffer) - 1))
		!= OF_FAILED) {
		buffer[length] = '\0';
		if (!strcasecmp("pegasos", buffer))
			gMachine |= MACHINE_PEGASOS;
	}

	if ((length = of_getprop(root, "name", buffer, sizeof(buffer) - 1))
		!= OF_FAILED) {
		buffer[length] = '\0';
		if (!strcasecmp("openbiosteam,openbios", buffer))
			gMachine |= MACHINE_QEMU;
	}
}


extern "C" void __attribute__((section(".text.start")))
_start(uint32 _unused1, uint32 _unused3, void *openFirmwareEntry)
{
	// According to the PowerPC bindings, OpenFirmware should have created
	// a stack of 32kB or higher for us at this point

	clear_bss();
	call_ctors();
		// call C++ constructors before doing anything else

	start(openFirmwareEntry);
}
