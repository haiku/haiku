/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"

#include <stdlib.h>

#include <arch/cpu.h>

#include <compat/dev/pci/pcireg.h>
#include <compat/dev/pci/pcivar.h>
#include <compat/machine/resource.h>
#include <compat/sys/mutex.h>
#include <compat/machine/bus.h>
#include <compat/sys/rman.h>
#include <compat/sys/bus.h>

// private kernel header to get B_NO_HANDLED_INFO
#include <int.h>

#include <PCI_x86.h>


//#define DEBUG_BUS_SPACE_RW
#ifdef DEBUG_BUS_SPACE_RW
#	define TRACE_BUS_SPACE_RW(x) driver_printf x
#else
#	define TRACE_BUS_SPACE_RW(x)
#endif

//#define DEBUG_PCI
#ifdef DEBUG_PCI
#	define TRACE_PCI(dev, format, args...) device_printf(dev, format , ##args)
#else
#	define TRACE_PCI(dev, format, args...) do { } while (0)
#endif


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))


struct internal_intr {
	device_t		dev;
	driver_filter_t	filter;
	driver_intr_t	*handler;
	void			*arg;
	int				irq;
	uint32			flags;
	bool			is_msi;

	thread_id		thread;
	sem_id			sem;
	int32			handling;
};

static int32 intr_wrapper(void *data);


static int
fls(int mask)
{
	int bit;
	if (mask == 0)
		return (0);
	for (bit = 1; mask != 1; bit++)
		mask = (unsigned int)mask >> 1;
	return (bit);
}


static area_id
map_mem(void **virtualAddr, phys_addr_t _phy, size_t size, uint32 protection,
	const char *name)
{
	uint32 offset = _phy & (B_PAGE_SIZE - 1);
	phys_addr_t physicalAddr = _phy - offset;
	area_id area;

	size = ROUNDUP(size + offset, B_PAGE_SIZE);
	area = map_physical_memory(name, physicalAddr, size, B_ANY_KERNEL_ADDRESS,
		protection, virtualAddr);
	if (area < B_OK)
		return area;

	*virtualAddr = (uint8 *)(*virtualAddr) + offset;

	return area;
}


static int
bus_alloc_irq_resource(device_t dev, struct resource *res)
{
	uint8 irq = pci_read_config(dev, PCI_interrupt_line, 1);
	if (irq == 0 || irq == 0xff)
		return -1;

	/* TODO: IRQ resources! */
	res->r_bustag = 0;
	res->r_bushandle = irq;

	return 0;
}


static int
bus_alloc_mem_resource(device_t dev, struct resource *res, int regid)
{
	uint32 addr = pci_read_config(dev, regid, 4) & PCI_address_memory_32_mask;
	uint32 size = 128 * 1024; /* XXX */
	void *virtualAddr;

	res->r_mapped_area = map_mem(&virtualAddr, addr, size, 0,
		"bus_alloc_resource(MEMORY)");
	if (res->r_mapped_area < B_OK)
		return -1;

	res->r_bustag = I386_BUS_SPACE_MEM;
	res->r_bushandle = (bus_space_handle_t)virtualAddr;
	return 0;
}


static int
bus_alloc_ioport_resource(device_t dev, struct resource *res, int regid)
{
	res->r_bustag = I386_BUS_SPACE_IO;
	res->r_bushandle = pci_read_config(dev, regid, 4) & PCI_address_io_mask;
	return 0;
}


