/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ACPI_H
#define ACPI_H

#include <SupportDefs.h>
#include <arch/x86/arch_acpi.h>

#ifdef __cplusplus
extern "C" {
#endif

struct scan_spots_struct {
	uint32 start;
	uint32 stop;
	uint32 length;
};

acpi_descriptor_header *acpi_find_table(const char *signature);
void acpi_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* ACPI_H */
