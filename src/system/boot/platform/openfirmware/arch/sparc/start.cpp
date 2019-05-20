/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Alexander von Gluck, kallisti5@unixzen.com
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include "start.h"

#include "machine.h"


void
determine_machine(void)
{
	gMachine = MACHINE_UNKNOWN;
}


extern "C" void __attribute__((section(".text.start")))
_start(int _reserved, int _argstr, int _arglen, int _unknown,
	void *openFirmwareEntry)
{
	// According to the sparc bindings, OpenFirmware should have created
	// a stack of 8kB or higher for us at this point, and window traps are
	// operational so it's possible to call the openFirmwareEntry safely.
	// The bss segment is already cleared by the firmware as well.

	call_ctors();
		// call C++ constructors before doing anything else

	start(openFirmwareEntry);
}

