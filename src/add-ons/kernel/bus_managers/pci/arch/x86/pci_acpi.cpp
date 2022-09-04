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


#include "pci_acpi.h"

#include <string.h>

#include <KernelExport.h>

#include <boot_item.h>
#include <acpi.h>


//#define TRACE_ACPI
#ifdef TRACE_ACPI
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)	dprintf(x)

static struct scan_spots_struct acpi_scan_spots[] = {
	{ 0x0, 0x1000, 0x1000 },
	{ 0x9f000, 0x10000, 0x1000 },
	{ 0xe0000, 0x110000, 0x20000 },
	{ 0xfd000, 0xfe000, 0x1000},
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
		TRACE("acpi: rsdp failed basic checksum\n");
		return B_BAD_DATA;
	}

	// for ACPI 2.0+ we need to also validate the extended checksum
	if (rsdp->revision > 0) {
		for (uint32 i = sizeof(acpi_rsdp_legacy);
			i < sizeof(acpi_rsdp_extended); i++) {
				checksum += data[i];
		}

		if ((checksum & 0xff) != 0) {
			ERROR("acpi: rsdp failed extended checksum\n");
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

	TRACE("acpi: found rsdp at %p oem id: %.6s, rev %d\n",
		rsdp, rsdp->oem_id, rsdp->revision);
	TRACE("acpi: rsdp points to rsdt at 0x%" B_PRIx32 "\n", rsdp->rsdt_address);

	uint32 length = 0;
	acpi_descriptor_header* rsdt = NULL;
	area_id rsdtArea = -1;
	if (rsdp->revision > 0) {
		length = rsdp->xsdt_length;
		rsdtArea = map_physical_memory("rsdt acpi",
			(uint32)rsdp->xsdt_address, rsdp->xsdt_length, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA, (void **)&rsdt);
		if (rsdt != NULL
			&& strncmp(rsdt->signature, ACPI_XSDT_SIGNATURE, 4) != 0) {
			delete_area(rsdtArea);
			rsdt = NULL;
			ERROR("acpi: invalid extended system description table\n");
		} else
			usingXsdt = true;
	}

	// if we're ACPI v1 or we fail to map the XSDT for some reason,
	// attempt to use the RSDT instead.
	if (rsdt == NULL) {
		// map and validate the root system description table
		rsdtArea = map_physical_memory("rsdt acpi",
			rsdp->rsdt_address, sizeof(acpi_descriptor_header),
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&rsdt);
		if (rsdt == NULL) {
			ERROR("acpi: couldn't map rsdt header\n");
			return B_ERROR;
		}
		if (strncmp(rsdt->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
			delete_area(rsdtArea);
			rsdt = NULL;
			ERROR("acpi: invalid root system description table\n");
			return B_ERROR;
		}

		length = rsdt->length;
		// Map the whole table, not just the header
		TRACE("acpi: rsdt length: %" B_PRIu32 "\n", length);
		delete_area(rsdtArea);
		rsdtArea = map_physical_memory("rsdt acpi",
			rsdp->rsdt_address, length, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA, (void **)&rsdt);
	}

	if (rsdt != NULL) {
		if (acpi_validate_rsdt(rsdt) != B_OK) {
			TRACE("acpi: rsdt failed checksum validation\n");
			delete_area(rsdtArea);
			return B_ERROR;
		} else {
			if (usingXsdt)
				sAcpiXsdt = rsdt;
			else
				sAcpiRsdt = rsdt;
			TRACE("acpi: found valid %s at %p\n",
				usingXsdt ? ACPI_XSDT_SIGNATURE : ACPI_RSDT_SIGNATURE,
				rsdt);
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
		ERROR("acpi: root system description table is empty\n");
		return NULL;
	}

	TRACE("acpi: searching %" B_PRId32 " entries for table '%.4s'\n", sNumEntries,
		signature);

	PointerType* pointer = (PointerType*)((uint8*)acpiSdt
		+ sizeof(acpi_descriptor_header));

	acpi_descriptor_header* header = NULL;
	area_id headerArea = -1;
	for (int32 j = 0; j < sNumEntries; j++, pointer++) {
		headerArea = map_physical_memory("acpi header", (uint32)*pointer,
				sizeof(acpi_descriptor_header), B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA, (void **)&header);

		if (header == NULL
			|| strncmp(header->signature, signature, 4) != 0) {
			// not interesting for us
			TRACE("acpi: Looking for '%.4s'. Skipping '%.4s'\n",
				signature, header != NULL ? header->signature : "null");

			if (header != NULL) {
				delete_area(headerArea);
				header = NULL;
			}

			continue;
		}

		TRACE("acpi: Found '%.4s' @ %p\n", signature, pointer);
		break;
	}


	if (header == NULL)
		return NULL;

	// Map the whole table, not just the header
	uint32 length = header->length;
	delete_area(headerArea);

	headerArea = map_physical_memory("acpi table",
		(uint32)*pointer, length, B_ANY_KERNEL_ADDRESS,
			B_KERNEL_READ_AREA, (void **)&header);
	return header;
}


void*
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
	// Try pointer from boot loader, if any
	phys_addr_t* acpiRootPointer = (phys_addr_t*)get_boot_item("ACPI_ROOT_POINTER", NULL);
	if (acpiRootPointer != NULL) {
		acpi_rsdp* rsdp = NULL;
		area_id rsdpArea = map_physical_memory("acpi rsdp",
			*acpiRootPointer, B_PAGE_SIZE,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&rsdp);
		if (rsdpArea < B_OK) {
			ERROR("acpi_init: couldn't map %s\n", strerror(rsdpArea));
		} else {
			status_t status = B_ERROR;
			if (strncmp((char*)rsdp, ACPI_RSDP_SIGNATURE, 8) == 0)
				status = acpi_check_rsdt(rsdp);
			delete_area(rsdpArea);
			if (status == B_OK)
				return;
		}
	}

	// Try to find the ACPI RSDP in EBDA
	uint8* start = NULL;
	area_id ebdaArea = map_physical_memory("ebda", 0, B_PAGE_SIZE,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&start);
	if (ebdaArea < 0) {
		ERROR("acpi_init: couldn't map 1K: %s\n", strerror(ebdaArea));
	} else {
		phys_addr_t ebda = *(uint16*)&start[0x40e];
		ebda <<= 4;
		TRACE("acpi_init: 0x%lx\n", ebda);
		delete_area(ebdaArea);
		if (ebda > 0x400) {
			acpi_scan_spots[0].start = ebda;
			acpi_scan_spots[0].stop = ebda + 0x1000;
			acpi_scan_spots[0].length = 0x1000;
			// will be scanned first
		}
	}

	// Try to find the ACPI RSDP.
	for (int32 i = 0; acpi_scan_spots[i].length > 0; i++) {
		acpi_rsdp* rsdp = NULL;

		TRACE("acpi_init: entry base 0x%" B_PRIx32 ", limit 0x%" B_PRIx32 "\n",
			acpi_scan_spots[i].start, acpi_scan_spots[i].stop);

		char* start = NULL;
		area_id rsdpArea = map_physical_memory("acpi rsdp",
			acpi_scan_spots[i].start, acpi_scan_spots[i].length,
			B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&start);
		if (rsdpArea < B_OK) {
			ERROR("acpi_init: couldn't map %s\n", strerror(rsdpArea));
			break;
		}
		for (char *pointer = start;
			(addr_t)pointer < (addr_t)start + acpi_scan_spots[i].length;
			pointer += 16) {
			if (strncmp(pointer, ACPI_RSDP_SIGNATURE, 8) == 0) {
				TRACE("acpi_init: found ACPI RSDP signature at %p\n",
					pointer);
				rsdp = (acpi_rsdp*)pointer;
			}
		}

		if (rsdp != NULL && acpi_check_rsdt(rsdp) == B_OK) {
			delete_area(rsdpArea);
			break;
		}
		delete_area(rsdpArea);
	}

}
