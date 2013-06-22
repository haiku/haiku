/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PCI_ACPI_H
#define PCI_ACPI_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct scan_spots_struct {
	uint32 start;
	uint32 stop;
	uint32 length;
};

void *acpi_find_table(const char *signature);
void acpi_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* PCI_ACPI_H */
