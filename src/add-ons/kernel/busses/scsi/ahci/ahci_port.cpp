/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_port.h"

#include <KernelExport.h>
#include <stdio.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)


AHCIPort::AHCIPort(AHCIController *controller, int index)
	: fIndex(index)
{
}
				

AHCIPort::~AHCIPort()
{
}

	
status_t
AHCIPort::Init()
{
	TRACE("AHCIPort::Init port %d\n", fIndex);
	return B_OK;
}


void		
AHCIPort::Uninit()
{
	TRACE("AHCIPort::Uninit port %d\n", fIndex);
}


void
AHCIPort::Interrupt()
{
	TRACE("AHCIPort::Interrupt port %d\n", fIndex);
}
