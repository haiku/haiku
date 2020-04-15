/*
 * Copyright 2009, Clemens Zeidler. All rights reserved.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <string.h>


#include "ACPIPrivate.h"
extern "C" {
#include "acpi.h"
}
#include <dpc.h>
#include <PCI.h>


//#define TRACE_ACPI_MODULE
#ifdef TRACE_ACPI_MODULE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


device_manager_info* gDeviceManager = NULL;
pci_module_info* gPCIManager = NULL;
dpc_module_info* gDPC = NULL;

module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager},
	{B_PCI_MODULE_NAME, (module_info**)&gPCIManager},
	{B_DPC_MODULE_NAME, (module_info**)&gDPC},
	{}
};


static float
acpi_module_supports_device(device_node* parent)
{
	// make sure parent is really device root
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "root"))
		return 0.0;

	return 1.0;
}


static status_t
acpi_module_register_device(device_node* parent)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI" }},

		{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_KEEP_DRIVER_LOADED }},
		{}
	};

	return gDeviceManager->register_node(parent, ACPI_ROOT_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_enumerate_child_devices(device_node* node, const char* root)
{
	char result[255];
	void* counter = NULL;

	TRACE(("acpi_enumerate_child_devices: recursing from %s\n", root));

	while (get_next_entry(ACPI_TYPE_ANY, root, result,
			sizeof(result), &counter) == B_OK) {
		uint32 type = get_object_type(result);
		device_node* deviceNode;

		switch (type) {
			case ACPI_TYPE_POWER:
			case ACPI_TYPE_PROCESSOR:
			case ACPI_TYPE_THERMAL:
			case ACPI_TYPE_DEVICE: {
				device_attr attrs[16] = {
					// info about device
					{ B_DEVICE_BUS, B_STRING_TYPE, { string: "acpi" }},

					// location on ACPI bus
					{ ACPI_DEVICE_PATH_ITEM, B_STRING_TYPE, { string: result }},

					// info about the device
					{ ACPI_DEVICE_TYPE_ITEM, B_UINT32_TYPE, { ui32: type }},

					{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_FIND_MULTIPLE_CHILDREN }},
					{ NULL }
				};

				uint32 attrCount = 4;
				char* hid = NULL;
				char* cidList[8] = { NULL };
				char* uid = NULL;
				if (type == ACPI_TYPE_DEVICE) {
					if (get_device_info(result, &hid, (char**)&cidList, 8,
						&uid) == B_OK) {
						if (hid != NULL) {
							attrs[attrCount].name = ACPI_DEVICE_HID_ITEM;
							attrs[attrCount].type = B_STRING_TYPE;
							attrs[attrCount].value.string = hid;
							attrCount++;
						}
						for (int i = 0; cidList[i] != NULL; i++) {
							attrs[attrCount].name = ACPI_DEVICE_CID_ITEM;
							attrs[attrCount].type = B_STRING_TYPE;
							attrs[attrCount].value.string = cidList[i];
							attrCount++;
						}
						if (uid != NULL) {
							attrs[attrCount].name = ACPI_DEVICE_UID_ITEM;
							attrs[attrCount].type = B_STRING_TYPE;
							attrs[attrCount].value.string = uid;
							attrCount++;
						}
					}
					uint32 addr;
					if (get_device_addr(result, &addr) == B_OK) {
						attrs[attrCount].name = ACPI_DEVICE_ADDR_ITEM;
						attrs[attrCount].type = B_UINT32_TYPE;
						attrs[attrCount].value.ui32 = addr;
						attrCount++;
					}
				}

				status_t status = gDeviceManager->register_node(node,
						ACPI_DEVICE_MODULE_NAME, attrs, NULL, &deviceNode);
				free(hid);
				free(uid);
				for (int i = 0; cidList[i] != NULL; i++)
					free(cidList[i]);
				if (status != B_OK)
					break;
				acpi_enumerate_child_devices(deviceNode, result);
				break;
			}
			default:
				acpi_enumerate_child_devices(node, result);
				break;
		}

	}

	return B_OK;
}


static status_t
acpi_module_register_child_devices(void* cookie)
{
	device_node* node = (device_node*)cookie;

	status_t status = gDeviceManager->publish_device(node, "acpi/namespace",
		ACPI_NS_DUMP_DEVICE_MODULE_NAME);
	if (status != B_OK)
		return status;

	if ((AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON) == 0) {
		dprintf("registering power button\n");
		device_attr attrs[] = {
			// info about device
			{ B_DEVICE_BUS, B_STRING_TYPE, { string: "acpi" }},

			// info about the device
			{ ACPI_DEVICE_HID_ITEM, B_STRING_TYPE, { string: "ACPI_FPB" }},
			{ ACPI_DEVICE_TYPE_ITEM, B_UINT32_TYPE, { ui32: ACPI_TYPE_DEVICE }},

			// consumer specification
			{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_FIND_MULTIPLE_CHILDREN }},
			{ NULL }
		};
		device_node* deviceNode;
		gDeviceManager->register_node(node, ACPI_DEVICE_MODULE_NAME, attrs,
				NULL, &deviceNode);
	}
	if ((AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON) == 0) {
		dprintf("registering sleep button\n");
		device_attr attrs[] = {
			// info about device
			{ B_DEVICE_BUS, B_STRING_TYPE, { string: "acpi" }},

			// info about the device
			{ ACPI_DEVICE_HID_ITEM, B_STRING_TYPE, { string: "ACPI_FSB" }},
			{ ACPI_DEVICE_TYPE_ITEM, B_UINT32_TYPE, { ui32: ACPI_TYPE_DEVICE }},

			// consumer specification
			{ B_DEVICE_FLAGS, B_UINT32_TYPE, { ui32: B_FIND_MULTIPLE_CHILDREN }},
			{ NULL }
		};
		device_node* deviceNode;
		gDeviceManager->register_node(node, ACPI_DEVICE_MODULE_NAME, attrs,
				NULL, &deviceNode);

	}

	return acpi_enumerate_child_devices(node, "\\");
}


static status_t
acpi_module_init(device_node* node, void** _cookie)
{
	*_cookie = node;
	return B_OK;
}


static void
acpi_module_uninit(void* cookie)
{
}


static int32
acpi_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			module_info* module;
			return get_module(B_ACPI_MODULE_NAME, &module);
				// this serializes our module initialization
		}

		case B_MODULE_UNINIT:
			return put_module(B_ACPI_MODULE_NAME);
	}

	return B_BAD_VALUE;
}


static struct acpi_root_info sACPIRootModule = {
	{
		{
			ACPI_ROOT_MODULE_NAME,
			0,
			acpi_module_std_ops
		},

		acpi_module_supports_device,
		acpi_module_register_device,
		acpi_module_init,
		acpi_module_uninit,
		acpi_module_register_child_devices,
		NULL,	// rescan devices
		NULL,	// device removed
	},

	get_handle,
	get_name,
	acquire_global_lock,
	release_global_lock,
	install_notify_handler,
	remove_notify_handler,
	update_all_gpes,
	enable_gpe,
	disable_gpe,
	clear_gpe,
	set_gpe,
	finish_gpe,
	install_gpe_handler,
	remove_gpe_handler,
	install_address_space_handler,
	remove_address_space_handler,
	enable_fixed_event,
	disable_fixed_event,
	fixed_event_status,
	reset_fixed_event,
	install_fixed_event_handler,
	remove_fixed_event_handler,
	get_next_entry,
	get_next_object,
	get_device,
	get_device_info,
	get_object_type,
	get_object,
	get_object_typed,
	ns_handle_to_pathname,
	evaluate_object,
	evaluate_method,
	get_irq_routing_table,
	get_current_resources,
	get_possible_resources,
	set_current_resources,
	walk_resources,
	prepare_sleep_state,
	enter_sleep_state,
	reboot,
	get_table
};


module_info* modules[] = {
	(module_info*)&gACPIModule,
	(module_info*)&sACPIRootModule,
	(module_info*)&acpi_ns_dump_module,
	(module_info*)&gACPIDeviceModule,
	(module_info*)&embedded_controller_driver_module,
	(module_info*)&embedded_controller_device_module,
	NULL
};
