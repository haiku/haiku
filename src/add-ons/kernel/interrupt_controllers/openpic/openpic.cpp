/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
/*-
 * Copyright (C) 2002 Benno Rice.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Benno Rice ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <KernelExport.h>

#include <AutoDeleter.h>
#include <bus/PCI.h>
#include <interrupt_controller.h>
#include <util/kernel_cpp.h>

#include "openpic.h"


#define OPENPIC_MODULE_NAME	"interrupt_controllers/openpic/device_v1"

enum {
	OPENPIC_MIN_REGISTER_SPACE_SIZE	= 0x21000,
	OPENPIC_MAX_REGISTER_SPACE_SIZE	= 0x40000,
};

struct openpic_supported_device {
	const char	*name;
	uint16		vendor_id;
	uint16		device_id;
	uint32		register_offset;
};

static openpic_supported_device sSupportedDevices[] = {
	{ "Intrepid I/O Controller",	0x106b,	0x003e,	0x40000 },
	{}
};

static device_manager_info *sDeviceManager;
static pci_module_info *sPCIBusManager;

struct openpic_info : interrupt_controller_info {
	openpic_info()
	{
		memset(this, 0, sizeof(openpic_info));
		register_area = -1;
	}

	~openpic_info()
	{
		// unmap registers)
		if (register_area >= 0)
			delete_area(register_area);

		// uninit parent node driver
		if (pci)
			//XXX do I mean it ?
			sDeviceManager->put_node(sDeviceManager->get_parent_node(node));
	}

	openpic_supported_device	*supported_device;
	device_node			*node;
	pci_device_module_info		*pci;
	pci_device					*device;

	addr_t						physical_registers;	// physical registers base
	addr_t						virtual_registers;	// virtual (mapped)
													// registers base
	area_id						register_area;		// register area
	size_t						register_space_size;
};


static openpic_supported_device *
openpic_check_supported_device(uint16 vendorID, uint16 deviceID)
{
	for (openpic_supported_device *supportedDevice = sSupportedDevices;
		 supportedDevice->name;
		 supportedDevice++) {
		if (supportedDevice->vendor_id == vendorID
			&& supportedDevice->device_id == deviceID) {
			return supportedDevice;
		}
	}

	return NULL;
}


static uint32
openpic_read(openpic_info *info, int reg)
{
	return B_SWAP_INT32(info->pci->read_io_32(info->device,
		info->virtual_registers + reg));
}


static void
openpic_write(openpic_info *info, int reg, uint32 val)
{
	info->pci->write_io_32(info->device, info->virtual_registers + reg,
		B_SWAP_INT32(val));
}


static int
openpic_read_irq(openpic_info *info, int cpu)
{
	return openpic_read(info, OPENPIC_IACK(cpu)) & OPENPIC_VECTOR_MASK;
}


static void
openpic_eoi(openpic_info *info, int cpu)
{
	openpic_write(info, OPENPIC_EOI(cpu), 0);
// the Linux driver does this:
//openpic_read(info, OPENPIC_EOI(cpu));
}


static void
openpic_enable_irq(openpic_info *info, int irq, int type)
{
// TODO: Align this code with the sequence recommended in the Open PIC
// Specification (v 1.2 section 5.2.2).
	uint32 x;

	x = openpic_read(info, OPENPIC_SRC_VECTOR(irq));
	x &= ~(OPENPIC_IMASK | OPENPIC_SENSE_LEVEL | OPENPIC_SENSE_EDGE);
	if (type == IRQ_TYPE_LEVEL)
		x |= OPENPIC_SENSE_LEVEL;
	else
		x |= OPENPIC_SENSE_EDGE;
	openpic_write(info, OPENPIC_SRC_VECTOR(irq), x);
}


static void
openpic_disable_irq(openpic_info *info, int irq)
{
	uint32 x;

	x = openpic_read(info, OPENPIC_SRC_VECTOR(irq));
	x |= OPENPIC_IMASK;
	openpic_write(info, OPENPIC_SRC_VECTOR(irq), x);
}


static void
openpic_set_priority(openpic_info *info, int cpu, int pri)
{
	uint32 x;

	x = openpic_read(info, OPENPIC_CPU_PRIORITY(cpu));
	x &= ~OPENPIC_CPU_PRIORITY_MASK;
	x |= pri;
	openpic_write(info, OPENPIC_CPU_PRIORITY(cpu), x);
}


static status_t
openpic_init(openpic_info *info)
{
	uint32 x = openpic_read(info, OPENPIC_FEATURE);
	const char *featureVersion;
	char versionBuffer[64];
	switch (x & OPENPIC_FEATURE_VERSION_MASK) {
		case 1:
			featureVersion = "1.0";
			break;
		case 2:
			featureVersion = "1.2";
			break;
		case 3:
			featureVersion = "1.3";
			break;
		default:
			snprintf(versionBuffer, sizeof(versionBuffer),
				"unknown (feature reg: 0x%lx)", x);
			featureVersion = versionBuffer;
			break;
	}

	info->cpu_count = ((x & OPENPIC_FEATURE_LAST_CPU_MASK) >>
	    OPENPIC_FEATURE_LAST_CPU_SHIFT) + 1;
	info->irq_count = ((x & OPENPIC_FEATURE_LAST_IRQ_MASK) >>
	    OPENPIC_FEATURE_LAST_IRQ_SHIFT) + 1;

	/*
	 * PSIM seems to report 1 too many IRQs
	 */
