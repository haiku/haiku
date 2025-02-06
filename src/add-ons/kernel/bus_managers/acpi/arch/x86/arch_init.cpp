/*
 * Copyright 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */


#include <ACPI.h>
#include <apic.h>

extern "C" {
#include "acpi.h"
#include "accommon.h"
#include "acdisasm.h"
#include "acnamesp.h"
}

#include "arch_init.h"


#define PIC_MODE 0
#define APIC_MODE 1


void
arch_init_interrupt_controller()
{
	ACPI_OBJECT arg;
	ACPI_OBJECT_LIST parameter;

	arg.Integer.Type = ACPI_TYPE_INTEGER;
	arg.Integer.Value = apic_available() ? APIC_MODE : PIC_MODE;

	parameter.Count = 1;
	parameter.Pointer = &arg;

	AcpiEvaluateObject(NULL, (ACPI_STRING)"\\_PIC", &parameter, NULL);
}
