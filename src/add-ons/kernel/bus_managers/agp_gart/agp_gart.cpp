/*
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2004-2006, Rudolf Cornelissen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

// TODO: rethink the AGP interface for more than one bridge/device!
//	(should be done with the new driver API then)

/*
	Notes:
	- currently we just setup all found devices with AGP interface to the same
	highest common mode, we don't distinquish different AGP busses.
	TODO: it might be a better idea to just setup one instead.

	- AGP3 defines 'asynchronous request size' and 'calibration cycle' fields
	in the status and command registers. Currently programming zero's which will
	make it work, although further optimisation is possible.

	- AGP3.5 also defines isochronous transfers which are not implemented here:
	the hardware keeps them disabled by default.
*/


#include <AGP.h>

#include <stdlib.h>

#include <KernelExport.h>
#include <PCI.h>

#include <util/OpenHashTable.h>
#include <kernel/lock.h>
#include <vm/vm_page.h>
#include <vm/vm_types.h>

#include <lock.h>


#define TRACE_AGP
#ifdef TRACE_AGP
#	define TRACE(x...) dprintf("\33[36mAGP:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[36mAGP:\33[0m " x)


#define MAX_DEVICES	  8

#define AGP_ID(address) (address)
#define AGP_STATUS(address) (address + 4)
#define AGP_COMMAND(address) (address + 8)

/* read and write to PCI config space */
#define get_pci_config(info, offset, size) \
	(sPCI->read_pci_config((info).bus, (info).device, (info).function, \
		(offset), (size)))
#define set_pci_config(info, offset, size, value) \
	(sPCI->write_pci_config((info).bus, (info).device, (info).function, \
		(offset), (size), (value)))

#define RESERVED_APERTURE			0x80000000
#define ALLOCATED_APERTURE			0x40000000
#define BIND_APERTURE				0x20000000
#define APERTURE_PUBLIC_FLAGS_MASK	0x0000ffff

struct aperture_memory {
	aperture_memory *next;
	aperture_memory *hash_link;
	addr_t		base;
	size_t		size;
	uint32		flags;
#if !defined(GART_TEST)
	union {
		vm_page	**pages;
		vm_page *page;
	};
#ifdef DEBUG_PAGE_ACCESS
	thread_id	allocating_thread;
#endif
#else
	area_id		area;
#endif
};

class Aperture;

class MemoryHashDefinition {
public:
	typedef addr_t KeyType;
	typedef aperture_memory ValueType;

	MemoryHashDefinition(aperture_info &info) : fInfo(info) {}

	size_t HashKey(const KeyType &base) const
		{ return (base - fInfo.base) / B_PAGE_SIZE; }
	size_t Hash(aperture_memory *memory) const
		{ return (memory->base - fInfo.base) / B_PAGE_SIZE; }
	bool Compare(const KeyType &base, aperture_memory *memory) const
		{ return base == memory->base; }
	aperture_memory *&GetLink(aperture_memory *memory) const
		{ return memory->hash_link; }

private:
	aperture_info	&fInfo;
};

typedef BOpenHashTable<MemoryHashDefinition> MemoryHashTable;

struct agp_device_info {
	uint8		address;	/* location of AGP interface in PCI capabilities */
	agp_info	info;
};

class Aperture {
public:
	Aperture(agp_gart_bus_module_info *module, void *aperture);
	~Aperture();

	status_t InitCheck() const { return fLock.sem >= B_OK ? B_OK : fLock.sem; }

	void DeleteMemory(aperture_memory *memory);
	aperture_memory *CreateMemory(size_t size, size_t alignment, uint32 flags);

	status_t AllocateMemory(aperture_memory *memory, uint32 flags);

	status_t UnbindMemory(aperture_memory *memory);
	status_t BindMemory(aperture_memory *memory, addr_t base, size_t size);

	status_t GetInfo(aperture_info *info);

	aperture_memory *GetMemory(addr_t base) { return fHashTable.Lookup(base); }

	addr_t Base() const { return fInfo.base; }
	addr_t Size() const { return fInfo.size; }
	int32 ID() const { return fID; }
	struct lock &Lock() { return fLock; }

private:
	bool _AdaptToReserved(addr_t &base, size_t &size, int32 *_offset = NULL);
	void _Free(aperture_memory *memory);
	void _Remove(aperture_memory *memory);
	status_t _Insert(aperture_memory *memory, size_t size, size_t alignment,
		uint32 flags);

