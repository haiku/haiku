/*
 * Copyright 2005, Oscar Lesta. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Drivers.h>
#include <KernelExport.h>
#include <ISA.h>
#include <PCI.h>

#include "poke.h"


////////////////////////////////////////////////////////////////////////////////

static status_t poke_open(const char*, uint32, void**);
static status_t poke_close(void*);
static status_t poke_free(void*);
static status_t poke_control(void*, uint32, void*, size_t);
static status_t poke_read(void*, off_t, void*, size_t*);
static status_t poke_write(void*, off_t, const void*, size_t*);

////////////////////////////////////////////////////////////////////////////////

static const char* poke_name[] = {
    "misc/"POKE_DEVICE_NAME,
    NULL
};


device_hooks poke_hooks = {
	poke_open,
	poke_close,
	poke_free,
	poke_control,
	poke_read,
	poke_write,
};

int32 api_version = B_CUR_DRIVER_API_VERSION;

isa_module_info* isa;
pci_module_info* pci;

static vint32 open_count;

////////////////////////////////////////////////////////////////////////////////


status_t
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	open_count = 0;

	if (get_module(B_ISA_MODULE_NAME, (module_info**)&isa) < B_OK)
		return ENOSYS;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&pci) < B_OK) {
		put_module(B_ISA_MODULE_NAME);
		return ENOSYS;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	put_module(B_ISA_MODULE_NAME);
	put_module(B_PCI_MODULE_NAME);
}


const char**
publish_devices(void)
{
	return poke_name;
}


device_hooks*
find_device(const char* name)
{
	return &poke_hooks;
}


////////////////////////////////////////////////////////////////////////////////
//	#pragma mark -


status_t
poke_open(const char* name, uint32 flags, void** cookie)
{
	*cookie = NULL;

	if (atomic_add(&open_count, 1) != 0) {
		atomic_add(&open_count, -1);
		return B_BUSY;
	}

	return B_OK;
}


status_t
poke_close(void* cookie)
{
	return B_OK;
}


status_t
poke_free(void* cookie)
{
	atomic_add(&open_count, -1);
	return B_OK;
}


status_t
poke_control(void* cookie, uint32 op, void* arg, size_t length)
{
	switch (op) {
		case POKE_PORT_READ:
		{
    		status_t result;
			port_io_args* ioctl = (port_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			result = B_OK;
			switch (ioctl->size) {
				case 1:
					ioctl->value = isa->read_io_8(ioctl->port);
				break;
				case 2:
	   				ioctl->value = isa->read_io_16(ioctl->port);
				break;
				case 4:
	   				ioctl->value = isa->read_io_32(ioctl->port);
				break;
				default:
					result = B_BAD_VALUE;
			}

			return result;
   		}

		case POKE_PORT_WRITE:
		{
    		status_t result;
			port_io_args* ioctl = (port_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			result = B_OK;
			switch (ioctl->size) {
				case 1:
					isa->write_io_8(ioctl->port, ioctl->value);
					break;
				case 2:
					isa->write_io_16(ioctl->port, ioctl->value);
					break;
				case 4:
					isa->write_io_32(ioctl->port, ioctl->value);
					break;
				default:
					result = B_BAD_VALUE;
			}

			return result;
   		}

		case POKE_PORT_INDEXED_READ:
		{
			port_io_args* ioctl = (port_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			isa->write_io_8(ioctl->port, ioctl->size);
			ioctl->value = isa->read_io_8(ioctl->port + 1);
			return B_OK;
		}

		case POKE_PORT_INDEXED_WRITE:
		{
			port_io_args* ioctl = (port_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			isa->write_io_8(ioctl->port, ioctl->size);
			isa->write_io_8(ioctl->port + 1, ioctl->value);
			return B_OK;
		}

		case POKE_PCI_READ_CONFIG:
		{
			pci_io_args* ioctl = (pci_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			ioctl->value = pci->read_pci_config(ioctl->bus, ioctl->device,
				ioctl->function, ioctl->offset, ioctl->size);
			return B_OK;
		}

		case POKE_PCI_WRITE_CONFIG:
		{
			pci_io_args* ioctl = (pci_io_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			pci->write_pci_config(ioctl->bus, ioctl->device, ioctl->function,
				ioctl->offset, ioctl->size, ioctl->value);
			return B_OK;
		}

		case POKE_GET_NTH_PCI_INFO:
		{
		    pci_info_args* ioctl = (pci_info_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			ioctl->status = pci->get_nth_pci_info(ioctl->index, ioctl->info);
			return B_OK;
		}

		case POKE_GET_PHYSICAL_ADDRESS:
		{
			mem_map_args* ioctl = (mem_map_args*)arg;
			physical_entry table;
			status_t result;

			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			result = get_memory_map(ioctl->address, ioctl->size, &table, 1);
			ioctl->physical_address = (void*)(addr_t)table.address;
				// TODO: mem_map_args::physical_address should be phys_addr_t!
			ioctl->size = table.size;
			return result;
		}

		case POKE_MAP_MEMORY:
		{
			mem_map_args* ioctl = (mem_map_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			ioctl->area = map_physical_memory(ioctl->name,
				(addr_t)ioctl->physical_address, ioctl->size, ioctl->flags,
				ioctl->protection, (void**)&ioctl->address);
			return ioctl->area;
		}

		case POKE_UNMAP_MEMORY:
		{
			mem_map_args* ioctl = (mem_map_args*)arg;
			if (ioctl->signature != POKE_SIGNATURE)
				return B_BAD_VALUE;

			return delete_area(ioctl->area);
		}
	}

	return B_BAD_VALUE;
}


status_t
poke_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_NOT_ALLOWED;
}


status_t
poke_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_NOT_ALLOWED;
}
