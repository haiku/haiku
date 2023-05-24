/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "X86PCIController.h"


device_manager_info* gDeviceManager;


pci_controller_module_info gPciControllerDriver = {
	.info = {
		.info = {
			.name = PCI_X86_DRIVER_MODULE_NAME,
		},
		.supports_device = X86PCIController::SupportsDevice,
		.register_device = X86PCIController::RegisterDevice,
		.init_driver = [](device_node* node, void** driverCookie) {
			return X86PCIController::InitDriver(node, *(X86PCIController**)driverCookie);
		},
		.uninit_driver = [](void* driverCookie) {
			return static_cast<X86PCIController*>(driverCookie)->UninitDriver();
		},
	},
	.read_pci_config = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32* value) {
		return static_cast<X86PCIController*>(cookie)
			->ReadConfig(bus, device, function, offset, size, *value);
	},
	.write_pci_config = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) {
		return static_cast<X86PCIController*>(cookie)
			->WriteConfig(bus, device, function, offset, size, value);
	},
	.get_max_bus_devices = [](void* cookie, int32* count) {
		return static_cast<X86PCIController*>(cookie)->GetMaxBusDevices(*count);
	},
	.read_pci_irq = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8 *irq) {
		return static_cast<X86PCIController*>(cookie)->ReadIrq(bus, device, function, pin, *irq);
	},
	.write_pci_irq = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8 irq) {
		return static_cast<X86PCIController*>(cookie)->WriteIrq(bus, device, function, pin, irq);
	},
	.get_range = [](void *cookie, uint32 index, pci_resource_range* range) {
		return static_cast<X86PCIController*>(cookie)->GetRange(index, range);
	},
	.finalize = [](void *cookie) {
		return static_cast<X86PCIController*>(cookie)->Finalize();
	}
};


_EXPORT module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};

_EXPORT module_info *modules[] = {
	(module_info *)&gPciControllerDriver,
	NULL
};
