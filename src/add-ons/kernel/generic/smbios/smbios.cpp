/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT license.
 */


#include "smbios.h"

#include <device_manager.h>
#include <KernelExport.h>
#include <module.h>

#include <stdlib.h>
#include <string.h>

#include <vm/vm.h>


#define TRACE_SMBIOS
#ifdef TRACE_SMBIOS
#	define TRACE(x...) dprintf (x)
#else
#	define TRACE(x...) ;
#endif


static device_manager_info* gDeviceManager;
static char* sHardwareVendor = NULL;
static char* sHardwareProduct = NULL;

struct smbios {
	uint32		anchor_string;
	uint8		entry_point_checksum;
	uint8		entry_point_length;
	struct {
		uint8	major;
		uint8	minor;
	} version;
	uint16		maximum_size;
	uint8		entry_point_revision;
	uint8		formatted_area[5];

	uint8		dmi_anchor_string[5];
	uint8		intermediate_checksum;
	uint16		structure_table_size;
	uint32		structure_table;
	uint16		num_structures;
	uint8		bcd_revision;
} _PACKED;


struct smbios3 {
	uint8		anchor_string[5];
	uint8		entry_point_checksum;
	uint8		entry_point_length;
	struct {
		uint8	major;
		uint8	minor;
		uint8	doc;
	} version;
	uint8		entry_point_revision;
	uint8		reserved;
	uint32		structure_table_size;
	uint64		structure_table;
} _PACKED;


struct smbios_structure_header {
	uint8		type;
	uint8		length;
	uint16		handle;
} _PACKED;


#define SMBIOS	"_SM_"
#define SMBIOS3	"_SM3_"

enum {
	SMBIOS_TYPE_BIOS 	= 0,
	SMBIOS_TYPE_SYSTEM,
};


struct smbios_system {
	struct smbios_structure_header header;
	uint8		manufacturer;
	uint8		product_name;
	uint8		version;
	uint8		serial_number;
	uint8		uuid[16];
	uint8		wakeup_type;
	uint8		sku_number;
	uint8		family;
} _PACKED;


static bool
smbios_match_vendor_product(const char* vendor, const char* product)
{
	if (vendor == NULL && product == NULL)
		return false;

	bool match = true;
	if (vendor != NULL && sHardwareVendor != NULL)
		match = strcmp(vendor, sHardwareVendor) == 0;
	if (match && product != NULL && sHardwareProduct != NULL)
		match = strcmp(product, sHardwareProduct) == 0;
	return match;
}


static const char *
smbios_get_string(struct smbios_structure_header* table, uint8* tableEnd,
	uint8 index)
{
	uint8* addr = (uint8*)table + table->length;
	uint8 i = 1;
	for (; addr < tableEnd && i < index && *addr != 0; i++) {
		while (*addr != 0 && addr < tableEnd)
			addr++;
		addr++;
	}
	if (i == index)
		return (const char*)addr;

	return NULL;
}


static void
smbios_scan()
{
	TRACE("smbios_scan\n");
	static bool scanDone = false;
	if (scanDone)
		return;

	// map SMBIOS area 0xf0000 - 0xfffff
	addr_t smBiosBase;
	area_id smbiosArea = map_physical_memory("pc bios", 0xf0000, 0x10000,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA, (void **)&smBiosBase);
	if (smbiosArea < 0)
		return;

	struct smbios *smbios = NULL;
	struct smbios3 *smbios3 = NULL;
	for (addr_t offset = 0; offset <= 0xffe0; offset += 0x10) {
		void* p = (void*)(smBiosBase + offset);
		if (memcmp(p, SMBIOS3, 5) == 0) {
			smbios3 = (struct smbios3 *)p;
			break;
		} else if (memcmp(p, SMBIOS, 4) == 0) {
			smbios = (struct smbios *)p;
		}
	}

	phys_addr_t tablePhysAddr = 0;
	size_t tablePhysLength = 0;
	void* table;
	status_t status;
	uint8* tableEnd;

	if (smbios != NULL) {
		tablePhysAddr = smbios->structure_table;
		tablePhysLength = smbios->structure_table_size;
	} else if (smbios3 != NULL) {
		tablePhysAddr = smbios3->structure_table;
		tablePhysLength = smbios3->structure_table_size;
	}

	if (tablePhysAddr == 0)
		goto err;

	table = malloc(tablePhysLength);
	if (table == NULL)
		goto err;
	status = vm_memcpy_from_physical(table, tablePhysAddr,
		tablePhysLength, false);
	if (status != B_OK)
		goto err;

	tableEnd = (uint8*)table + tablePhysLength;
	for (uint8* addr = (uint8*)table;
		(addr + sizeof(struct smbios_structure_header)) < tableEnd;) {
		struct smbios_structure_header* table
			= (struct smbios_structure_header*)addr;

		if (table->type == SMBIOS_TYPE_SYSTEM) {
			struct smbios_system *system = (struct smbios_system*)table;
			TRACE("found System Information at %p\n", table);
			TRACE("found vendor %u product %u\n", system->manufacturer,
				system->product_name);
			const char* vendor = smbios_get_string(table, tableEnd,
				system->manufacturer);
			const char* product = smbios_get_string(table, tableEnd,
				system->product_name);
			if (vendor != NULL)
				sHardwareVendor = strdup(vendor);
			if (product != NULL)
				sHardwareProduct = strdup(product);
			break;
		}
		addr += table->length;
		for (; addr + 1 < tableEnd; addr++) {
			if (*addr == 0 && *(addr + 1) == 0)
				break;
		}
		addr += 2;
	}

	scanDone = true;
	TRACE("smbios_scan found vendor %s product %s\n", sHardwareVendor,
		sHardwareProduct);
err:
	delete_area(smbiosArea);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			smbios_scan();
			return B_OK;
		case B_MODULE_UNINIT:
			free(sHardwareVendor);
			free(sHardwareProduct);
			return B_OK;
		default:
			return B_ERROR;
	}
}



module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager},
	{}
};


static smbios_module_info sSMBIOSModule = {
	{
		SMBIOS_MODULE_NAME,
		B_KEEP_LOADED,
		std_ops
	},

	smbios_match_vendor_product,
};


module_info *modules[] = {
	(module_info*)&sSMBIOSModule,
	NULL
};