	struct lock					fLock;
	agp_gart_bus_module_info	*fModule;
	int32						fID;
	aperture_info				fInfo;
	MemoryHashTable				fHashTable;
	aperture_memory				*fFirstMemory;
	void						*fPrivateAperture;

public:
	Aperture					*fNext;
};

class ApertureHashDefinition {
public:
	typedef int32 KeyType;
	typedef Aperture ValueType;

	size_t HashKey(const KeyType &id) const
		{ return id; }
	size_t Hash(Aperture *aperture) const
		{ return aperture->ID(); }
	bool Compare(const KeyType &id, Aperture *aperture) const
		{ return id == aperture->ID(); }
	Aperture *&GetLink(Aperture *aperture) const
		{ return aperture->fNext; }
};

typedef BOpenHashTable<ApertureHashDefinition> ApertureHashTable;


static agp_device_info sDeviceInfos[MAX_DEVICES];
static uint32 sDeviceCount;
static pci_module_info *sPCI;
static int32 sAcquired;
static ApertureHashTable sApertureHashTable;
static int32 sNextApertureID;
static struct lock sLock;


//	#pragma mark - private support functions


/*!	Makes sure that all bits lower than the maximum supported rate is set. */
static uint32
fix_rate_support(uint32 command)
{
	if ((command & AGP_3_MODE) != 0) {
		if ((command & AGP_3_8x) != 0)
			command |= AGP_3_4x;

		command &= ~AGP_RATE_MASK | AGP_3_8x | AGP_3_4x;
		command |= AGP_SBA;
			// SBA is required for AGP3
	} else {
		/* AGP 2.0 scheme applies */
		if ((command & AGP_2_4x) != 0)
			command |= AGP_2_2x;
		if ((command & AGP_2_2x) != 0)
			command |= AGP_2_1x;
	}

	return command;
}


/*!	Makes sure that only the highest rate bit is set. */
static uint32
fix_rate_command(uint32 command)
{
	if ((command & AGP_3_MODE) != 0) {
		if ((command & AGP_3_8x) != 0)
			command &= ~AGP_3_4x;
	} else {
		/* AGP 2.0 scheme applies */
		if ((command & AGP_2_4x) != 0)
			command &= ~(AGP_2_2x | AGP_2_1x);
		if ((command & AGP_2_2x) != 0)
			command &= ~AGP_2_1x;
	}

	return command;
}


/*!	Checks the capabilities of the device, and removes everything from
	\a command that the device does not support.
*/
static void
check_capabilities(agp_device_info &deviceInfo, uint32 &command)
{
	uint32 agpStatus = deviceInfo.info.interface.status;
	if (deviceInfo.info.class_base == PCI_bridge) {
		// make sure the AGP rate support mask is correct
		// (ie. has the lower bits set)
		agpStatus = fix_rate_support(agpStatus);
	}

	TRACE("device %u.%u.%u has AGP capabilities %" B_PRIx32 "\n", deviceInfo.info.bus,
		deviceInfo.info.device, deviceInfo.info.function, agpStatus);

	// block non-supported AGP modes
	command &= (agpStatus & (AGP_3_MODE | AGP_RATE_MASK))
		| ~(AGP_3_MODE | AGP_RATE_MASK);

	// If no AGP mode is supported at all, nothing remains:
	// devices exist that have the AGP style connector with AGP style registers,
	// but not the features!
	// (confirmed Matrox Millenium II AGP for instance)
	if ((agpStatus & AGP_RATE_MASK) == 0)
		command = 0;

	// block side band adressing if not supported
	if ((agpStatus & AGP_SBA) == 0)
		command &= ~AGP_SBA;

	// block fast writes if not supported
	if ((agpStatus & AGP_FAST_WRITE) == 0)
		command &= ~AGP_FAST_WRITE;

	// adjust maximum request depth to least depth supported
	// note: this is writable only in the graphics card
	uint8 requestDepth = ((agpStatus & AGP_REQUEST) >> AGP_REQUEST_SHIFT);
	if (requestDepth < ((command & AGP_REQUEST) >> AGP_REQUEST_SHIFT)) {
		command &= ~AGP_REQUEST;
		command |= (requestDepth << AGP_REQUEST_SHIFT);
	}
}


