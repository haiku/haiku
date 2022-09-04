/*
 * Copyright 2022, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ACPI.h>

extern "C" {
#include "acpi.h"
#include "accommon.h"
}

#include "arch_init.h"

ACPI_PHYSICAL_ADDRESS
arch_init_find_root_pointer()
{
	return 0;
}


void
arch_init_interrupt_controller()
{
	//stub
}
