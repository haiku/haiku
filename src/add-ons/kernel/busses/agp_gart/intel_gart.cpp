/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <AreaKeeper.h>
#include <intel_extreme.h>

#include <stdlib.h>

#include <AGP.h>
#include <KernelExport.h>
#include <PCI.h>


//#define TRACE_INTEL
#ifdef TRACE_INTEL
#	define TRACE(x...) dprintf("\33[33magp-intel:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif

#ifndef __HAIKU__
#	define B_KERNEL_READ_AREA	0
#	define B_KERNEL_WRITE_AREA	0
#endif

/* read and write to PCI config space */
#define get_pci_config(info, offset, size) \
	(sPCI->read_pci_config((info).bus, (info).device, (info).function, \
		(offset), (size)))
#define set_pci_config(info, offset, size, value) \
	(sPCI->write_pci_config((info).bus, (info).device, (info).function, \
		(offset), (size), (value)))
#define write32(address, data) \
	(*((volatile uint32*)(address)) = (data))
#define read32(address) \
	(*((volatile uint32*)(address)))


const struct supported_device {
	uint32		bridge_id;
	uint32		display_id;
	uint32		type;
	const char	*name;
} kSupportedDevices[] = {
	{0x3575, 0x3577, INTEL_TYPE_83x, "i830GM"},
	{0x2560, 0x2562, INTEL_TYPE_83x, "i845G"},
	{0x3580, 0x3582, INTEL_TYPE_85x, "i855G"},
	{0x358c, 0x358e, INTEL_TYPE_85x, "i855G"},
	{0x2570, 0x2572, INTEL_TYPE_85x, "i865G"},

//	{0x2792, INTEL_TYPE_91x, "i910"},
//	{0x258a, INTEL_TYPE_91x, "i915"},
	{0x2580, 0x2582, INTEL_TYPE_915, "i915G"},
	{0x2590, 0x2592, INTEL_TYPE_915M, "i915GM"},
	{0x2770, 0x2772, INTEL_TYPE_945, "i945G"},
	{0x27a0, 0x27a2, INTEL_TYPE_945M, "i945GM"},
	{0x27ac, 0x27ae, INTEL_TYPE_945M, "i945GME"},

	{0x2970, 0x2972, INTEL_TYPE_965, "i946GZ"},
	{0x2980, 0x2982, INTEL_TYPE_965, "G35"},
	{0x2990, 0x2992, INTEL_TYPE_965, "i965Q"},
	{0x29a0, 0x29a2, INTEL_TYPE_965, "i965G"},
	{0x2a00, 0x2a02, INTEL_TYPE_965, "i965GM"},
	{0x2a10, 0x2a12, INTEL_TYPE_965, "i965GME"},

	{0x29b0, 0x29b2, INTEL_TYPE_G33, "G33"},
	{0x29c0, 0x29c2, INTEL_TYPE_G33, "Q35"},
	{0x29d0, 0x29d2, INTEL_TYPE_G33, "Q33"},

	{0x2a40, 0x2a42, INTEL_TYPE_GM45, "GM45"},
	{0x2e00, 0x2e02, INTEL_TYPE_G45, "IGD"},
	{0x2e10, 0x2e12, INTEL_TYPE_G45, "Q45"},
	{0x2e20, 0x2e22, INTEL_TYPE_G45, "G45"},
	{0x2e30, 0x2e32, INTEL_TYPE_G45, "G41"},
	{0x2e40, 0x2e42, INTEL_TYPE_G45, "B43"},
	{0x2e90, 0x2e92, INTEL_TYPE_G45, "B43"},

	{0xa000, 0xa001, INTEL_TYPE_IGDG, "Atom_Dx10"},
	{0xa010, 0xa011, INTEL_TYPE_IGDGM, "Atom_N4x0"},

#if 0
	{0x0040, 0x0042, INTEL_TYPE_ILKG, "IronLake Desktop"},
	{0x0044, 0x0046, INTEL_TYPE_ILKGM, "IronLake Mobile"},
	{0x0062, 0x0046, INTEL_TYPE_ILKGM, "IronLake Mobile"},
	{0x006a, 0x0046, INTEL_TYPE_ILKGM, "IronLake Mobile"},
#endif

	{0x0100, 0x0102, INTEL_TYPE_SNBG, "SandyBridge Desktop GT1"},
	{0x0100, 0x0112, INTEL_TYPE_SNBG, "SandyBridge Desktop GT2"},
	{0x0100, 0x0122, INTEL_TYPE_SNBG, "SandyBridge Desktop GT2+"},
	{0x0104, 0x0106, INTEL_TYPE_SNBGM, "SandyBridge Mobile GT1"},
	{0x0104, 0x0116, INTEL_TYPE_SNBGM, "SandyBridge Mobile GT2"},
	{0x0104, 0x0126, INTEL_TYPE_SNBGM, "SandyBridge Mobile GT2+"},
	{0x0108, 0x010a, INTEL_TYPE_SNBGS, "SandyBridge Server"}
};

