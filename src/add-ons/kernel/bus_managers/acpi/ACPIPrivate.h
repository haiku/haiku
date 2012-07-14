/*
 * Copyright 2009, Clemens Zeidler. All rights reserved.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __ACPI_PRIV_H__
#define __ACPI_PRIV_H__


#include <sys/cdefs.h>

#include <device_manager.h>
#include <KernelExport.h>
#include <ACPI.h>
#include <PCI.h>

// name of ACPI root module
#define ACPI_ROOT_MODULE_NAME 	"bus_managers/acpi/root/driver_v1"

// name of ACPI device modules
#define ACPI_DEVICE_MODULE_NAME "bus_managers/acpi/driver_v1"

// name of the ACPI namespace device
#define ACPI_NS_DUMP_DEVICE_MODULE_NAME "bus_managers/acpi/namespace/device_v1"


__BEGIN_DECLS

extern device_manager_info* gDeviceManager;
extern pci_module_info* gPCIManager;

// information about one ACPI device
typedef struct acpi_device_cookie {
	char*			path;			// path
	acpi_handle		handle;
	uint32			type;			// type
	device_node*	node;
	char			name[32];		// name (for fast log)
} acpi_device_cookie;


// ACPI root.
typedef struct acpi_root_info {
	driver_module_info info;

	status_t	(*get_handle)(acpi_handle parent, const char *pathname,
					acpi_handle *retHandle);

	/* Global Lock */

	status_t	(*acquire_global_lock)(uint16 timeout, uint32 *handle);
	status_t	(*release_global_lock)(uint32 handle);

	/* Notify Handler */

    status_t	(*install_notify_handler)(acpi_handle device,
    				uint32 handlerType, acpi_notify_handler handler,
    				void *context);
	status_t	(*remove_notify_handler)(acpi_handle device,
    				uint32 handlerType, acpi_notify_handler handler);

	/* GPE Handler */
	status_t	(*update_all_gpes)();
	status_t	(*enable_gpe)(acpi_handle handle, uint32 gpeNumber);
	status_t	(*disable_gpe)(acpi_handle handle, uint32 gpeNumber);
	status_t	(*clear_gpe)(acpi_handle handle, uint32 gpeNumber);
	status_t	(*set_gpe)(acpi_handle handle, uint32 gpeNumber,
					uint8 action);
	status_t	(*finish_gpe)(acpi_handle handle, uint32 gpeNumber);
	status_t	(*install_gpe_handler)(acpi_handle handle, uint32 gpeNumber,
					uint32 type, acpi_gpe_handler handler, void *data);
	status_t	(*remove_gpe_handler)(acpi_handle handle, uint32 gpeNumber,
					acpi_gpe_handler address);

	/* Address Space Handler */

	status_t	(*install_address_space_handler)(acpi_handle handle,
					uint32 spaceId,
					acpi_adr_space_handler handler,
					acpi_adr_space_setup setup,	void *data);
	status_t	(*remove_address_space_handler)(acpi_handle handle,
					uint32 spaceId,
					acpi_adr_space_handler handler);

	/* Fixed Event Management */

	void		(*enable_fixed_event)(uint32 event);
	void		(*disable_fixed_event)(uint32 event);

	uint32		(*fixed_event_status) (uint32 event);
					/* Returns 1 if event set, 0 otherwise */
	void		(*reset_fixed_event) (uint32 event);

	status_t	(*install_fixed_event_handler)(uint32 event,
					interrupt_handler *handler, void *data);
	status_t	(*remove_fixed_event_handler)(uint32 event,
					interrupt_handler *handler);

	/* Namespace Access */

	status_t	(*get_next_entry)(uint32 objectType, const char *base,
					char *result, size_t length, void **_counter);
	status_t	(*get_device)(const char *hid, uint32 index, char *result,
					size_t resultLength);

	status_t	(*get_device_hid)(const char *path, char *hid,
					size_t hidLength);
	uint32		(*get_object_type)(const char *path);
	status_t	(*get_object)(const char *path,
					acpi_object_type **_returnValue);
	status_t	(*get_object_typed)(const char *path,
					acpi_object_type **_returnValue, uint32 objectType);
	status_t	(*ns_handle_to_pathname)(acpi_handle targetHandle,
					acpi_data *buffer);

	/* Control method execution and data acquisition */

	status_t	(*evaluate_object)(const char* object,
					acpi_object_type *returnValue, size_t bufferLength);
	status_t	(*evaluate_method)(acpi_handle handle, const char *method,
					acpi_objects *args, acpi_data *returnValue);

	/* Resource info */

	status_t	(*get_irq_routing_table)(acpi_handle busDeviceHandle,
					acpi_data *retBuffer);
	status_t	(*get_current_resources)(acpi_handle busDeviceHandle,
					acpi_data *retBuffer);
	status_t	(*get_possible_resources)(acpi_handle busDeviceHandle,
					acpi_data *retBuffer);
	status_t	(*set_current_resources)(acpi_handle busDeviceHandle,
					acpi_data *buffer);

	/* Power state setting */

	status_t	(*prepare_sleep_state)(uint8 state, void (*wakeFunc)(void),
					size_t size);
	status_t	(*enter_sleep_state)(uint8 state, uint8 flags);
	status_t	(*reboot)(void);

	/* Table Access */
	status_t	(*get_table)(const char *signature, uint32 instance,
					void **tableHeader);
} acpi_root_info;