struct resource *
bus_alloc_resource(device_t dev, int type, int *rid, unsigned long start,
	unsigned long end, unsigned long count, uint32 flags)
{
	struct resource *res;
	int result = -1;

	if (type != SYS_RES_IRQ && type != SYS_RES_MEMORY
		&& type != SYS_RES_IOPORT)
		return NULL;

	device_printf(dev, "bus_alloc_resource(%i, [%i], 0x%lx, 0x%lx, 0x%lx,"
		"0x%lx)\n", type, *rid, start, end, count, flags);

	// maybe a local array of resources is enough
	res = malloc(sizeof(struct resource));
	if (res == NULL)
		return NULL;

	if (type == SYS_RES_IRQ) {
		if (*rid == 0) {
			// pinned interrupt
			result = bus_alloc_irq_resource(dev, res);
		} else {
			// msi or msi-x interrupt at index *rid - 1
			pci_info *info;
			info = &((struct root_device_softc *)dev->root->softc)->pci_info;
			res->r_bustag = 1;
			res->r_bushandle = info->u.h0.interrupt_line + *rid - 1;
			result = 0;

			// TODO: msi-x interrupts
		}
	} else if (type == SYS_RES_MEMORY)
		result = bus_alloc_mem_resource(dev, res, *rid);
	else if (type == SYS_RES_IOPORT)
		result = bus_alloc_ioport_resource(dev, res, *rid);

	if (result < 0) {
		free(res);
		return NULL;
	}

	res->r_type = type;
	return res;
}


int
bus_release_resource(device_t dev, int type, int rid, struct resource *res)
{
	if (res->r_type != type)
		panic("bus_release_resource: mismatch");

	if (type == SYS_RES_MEMORY)
		delete_area(res->r_mapped_area);

	free(res);
	return 0;
}


int
bus_alloc_resources(device_t dev, struct resource_spec *resourceSpec,
	struct resource **resources)
{
	int i;

	for (i = 0; resourceSpec[i].type != -1; i++) {
		resources[i] = bus_alloc_resource_any(dev,
			resourceSpec[i].type, &resourceSpec[i].rid, resourceSpec[i].flags);
		if (resources[i] == NULL
			&& (resourceSpec[i].flags & RF_OPTIONAL) == 0) {
			for (++i; resourceSpec[i].type != -1; i++) {
				resources[i] = NULL;
			}

			bus_release_resources(dev, resourceSpec, resources);
			return ENXIO;
		}
	}
	return 0;
}


void
bus_release_resources(device_t dev, const struct resource_spec *resourceSpec,
	struct resource **resources)
{
	int i;

	for (i = 0; resourceSpec[i].type != -1; i++) {
		if (resources[i] == NULL)
			continue;

		bus_release_resource(dev, resourceSpec[i].type, resourceSpec[i].rid,
			resources[i]);
		resources[i] = NULL;
	}
}


bus_space_handle_t
rman_get_bushandle(struct resource *res)
{
	return res->r_bushandle;
}


bus_space_tag_t
rman_get_bustag(struct resource *res)
{
	return res->r_bustag;
}


//	#pragma mark - Interrupt handling


static int32
intr_wrapper(void *data)
{
	struct internal_intr *intr = data;

	//device_printf(intr->dev, "in interrupt handler.\n");

	if (!HAIKU_CHECK_DISABLE_INTERRUPTS(intr->dev))
		return B_UNHANDLED_INTERRUPT;

	release_sem_etc(intr->sem, 1, B_DO_NOT_RESCHEDULE);
	return intr->handling ? B_HANDLED_INTERRUPT : B_INVOKE_SCHEDULER;
}


static int32
intr_fast_wrapper(void *data)
{
	struct internal_intr *intr = data;

	intr->handler(intr->arg);

	// We don't know if the interrupt has been handled.
	return B_UNHANDLED_INTERRUPT;
}


static int32
intr_handler(void *data)
{
	struct internal_intr *intr = data;
	status_t status;

	while (1) {
		status = acquire_sem(intr->sem);
		if (status < B_OK)
			break;

		//device_printf(intr->dev, "in soft interrupt handler.\n");

		atomic_or(&intr->handling, 1);
		intr->handler(intr->arg);
		atomic_and(&intr->handling, 0);
		HAIKU_REENABLE_INTERRUPTS(intr->dev);
	}

	return 0;
}


static void
free_internal_intr(struct internal_intr *intr)
{
	if (intr->sem >= B_OK) {
		status_t status;
		delete_sem(intr->sem);
		wait_for_thread(intr->thread, &status);
	}

	free(intr);
}