struct intel_info {
	pci_info	bridge;
	pci_info	display;
	uint32		type;

	uint32		*gtt_base;
	addr_t		gtt_physical_base;
	area_id		gtt_area;
	size_t		gtt_entries;
	size_t		gtt_stolen_entries;

	vuint32		*registers;
	area_id		registers_area;

	addr_t		aperture_base;
	addr_t		aperture_physical_base;
	area_id		aperture_area;
	size_t		aperture_size;
	size_t		aperture_stolen_size;

	phys_addr_t	scratch_page;
	area_id		scratch_area;
};

static intel_info sInfo;
static pci_module_info* sPCI;


static bool
has_display_device(pci_info &info, uint32 deviceID)
{
	for (uint32 index = 0; sPCI->get_nth_pci_info(index, &info) == B_OK;
			index++) {
		if (info.vendor_id != VENDOR_ID_INTEL
			|| info.device_id != deviceID
			|| info.class_base != PCI_display)
			continue;

		return true;
	}

	return false;
}


static void
determine_memory_sizes(intel_info &info, size_t &gttSize, size_t &stolenSize)
{
	// read stolen memory from the PCI configuration of the PCI bridge
	uint8 controlRegister = INTEL_GRAPHICS_MEMORY_CONTROL;
	if ((info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_SNB)
		controlRegister = SNB_GRAPHICS_MEMORY_CONTROL;

	uint16 memoryConfig = get_pci_config(info.bridge, controlRegister, 2);
	size_t memorySize = 1 << 20; // 1 MB
	gttSize = 0;
	stolenSize = 0;

	if (info.type == INTEL_TYPE_965) {
		switch (memoryConfig & i965_GTT_MASK) {
			case i965_GTT_128K:
				gttSize = 128 << 10;
				break;
			case i965_GTT_256K:
				gttSize = 256 << 10;
				break;
			case i965_GTT_512K:
				gttSize = 512 << 10;
				break;
		}
	} else if (info.type == INTEL_TYPE_G33
	           || (info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_IGD) {
		switch (memoryConfig & G33_GTT_MASK) {
			case G33_GTT_1M:
				gttSize = 1 << 20;
				break;
			case G33_GTT_2M:
				gttSize = 2 << 20;
				break;
		}
	} else if ((info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_G4x
			|| (info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_ILK) {
		switch (memoryConfig & G4X_GTT_MASK) {
			case G4X_GTT_NONE:
				gttSize = 0;
				break;
			case G4X_GTT_1M_NO_IVT:
				gttSize = 1 << 20;
				break;
			case G4X_GTT_2M_NO_IVT:
			case G4X_GTT_2M_IVT:
				gttSize = 2 << 20;
				break;
			case G4X_GTT_3M_IVT:
				gttSize = 3 << 20;
				break;
			case G4X_GTT_4M_IVT:
				gttSize = 4 << 20;
				break;
		}
	} else if ((info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_SNB) {
		switch (memoryConfig & SNB_GTT_SIZE_MASK) {
			case SNB_GTT_SIZE_NONE:
				gttSize = 0;
				break;
			case SNB_GTT_SIZE_1MB:
				gttSize = 1 << 20;
				break;
			case SNB_GTT_SIZE_2MB:
				gttSize = 2 << 20;
				break;
		}
	} else {
		// older models have the GTT as large as their frame buffer mapping
		// TODO: check if the i9xx version works with the i8xx chips as well
		size_t frameBufferSize = 0;
		if ((info.type & INTEL_TYPE_8xx) != 0) {
			if (info.type == INTEL_TYPE_83x
				&& (memoryConfig & MEMORY_MASK) == i830_FRAME_BUFFER_64M)
				frameBufferSize = 64 << 20;
			else
				frameBufferSize = 128 << 20;
		} else if ((info.type & INTEL_TYPE_9xx) != 0)
			frameBufferSize = info.display.u.h0.base_register_sizes[2];

		TRACE("frame buffer size %lu MB\n", frameBufferSize >> 20);
		gttSize = frameBufferSize / 1024;
	}

	// TODO: test with different models!

	if (info.type == INTEL_TYPE_83x) {
		// Older chips
		switch (memoryConfig & STOLEN_MEMORY_MASK) {
			case i830_LOCAL_MEMORY_ONLY:
				// TODO: determine its size!
				dprintf("intel_gart: getting local memory size not "
					"implemented.\n");
				break;
			case i830_STOLEN_512K:
				memorySize >>= 1;
				break;
			case i830_STOLEN_1M:
				// default case
				break;
			case i830_STOLEN_8M:
				memorySize *= 8;
				break;
		}
	} else if ((info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_SNB) {
		switch (memoryConfig & SNB_STOLEN_MEMORY_MASK) {
			case SNB_STOLEN_MEMORY_32MB:
				memorySize *= 32;
				break;
			case SNB_STOLEN_MEMORY_64MB:
				memorySize *= 64;
				break;
			case SNB_STOLEN_MEMORY_96MB:
				memorySize *= 96;
				break;
			case SNB_STOLEN_MEMORY_128MB:
				memorySize *= 128;
				break;
			case SNB_STOLEN_MEMORY_160MB:
				memorySize *= 160;
				break;
			case SNB_STOLEN_MEMORY_192MB:
				memorySize *= 192;
				break;
			case SNB_STOLEN_MEMORY_224MB:
				memorySize *= 224;
				break;
			case SNB_STOLEN_MEMORY_256MB:
				memorySize *= 256;
				break;
			case SNB_STOLEN_MEMORY_288MB:
				memorySize *= 288;
				break;
			case SNB_STOLEN_MEMORY_320MB:
				memorySize *= 320;
				break;
			case SNB_STOLEN_MEMORY_352MB:
				memorySize *= 352;
				break;
			case SNB_STOLEN_MEMORY_384MB:
				memorySize *= 384;
				break;
			case SNB_STOLEN_MEMORY_416MB:
				memorySize *= 416;
				break;
			case SNB_STOLEN_MEMORY_448MB:
				memorySize *= 448;
				break;
			case SNB_STOLEN_MEMORY_480MB:
				memorySize *= 480;
				break;
			case SNB_STOLEN_MEMORY_512MB:
				memorySize *= 512;
				break;
		}
	} else if (info.type == INTEL_TYPE_85x
		|| (info.type & INTEL_TYPE_9xx) == INTEL_TYPE_9xx) {
		switch (memoryConfig & STOLEN_MEMORY_MASK) {
			case i855_STOLEN_MEMORY_4M:
				memorySize *= 4;
				break;
			case i855_STOLEN_MEMORY_8M:
				memorySize *= 8;
				break;
			case i855_STOLEN_MEMORY_16M:
				memorySize *= 16;
				break;
			case i855_STOLEN_MEMORY_32M:
				memorySize *= 32;
				break;
			case i855_STOLEN_MEMORY_48M:
				memorySize *= 48;
				break;
			case i855_STOLEN_MEMORY_64M:
				memorySize *= 64;
				break;
			case i855_STOLEN_MEMORY_128M:
				memorySize *= 128;
				break;
			case i855_STOLEN_MEMORY_256M:
				memorySize *= 256;
				break;
			case G4X_STOLEN_MEMORY_96MB:
				memorySize *= 96;
				break;
			case G4X_STOLEN_MEMORY_160MB:
				memorySize *= 160;
				break;
			case G4X_STOLEN_MEMORY_224MB:
				memorySize *= 224;
				break;
			case G4X_STOLEN_MEMORY_352MB:
				memorySize *= 352;
				break;
		}
	} else {
		// TODO: error out!
		memorySize = 4096;
	}

	stolenSize = memorySize - 4096;
}


static void
set_gtt_entry(intel_info &info, uint32 offset, phys_addr_t physicalAddress)
{
	// TODO: this is not 64-bit safe!
	write32(info.gtt_base + (offset >> GTT_PAGE_SHIFT),
		(uint32)physicalAddress | GTT_ENTRY_VALID);
}


static void
intel_unmap(intel_info &info)
{
	delete_area(info.registers_area);
	delete_area(info.gtt_area);
	delete_area(info.scratch_area);
	delete_area(info.aperture_area);
	info.aperture_size = 0;
}


static status_t
intel_map(intel_info &info)
{
	int fbIndex = 0;
	int mmioIndex = 1;
	if ((info.type & INTEL_TYPE_FAMILY_MASK) == INTEL_TYPE_9xx) {
		// for some reason Intel saw the need to change the order of the
		// mappings with the introduction of the i9xx family
		mmioIndex = 0;
		fbIndex = 2;
	}

	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("intel GMCH mmio",
		(void*)info.display.u.h0.base_registers[mmioIndex],
		info.display.u.h0.base_register_sizes[mmioIndex], B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf("agp_intel: could not map memory I/O!\n");
		return info.registers_area;
	}

	// make sure bus master, memory-mapped I/O, and frame buffer is enabled
	set_pci_config(info.display, PCI_command, 2,
		get_pci_config(info.display, PCI_command, 2)
			| PCI_command_io | PCI_command_memory | PCI_command_master);

	void* scratchAddress;
	AreaKeeper scratchCreator;
	info.scratch_area = scratchCreator.Create("intel GMCH scratch",
		&scratchAddress, B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (scratchCreator.InitCheck() < B_OK) {
		dprintf("agp_intel: could not create scratch page!\n");
		return info.scratch_area;
	}

	physical_entry entry;
	if (get_memory_map(scratchAddress, B_PAGE_SIZE, &entry, 1) != B_OK)
		return B_ERROR;

	if ((info.type & INTEL_TYPE_FAMILY_MASK) == INTEL_TYPE_9xx) {
		if ((info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_G4x
			|| (info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_ILK
			|| (info.type & INTEL_TYPE_GROUP_MASK) == INTEL_TYPE_SNB) {
			info.gtt_physical_base = info.display.u.h0.base_registers[mmioIndex]
					+ (2UL << 20);
		} else
			info.gtt_physical_base
				= get_pci_config(info.display, i915_GTT_BASE, 4);
	} else {
		info.gtt_physical_base = read32(info.registers
			+ INTEL_PAGE_TABLE_CONTROL) & ~PAGE_TABLE_ENABLED;
		if (info.gtt_physical_base == 0) {
			// TODO: not sure how this is supposed to work under Linux/FreeBSD,
			// but on my i865, this code is needed for Haiku.
			dprintf("intel_gart: Use GTT address fallback.\n");
			info.gtt_physical_base = info.display.u.h0.base_registers[mmioIndex]
				+ i830_GTT_BASE;
		}
	}

	size_t gttSize, stolenSize;
	determine_memory_sizes(info, gttSize, stolenSize);

	info.gtt_entries = gttSize / 4096;
	info.gtt_stolen_entries = stolenSize / 4096;

	TRACE("GTT base %lx, size %lu, entries %lu, stolen %lu\n",
		info.gtt_physical_base, gttSize, info.gtt_entries, stolenSize);

	AreaKeeper gttMapper;
	info.gtt_area = gttMapper.Map("intel GMCH gtt",
		(void*)info.gtt_physical_base, gttSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&info.gtt_base);
	if (gttMapper.InitCheck() < B_OK) {
		dprintf("intel_gart: could not map GTT!\n");
		return info.gtt_area;
	}

	info.aperture_physical_base = info.display.u.h0.base_registers[fbIndex];
	info.aperture_stolen_size = stolenSize;
	if (info.aperture_size == 0)
		info.aperture_size = info.display.u.h0.base_register_sizes[fbIndex];

	dprintf("intel_gart: detected %ld MB of stolen memory, aperture "
		"size %ld MB, GTT size %ld KB\n", (stolenSize + (1023 << 10)) >> 20,
		info.aperture_size >> 20, gttSize >> 10);

	dprintf("intel_gart: GTT base = 0x%lx\n", info.gtt_physical_base);
	dprintf("intel_gart: MMIO base = 0x%lx\n",
		info.display.u.h0.base_registers[mmioIndex]);
	dprintf("intel_gart: GMR base = 0x%lx\n", info.aperture_physical_base);

	AreaKeeper apertureMapper;
	info.aperture_area = apertureMapper.Map("intel graphics aperture",
		(void*)info.aperture_physical_base, info.aperture_size,
		B_ANY_KERNEL_BLOCK_ADDRESS | B_MTR_WC,
		B_READ_AREA | B_WRITE_AREA, (void**)&info.aperture_base);
	if (apertureMapper.InitCheck() < B_OK) {
		// try again without write combining
		dprintf(DEVICE_NAME ": enabling write combined mode failed.\n");

		info.aperture_area = apertureMapper.Map("intel graphics aperture",
			(void*)info.aperture_physical_base, info.aperture_size,
			B_ANY_KERNEL_BLOCK_ADDRESS, B_READ_AREA | B_WRITE_AREA,
			(void**)&info.aperture_base);
	}
	if (apertureMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": could not map graphics aperture!\n");
		return info.aperture_area;
	}

	info.scratch_page = entry.address;

	gttMapper.Detach();
	mmioMapper.Detach();
	scratchCreator.Detach();
	apertureMapper.Detach();

	return B_OK;
}


//	#pragma mark - module interface


status_t
intel_create_aperture(uint8 bus, uint8 device, uint8 function, size_t size,
	void** _aperture)
{
	// TODO: we currently only support a single AGP bridge!
	if ((bus != sInfo.bridge.bus || device != sInfo.bridge.device
			|| function != sInfo.bridge.function)
		&& (bus != sInfo.display.bus || device != sInfo.display.device
			|| function != sInfo.display.function))
		return B_BAD_VALUE;

	sInfo.aperture_size = size;

	if (intel_map(sInfo) < B_OK)
		return B_ERROR;

	uint16 gmchControl = get_pci_config(sInfo.bridge,
		INTEL_GRAPHICS_MEMORY_CONTROL, 2) | MEMORY_CONTROL_ENABLED;
	set_pci_config(sInfo.bridge, INTEL_GRAPHICS_MEMORY_CONTROL, 2, gmchControl);

	write32(sInfo.registers + INTEL_PAGE_TABLE_CONTROL,
		sInfo.gtt_physical_base | PAGE_TABLE_ENABLED);
	read32(sInfo.registers + INTEL_PAGE_TABLE_CONTROL);

	if (sInfo.scratch_page != 0) {
		for (size_t i = sInfo.gtt_stolen_entries; i < sInfo.gtt_entries; i++) {
			set_gtt_entry(sInfo, i << GTT_PAGE_SHIFT, sInfo.scratch_page);
		}
		read32(sInfo.gtt_base + sInfo.gtt_entries - 1);
	}

	asm("wbinvd;");

	*_aperture = NULL;
	return B_OK;
}


void
intel_delete_aperture(void* aperture)
{
	intel_unmap(sInfo);
}


static status_t
intel_get_aperture_info(void* aperture, aperture_info* info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	info->base = sInfo.aperture_base;
	info->physical_base = sInfo.aperture_physical_base;
	info->size = sInfo.aperture_size;
	info->reserved_size = sInfo.aperture_stolen_size;

	return B_OK;
}


status_t
intel_set_aperture_size(void* aperture, size_t size)
{
	return B_ERROR;
}


static status_t
intel_bind_page(void* aperture, uint32 offset, phys_addr_t physicalAddress)
{
	//TRACE("bind_page(offset %lx, physical %lx)\n", offset, physicalAddress);

	set_gtt_entry(sInfo, offset, physicalAddress);
	return B_OK;
}


static status_t
intel_unbind_page(void* aperture, uint32 offset)
{
	//TRACE("unbind_page(offset %lx)\n", offset);

	if (sInfo.scratch_page != 0)
		set_gtt_entry(sInfo, offset, sInfo.scratch_page);

	return B_OK;
}


void
intel_flush_tlbs(void* aperture)
{
	read32(sInfo.gtt_base + sInfo.gtt_entries - 1);
	asm("wbinvd;");
}


//	#pragma mark -


static status_t
intel_init()
{
	TRACE("bus manager init\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&sPCI) != B_OK)
		return B_ERROR;

	bool found = false;

	for (uint32 index = 0; sPCI->get_nth_pci_info(index, &sInfo.bridge) == B_OK;
			index++) {
		if (sInfo.bridge.vendor_id != VENDOR_ID_INTEL
			|| sInfo.bridge.class_base != PCI_bridge)
			continue;

		// check device
		for (uint32 i = 0; i < sizeof(kSupportedDevices)
				/ sizeof(kSupportedDevices[0]); i++) {
			if (sInfo.bridge.device_id == kSupportedDevices[i].bridge_id) {
				sInfo.type = kSupportedDevices[i].type;
				found = has_display_device(sInfo.display,
					kSupportedDevices[i].display_id);
			}
		}

		if (found)
			break;
	}

	if (!found)
		return ENODEV;

	TRACE("found intel bridge\n");
	return B_OK;
}


static void
intel_uninit()
{
}


static int32
intel_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return intel_init();
		case B_MODULE_UNINIT:
			intel_uninit();
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct agp_gart_bus_module_info sIntelModuleInfo = {
	{
		"busses/agp_gart/intel/v0",
		0,
		intel_std_ops
	},

	intel_create_aperture,
	intel_delete_aperture,

	intel_get_aperture_info,
	intel_set_aperture_size,
	intel_bind_page,
	intel_unbind_page,
	intel_flush_tlbs
};

module_info* modules[] = {
	(module_info*)&sIntelModuleInfo,
	NULL
};
