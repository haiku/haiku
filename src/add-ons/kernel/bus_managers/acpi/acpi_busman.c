/*
 * Copyright 2009, Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2009, Clemens Zeidler, haiku@clemens-zeidler.de
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Bryan Varner. All rights reserved.
 * Copyright 2005, Nathan Whitehorn. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>
#include <dpc.h>
#include <KernelExport.h>
#include <PCI.h>

#include <safemode.h>

#include "acpi.h"
#include "accommon.h"
#include "acdisasm.h"
#include "acnamesp.h"
#include "acpi_priv.h"

//#define TRACE_ACPI_BUS
#ifdef TRACE_ACPI_BUS
#define TRACE(x...) dprintf("acpi: " x)
#else
#define TRACE(x...)
#endif

#define ERROR(x...) dprintf("acpi: " x)

#define ACPI_DEVICE_ID_LENGTH	0x08

extern pci_module_info* gPCIManager;
extern dpc_module_info* gDPC;
void* gDPCHandle = NULL;


static ACPI_STATUS
get_device_by_hid_callback(ACPI_HANDLE object, UINT32 depth, void* context,
	void** _returnValue)
{
	uint32* counter = (uint32*)context;
	ACPI_STATUS status;
	ACPI_BUFFER buffer;

	TRACE("get_device_by_hid_callback %p, %ld, %p\n", object, depth, context);

	*_returnValue = NULL;

	if (counter[0] == counter[1]) {
		buffer.Length = 254;
		buffer.Pointer = malloc(255);

		status = AcpiGetName(object, ACPI_FULL_PATHNAME, &buffer);
		if (status != AE_OK) {
			free(buffer.Pointer);
			return AE_CTRL_TERMINATE;
		}

		((char*)buffer.Pointer)[buffer.Length] = '\0';
		*_returnValue = buffer.Pointer;
		return AE_CTRL_TERMINATE;
	}

	counter[1]++;
	return AE_OK;
}


//	#pragma mark - ACPI bus manager API


static status_t
acpi_std_ops(int32 op,...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			ACPI_STATUS status;
			ACPI_OBJECT arg;
			ACPI_OBJECT_LIST parameter;
			uint32 flags;
			void *settings;
			bool acpiDisabled = false;
			bool acpiAvoidFullInit = false;

			settings = load_driver_settings("kernel");
			if (settings != NULL) {
				acpiDisabled = !get_driver_boolean_parameter(settings, "acpi",
					true, true);
				acpiAvoidFullInit = get_driver_boolean_parameter(settings,
					"acpi_avoid_full_init", false, false);
				unload_driver_settings(settings);
			}

			if (!acpiDisabled) {
				// check if safemode settings disable ACPI
				settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
				if (settings != NULL) {
					acpiDisabled = get_driver_boolean_parameter(settings,
						B_SAFEMODE_DISABLE_ACPI, false, false);
					unload_driver_settings(settings);
				}
			}

			if (acpiDisabled) {
				ERROR("ACPI disabled\n");
				return ENOSYS;
			}

			if (gDPC->new_dpc_queue(&gDPCHandle, "acpi_task",
				B_NORMAL_PRIORITY) != B_OK) {
				ERROR("failed to create os execution queue\n");
				return B_ERROR;
			}

			AcpiGbl_EnableInterpreterSlack = true;
//			AcpiGbl_CreateOSIMethod = true;

#ifdef ACPI_DEBUG_OUTPUT
			AcpiDbgLevel = ACPI_DEBUG_ALL | ACPI_LV_VERBOSE;
			AcpiDbgLayer = ACPI_ALL_COMPONENTS;
#endif

			status = AcpiInitializeSubsystem();
			if (ACPI_FAILURE(status)) {
				ERROR("AcpiInitializeSubsystem failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiInitializeTables(NULL, 0, TRUE);
			if (ACPI_FAILURE(status)) {
				ERROR("AcpiInitializeTables failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiLoadTables();
			if (ACPI_FAILURE(status)) {
				ERROR("AcpiLoadTables failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			/* Install the default address space handlers. */
			status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
				ACPI_ADR_SPACE_SYSTEM_MEMORY, ACPI_DEFAULT_HANDLER, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				ERROR("Could not initialise SystemMemory handler: %s\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
				ACPI_ADR_SPACE_SYSTEM_IO, ACPI_DEFAULT_HANDLER, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				ERROR("Could not initialise SystemIO handler: %s\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiInstallAddressSpaceHandler(ACPI_ROOT_OBJECT,
				ACPI_ADR_SPACE_PCI_CONFIG, ACPI_DEFAULT_HANDLER, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				ERROR("Could not initialise PciConfig handler: %s\n",
					AcpiFormatException(status));
				goto err;
			}

			arg.Integer.Type = ACPI_TYPE_INTEGER;
			arg.Integer.Value = 0;

			parameter.Count = 1;
			parameter.Pointer = &arg;

			AcpiEvaluateObject(NULL, "\\_PIC", &parameter, NULL);

			flags = acpiAvoidFullInit ?
					ACPI_NO_DEVICE_INIT | ACPI_NO_OBJECT_INIT :
					ACPI_FULL_INITIALIZATION;

			// FreeBSD seems to pass in the above flags here as
			// well but specs don't define ACPI_NO_DEVICE_INIT
			// and ACPI_NO_OBJECT_INIT here.
			status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
			if (ACPI_FAILURE(status)) {
				ERROR("AcpiEnableSubsystem failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiInitializeObjects(flags);
			if (ACPI_FAILURE(status)) {
				ERROR("AcpiInitializeObjects failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			/* Phew. Now in ACPI mode */
			TRACE("ACPI initialized\n");
			return B_OK;

		err:
			return B_ERROR;
		}

		case B_MODULE_UNINIT:
		{
			if (AcpiTerminate() != AE_OK)
				ERROR("Could not bring system out of ACPI mode. Oh well.\n");

			gDPC->delete_dpc_queue(gDPCHandle);
			gDPCHandle = NULL;
			break;
		}

		default:
			return B_ERROR;
	}
	return B_OK;
}


status_t
get_handle(acpi_handle parent, const char *pathname, acpi_handle *retHandle)
{
	return AcpiGetHandle(parent, (ACPI_STRING)pathname, retHandle) == AE_OK
		? B_OK : B_ERROR;
}


status_t
acquire_global_lock(uint16 timeout, uint32 *handle)
{
	return AcpiAcquireGlobalLock(timeout, (UINT32*)handle) == AE_OK
		? B_OK : B_ERROR;
}


status_t
release_global_lock(uint32 handle)
{
	return AcpiReleaseGlobalLock(handle) == AE_OK ? B_OK : B_ERROR;
}


status_t
install_notify_handler(acpi_handle device, uint32 handlerType,
	acpi_notify_handler handler, void *context)
{
	return AcpiInstallNotifyHandler(device, handlerType,
		(ACPI_NOTIFY_HANDLER)handler, context) == AE_OK ? B_OK : B_ERROR;
}


status_t
remove_notify_handler(acpi_handle device, uint32 handlerType,
	acpi_notify_handler handler)
{
	return AcpiRemoveNotifyHandler(device, handlerType,
		(ACPI_NOTIFY_HANDLER)handler) == AE_OK ? B_OK : B_ERROR;
}


status_t
update_all_gpes()
{
	return AcpiUpdateAllGpes() == AE_OK ? B_OK : B_ERROR;
}


status_t
enable_gpe(acpi_handle handle, uint32 gpeNumber)
{
	return AcpiEnableGpe(handle, gpeNumber) == AE_OK ? B_OK : B_ERROR;
}


status_t
disable_gpe(acpi_handle handle, uint32 gpeNumber)
{
	return AcpiDisableGpe(handle, gpeNumber) == AE_OK ? B_OK : B_ERROR;
}


status_t
clear_gpe(acpi_handle handle, uint32 gpeNumber)
{
	return AcpiClearGpe(handle, gpeNumber) == AE_OK ? B_OK : B_ERROR;
}


status_t
set_gpe(acpi_handle handle, uint32 gpeNumber, uint8 action)
{
	return AcpiSetGpe(handle, gpeNumber, action) == AE_OK ? B_OK : B_ERROR;
}


status_t
finish_gpe(acpi_handle handle, uint32 gpeNumber)
{
	return AcpiFinishGpe(handle, gpeNumber) == AE_OK ? B_OK : B_ERROR;
}


status_t
install_gpe_handler(acpi_handle handle, uint32 gpeNumber, uint32 type,
	acpi_gpe_handler handler, void *data)
{
	return AcpiInstallGpeHandler(handle, gpeNumber, type,
		(ACPI_GPE_HANDLER)handler, data) == AE_OK ? B_OK : B_ERROR;
}


status_t
remove_gpe_handler(acpi_handle handle, uint32 gpeNumber,
	acpi_gpe_handler address)
{
	return AcpiRemoveGpeHandler(handle, gpeNumber, (ACPI_GPE_HANDLER)address)
		== AE_OK ? B_OK : B_ERROR;
}


status_t
install_address_space_handler(acpi_handle handle, uint32 spaceId,
	acpi_adr_space_handler handler, acpi_adr_space_setup setup,	void *data)
{
	return AcpiInstallAddressSpaceHandler(handle, spaceId,
		(ACPI_ADR_SPACE_HANDLER)handler, (ACPI_ADR_SPACE_SETUP)setup, data)
		== AE_OK ? B_OK : B_ERROR;
}


status_t
remove_address_space_handler(acpi_handle handle, uint32 spaceId,
	acpi_adr_space_handler handler)
{
	return AcpiRemoveAddressSpaceHandler(handle, spaceId,
		(ACPI_ADR_SPACE_HANDLER)handler) == AE_OK ? B_OK : B_ERROR;
}


void
enable_fixed_event(uint32 event)
{
	AcpiEnableEvent(event, 0);
}


void
disable_fixed_event(uint32 event)
{
	AcpiDisableEvent(event, 0);
}


uint32
fixed_event_status(uint32 event)
{
	ACPI_EVENT_STATUS status = 0;
	AcpiGetEventStatus(event, &status);
	return status/* & ACPI_EVENT_FLAG_SET*/;
}


void
reset_fixed_event(uint32 event)
{
	AcpiClearEvent(event);
}


status_t
install_fixed_event_handler(uint32 event, interrupt_handler* handler,
	void *data)
{
	return AcpiInstallFixedEventHandler(event, (void*)handler, data) == AE_OK
		? B_OK : B_ERROR;
}


status_t
remove_fixed_event_handler(uint32 event, interrupt_handler* handler)
{
	return AcpiRemoveFixedEventHandler(event, (void*)handler) == AE_OK
		? B_OK : B_ERROR;
}


status_t
get_next_entry(uint32 objectType, const char *base, char *result,
	size_t length, void **counter)
{
	ACPI_HANDLE parent, child, newChild;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	TRACE("get_next_entry %ld, %s\n", objectType, base);

	if (base == NULL || !strcmp(base, "\\")) {
		parent = ACPI_ROOT_OBJECT;
	} else {
		status = AcpiGetHandle(NULL, (ACPI_STRING)base, &parent);
		if (status != AE_OK)
			return B_ENTRY_NOT_FOUND;
	}

	child = *counter;

	status = AcpiGetNextObject(objectType, parent, child, &newChild);
	if (status != AE_OK)
		return B_ENTRY_NOT_FOUND;

	*counter = newChild;
	buffer.Length = length;
	buffer.Pointer = result;

	status = AcpiGetName(newChild, ACPI_FULL_PATHNAME, &buffer);
	if (status != AE_OK)
		return B_NO_MEMORY; /* Corresponds to AE_BUFFER_OVERFLOW */

	return B_OK;
}


status_t
get_device(const char* hid, uint32 index, char* result, size_t resultLength)
{
	ACPI_STATUS status;
	uint32 counter[2] = {index, 0};
	char *buffer = NULL;

	TRACE("get_device %s, index %ld\n", hid, index);
	status = AcpiGetDevices((ACPI_STRING)hid, (void*)&get_device_by_hid_callback,
		counter, (void**)&buffer);
	if (status != AE_OK || buffer == NULL)
		return B_ENTRY_NOT_FOUND;

	strlcpy(result, buffer, resultLength);
	free(buffer);
	return B_OK;
}


status_t
get_device_hid(const char *path, char *hid, size_t bufferLength)
{
	ACPI_HANDLE handle;
	ACPI_OBJECT info;
	ACPI_BUFFER infoBuffer;

	TRACE("get_device_hid: path %s, hid %s\n", path, hid);
	if (AcpiGetHandle(NULL, (ACPI_STRING)path, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

	if (bufferLength < ACPI_DEVICE_ID_LENGTH)
		return B_BUFFER_OVERFLOW;

	infoBuffer.Pointer = &info;
	infoBuffer.Length = sizeof(ACPI_OBJECT);
	info.String.Pointer = hid;
	info.String.Length = 9;
	info.String.Type = ACPI_TYPE_STRING;

	if (AcpiEvaluateObject(handle, "_HID", NULL, &infoBuffer) != AE_OK)
		return B_BAD_TYPE;

	if (info.Type == ACPI_TYPE_INTEGER) {
		uint32 eisaId = AcpiUtDwordByteSwap(info.Integer.Value);

		hid[0] = (char) ('@' + ((eisaId >> 26) & 0x1f));
		hid[1] = (char) ('@' + ((eisaId >> 21) & 0x1f));
		hid[2] = (char) ('@' + ((eisaId >> 16) & 0x1f));
		hid[3] = AcpiUtHexToAsciiChar((ACPI_INTEGER)eisaId, 12);
		hid[4] = AcpiUtHexToAsciiChar((ACPI_INTEGER)eisaId, 8);
		hid[5] = AcpiUtHexToAsciiChar((ACPI_INTEGER)eisaId, 4);
		hid[6] = AcpiUtHexToAsciiChar((ACPI_INTEGER)eisaId, 0);
		hid[7] = 0;
	}

	hid[ACPI_DEVICE_ID_LENGTH] = '\0';
	return B_OK;
}


uint32
get_object_type(const char* path)
{
	ACPI_HANDLE handle;
	ACPI_OBJECT_TYPE type;

	if (AcpiGetHandle(NULL, (ACPI_STRING)path, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

	AcpiGetType(handle, &type);
	return type;
}


status_t
get_object(const char* path, acpi_object_type** _returnValue)
{
	ACPI_HANDLE handle;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	status = AcpiGetHandle(NULL, (ACPI_STRING)path, &handle);
	if (status != AE_OK)
		return B_ENTRY_NOT_FOUND;

	buffer.Pointer = NULL;
	buffer.Length = ACPI_ALLOCATE_BUFFER;

	status = AcpiEvaluateObject(handle, NULL, NULL, &buffer);

	*_returnValue = buffer.Pointer;
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
get_object_typed(const char* path, acpi_object_type** _returnValue,
	uint32 objectType)
{
	ACPI_HANDLE handle;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	status = AcpiGetHandle(NULL, (ACPI_STRING)path, &handle);
	if (status != AE_OK)
		return B_ENTRY_NOT_FOUND;

	buffer.Pointer = NULL;
	buffer.Length = ACPI_ALLOCATE_BUFFER;

	status = AcpiEvaluateObjectTyped(handle, NULL, NULL, &buffer, objectType);

	*_returnValue = buffer.Pointer;
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
ns_handle_to_pathname(acpi_handle targetHandle, acpi_data *buffer)
{
	status_t status = AcpiNsHandleToPathname(targetHandle,
		(ACPI_BUFFER*)buffer);
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
evaluate_object(const char* object, acpi_object_type* returnValue,
	size_t bufferLength)
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	buffer.Pointer = returnValue;
	buffer.Length = bufferLength;

	status = AcpiEvaluateObject(NULL, (ACPI_STRING)object, NULL,
		returnValue != NULL ? &buffer : NULL);
	if (status == AE_BUFFER_OVERFLOW)
		dprintf("evaluate_object: the passed buffer is too small!\n");

	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
evaluate_method(acpi_handle handle, const char* method,
	acpi_objects *args, acpi_data *returnValue)
{
	ACPI_STATUS status;

	status = AcpiEvaluateObject(handle, (ACPI_STRING)method,
		(ACPI_OBJECT_LIST*)args, (ACPI_BUFFER*)returnValue);
	if (status == AE_BUFFER_OVERFLOW)
		dprintf("evaluate_method: the passed buffer is too small!\n");

	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
get_irq_routing_table(acpi_handle busDeviceHandle, acpi_data *retBuffer)
{
	ACPI_STATUS status;

	status = AcpiGetIrqRoutingTable(busDeviceHandle, (ACPI_BUFFER*)retBuffer);
	if (status == AE_BUFFER_OVERFLOW)
		dprintf("evaluate_method: the passed buffer is too small!\n");

	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
get_current_resources(acpi_handle busDeviceHandle, acpi_data *retBuffer)
{
	return AcpiGetCurrentResources(busDeviceHandle, (ACPI_BUFFER*)retBuffer)
		== AE_OK ? B_OK : B_ERROR;
}


status_t
get_possible_resources(acpi_handle busDeviceHandle, acpi_data *retBuffer)
{
	return AcpiGetPossibleResources(busDeviceHandle, (ACPI_BUFFER*)retBuffer)
		== AE_OK ? B_OK : B_ERROR;
}


status_t
set_current_resources(acpi_handle busDeviceHandle, acpi_data *buffer)
{
	return AcpiSetCurrentResources(busDeviceHandle, (ACPI_BUFFER*)buffer)
		== AE_OK ? B_OK : B_ERROR;
}


status_t
prepare_sleep_state(uint8 state, void (*wakeFunc)(void), size_t size)
{
	ACPI_STATUS acpiStatus;

	TRACE("prepare_sleep_state %d, %p, %ld\n", state, wakeFunc, size);

	if (state != ACPI_POWER_STATE_OFF) {
		physical_entry wakeVector;
		status_t status;

		// Note: The supplied code must already be locked into memory.
		status = get_memory_map(wakeFunc, size, &wakeVector, 1);
		if (status != B_OK)
			return status;

#if ACPI_MACHINE_WIDTH == 32
#	if B_HAIKU_PHYSICAL_BITS > 32
		if (wakeVector.address >= 0x100000000LL) {
			ERROR("prepare_sleep_state(): ACPI_MACHINE_WIDTH == 32, but we "
				"have a physical address >= 4 GB\n");
		}
#	endif
		acpiStatus = AcpiSetFirmwareWakingVector(wakeVector.address);
#else
		acpiStatus = AcpiSetFirmwareWakingVector64(wakeVector.address);
#endif
		if (acpiStatus != AE_OK)
			return B_ERROR;
	}

	acpiStatus = AcpiEnterSleepStatePrep(state);
	if (acpiStatus != AE_OK)
		return B_ERROR;

	return B_OK;
}


status_t
enter_sleep_state(uint8 state)
{
	ACPI_STATUS status;

	TRACE("enter_sleep_state %d\n", state);

	status = AcpiEnterSleepState(state);
	if (status != AE_OK)
		return B_ERROR;

	/*status = AcpiLeaveSleepState(state);
	if (status != AE_OK)
		return B_ERROR;*/

	return B_OK;
}


status_t
reboot(void)
{
	ACPI_STATUS status;

	TRACE("reboot\n");

	status = AcpiReset();
	if (status == AE_NOT_EXIST)
		return B_UNSUPPORTED;

	if (status != AE_OK) {
		ERROR("Reset failed, status = %d\n", status);
		return B_ERROR;
	}

	snooze(1000000);
	ERROR("Reset failed, timeout\n");
	return B_ERROR;
}


status_t
get_table(const char* signature, uint32 instance, void** tableHeader)
{
	return AcpiGetTable((char*)signature, instance,
		(ACPI_TABLE_HEADER**)tableHeader) == AE_OK ? B_OK : B_ERROR;
}


struct acpi_module_info gACPIModule = {
	{
		B_ACPI_MODULE_NAME,
		B_KEEP_LOADED,
		acpi_std_ops
	},

	get_handle,
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
	get_device,
	get_device_hid,
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
	prepare_sleep_state,
	enter_sleep_state,
	reboot,
	get_table
};