// 	if (sc->sc_psim)
// 		sc->sc_nirq--;

	dprintf("openpic: Version %s, supports %d CPUs and %d irqs\n",
		    featureVersion, info->cpu_count, info->irq_count);

	/* disable all interrupts */
	for (int irq = 0; irq < info->irq_count; irq++)
		openpic_write(info, OPENPIC_SRC_VECTOR(irq), OPENPIC_IMASK);

	openpic_set_priority(info, 0, 15);

	/* we don't need 8259 passthrough mode */
	x = openpic_read(info, OPENPIC_CONFIG);
	x |= OPENPIC_CONFIG_8259_PASSTHRU_DISABLE;
	openpic_write(info, OPENPIC_CONFIG, x);

	/* send all interrupts to cpu 0 */
	for (int irq = 0; irq < info->irq_count; irq++)
		openpic_write(info, OPENPIC_IDEST(irq), 1 << 0);

	for (int irq = 0; irq < info->irq_count; irq++) {
		x = irq;
		x |= OPENPIC_IMASK;
		x |= OPENPIC_POLARITY_POSITIVE;
		x |= OPENPIC_SENSE_LEVEL;
		x |= 8 << OPENPIC_PRIORITY_SHIFT;
		openpic_write(info, OPENPIC_SRC_VECTOR(irq), x);
	}

	/* XXX IPI */
	/* XXX set spurious intr vector */

	openpic_set_priority(info, 0, 0);

	/* clear all pending interrupts */
	for (int irq = 0; irq < info->irq_count; irq++) {
		openpic_read_irq(info, 0);
		openpic_eoi(info, 0);
	}

	return B_OK;
}


// #pragma mark - driver interface


static status_t
openpic_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


static float
openpic_supports_device(device_node *parent)
{
	const char *bus;
	uint16 vendorID;
	uint16 deviceID;

	// get the bus (should be PCI)
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			!= B_OK) {
		return B_ERROR;
	}

	// get vendor and device ID
	if (sDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID,
			&vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent, B_DEVICE_ID,
			&deviceID, false) != B_OK) {
		return B_ERROR;
	}

	// check, whether bus, vendor and device ID match
	if (strcmp(bus, "pci") != 0
		|| !openpic_check_supported_device(vendorID, deviceID)) {
		return 0.0;
	}

	return 0.6;
}


static status_t
openpic_register_device(device_node *parent)
{
#if 0 //XXX: what do I do ?
  // get interface to PCI device
  pci_device_module_info *pci;
  pci_device *device;
  driver_module_info *driver;
  void *cookie;
  status_t error;
  error = sDeviceManager->get_driver(parent, &driver, &cookie);
  if (error < B_OK)
    return error;
  error = driver->init_driver(parent, cookie);
  // (driver_module_info**)&pci, (void**)&device); // wtf?
  if (error != B_OK)
    return error;

  sDeviceManager->uninit_driver(parent);
#endif
  device_node *newNode;
  device_attr attrs[] = {
    // info about ourself
    //{ B_DRIVER_MODULE, B_STRING_TYPE, { string: OPENPIC_MODULE_NAME }},
    //XXX: that's inconsistent with the header!
    //{ B_DEVICE_TYPE, B_STRING_TYPE,
    //	{ string: B_INTERRUPT_CONTROLLER_DRIVER_TYPE }},

    {}
  };

  // HACK: to get it compiled, I will break anything.
  return sDeviceManager->register_node(parent, NULL, attrs, NULL, &newNode);
}