int
bus_setup_intr(device_t dev, struct resource *res, int flags,
	driver_filter_t filter, driver_intr_t handler, void *arg, void **_cookie)
{
	/* TODO check MPSAFE etc */

	struct internal_intr *intr = (struct internal_intr *)malloc(
		sizeof(struct internal_intr));
	char semName[64];
	status_t status;

	if (intr == NULL)
		return B_NO_MEMORY;

	intr->dev = dev;
	intr->filter = filter;
	intr->handler = handler;
	intr->arg = arg;
	intr->irq = res->r_bushandle;
	intr->flags = flags;
	intr->is_msi = false;
	intr->sem = -1;
	intr->thread = -1;

	if (filter != NULL) {
		status = install_io_interrupt_handler(intr->irq,
			(interrupt_handler)intr->filter, intr->arg, 0);
	} else if ((flags & INTR_FAST) != 0) {
		status = install_io_interrupt_handler(intr->irq,
			intr_fast_wrapper, intr, B_NO_HANDLED_INFO);
	} else {
		snprintf(semName, sizeof(semName), "%s intr", dev->device_name);

		intr->sem = create_sem(0, semName);
		if (intr->sem < B_OK) {
			free(intr);
			return B_NO_MEMORY;
		}

		snprintf(semName, sizeof(semName), "%s intr handler", dev->device_name);

		intr->thread = spawn_kernel_thread(intr_handler, semName,
			B_REAL_TIME_DISPLAY_PRIORITY, intr);
		if (intr->thread < B_OK) {
			delete_sem(intr->sem);
			free(intr);
			return B_NO_MEMORY;
		}

		status = install_io_interrupt_handler(intr->irq,
			intr_wrapper, intr, B_NO_HANDLED_INFO);
	}

	if (status == B_OK && res->r_bustag == 1 && gPCIx86 != NULL) {
		// this is an msi, enable it
		pci_info *info
			= &((struct root_device_softc *)dev->root->softc)->pci_info;
		if (gPCIx86->enable_msi(info->bus, info->device,
				info->function) != B_OK) {
			device_printf(dev, "enabling msi failed\n");
			bus_teardown_intr(dev, res, intr);
			return ENODEV;
		}

		intr->is_msi = true;
	}

	if (status < B_OK) {
		free_internal_intr(intr);
		return status;
	}

	resume_thread(intr->thread);

	*_cookie = intr;
	return 0;
}


int
bus_teardown_intr(device_t dev, struct resource *res, void *arg)
{
	struct internal_intr *intr = arg;

	if (intr->is_msi && gPCIx86 != NULL) {
		// disable msi generation
		pci_info *info
			= &((struct root_device_softc *)dev->root->softc)->pci_info;
		gPCIx86->disable_msi(info->bus, info->device, info->function);
	}

	if (intr->filter != NULL) {
		remove_io_interrupt_handler(intr->irq, (interrupt_handler)intr->filter,
			intr->arg);
	} else if (intr->flags & INTR_FAST) {
		remove_io_interrupt_handler(intr->irq, intr_fast_wrapper, intr);
	} else {
		remove_io_interrupt_handler(intr->irq, intr_wrapper, intr);
	}

	free_internal_intr(intr);
	return 0;
}


//	#pragma mark - bus functions


bus_dma_tag_t
bus_get_dma_tag(device_t dev)
{
	return NULL;
}


int
bus_generic_suspend(device_t dev)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


int
bus_generic_resume(device_t dev)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


void
bus_generic_shutdown(device_t dev)
{
	UNIMPLEMENTED();
}


int
bus_print_child_header(device_t dev, device_t child)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


int
bus_print_child_footer(device_t dev, device_t child)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


int
bus_generic_print_child(device_t dev, device_t child)
{
	UNIMPLEMENTED();
	return B_ERROR;
}


void
bus_generic_driver_added(device_t dev, driver_t *driver)
{
	UNIMPLEMENTED();
}


