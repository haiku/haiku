#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AGP.h>
#include <PCI.h>
#include <KernelExport.h>


extern "C" status_t _add_builtin_module(module_info *info);

extern module_info* modules[];
static agp_gart_module_info* sGART;
static pci_module_info sPCI;
static void* sApertureBase;


//	#pragma mark - Kernel & PCI emulation


static long
pci_get_nth_pci_info(long index, pci_info* info)
{
	return B_ENTRY_NOT_FOUND;
}


static status_t
pci_std_ops(int32, ...)
{
	return B_OK;
}


extern "C" status_t
get_memory_map(const void* address, ulong numBytes, physical_entry* table,
	long numEntries)
{
	table[0].address = (void *)((addr_t)address - 0x100000);
	table[0].size = numBytes;
	//dprintf("GET_MEMORY_MAP: %p -> %p\n", address, table[0].address);
	return B_OK;
}


//	#pragma mark - GART bus module


status_t
gart_create_aperture(uint8 bus, uint8 device, uint8 function, size_t size,
	void **_aperture)
{
	dprintf("  gart_create_aperture(%d.%d.%d, %lu bytes)\n", bus, device,
		function, size);

	sApertureBase = memalign(65536, B_PAGE_SIZE);
	*_aperture = (void *)0x42;
	return B_OK;
}


void
gart_delete_aperture(void *aperture)
{
	dprintf("  gart_delete_aperture(%p)\n", aperture);
}


static status_t
gart_get_aperture_info(void *aperture, aperture_info *info)
{
	dprintf("  gart_get_aperture_info(%p)\n", aperture);

	info->base = (addr_t)sApertureBase;
	info->physical_base = 0x800000;
	info->size = 65536;
	info->reserved_size = 3 * 4096;

	return B_OK;
}


status_t
gart_set_aperture_size(void *aperture, size_t size)
{
	dprintf("  gart_set_aperture_size(%p, %lu)\n", aperture, size);
	return B_ERROR;
}


static status_t
gart_bind_page(void *aperture, uint32 offset, addr_t physicalAddress)
{
	dprintf("  gart_bind_page(%p, offset %lx, physical %lx)\n", aperture,
		offset, physicalAddress);
	return B_OK;
}


static status_t
gart_unbind_page(void *aperture, uint32 offset)
{
	dprintf("  gart_unbind_page(%p, offset %lx)\n", aperture, offset);
	return B_OK;
}


void
gart_flush_tlbs(void *aperture)
{
	dprintf("  gart_flush_tlbs(%p)\n", aperture);
}


static int32
gart_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			dprintf("GART init\n");
			return B_OK;
		case B_MODULE_UNINIT:
			dprintf("GART uninit\n");
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct agp_gart_bus_module_info sGARTModuleInfo = {
	{
		"busses/agp_gart/test/v0",
		0,
		gart_std_ops
	},

	gart_create_aperture,
	gart_delete_aperture,

	gart_get_aperture_info,
	gart_set_aperture_size,
	gart_bind_page,
	gart_unbind_page,
	gart_flush_tlbs
};


//	#pragma mark - Tests


void
allocate(aperture_id aperture, size_t size, size_t alignment, uint32 flags,
	addr_t& base, addr_t& physical)
{
	printf("Alloc %lu bytes, alignment %ld%s\n", size, alignment,
		flags & B_APERTURE_NEED_PHYSICAL ? ", need-physical"
		: flags & B_APERTURE_NON_RESERVED ? ", non-reserved" : "");

	status_t status = sGART->allocate_memory(aperture, size, alignment, flags,
		&base, &physical);
	if (status == B_OK)
		printf("  -> base %lx, physical %lx\n", base, physical);
	else
		printf("  -> failed: %s\n", strerror(status));	
}


void
test_gart()
{
	addr_t apertureBase;
	aperture_id aperture = sGART->map_aperture(0, 0, 0, 0, &apertureBase);
	printf("Map Aperture: %ld, base %lx\n", aperture, apertureBase);

	aperture_info info;
	sGART->get_aperture_info(aperture, &info);
	printf("Aperture: base %lx, physical base %lx, size %ld, reserved %ld\n",
		info.base, info.physical_base, info.size, info.reserved_size);

	addr_t base[5], physical[5];
	allocate(aperture, 2 * B_PAGE_SIZE, 0, 0, base[0], physical[0]);
	allocate(aperture, 4 * B_PAGE_SIZE, 0, B_APERTURE_NON_RESERVED, base[1],
		physical[1]);
	allocate(aperture, 1 * B_PAGE_SIZE, 0, B_APERTURE_NEED_PHYSICAL, base[2],
		physical[2]);
	sGART->deallocate_memory(aperture, base[2]);
	allocate(aperture, 1 * B_PAGE_SIZE, 4 * B_PAGE_SIZE, 0, base[2],
		physical[2]);

	sGART->deallocate_memory(aperture, base[1]);

	allocate(aperture, 5 * B_PAGE_SIZE, 0, 0, base[1], physical[1]);

	sGART->deallocate_memory(aperture, base[2]);
	sGART->deallocate_memory(aperture, base[0]);
	if (sGART->deallocate_memory(aperture, 0x12345) == B_OK)
		debugger("Non-allocated succeeded to be freed!\n");

	void *buffer = memalign(3 * B_PAGE_SIZE, B_PAGE_SIZE);
	status_t status = sGART->bind_aperture(aperture, -1, (addr_t)buffer,
		3 * B_PAGE_SIZE, 0, false, 0, &base[3]);
	if (status < B_OK)
		printf("binding memory failed: %s\n", strerror(status));

	allocate(aperture, 25 * B_PAGE_SIZE, 0, 0, base[0], physical[0]);
		// will fail
	allocate(aperture, 4 * B_PAGE_SIZE, 0, 0, base[0], physical[0]);

	void *address;
	area_id area = create_area("test", &address, B_ANY_ADDRESS, 2 * B_PAGE_SIZE,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	printf("Area %ld, address %p\n", area, address);
	status = sGART->bind_aperture(aperture, area, 0, 0, 0, false, 0, &base[4]);
	if (status < B_OK)
		printf("binding area failed: %s\n", strerror(status));

	sGART->unbind_aperture(aperture, base[3]);
	sGART->unbind_aperture(aperture, base[4]);
//	sGART->deallocate_memory(aperture, base[0]);

	free(buffer);
	delete_area(area);

	sGART->unmap_aperture(aperture);
}


int
main(int argc, char** argv)
{
	sPCI.binfo.minfo.name = B_PCI_MODULE_NAME;
	sPCI.binfo.minfo.std_ops = pci_std_ops;
	sPCI.get_nth_pci_info = pci_get_nth_pci_info;

	_add_builtin_module(modules[0]);
	_add_builtin_module((module_info*)&sPCI);
	_add_builtin_module((module_info*)&sGARTModuleInfo);

	if (get_module(B_AGP_GART_MODULE_NAME, (module_info**)&sGART) != B_OK)
		return 1;

	test_gart();

	put_module(B_AGP_GART_MODULE_NAME);
	return 0;
}