static status_t
openpic_init_driver(device_node *node, void **cookie)
{
	// OK, this module is broken for now. But it compiles.
	return B_ERROR;
	openpic_info *info = new(nothrow) openpic_info;
	if (!info)
		return B_NO_MEMORY;
	ObjectDeleter<openpic_info> infoDeleter(info);

	info->node = node;

	// get interface to PCI device
	void *aCookie;
	void *anotherCookie; // possibly the same cookie.
	driver_module_info *driver;
	status_t status = sDeviceManager->get_driver(sDeviceManager->get_parent_node(node),
												 &driver, &aCookie);
	if (status != B_OK)
		return status;

	driver->init_driver(node, &anotherCookie);

	/* status = sDeviceManager->init_driver(
		sDeviceManager->get_parent(node), NULL,
		(driver_module_info**)&info->pci, (void**)&info->device);
		if (status != B_OK)
		return status; */

	// get the pci info for the device
	pci_info pciInfo;
	info->pci->get_pci_info(info->device, &pciInfo);

	// find supported device info
	info->supported_device = openpic_check_supported_device(pciInfo.vendor_id,
		pciInfo.device_id);
	if (!info->supported_device) {
		dprintf("openpic: device (0x%04hx:0x%04hx) not supported\n",
			pciInfo.vendor_id, pciInfo.device_id);
		return B_ERROR;
	}
	dprintf("openpic: found supported device: %s (0x%04hx:0x%04hx)\n",
		info->supported_device->name, pciInfo.vendor_id, pciInfo.device_id);

	// get register space
	addr_t physicalRegisterBase = pciInfo.u.h0.base_registers[0];
	uint32 registerSpaceSize = pciInfo.u.h0.base_register_sizes[0];
	if (registerSpaceSize < info->supported_device->register_offset
		|| registerSpaceSize - info->supported_device->register_offset
			< OPENPIC_MIN_REGISTER_SPACE_SIZE) {
		dprintf("openpic: register space too small\n");
	}
	physicalRegisterBase += info->supported_device->register_offset;
	registerSpaceSize -= info->supported_device->register_offset;
	if (registerSpaceSize > OPENPIC_MAX_REGISTER_SPACE_SIZE)
		registerSpaceSize = OPENPIC_MAX_REGISTER_SPACE_SIZE;

	// map register space
	void *virtualRegisterBase = NULL;
	area_id registerArea = map_physical_memory("openpic registers",
		physicalRegisterBase, registerSpaceSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &virtualRegisterBase);
	if (registerArea < 0)
		return info->register_area;

	info->physical_registers = physicalRegisterBase;
	info->register_space_size = registerSpaceSize;
	info->register_area = registerArea;
	info->virtual_registers = (addr_t)virtualRegisterBase;

	// init the controller
	status = openpic_init(info);
	if (status != B_OK)
		return status;

	// keep the info
	infoDeleter.Detach();
	*cookie = info;

	dprintf("openpic_init_driver(): Successfully initialized!\n");

	return B_OK;
}


static void
openpic_uninit_driver(void *cookie)
{
	openpic_info *info = (openpic_info*)cookie;

	delete info;
}


static void
openpic_device_removed(void *driverCookie)
{
	// TODO: ...
}


// FIXME: I don't think this is needed...
/*static void
openpic_get_paths(const char **_bus, const char **_device)
{
	static const char *kBus[] = { "pci", NULL };
//	static const char *kDevice[] = { "drivers/dev/disk/ide", NULL };

	*_bus = kBus;
//	*_device = kDevice;
	*_device = NULL;
}*/


// #pragma mark - interrupt_controller interface


static status_t
openpic_get_controller_info(void *cookie, interrupt_controller_info *_info)
{
	if (!_info)
		return B_BAD_VALUE;

	openpic_info *info = (openpic_info*)cookie;

	*_info = *info;

	return B_OK;
}


static status_t
openpic_enable_io_interrupt(void *cookie, int irq, int type)
{
	openpic_info *info = (openpic_info*)cookie;

	openpic_enable_irq(info, irq, type);

	return B_OK;
}


static status_t
openpic_disable_io_interrupt(void *cookie, int irq)
{
	openpic_info *info = (openpic_info*)cookie;

	openpic_disable_irq(info, irq);

	return B_OK;
}


static int
openpic_acknowledge_io_interrupt(void *cookie)
{
	openpic_info *info = (openpic_info*)cookie;

	int cpu = 0;
	// Note: We direct all I/O interrupts to CPU 0. We could nevertheless
	// check against the value of the "Who Am I Register".

	int irq = openpic_read_irq(info, cpu);
	if (irq == 255)
		return -1;	// spurious interrupt

	// signal end of interrupt
	openpic_eoi(info, cpu);

	return irq;
}


static interrupt_controller_module_info sControllerModuleInfo = {
	{
		{
			OPENPIC_MODULE_NAME,
			0,
			openpic_std_ops
		},

		openpic_supports_device,
		openpic_register_device,
		openpic_init_driver,
		openpic_uninit_driver,
		NULL, // HACK: register_child_devices
		NULL, // HACK: rescan_child_devices
		openpic_device_removed,
		NULL,	// suspend
		NULL // resume
	},

	openpic_get_controller_info,
	openpic_enable_io_interrupt,
	openpic_disable_io_interrupt,
	openpic_acknowledge_io_interrupt,
};

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_PCI_MODULE_NAME, (module_info**)&sPCIBusManager},
	{}
};

module_info *modules[] = {
	(module_info *)&sControllerModuleInfo,
	NULL
};
