/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
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
static acpi_descriptor_header* sAcpiXsdt; // Extended System Description Table
static int32 sNumEntries = -1;


static status_t
acpi_validate_rsdp(acpi_rsdp* rsdp)
{
	const char* data = (const char*)rsdp;
	unsigned char checksum = 0;
	for (uint32 i = 0; i < sizeof(acpi_rsdp_legacy); i++)
		checksum += data[i];

	if ((checksum & 0xff) != 0) {
		TRACE(("acpi: rsdp failed basic checksum\n"));
		return B_BAD_DATA;
	}

	// for ACPI 2.0+ we need to also validate the extended checksum
	if (rsdp->revision > 0) {
		for (uint32 i = sizeof(acpi_rsdp_legacy);
			i < sizeof(acpi_rsdp_extended); i++) {
				checksum += data[i];
		}

		if ((checksum & 0xff) != 0) {
			TRACE(("acpi: rsdp failed extended checksum\n"));
			return B_BAD_DATA;
		}
	}

	return B_OK;
}


static status_t
acpi_validate_rsdt(acpi_descriptor_header* rsdt)
{
	const char* data = (const char*)rsdt;
	unsigned char checksum = 0;
	for (uint32 i = 0; i < rsdt->length; i++)
		checksum += data[i];

	return checksum == 0 ? B_OK : B_BAD_DATA;
}


static status_t
acpi_check_rsdt(acpi_rsdp* rsdp)
{
	if (acpi_validate_rsdp(rsdp) != B_OK)
		return B_BAD_DATA;

	bool usingXsdt = false;

	TRACE(("acpi: found rsdp at %p oem id: %.6s, rev %d\n",
		rsdp, rsdp->oem_id, rsdp->revision));
	TRACE(("acpi: rsdp points to rsdt at 0x%lx\n", rsdp->rsdt_address));

	uint32 length = 0;
	acpi_descriptor_header* rsdt = NULL;
	if (rsdp->revision > 0) {
		length = rsdp->xsdt_length;
		rsdt = (acpi_descriptor_header*)mmu_map_physical_memory(
			(uint32)rsdp->xsdt_address, rsdp->xsdt_length, kDefaultPageFlags);
		if (rsdt != NULL
			&& strncmp(rsdt->signature, ACPI_XSDT_SIGNATURE, 4) != 0) {
			mmu_free(rsdt, rsdp->xsdt_length);
			rsdt = NULL;
			TRACE(("acpi: invalid extended system description table\n"));
		} else
			usingXsdt = true;
	}

	// if we're ACPI v1 or we fail to map the XSDT for some reason,
	// attempt to use the RSDT instead.
	if (rsdt == NULL) {
		// map and validate the root system description table
		rsdt = (acpi_descriptor_header*)mmu_map_physical_memory(
			rsdp->rsdt_address, sizeof(acpi_descriptor_header),
			kDefaultPageFlags);
		if (rsdt != NULL
			&& strncmp(rsdt->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
			mmu_free(rsdt, sizeof(acpi_descriptor_header));
			rsdt = NULL;
			TRACE(("acpi: invalid root system description table\n"));
			return B_ERROR;
		}

		length = rsdt->length;
		// Map the whole table, not just the header
		TRACE(("acpi: rsdt length: %lu\n", length));
		mmu_free(rsdt, sizeof(acpi_descriptor_header));
		rsdt = (acpi_descriptor_header*)mmu_map_physical_memory(
			rsdp->rsdt_address, length, kDefaultPageFlags);
	}

	if (rsdt != NULL) {
		if (acpi_validate_rsdt(rsdt) != B_OK) {
			TRACE(("acpi: rsdt failed checksum validation\n"));
			mmu_free(rsdt, length);
			return B_ERROR;
		} else {
			if (usingXsdt)
				sAcpiXsdt = rsdt;
			else
				sAcpiRsdt = rsdt;
			TRACE(("acpi: found valid %s at %p\n",
				usingXsdt ? ACPI_XSDT_SIGNATURE : ACPI_RSDT_SIGNATURE,
				rsdt));
		}
	} else
		return B_ERROR;

	return B_OK;
}


template<typename PointerType>
acpi_descriptor_header*
acpi_find_table_generic(const char* signature, acpi_descriptor_header* acpiSdt)
{
	if (acpiSdt == NULL)
		return NULL;

	if (sNumEntries == -1) {
		// if using the xsdt, our entries are 64 bits wide.
		sNumEntries = (acpiSdt->length
			- sizeof(acpi_descriptor_header))
				/ sizeof(PointerType);
	}

	if (sNumEntries <= 0) {
		TRACE(("acpi: root system description table is empty\n"));
		return NULL;
	}

	TRACE(("acpi: searching %ld entries for table '%.4s'\n", sNumEntries,
		signature));

	PointerType* pointer = (PointerType*)((uint8*)acpiSdt
		+ sizeof(acpi_descriptor_header));

	acpi_descriptor_header* header = NULL;
	for (int32 j = 0; j < sNumEntries; j++, pointer++) {
		header = (acpi_descriptor_header*)
			mmu_map_physical_memory((uint32)*pointer,
				sizeof(acpi_descriptor_header), kDefaultPageFlags);

		if (header == NULL
			|| strncmp(header->signature, signature, 4) != 0) {
			// not interesting for us
			TRACE(("acpi: Looking for '%.4s'. Skipping '%.4s'\n",
				signature, header != NULL ? header->signature : "null"));

			if (header != NULL) {
				mmu_free(header, sizeof(acpi_descriptor_header));
				header = NULL;
			}

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

	return (acpi_descriptor_header*)mmu_map_physical_memory(
		(uint32)*pointer, length, kDefaultPageFlags);
}


acpi_descriptor_header*
acpi_find_table(const char* signature)
{
	if (sAcpiRsdt != NULL)
		return acpi_find_table_generic<uint32>(signature, sAcpiRsdt);
	else if (sAcpiXsdt != NULL)
		return acpi_find_table_generic<uint64>(signature, sAcpiXsdt);

	return NULL;
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
