/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2007, Michael Lotz, mmlr@mlotz.ch
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
*/


#include <string.h>

#include <KernelExport.h>
#include <SupportDefs.h>

#include <arch/x86/arch_acpi.h>
#include <boot/stage2.h>
#include <boot/platform.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "acpi.h"
#include "mmu.h"


#define TRACE_ACPI
#ifdef TRACE_ACPI
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


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
	TRACE(("acpi: rsdp points to rsdt at 0x%x\n", rsdp->rsdt_address));

	acpi_descriptor_header* rsdt = NULL;
	if (rsdp->revision > 0) {
		rsdt = (acpi_descriptor_header*)(addr_t)rsdp->xsdt_address;
		if (rsdt != NULL
			&& strncmp(rsdt->signature, ACPI_XSDT_SIGNATURE, 4) != 0) {
			rsdt = NULL;
			TRACE(("acpi: invalid extended system description table\n"));
		} else
			usingXsdt = true;
	}

	// if we're ACPI v1 or we fail to map the XSDT for some reason,
	// attempt to use the RSDT instead.
	if (rsdt == NULL) {
		// validate the root system description table
		rsdt = (acpi_descriptor_header*)(addr_t)rsdp->rsdt_address;
		if (rsdt == NULL) {
			TRACE(("acpi: couldn't map rsdt header\n"));
			return B_ERROR;
		}
		if (strncmp(rsdt->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
			rsdt = NULL;
			TRACE(("acpi: invalid root system description table\n"));
			return B_ERROR;
		}

		TRACE(("acpi: rsdt length: %u\n", rsdt->length));
	}

	if (rsdt != NULL) {
		if (acpi_validate_rsdt(rsdt) != B_OK) {
			TRACE(("acpi: rsdt failed checksum validation\n"));
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
		sNumEntries = (acpiSdt->length - sizeof(acpi_descriptor_header))
			/ sizeof(PointerType);
	}

	if (sNumEntries <= 0) {
		TRACE(("acpi: root system description table is empty\n"));
		return NULL;
	}

	TRACE(("acpi: searching %d entries for table '%.4s'\n", sNumEntries,
		signature));

	PointerType* pointer = (PointerType*)((uint8*)acpiSdt
		+ sizeof(acpi_descriptor_header));

	acpi_descriptor_header* header = NULL;
	for (int32 j = 0; j < sNumEntries; j++, pointer++) {
		header = (acpi_descriptor_header*)(addr_t)*pointer;
		if (header != NULL && strncmp(header->signature, signature, 4) == 0) {
			TRACE(("acpi: Found '%.4s' @ %p\n", signature, pointer));
			return header;
		}

		TRACE(("acpi: Looking for '%.4s'. Skipping '%.4s'\n",
			signature, header != NULL ? header->signature : "null"));
		header = NULL;
		continue;
	}

	return NULL;
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
	efi_guid acpi = ACPI_20_TABLE_GUID;
	efi_configuration_table *table = kSystemTable->ConfigurationTable;
	size_t entries = kSystemTable->NumberOfTableEntries;

	// Try to find the ACPI RSDP.
	for (uint32 i = 0; i < entries; i++) {
		acpi_rsdp *rsdp = NULL;

		efi_guid vendor = table[i].VendorGuid;

		if (vendor.data1 == acpi.data1
			&& vendor.data2 == acpi.data2
			&& vendor.data3 == acpi.data3
			&& strncmp((char *)vendor.data4, (char *)acpi.data4, 8) == 0) {
			rsdp = (acpi_rsdp *)(table[i].VendorTable);
			if (strncmp((char *)rsdp, ACPI_RSDP_SIGNATURE, 8) == 0)
				TRACE(("acpi_init: found ACPI RSDP signature at %p\n", rsdp));

			if (rsdp != NULL && acpi_check_rsdt(rsdp) == B_OK) {
				gKernelArgs.arch_args.acpi_root = rsdp;
				break;
			}
		}
	}
}
