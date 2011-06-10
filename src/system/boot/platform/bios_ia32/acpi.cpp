/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2007, Michael Lotz, mmlr@mlotz.ch
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include "acpi.h"
#include "mmu.h"

#include <string.h>

#include <KernelExport.h>

#include <arch/x86/arch_acpi.h>


//#define TRACE_ACPI
#ifdef TRACE_ACPI
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static struct scan_spots_struct acpi_scan_spots[] = {
	{ 0x0, 0x400, 0x400 - 0x0 },
	{ 0xe0000, 0x100000, 0x100000 - 0xe0000 },
	{ 0, 0, 0 }
};

static acpi_descriptor_header* sAcpiRsdt; // System Description Table
static int32 sNumEntries = -1;


static status_t
acpi_check_rsdt(acpi_rsdp* rsdp)
{
	TRACE(("acpi: found rsdp at %p oem id: %.6s\n", rsdp, rsdp->oem_id));
	TRACE(("acpi: rsdp points to rsdt at 0x%lx\n", rsdp->rsdt_address));

	// map and validate the root system description table
	acpi_descriptor_header* rsdt
		= (acpi_descriptor_header*)mmu_map_physical_memory(
		rsdp->rsdt_address, sizeof(acpi_descriptor_header),
		kDefaultPageFlags);
	if (rsdt == NULL
		|| strncmp(rsdt->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
		if (rsdt != NULL)
			mmu_free(rsdt, sizeof(acpi_descriptor_header));
		TRACE(("acpi: invalid root system description table\n"));
		return B_ERROR;
	}

	sAcpiRsdt = rsdt;
	return B_OK;
}


acpi_descriptor_header*
acpi_find_table(const char* signature)
{
	if (sAcpiRsdt == NULL)
		return NULL;

	if (sNumEntries == -1) {
		sNumEntries = (sAcpiRsdt->length
			- sizeof(acpi_descriptor_header)) / 4;
	}

	if (sNumEntries <= 0) {
		TRACE(("acpi: root system description table is empty\n"));
		return NULL;
	}

	TRACE(("acpi: searching %ld entries for table '%.4s'\n", sNumEntries,
		signature));

	uint32* pointer = (uint32*)((uint8*)sAcpiRsdt
		+ sizeof(acpi_descriptor_header));

	acpi_descriptor_header* header = NULL;
	for (int32 j = 0; j < sNumEntries; j++, pointer++) {
		header = (acpi_descriptor_header*)
			mmu_map_physical_memory(*pointer,
					sizeof(acpi_descriptor_header), kDefaultPageFlags);
		if (header == NULL
			|| strncmp(header->signature, signature, 4) != 0) {
			// not interesting for us
			TRACE(("acpi: Looking for '%.4s'. Skipping '%.4s'\n",
				signature, header != NULL ? header->signature : "null"));

			if (header != NULL)
				mmu_free(header, sizeof(acpi_descriptor_header));

			continue;
		}

		TRACE(("acpi: Found '%.4s' @ %p\n", signature, pointer));
		break;
	}

	
	if (header == NULL)
		return NULL;

	// Map the whole table, not just the header
	uint32 length = header->length;
	mmu_free(header, sizeof(acpi_descriptor_header));

	return (acpi_descriptor_header*)mmu_map_physical_memory(*pointer,
		length, kDefaultPageFlags);
}


void
acpi_init()
{
	// Try to find the ACPI RSDP.
	for (int32 i = 0; acpi_scan_spots[i].length > 0; i++) {
		acpi_rsdp* rsdp = NULL;

		TRACE(("acpi_init: entry base 0x%lx, limit 0x%lx\n",
			acpi_scan_spots[i].start, acpi_scan_spots[i].stop));

		for (char* pointer = (char*)acpi_scan_spots[i].start;
		     (uint32)pointer < acpi_scan_spots[i].stop; pointer += 16) {
			if (strncmp(pointer, ACPI_RSDP_SIGNATURE, 8) == 0) {
				TRACE(("acpi_init: found ACPI RSDP signature at %p\n",
					pointer));
				rsdp = (acpi_rsdp*)pointer;
			}
		}

		if (rsdp != NULL && acpi_check_rsdt(rsdp) == B_OK)
			break;
	}
}