/*!	Checks the PCI capabilities if the device is an AGP device
*/
static bool
is_agp_device(pci_info &info, uint8 *_address)
{
	// Check if device implements a list of capabilities
	if ((get_pci_config(info, PCI_status, 2) & PCI_status_capabilities) == 0)
		return false;

	// Get pointer to PCI capabilities list
	// (AGP devices only, no need to take cardbus into account)
	uint8 address = get_pci_config(info, PCI_capabilities_ptr, 1);

	while (true) {
		uint8 id = get_pci_config(info, address, 1);
		uint8 next = get_pci_config(info, address + 1, 1) & ~0x3;

		if (id == PCI_cap_id_agp) {
			// is an AGP device
			if (_address != NULL)
				*_address = address;
			return true;
		}
		if (next == 0) {
			// end of list
			break;
		}

		address = next;
	}

	return false;
}


static status_t
get_next_agp_device(uint32 *_cookie, pci_info &info, agp_device_info &device)
{
	uint32 index = *_cookie;

	// find devices

	for (; sPCI->get_nth_pci_info(index, &info) == B_OK; index++) {
		// is it a bridge or a graphics card?
		if ((info.class_base != PCI_bridge || info.class_sub != PCI_host)
			&& info.class_base != PCI_display)
			continue;

		if (is_agp_device(info, &device.address)) {
			device.info.vendor_id = info.vendor_id;
			device.info.device_id = info.device_id;
			device.info.bus = info.bus;
			device.info.device = info.device;
			device.info.function = info.function;
			device.info.class_sub = info.class_sub;
			device.info.class_base = info.class_base;

			/* get the contents of the AGP registers from this device */
			device.info.interface.capability_id = get_pci_config(info,
				AGP_ID(device.address), 4);
			device.info.interface.status = get_pci_config(info,
				AGP_STATUS(device.address), 4);
			device.info.interface.command = get_pci_config(info,
				AGP_COMMAND(device.address), 4);

			*_cookie = index + 1;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static void
set_agp_command(agp_device_info &deviceInfo, uint32 command)
{
	set_pci_config(deviceInfo.info, AGP_COMMAND(deviceInfo.address), 4, command);
	deviceInfo.info.interface.command = get_pci_config(deviceInfo.info,
		AGP_COMMAND(deviceInfo.address), 4);
}


static void
set_pci_mode()
{
	TRACE("set PCI mode on all AGP capable devices.\n");

	// First program all graphics cards

	for (uint32 index = 0; index < sDeviceCount; index++) {
		agp_device_info &deviceInfo = sDeviceInfos[index];
		if (deviceInfo.info.class_base != PCI_display)
			continue;

		set_agp_command(deviceInfo, 0);
	}

	// Then program all bridges - it's the other around for AGP mode

	for (uint32 index = 0; index < sDeviceCount; index++) {
		agp_device_info &deviceInfo = sDeviceInfos[index];
		if (deviceInfo.info.class_base != PCI_bridge)
			continue;

		set_agp_command(deviceInfo, 0);
	}

	// Wait 10mS for the bridges to recover (failsafe!)
	// Note: some SiS bridge chipsets apparantly require 5mS to recover
	// or the master (graphics card) cannot be initialized correctly!
	snooze(10000);
}


status_t
get_area_base_and_size(area_id area, addr_t &base, size_t &size)
{
	area_info info;
	status_t status = get_area_info(area, &info);
	if (status < B_OK)
		return status;

	base = (addr_t)info.address;
	size = info.size;
	return B_OK;
}


Aperture *
get_aperture(aperture_id id)
{
	Autolock _(sLock);
	return sApertureHashTable.Lookup(id);
}


//	#pragma mark - Aperture


Aperture::Aperture(agp_gart_bus_module_info *module, void *aperture)
	:
	fModule(module),
	fHashTable(fInfo),
	fFirstMemory(NULL),
	fPrivateAperture(aperture)
{
	fModule->get_aperture_info(fPrivateAperture, &fInfo);
	fID = atomic_add(&sNextApertureID, 1);
	init_lock(&fLock, "aperture");
}


Aperture::~Aperture()
{
	while (fFirstMemory != NULL) {
		DeleteMemory(fFirstMemory);
	}

	fModule->delete_aperture(fPrivateAperture);
	put_module(fModule->info.name);
}


status_t
Aperture::GetInfo(aperture_info *info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	*info = fInfo;
	return B_OK;
}


void
Aperture::DeleteMemory(aperture_memory *memory)
{
	TRACE("delete memory %p\n", memory);

	UnbindMemory(memory);
	_Free(memory);
	_Remove(memory);
	fHashTable.Remove(memory);
	delete memory;
}


aperture_memory *
Aperture::CreateMemory(size_t size, size_t alignment, uint32 flags)
{
	aperture_memory *memory = new(std::nothrow) aperture_memory;
	if (memory == NULL)
		return NULL;

	status_t status = _Insert(memory, size, alignment, flags);
	if (status < B_OK) {
		ERROR("Aperture::CreateMemory(): did not find a free space large for "
			"this memory object\n");
		delete memory;
		return NULL;
	}

	TRACE("create memory %p, base %" B_PRIxADDR ", size %" B_PRIxSIZE
		", flags %" B_PRIx32 "\n", memory, memory->base, memory->size, flags);

	memory->flags = flags;
#if !defined(GART_TEST)
	memory->pages = NULL;
#else
	memory->area = -1;
#endif

	fHashTable.Insert(memory);
	return memory;
}


bool
Aperture::_AdaptToReserved(addr_t &base, size_t &size, int32 *_offset)
{
	addr_t reservedEnd = fInfo.base + fInfo.reserved_size;
	if (reservedEnd <= base)
		return false;

	if (reservedEnd >= base + size) {
		size = 0;
		return true;
	}

	if (_offset != NULL)
		*_offset = reservedEnd - base;

	size -= reservedEnd - base;
	base = reservedEnd;
	return true;
}


status_t
Aperture::AllocateMemory(aperture_memory *memory, uint32 flags)
{
	// We don't need to allocate reserved memory - it's
	// already there for us to use
	addr_t base = memory->base;
	size_t size = memory->size;
	if (_AdaptToReserved(base, size)) {
		if (size == 0) {
			TRACE("allocation is made of reserved memory\n");
			return B_OK;
		}

		memset((void *)memory->base, 0, memory->size - size);
	}
	TRACE("allocate %ld bytes out of %ld\n", size, memory->size);

#if !defined(GART_TEST)
	uint32 count = size / B_PAGE_SIZE;

	if ((flags & B_APERTURE_NEED_PHYSICAL) != 0) {
		physical_address_restrictions restrictions = {};
#if B_HAIKU_PHYSICAL_BITS > 32
		restrictions.high_address = (phys_addr_t)1 << 32;
			// TODO: Work-around until intel_gart can deal with physical
			// addresses > 4 GB.
#endif
		memory->page = vm_page_allocate_page_run(
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR, count, &restrictions,
			VM_PRIORITY_SYSTEM);
		if (memory->page == NULL) {
			ERROR("Aperture::AllocateMemory(): vm_page_allocate_page_run() "
				"failed (with B_APERTURE_NEED_PHYSICAL)\n");
			return B_NO_MEMORY;
		}
	} else {
		// Allocate table to hold the pages
		memory->pages = (vm_page **)malloc(count * sizeof(vm_page *));
		if (memory->pages == NULL)
			return B_NO_MEMORY;

#if B_HAIKU_PHYSICAL_BITS > 32
		// TODO: Work-around until intel_gart can deal with physical
		// addresses > 4 GB.
		physical_address_restrictions restrictions = {};
		restrictions.high_address = (phys_addr_t)1 << 32;
		vm_page* page = vm_page_allocate_page_run(
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR, count, &restrictions,
			VM_PRIORITY_SYSTEM);
		if (page == NULL) {
			ERROR("Aperture::AllocateMemory(): vm_page_allocate_page_run() "
				"failed (without B_APERTURE_NEED_PHYSICAL)\n");
			return B_NO_MEMORY;
		}

		for (uint32 i = 0; i < count; i++)
			memory->pages[i] = page + i;
#else
		vm_page_reservation reservation;
		vm_page_reserve_pages(&reservation, count, VM_PRIORITY_SYSTEM);
		for (uint32 i = 0; i < count; i++) {
			memory->pages[i] = vm_page_allocate_page(&reservation,
				PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		}
		vm_page_unreserve_pages(&reservation);
#endif
	}

#ifdef DEBUG_PAGE_ACCESS
	memory->allocating_thread = find_thread(NULL);
#endif

#else	// GART_TEST
	void *address;
	memory->area = create_area("GART memory", &address, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK | ((flags & B_APERTURE_NEED_PHYSICAL) != 0
			? B_CONTIGUOUS : 0), 0);
	if (memory->area < B_OK) {
		ERROR("Aperture::AllocateMemory(): create_area() failed\n");
		return B_NO_MEMORY;
	}
#endif

	memory->flags |= ALLOCATED_APERTURE;
	return B_OK;
}


status_t
Aperture::UnbindMemory(aperture_memory *memory)
{
	if ((memory->flags & BIND_APERTURE) == 0)
		return B_BAD_VALUE;

	// We must not unbind reserved memory
	addr_t base = memory->base;
	size_t size = memory->size;
	if (_AdaptToReserved(base, size) && size == 0) {
		memory->flags &= ~BIND_APERTURE;
		return B_OK;
	}

	addr_t start = base - Base();
	TRACE("unbind %ld bytes at %lx\n", size, start);

	for (addr_t offset = 0; offset < memory->size; offset += B_PAGE_SIZE) {
		status_t status = fModule->unbind_page(fPrivateAperture, start + offset);
		if (status < B_OK)
			return status;
	}

	memory->flags &= ~BIND_APERTURE;
	fModule->flush_tlbs(fPrivateAperture);
	return B_OK;
}


status_t
Aperture::BindMemory(aperture_memory *memory, addr_t address, size_t size)
{
	bool physical = false;

	if ((memory->flags & ALLOCATED_APERTURE) != 0) {
		// We allocated this memory, get the base and size from there
		size = memory->size;
		physical = true;
	}

	// We don't need to bind reserved memory
	addr_t base = memory->base;
	int32 offset;
	if (_AdaptToReserved(base, size, &offset)) {
		if (size == 0) {
			TRACE("reserved memory already bound\n");
			memory->flags |= BIND_APERTURE;
			return B_OK;
		}

		address += offset;
	}

	addr_t start = base - Base();
	TRACE("bind %ld bytes at %lx\n", size, base);

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		phys_addr_t physicalAddress = 0;
		status_t status;

		if (!physical) {
			physical_entry entry;
			status = get_memory_map((void *)(address + offset), B_PAGE_SIZE,
				&entry, 1);
			if (status < B_OK) {
				ERROR("Aperture::BindMemory(): get_memory_map() failed\n");
				return status;
			}

			physicalAddress = entry.address;
		} else {
			uint32 index = offset >> PAGE_SHIFT;
			vm_page *page;
			if ((memory->flags & B_APERTURE_NEED_PHYSICAL) != 0)
				page = memory->page + index;
			else
				page = memory->pages[index];

			physicalAddress
				= (phys_addr_t)page->physical_page_number << PAGE_SHIFT;
		}

		status = fModule->bind_page(fPrivateAperture, start + offset,
			physicalAddress);
		if (status < B_OK) {
			ERROR("Aperture::BindMemory(): bind_page() failed\n");
			return status;
		}
	}

	memory->flags |= BIND_APERTURE;
	fModule->flush_tlbs(fPrivateAperture);
	return B_OK;
}


void
Aperture::_Free(aperture_memory *memory)
{
	if ((memory->flags & ALLOCATED_APERTURE) == 0)
		return;

#if !defined(GART_TEST)
	// Remove the stolen area from the allocation
	size_t size = memory->size;
	addr_t reservedEnd = fInfo.base + fInfo.reserved_size;
	if (memory->base < reservedEnd)
		size -= reservedEnd - memory->base;

	// Free previously allocated pages and page table
	uint32 count = size / B_PAGE_SIZE;

	if ((memory->flags & B_APERTURE_NEED_PHYSICAL) != 0) {
		vm_page *page = memory->page;
		for (uint32 i = 0; i < count; i++, page++) {
			DEBUG_PAGE_ACCESS_TRANSFER(page, memory->allocating_thread);
			vm_page_set_state(page, PAGE_STATE_FREE);
		}

		memory->page = NULL;
	} else {
		for (uint32 i = 0; i < count; i++) {
			DEBUG_PAGE_ACCESS_TRANSFER(memory->pages[i],
				memory->allocating_thread);
			vm_page_set_state(memory->pages[i], PAGE_STATE_FREE);
		}

		free(memory->pages);
		memory->pages = NULL;
	}
#else
	delete_area(memory->area);
	memory->area = -1;
#endif

	memory->flags &= ~ALLOCATED_APERTURE;
}


void
Aperture::_Remove(aperture_memory *memory)
{
	aperture_memory *current = fFirstMemory, *last = NULL;

	while (current != NULL) {
		if (memory == current) {
			if (last != NULL) {
				last->next = current->next;
			} else {
				fFirstMemory = current->next;
			}
			break;
		}

		last = current;
		current = current->next;
	}
}


status_t
Aperture::_Insert(aperture_memory *memory, size_t size, size_t alignment,
	uint32 flags)
{
	aperture_memory *last = NULL;
	aperture_memory *next;
	bool foundSpot = false;

	// do some sanity checking
	if (size == 0 || size > fInfo.size)
		return B_BAD_VALUE;

	if (alignment < B_PAGE_SIZE)
		alignment = B_PAGE_SIZE;

	addr_t start = fInfo.base;
	if ((flags & (B_APERTURE_NON_RESERVED | B_APERTURE_NEED_PHYSICAL)) != 0)
		start += fInfo.reserved_size;

	start = ROUNDUP(start, alignment);
	if (start > fInfo.base - 1 + fInfo.size || start < fInfo.base)
		return B_NO_MEMORY;

	// walk up to the spot where we should start searching

	next = fFirstMemory;
	while (next) {
		if (next->base >= start + size) {
			// we have a winner
			break;
		}
		last = next;
		next = next->next;
	}

	// find a big enough hole
	if (last == NULL) {
		// see if we can build it at the beginning of the virtual map
		if (next == NULL || (next->base >= ROUNDUP(start, alignment) + size)) {
			memory->base = ROUNDUP(start, alignment);
			foundSpot = true;
		} else {
			last = next;
			next = next->next;
		}
	}

	if (!foundSpot) {
		// keep walking
		while (next != NULL) {
			if (next->base >= ROUNDUP(last->base + last->size, alignment) + size) {
				// we found a spot (it'll be filled up below)
				break;
			}
			last = next;
			next = next->next;
		}

		if ((fInfo.base + (fInfo.size - 1)) >= (ROUNDUP(last->base + last->size,
				alignment) + (size - 1))) {
			// got a spot
			foundSpot = true;
			memory->base = ROUNDUP(last->base + last->size, alignment);
			if (memory->base < start)
				memory->base = start;
		}

		if (!foundSpot)
			return B_NO_MEMORY;
	}

	memory->size = size;
	if (last) {
		memory->next = last->next;
		last->next = memory;
	} else {
		memory->next = fFirstMemory;
		fFirstMemory = memory;
	}

	return B_OK;
}


//	#pragma mark - AGP module interface


status_t
get_nth_agp_info(uint32 index, agp_info *info)
{
	TRACE("get_nth_agp_info(index %" B_PRIu32 ")\n", index);

	if (index >= sDeviceCount)
		return B_BAD_VALUE;

	// refresh from the contents of the AGP registers from this device
	sDeviceInfos[index].info.interface.status = get_pci_config(
		sDeviceInfos[index].info, AGP_STATUS(sDeviceInfos[index].address), 4);
	sDeviceInfos[index].info.interface.command = get_pci_config(
		sDeviceInfos[index].info, AGP_COMMAND(sDeviceInfos[index].address), 4);

	*info = sDeviceInfos[index].info;
	return B_OK;
}


status_t
acquire_agp(void)
{
	if (atomic_or(&sAcquired, 1) == 1)
		return B_BUSY;

	return B_OK;
}


void
release_agp(void)
{
	atomic_and(&sAcquired, 0);
}


uint32
set_agp_mode(uint32 command)
{
	TRACE("set_agp_mode(command %" B_PRIx32 ")\n", command);

	if ((command & AGP_ENABLE) == 0) {
		set_pci_mode();
		return 0;
	}

	// Make sure we accept all modes lower than requested one and we
	// reset reserved bits
	command = fix_rate_support(command);

	// iterate through our device list to find the common capabilities supported
	for (uint32 index = 0; index < sDeviceCount; index++) {
		agp_device_info &deviceInfo = sDeviceInfos[index];

		// Refresh from the contents of the AGP capability registers
		// (note: some graphics driver may have been tweaking, like nvidia)
		deviceInfo.info.interface.status = get_pci_config(deviceInfo.info,
			AGP_STATUS(deviceInfo.address), 4);

		check_capabilities(deviceInfo, command);
	}

	command = fix_rate_command(command);
	TRACE("set AGP command %" B_PRIx32 " on all capable devices.\n", command);

	// The order of programming differs for enabling/disabling AGP mode
	// (see AGP specification)

	// First program all bridges (master)

	for (uint32 index = 0; index < sDeviceCount; index++) {
		agp_device_info &deviceInfo = sDeviceInfos[index];
		if (deviceInfo.info.class_base != PCI_bridge)
			continue;

		set_agp_command(deviceInfo, command);
	}

	// Wait 10mS for the bridges to recover (failsafe, see set_pci_mode()!)
	snooze(10000);

	// Then all graphics cards (target)

	for (uint32 index = 0; index < sDeviceCount; index++) {
		agp_device_info &deviceInfo = sDeviceInfos[index];
		if (deviceInfo.info.class_base != PCI_display)
			continue;

		set_agp_command(deviceInfo, command);
	}

	return command;
}


//	#pragma mark - GART module interface


static aperture_id
map_aperture(uint8 bus, uint8 device, uint8 function, size_t size,
	addr_t *_apertureBase)
{
	void *iterator = open_module_list("busses/agp_gart");
	status_t status = B_ENTRY_NOT_FOUND;
	Aperture *aperture = NULL;

	Autolock _(sLock);

	while (true) {
		char name[256];
		size_t nameLength = sizeof(name);
		if (read_next_module_name(iterator, name, &nameLength) != B_OK)
			break;

		agp_gart_bus_module_info *module;
		if (get_module(name, (module_info **)&module) == B_OK) {
			void *privateAperture;
			status = module->create_aperture(bus, device, function, size,
				&privateAperture);
			if (status < B_OK) {
				put_module(name);
				continue;
			}

			aperture = new(std::nothrow) Aperture(module, privateAperture);
			status = aperture->InitCheck();
			if (status == B_OK) {
				if (_apertureBase != NULL)
					*_apertureBase = aperture->Base();

				sApertureHashTable.Insert(aperture);
			} else {
				delete aperture;
				aperture = NULL;
			}
			break;
		}
	}

	close_module_list(iterator);
	return aperture != NULL ? aperture->ID() : status;
}


static aperture_id
map_custom_aperture(gart_bus_module_info *module, addr_t *_apertureBase)
{
	return B_ERROR;
}


static status_t
unmap_aperture(aperture_id id)
{
	Autolock _(sLock);
	Aperture *aperture = sApertureHashTable.Lookup(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	sApertureHashTable.Remove(aperture);
	delete aperture;
	return B_OK;
}


static status_t
get_aperture_info(aperture_id id, aperture_info *info)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	Autolock _(aperture->Lock());
	return aperture->GetInfo(info);
}


static status_t
allocate_memory(aperture_id id, size_t size, size_t alignment, uint32 flags,
	addr_t *_apertureBase, phys_addr_t *_physicalBase)
{
	if ((flags & ~APERTURE_PUBLIC_FLAGS_MASK) != 0 || _apertureBase == NULL)
		return B_BAD_VALUE;

	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	size = ROUNDUP(size, B_PAGE_SIZE);

	Autolock _(aperture->Lock());

	aperture_memory *memory = aperture->CreateMemory(size, alignment, flags);
	if (memory == NULL)
		return B_NO_MEMORY;

	status_t status = aperture->AllocateMemory(memory, flags);
	if (status == B_OK)
		status = aperture->BindMemory(memory, 0, 0);
	if (status < B_OK) {
		aperture->DeleteMemory(memory);
		return status;
	}

	if (_physicalBase != NULL && (flags & B_APERTURE_NEED_PHYSICAL) != 0) {
#if !defined(GART_TEST)
		*_physicalBase
			= (phys_addr_t)memory->page->physical_page_number * B_PAGE_SIZE;
#else
		physical_entry entry;
		status = get_memory_map((void *)memory->base, B_PAGE_SIZE, &entry, 1);
		if (status < B_OK) {
			aperture->DeleteMemory(memory);
			return status;
		}

		*_physicalBase = entry.address;
#endif
	}

	*_apertureBase = memory->base;
	return B_OK;
}


static status_t
free_memory(aperture_id id, addr_t base)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	Autolock _(aperture->Lock());
	aperture_memory *memory = aperture->GetMemory(base);
	if (memory == NULL)
		return B_BAD_VALUE;

	aperture->DeleteMemory(memory);
	return B_OK;
}


static status_t
reserve_aperture(aperture_id id, size_t size, addr_t *_apertureBase)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	return B_ERROR;
}


static status_t
unreserve_aperture(aperture_id id, addr_t apertureBase)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	return B_ERROR;
}


