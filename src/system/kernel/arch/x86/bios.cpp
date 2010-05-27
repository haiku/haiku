/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <vm/vm_types.h>

#include <arch/x86/bios.h>


//#define TRACE_BIOS
#ifdef TRACE_BIOS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct smbios {
	uint32		anchor_string;
	uint8		entry_point_checksum;
	uint8		entry_point_length;
	struct {
		uint8	major;
		uint8	minor;
	} version;
	uint16		maximum_size;
	uint8		formatted_area[5];

	// this part is the legacy DMI compatible structure
	uint8		dmi_anchor_string[5];
	uint8		intermediate_checksum;
	uint16		structure_table_size;
	uint32		structure_table;
	uint16		num_structures;
	uint8		bcd_revision;
} _PACKED;

struct bios32 {
	uint32		anchor_string;
	uint32		service_directory;
	uint8		revision;
	uint8		size;	// in "paragraph" (16 byte) units
	uint8		checksum;
	uint8		_reserved[5];
};

enum {
	BIOS32	= '_23_',
	SMBIOS	= '_MS_',
	DMI		= '_IMD',
};


addr_t gBiosBase;
static addr_t sBios32ServiceDirectory;


static bool
check_checksum(addr_t base, size_t length)
{
	uint8 *bytes = (uint8 *)base;
	uint8 sum = 0;

	for (uint32 i = 0; i < length; i++)
		sum += bytes[i];

	return sum == 0;
}


//	#pragma mark -
//	public functions


/**	This function fills the provided bios32_service structure with
 *	the values that identify BIOS service.
 *	Returns B_OK on successful completion, otherwise B_ERROR if
 *	the BIOS32 service directory is not available, or B_BAD_VALUE
 *	in case the identifier is not known or present.
 */

extern "C" status_t
get_bios32_service(uint32 identifier, struct bios32_service *service)
{
	TRACE(("get_bios32_service(identifier = %#lx)\n", identifier));

	if (sBios32ServiceDirectory == 0)
		return B_ERROR;

	uint32 eax = 0, ebx = 0, ecx = 0, edx = 0;

	asm("movl	%4, %%eax;		"	// set service parameters
		"xorl	%%ebx, %%ebx;	"
		"movl	%5, %%ecx;		"
		"pushl	%%cs;			"	// emulate far call by register
		"call	*%%ecx;			"
		"movl	%%eax, %0;		"	// save return values
		"movl	%%ebx, %1;		"
		"movl	%%ecx, %2;		"
		"movl	%%edx, %3;		"
		: "=m" (eax), "=m" (ebx), "=m" (ecx), "=m" (edx)
		: "m" (identifier), "m" (sBios32ServiceDirectory)
		: "eax", "ebx", "ecx", "edx", "memory");

	if ((eax & 0xff) != 0)
		return B_BAD_VALUE;

	service->base = ebx;
	service->size = ecx;
	service->offset = edx;

	return B_OK;
}


extern "C" status_t
bios_init(void)
{
	// map BIOS area 0xe0000 - 0xfffff
	area_id biosArea = map_physical_memory("pc bios", 0xe0000, 0x20000,
		B_ANY_KERNEL_ADDRESS | B_MTR_WB,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void **)&gBiosBase);
	if (biosArea < 0)
		return biosArea;

	TRACE(("PC BIOS mapped to %p\n", (void *)gBiosBase));

	// ToDo: add driver settings support to disable the services below

	// search for available BIOS services

	addr_t base = gBiosBase;
	addr_t end = base + 0x20000;

	while (base < end) {
		switch (*(uint32 *)base) {
			case BIOS32:
				if (check_checksum(base, sizeof(struct bios32))) {
					struct bios32 *bios32 = (struct bios32 *)base;

					TRACE(("bios32 revision %d\n", bios32->revision));
					TRACE(("bios32 service directory at: %lx\n", bios32->service_directory));

					if (bios32->service_directory >= 0xe0000
						&& bios32->service_directory <= 0xfffff)
						sBios32ServiceDirectory = gBiosBase - 0xe0000 + bios32->service_directory;
				}
				break;
			case SMBIOS:
				// SMBIOS contains the legacy DMI structure, so we have to
				// make sure it won't be found
				base += 16;
				TRACE(("probably found SMBIOS structure.\n"));
				break;
			case DMI:
				TRACE(("probably found DMI legacy structure.\n"));
				break;
		}

		// get on to next "paragraph"
		base += 16;
	}

	return B_OK;
}