extern struct acpi_module_info gACPIModule;

extern struct device_module_info acpi_ns_dump_module;

extern struct driver_module_info embedded_controller_driver_module;
extern struct device_module_info embedded_controller_device_module;

extern acpi_device_module_info gACPIDeviceModule;


status_t get_handle(acpi_handle parent, const char* pathname,
	acpi_handle* retHandle);

status_t acquire_global_lock(uint16 timeout, uint32* handle);
status_t release_global_lock(uint32 handle);

status_t install_notify_handler(acpi_handle device,	uint32 handlerType,
	acpi_notify_handler handler, void* context);
status_t remove_notify_handler(acpi_handle device, uint32 handlerType,
	acpi_notify_handler handler);

status_t update_all_gpes();
status_t enable_gpe(acpi_handle handle, uint32 gpeNumber);
status_t disable_gpe(acpi_handle handle, uint32 gpeNumber);
status_t clear_gpe(acpi_handle handle, uint32 gpeNumber);
status_t set_gpe(acpi_handle handle, uint32 gpeNumber, uint8 action);
status_t finish_gpe(acpi_handle handle, uint32 gpeNumber);
status_t install_gpe_handler(acpi_handle handle, uint32 gpeNumber, uint32 type,
	acpi_gpe_handler handler, void* data);
status_t remove_gpe_handler(acpi_handle handle, uint32 gpeNumber,
	acpi_gpe_handler address);

status_t install_address_space_handler(acpi_handle handle, uint32 spaceID,
	acpi_adr_space_handler handler, acpi_adr_space_setup setup, void* data);
status_t remove_address_space_handler(acpi_handle handle, uint32 spaceID,
	acpi_adr_space_handler handler);

void enable_fixed_event(uint32 event);
void disable_fixed_event(uint32 event);

uint32 fixed_event_status(uint32 event);
void reset_fixed_event(uint32 event);

status_t install_fixed_event_handler(uint32 event, interrupt_handler* handler,
	void* data);
status_t remove_fixed_event_handler(uint32 event, interrupt_handler* handler);

status_t get_next_entry(uint32 object_type, const char* base, char* result,
	size_t length, void** _counter);
status_t get_device(const char* hid, uint32 index, char* result,
	size_t resultLength);

status_t get_device_hid(const char* path, char* hid, size_t hidLength);
uint32 get_object_type(const char* path);
status_t get_object(const char* path, acpi_object_type** _returnValue);
status_t get_object_typed(const char* path, acpi_object_type** _returnValue,
	uint32 object_type);
status_t ns_handle_to_pathname(acpi_handle targetHandle, acpi_data* buffer);

status_t evaluate_object(const char* object, acpi_object_type* returnValue,
	size_t bufferLength);
status_t evaluate_method(acpi_handle handle, const char* method,
	acpi_objects* args, acpi_data* returnValue);

status_t get_irq_routing_table(acpi_handle busDeviceHandle,
	acpi_data* returnValue);
status_t get_current_resources(acpi_handle busDeviceHandle,
	acpi_data* returnValue);
status_t get_possible_resources(acpi_handle busDeviceHandle,
	acpi_data* returnValue);
status_t set_current_resources(acpi_handle busDeviceHandle,
	acpi_data* buffer);

status_t prepare_sleep_state(uint8 state, void (*wakeFunc)(void), size_t size);
status_t enter_sleep_state(uint8 state, uint8 flags);

status_t reboot(void);

status_t get_table(const char* signature, uint32 instance, void** tableHeader);

__END_DECLS


#endif	/* __ACPI_PRIV_H__ */