static status_t
bind_aperture(aperture_id id, area_id area, addr_t base, size_t size,
	size_t alignment, addr_t reservedBase, addr_t *_apertureBase)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	if (area < 0) {
		if (size == 0 || size > aperture->Size()
			|| (base & (B_PAGE_SIZE - 1)) != 0
			|| base == 0)
			return B_BAD_VALUE;

		size = ROUNDUP(size, B_PAGE_SIZE);
	}

	if (area >= 0) {
		status_t status = get_area_base_and_size(area, base, size);
		if (status < B_OK)
			return status;
	}

	Autolock _(aperture->Lock());
	aperture_memory *memory = NULL;
	if (reservedBase != 0) {
		// use reserved aperture to bind the pages
		memory = aperture->GetMemory(reservedBase);
		if (memory == NULL)
			return B_BAD_VALUE;
	} else {
		// create new memory object
		memory = aperture->CreateMemory(size, alignment,
			B_APERTURE_NON_RESERVED);
		if (memory == NULL)
			return B_NO_MEMORY;
	}

	// just bind the physical pages backing the memory into the GART

	status_t status = aperture->BindMemory(memory, base, size);
	if (status < B_OK) {
		if (reservedBase != 0)
			aperture->DeleteMemory(memory);

		return status;
	}

	if (_apertureBase != NULL)
		*_apertureBase = memory->base;

	return B_OK;
}