#define BUS_SPACE_READ(size, type, fun) \
	type bus_space_read_##size(bus_space_tag_t tag, \
		bus_space_handle_t handle, bus_size_t offset) \
	{ \
		type value; \
		if (tag == I386_BUS_SPACE_IO) \
			value = fun(handle + offset); \
		else \
			value = *(volatile type *)(handle + offset); \
		if (tag == I386_BUS_SPACE_IO) \
			TRACE_BUS_SPACE_RW(("bus_space_read_%s(0x%lx, 0x%lx, 0x%lx) = 0x%lx\n", \
				#size, (uint32)tag, (uint32)handle, (uint32)offset, (uint32)value)); \
		return value; \
	}

#define BUS_SPACE_WRITE(size, type, fun) \
	void bus_space_write_##size(bus_space_tag_t tag, \
		bus_space_handle_t handle, bus_size_t offset, type value) \
	{ \
		if (tag == I386_BUS_SPACE_IO) \
			TRACE_BUS_SPACE_RW(("bus_space_write_%s(0x%lx, 0x%lx, 0x%lx, 0x%lx)\n", \
				#size, (uint32)tag, (uint32)handle, (uint32)offset, (uint32)value)); \
		if (tag == I386_BUS_SPACE_IO) \
			fun(value, handle + offset); \
		else \
			*(volatile type *)(handle + offset) = value; \
	}

BUS_SPACE_READ(1, uint8_t, in8)
BUS_SPACE_READ(2, uint16_t, in16)
BUS_SPACE_READ(4, uint32_t, in32)

BUS_SPACE_WRITE(1, uint8_t, out8)
BUS_SPACE_WRITE(2, uint16_t, out16)
BUS_SPACE_WRITE(4, uint32_t, out32)

int
bus_child_present(device_t child)
{
	device_t parent = device_get_parent(child);
	if (parent == NULL)
		return 0;

	return bus_child_present(parent);
}


//	#pragma mark - PCI functions


uint32_t
pci_read_config(device_t dev, int offset, int size)
{
	pci_info *info = &((struct root_device_softc *)dev->root->softc)->pci_info;

	uint32_t value = gPci->read_pci_config(info->bus, info->device,
		info->function, offset, size);
	TRACE_PCI(dev, "pci_read_config(%i, %i) = 0x%x\n", offset, size, value);
	return value;
}


void
pci_write_config(device_t dev, int offset, uint32_t value, int size)
{
	pci_info *info = &((struct root_device_softc *)dev->root->softc)->pci_info;

	TRACE_PCI(dev, "pci_write_config(%i, 0x%x, %i)\n", offset, value, size);

	gPci->write_pci_config(info->bus, info->device, info->function, offset,
		size, value);
}


uint16_t
pci_get_vendor(device_t dev)
{
	return pci_read_config(dev, PCI_vendor_id, 2);
}


uint16_t
pci_get_device(device_t dev)
{
	return pci_read_config(dev, PCI_device_id, 2);
}


uint16_t
pci_get_subvendor(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_vendor_id, 2);
}


uint16_t
pci_get_subdevice(device_t dev)
{
	return pci_read_config(dev, PCI_subsystem_id, 2);
}


uint8_t
pci_get_revid(device_t dev)
{
	return pci_read_config(dev, PCI_revision, 1);
}


uint32_t
pci_get_domain(device_t dev)
{
	return 0;
}

uint32_t 
pci_get_devid(device_t dev)
{
	return pci_read_config(dev, PCI_device_id, 2) << 16 |
		pci_read_config(dev, PCI_vendor_id, 2);
}

uint8_t 
pci_get_cachelnsz(device_t dev)
{
	return pci_read_config(dev, PCI_line_size, 1);
}

uint8_t *
pci_get_ether(device_t dev)
{
	/* used in if_dc to get the MAC from CardBus CIS for Xircom card */
	return NULL; /* NULL is handled in the caller correctly */
}

uint8_t
pci_get_bus(device_t dev)
{
	pci_info *info
		= &((struct root_device_softc *)dev->root->softc)->pci_info;
	return info->bus;
}


uint8_t
pci_get_slot(device_t dev)
{
	pci_info *info
		= &((struct root_device_softc *)dev->root->softc)->pci_info;
	return info->device;
}


uint8_t
pci_get_function(device_t dev)
{
	pci_info *info
		= &((struct root_device_softc *)dev->root->softc)->pci_info;
	return info->function;
}


device_t
pci_find_dbsf(uint32_t domain, uint8_t bus, uint8_t slot, uint8_t func)
{
	// We don't support that yet - if we want to support the multi port
	// feature of the Broadcom BCM 570x driver, we would have to change
	// that.
	return NULL;
}


static void
pci_set_command_bit(device_t dev, uint16_t bit)
{
	uint16_t command = pci_read_config(dev, PCI_command, 2);
	pci_write_config(dev, PCI_command, command | bit, 2);
}


int
pci_enable_busmaster(device_t dev)
{
	pci_set_command_bit(dev, PCI_command_master);
	return 0;
}


int
pci_enable_io(device_t dev, int space)
{
	/* adapted from FreeBSD's pci_enable_io_method */
	int bit = 0;

	switch (space) {
		case SYS_RES_IOPORT:
			bit = PCI_command_io;
			break;
		case SYS_RES_MEMORY:
			bit = PCI_command_memory;
			break;
		default:
			return EINVAL;
	}

	pci_set_command_bit(dev, bit);
	if (pci_read_config(dev, PCI_command, 2) & bit)
		return 0;

	device_printf(dev, "pci_enable_io(%d) failed.\n", space);

	return ENXIO;
}


int
pci_find_cap(device_t dev, int capability, int *capreg)
{
	return pci_find_extcap(dev, capability, capreg);
}


int
pci_find_extcap(device_t child, int capability, int *_capabilityRegister)
{
	uint8 capabilityPointer;
	uint8 headerType;
	uint16 status;

	status = pci_read_config(child, PCIR_STATUS, 2);
	if ((status & PCIM_STATUS_CAPPRESENT) == 0)
		return ENXIO;

	headerType = pci_read_config(child, PCI_header_type, 1);
	switch (headerType & PCIM_HDRTYPE) {
		case 0:
		case 1:
			capabilityPointer = PCIR_CAP_PTR;
			break;
		case 2:
			capabilityPointer = PCIR_CAP_PTR_2;
			break;
		default:
			return ENXIO;
	}
	capabilityPointer = pci_read_config(child, capabilityPointer, 1);

	while (capabilityPointer != 0) {
		if (pci_read_config(child, capabilityPointer + PCICAP_ID, 1)
				== capability) {
			if (_capabilityRegister != NULL)
				*_capabilityRegister = capabilityPointer;
			return 0;
		}
		capabilityPointer = pci_read_config(child,
			capabilityPointer + PCICAP_NEXTPTR, 1);
	}

	return ENOENT;
}


int
pci_msi_count(device_t dev)
{
	pci_info *info;
	if (gPCIx86 == NULL)
		return 0;

	info = &((struct root_device_softc *)dev->root->softc)->pci_info;
	return gPCIx86->get_msi_count(info->bus, info->device, info->function);
}


int
pci_alloc_msi(device_t dev, int *count)
{
	pci_info *info;
	uint8 startVector = 0;
	if (gPCIx86 == NULL)
		return ENODEV;

	info = &((struct root_device_softc *)dev->root->softc)->pci_info;

	if (gPCIx86->configure_msi(info->bus, info->device, info->function, *count,
			&startVector) != B_OK) {
		return ENODEV;
	}

	info->u.h0.interrupt_line = startVector;
	return EOK;
}


int
pci_release_msi(device_t dev)
{
	pci_info *info;
	if (gPCIx86 == NULL)
		return ENODEV;

	info = &((struct root_device_softc *)dev->root->softc)->pci_info;
	gPCIx86->unconfigure_msi(info->bus, info->device, info->function);
	return EOK;
}


int
pci_msix_count(device_t dev)
{
	return 0;
}


int
pci_alloc_msix(device_t dev, int *count)
{
	return ENODEV;
}


int
pci_get_max_read_req(device_t dev)
{
	int cap;
	uint16_t val;

	if (pci_find_extcap(dev, PCIY_EXPRESS, &cap) != 0)
		return (0);
	val = pci_read_config(dev, cap + PCIR_EXPRESS_DEVICE_CTL, 2);
	val &= PCIM_EXP_CTL_MAX_READ_REQUEST;
	val >>= 12;
	return (1 << (val + 7));
}


int
pci_set_max_read_req(device_t dev, int size)
{
	int cap;
	uint16_t val;

	if (pci_find_extcap(dev, PCIY_EXPRESS, &cap) != 0)
		return (0);
	if (size < 128)
		size = 128;
	if (size > 4096)
		size = 4096;
	size = (1 << (fls(size) - 1));
	val = pci_read_config(dev, cap + PCIR_EXPRESS_DEVICE_CTL, 2);
	val &= ~PCIM_EXP_CTL_MAX_READ_REQUEST;
	val |= (fls(size) - 8) << 12;
	pci_write_config(dev, cap + PCIR_EXPRESS_DEVICE_CTL, val, 2);
	return (size);
}


int
pci_get_powerstate(device_t dev)
{
	int capabilityRegister;
	uint16 status;
	int powerState = PCI_POWERSTATE_D0;

	if (pci_find_extcap(dev, PCIY_PMG, &capabilityRegister) != EOK)
		return powerState;

	status = pci_read_config(dev, capabilityRegister + PCIR_POWER_STATUS, 2);
	switch (status & PCI_pm_mask) {
		case PCI_pm_state_d0:
			break;
		case PCI_pm_state_d1:
			powerState = PCI_POWERSTATE_D1;
			break;
		case PCI_pm_state_d2:
			powerState = PCI_POWERSTATE_D2;
			break;
		case PCI_pm_state_d3:
			powerState = PCI_POWERSTATE_D3;
			break;
		default:
			powerState = PCI_POWERSTATE_UNKNOWN;
			break;
	}

	TRACE_PCI(dev, "%s: D%i\n", __func__, powerState);
	return powerState;
}


int
pci_set_powerstate(device_t dev, int newPowerState)
{
	int capabilityRegister;
	int oldPowerState;
	uint8 currentPowerManagementStatus;
	uint8 newPowerManagementStatus;
	uint16 powerManagementCapabilities;
	bigtime_t stateTransitionDelayInUs = 0;

	if (pci_find_extcap(dev, PCIY_PMG, &capabilityRegister) != EOK)
		return EOPNOTSUPP;

	oldPowerState = pci_get_powerstate(dev);
	if (oldPowerState == newPowerState)
		return EOK;

	switch (max(oldPowerState, newPowerState)) {
		case PCI_POWERSTATE_D2:
			stateTransitionDelayInUs = 200;
			break;
		case PCI_POWERSTATE_D3:
			stateTransitionDelayInUs = 10000;
			break;
	}

	currentPowerManagementStatus = pci_read_config(dev, capabilityRegister
		+ PCIR_POWER_STATUS, 2);
	newPowerManagementStatus = currentPowerManagementStatus & ~PCI_pm_mask;
	powerManagementCapabilities = pci_read_config(dev, capabilityRegister
		+ PCIR_POWER_CAP, 2);

	switch (newPowerState) {
		case PCI_POWERSTATE_D0:
			newPowerManagementStatus |= PCIM_PSTAT_D0;
			break;
		case PCI_POWERSTATE_D1:
			if ((powerManagementCapabilities & PCI_pm_d1supp) == 0)
				return EOPNOTSUPP;
			newPowerManagementStatus |= PCIM_PSTAT_D1;
			break;
		case PCI_POWERSTATE_D2:
			if ((powerManagementCapabilities & PCI_pm_d2supp) == 0)
				return EOPNOTSUPP;
			newPowerManagementStatus |= PCIM_PSTAT_D2;
			break;
		case PCI_POWERSTATE_D3:
			newPowerManagementStatus |= PCIM_PSTAT_D3;
			break;
		default:
			return EINVAL;
	}

	TRACE_PCI(dev, "%s: D%i -> D%i\n", __func__, oldPowerState, newPowerState);
	pci_write_config(dev, capabilityRegister + PCIR_POWER_STATUS, newPowerState,
		2);
	if (stateTransitionDelayInUs != 0)
		snooze(stateTransitionDelayInUs);

	return EOK;
}