static status_t
unbind_aperture(aperture_id id, addr_t base)
{
	Aperture *aperture = get_aperture(id);
	if (aperture == NULL)
		return B_ENTRY_NOT_FOUND;

	Autolock _(aperture->Lock());
	aperture_memory *memory = aperture->GetMemory(base);
	if (memory == NULL || (memory->flags & BIND_APERTURE) == 0)
		return B_BAD_VALUE;

	if ((memory->flags & ALLOCATED_APERTURE) != 0)
		panic("unbind memory %lx (%p) allocated by agp_gart.", base, memory);

	status_t status = aperture->UnbindMemory(memory);
	if (status < B_OK)
		return status;

	if ((memory->flags & RESERVED_APERTURE) == 0)
		aperture->DeleteMemory(memory);

	return B_OK;
}


//	#pragma mark -


static status_t
agp_init(void)
{
	TRACE("bus manager init\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&sPCI) != B_OK)
		return B_ERROR;

	uint32 cookie = 0;
	sDeviceCount = 0;
	pci_info info;
	while (get_next_agp_device(&cookie, info, sDeviceInfos[sDeviceCount])
			== B_OK) {
		sDeviceCount++;
	}

	TRACE("found %" B_PRId32 " AGP devices\n", sDeviceCount);

	// Since there can be custom aperture modules (for memory management only),
	// we always succeed if we could get the resources we need.

	new(&sApertureHashTable) ApertureHashTable();
	return init_lock(&sLock, "agp_gart");
}


void
agp_uninit(void)
{
	TRACE("bus manager uninit\n");

	ApertureHashTable::Iterator iterator = sApertureHashTable.GetIterator();
	while (iterator.HasNext()) {
		Aperture *aperture = iterator.Next();
		sApertureHashTable.Remove(aperture);
		delete aperture;
	}

	put_module(B_PCI_MODULE_NAME);
}


static int32
agp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return agp_init();
		case B_MODULE_UNINIT:
			agp_uninit();
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct agp_gart_module_info sAGPModuleInfo = {
	{
		{
			B_AGP_GART_MODULE_NAME,
			B_KEEP_LOADED,		// Keep loaded, even if no driver requires it
			agp_std_ops
		},
		NULL 					// the rescan function
	},
	get_nth_agp_info,
	acquire_agp,
	release_agp,
	set_agp_mode,

	map_aperture,
	map_custom_aperture,
	unmap_aperture,
	get_aperture_info,
	allocate_memory,
	free_memory,
	reserve_aperture,
	unreserve_aperture,
	bind_aperture,
	unbind_aperture,
};

module_info *modules[] = {
	(module_info *)&sAGPModuleInfo,
	NULL
};
