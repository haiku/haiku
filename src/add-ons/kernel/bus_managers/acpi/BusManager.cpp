/*
 * Copyright 2009, Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2009, Clemens Zeidler, haiku@clemens-zeidler.de
 * Copyright 2008-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Bryan Varner. All rights reserved.
 * Copyright 2005, Nathan Whitehorn. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>
#include <apic.h>
#include <dpc.h>
#include <KernelExport.h>
#include <PCI.h>

#include <safemode.h>

extern "C" {
#include "acpi.h"
#include "accommon.h"
#include "acdisasm.h"
#include "acnamesp.h"
}
#include "ACPIPrivate.h"

//#define TRACE_ACPI_BUS
#ifdef TRACE_ACPI_BUS
#define TRACE(x...) dprintf("acpi: " x)
#else
#define TRACE(x...)
#endif

#define ERROR(x...) dprintf("acpi: " x)

#define PIC_MODE 0
#define APIC_MODE 1

#define ACPI_DEVICE_ID_LENGTH	0x08

extern dpc_module_info* gDPC;
void* gDPCHandle = NULL;


static bool
checkAndLogFailure(const ACPI_STATUS status, const char* msg)
{
	bool failure = ACPI_FAILURE(status);
	if (failure)
		dprintf("acpi: %s %s\n", msg, AcpiFormatException(status));

	return failure;
}


static ACPI_STATUS
get_device_by_hid_callback(ACPI_HANDLE object, UINT32 depth, void* context,
	void** _returnValue)
{
	uint32* counter = (uint32*)context;
	ACPI_BUFFER buffer;

	TRACE("get_device_by_hid_callback %p, %d, %p\n", object, depth, context);

	*_returnValue = NULL;

	if (counter[0] == counter[1]) {
		buffer.Length = 254;
		buffer.Pointer = malloc(255);

		if (checkAndLogFailure(AcpiGetName(object, ACPI_FULL_PATHNAME, &buffer),
				"Failed to find device")) {
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


#ifdef ACPI_DEBUG_OUTPUT


static void
globalGPEHandler(UINT32 eventType, ACPI_HANDLE device, UINT32 eventNumber,
	void* context)
{
	ACPI_BUFFER path;
	char deviceName[256];
	path.Length = sizeof(deviceName);
	path.Pointer = deviceName;

	ACPI_STATUS status = AcpiNsHandleToPathname(device, &path);
	if (ACPI_FAILURE(status))
		strcpy(deviceName, "(missing)");

	switch (eventType) {
		case ACPI_EVENT_TYPE_GPE:
			dprintf("acpi: GPE Event %d for %s\n", eventNumber, deviceName);
			break;

		case ACPI_EVENT_TYPE_FIXED:
		{
			switch (eventNumber) {
				case ACPI_EVENT_PMTIMER:
					dprintf("acpi: PMTIMER(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_GLOBAL:
					dprintf("acpi: Global(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_POWER_BUTTON:
					dprintf("acpi: Powerbutton(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_SLEEP_BUTTON:
					dprintf("acpi: sleepbutton(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_RTC:
					dprintf("acpi: RTC(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				default:
					dprintf("acpi: unknown fixed(%d) event for %s\n",
						eventNumber, deviceName);
			}
			break;
		}

		default:
			dprintf("acpi: unknown event type (%d:%d)  event for %s\n",
				eventType, eventNumber, deviceName);
	}
}


static void globalNotifyHandler(ACPI_HANDLE device, UINT32 value, void* context)
{
	ACPI_BUFFER path;
	char deviceName[256];
	path.Length = sizeof(deviceName);
	path.Pointer = deviceName;

	ACPI_STATUS status = AcpiNsHandleToPathname(device, &path);
	if (ACPI_FAILURE(status))
		strcpy(deviceName, "(missing)");

	dprintf("acpi: Notify event %d for %s\n", value, deviceName);
}


#endif


//	#pragma mark - ACPI bus manager API


static status_t
acpi_std_ops(int32 op,...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			ACPI_OBJECT arg;
			ACPI_OBJECT_LIST parameter;
			void *settings;
			bool acpiDisabled = false;
			AcpiGbl_CopyDsdtLocally = true;

			settings = load_driver_settings("kernel");
			if (settings != NULL) {
				acpiDisabled = !get_driver_boolean_parameter(settings, "acpi",
					true, true);
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
					B_URGENT_DISPLAY_PRIORITY + 1) != B_OK) {
				ERROR("failed to create os execution queue\n");
				return B_ERROR;
			}

#ifdef ACPI_DEBUG_OUTPUT
			AcpiDbgLevel = ACPI_DEBUG_ALL | ACPI_LV_VERBOSE;
			AcpiDbgLayer = ACPI_ALL_COMPONENTS;
#endif

			if (checkAndLogFailure(AcpiInitializeSubsystem(),
					"AcpiInitializeSubsystem failed"))
				goto err;

			if (checkAndLogFailure(AcpiInitializeTables(NULL, 0, TRUE),
					"AcpiInitializeTables failed"))
				goto err;

			if (checkAndLogFailure(AcpiLoadTables(),
					"AcpiLoadTables failed"))
				goto err;

			/* Install the default address space handlers. */

			arg.Integer.Type = ACPI_TYPE_INTEGER;
			arg.Integer.Value = apic_available() ? APIC_MODE : PIC_MODE;

			parameter.Count = 1;
			parameter.Pointer = &arg;

			AcpiEvaluateObject(NULL, (ACPI_STRING)"\\_PIC", &parameter, NULL);

			if (checkAndLogFailure(AcpiEnableSubsystem(
						ACPI_FULL_INITIALIZATION),
					"AcpiEnableSubsystem failed"))
				goto err;

			if (checkAndLogFailure(AcpiInitializeObjects(
						ACPI_FULL_INITIALIZATION),
					"AcpiInitializeObjects failed"))
				goto err;

			//TODO: Walk namespace init ALL _PRW's

#ifdef ACPI_DEBUG_OUTPUT
			checkAndLogFailure(
				AcpiInstallGlobalEventHandler(globalGPEHandler, NULL),
				"Failed to install global GPE-handler.");

			checkAndLogFailure(AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT,
					ACPI_ALL_NOTIFY, globalNotifyHandler, NULL),
				"Failed to install global Notify-handler.");
#endif
			checkAndLogFailure(AcpiEnableAllRuntimeGpes(),
				"Failed to enable all runtime Gpes");

			checkAndLogFailure(AcpiUpdateAllGpes(),
				"Failed to update all Gpes");

			TRACE("ACPI initialized\n");
			return B_OK;

		err:
			return B_ERROR;
		}

		case B_MODULE_UNINIT:
		{
			checkAndLogFailure(AcpiTerminate(),
				"Could not bring system out of ACPI mode. Oh well.");

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
get_name(acpi_handle handle, uint32 nameType, char* returnedName,
	size_t bufferLength)
{
	ACPI_BUFFER buffer = {bufferLength, (void*)returnedName};
	return AcpiGetName(handle, nameType, &buffer) == AE_OK ? B_OK : B_ERROR;
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
install_fixed_event_handler(uint32 event, acpi_event_handler handler,
	void *data)
{
	return AcpiInstallFixedEventHandler(event, (ACPI_EVENT_HANDLER)handler, data) == AE_OK
		? B_OK : B_ERROR;
}


status_t
remove_fixed_event_handler(uint32 event, acpi_event_handler handler)
{
	return AcpiRemoveFixedEventHandler(event, (ACPI_EVENT_HANDLER)handler) == AE_OK
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
get_next_object(uint32 objectType, acpi_handle parent,
	acpi_handle* currentChild)
{
	acpi_handle child = *currentChild;
	return AcpiGetNextObject(objectType, parent, child, currentChild) == AE_OK
		? B_OK : B_ERROR;
}


status_t
get_device(const char* hid, uint32 index, char* result, size_t resultLength)
{
	ACPI_STATUS status;
	uint32 counter[2] = {index, 0};
	char *buffer = NULL;

	TRACE("get_device %s, index %ld\n", hid, index);
	status = AcpiGetDevices((ACPI_STRING)hid, (ACPI_WALK_CALLBACK)&get_device_by_hid_callback,
		counter, (void**)&buffer);
	if (status != AE_OK || buffer == NULL)
		return B_ENTRY_NOT_FOUND;

	strlcpy(result, buffer, resultLength);
	free(buffer);
	return B_OK;
}


status_t
get_device_info(const char *path, char** hid, char** cidList,
	size_t cidListCount, char** uid)
{
	ACPI_HANDLE handle;
	ACPI_DEVICE_INFO *info;

	TRACE("get_device_info: path %s\n", path);
	if (AcpiGetHandle(NULL, (ACPI_STRING)path, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

	if (AcpiGetObjectInfo(handle, &info) != AE_OK)
		return B_BAD_TYPE;

	if ((info->Valid & ACPI_VALID_HID) != 0 && hid != NULL)
		*hid = strndup(info->HardwareId.String, info->HardwareId.Length);

	if ((info->Valid & ACPI_VALID_CID) != 0 && cidList != NULL) {
		if (cidListCount > info->CompatibleIdList.Count)
			cidListCount = info->CompatibleIdList.Count;
		for (size_t i = 0; i < cidListCount; i++) {
			cidList[i] = strndup(info->CompatibleIdList.Ids[i].String,
				info->CompatibleIdList.Ids[i].Length);
		}
	}

	if ((info->Valid & ACPI_VALID_UID) != 0 && uid != NULL)
		*uid = strndup(info->UniqueId.String, info->UniqueId.Length);

	AcpiOsFree(info);
	return B_OK;
}


status_t
get_device_addr(const char *path, uint32 *addr)
{
	ACPI_HANDLE handle;

	TRACE("get_device_adr: path %s, hid %s\n", path, hid);
	if (AcpiGetHandle(NULL, (ACPI_STRING)path, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

	status_t status = B_BAD_VALUE;
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);
	if (addr != NULL
		&& evaluate_method(handle, "_ADR", NULL, &buf) == B_OK
		&& object.object_type == ACPI_TYPE_INTEGER) {
		status = B_OK;
		*addr = object.integer.integer;
	}
	return status;
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

	*_returnValue = (acpi_object_type*)buffer.Pointer;
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

	*_returnValue = (acpi_object_type*)buffer.Pointer;
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
ns_handle_to_pathname(acpi_handle targetHandle, acpi_data *buffer)
{
	status_t status = AcpiNsHandleToPathname(targetHandle,
		(ACPI_BUFFER*)buffer, false);
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
evaluate_object(acpi_handle handle, const char* object, acpi_objects *args,
	acpi_object_type* returnValue, size_t bufferLength)
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	buffer.Pointer = returnValue;
	buffer.Length = bufferLength;

	status = AcpiEvaluateObject(handle, (ACPI_STRING)object,
		(ACPI_OBJECT_LIST*)args, returnValue != NULL ? &buffer : NULL);
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
		dprintf("get_irq_routing_table: the passed buffer is too small!\n");

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
walk_resources(acpi_handle busDeviceHandle, char* method,
	acpi_walk_resources_callback callback, void* context)
{
	return AcpiWalkResources(busDeviceHandle, method,
		(ACPI_WALK_RESOURCE_CALLBACK)callback, context);
}


status_t
walk_namespace(acpi_handle busDeviceHandle, uint32 objectType,
	uint32 maxDepth, acpi_walk_callback descendingCallback,
	acpi_walk_callback ascendingCallback, void* context, void** returnValue)
{
	return AcpiWalkNamespace(objectType, busDeviceHandle, maxDepth,
		(ACPI_WALK_CALLBACK)descendingCallback,
		(ACPI_WALK_CALLBACK)ascendingCallback, context, returnValue);
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
		status = get_memory_map((const void*)wakeFunc, size, &wakeVector, 1);
		if (status != B_OK)
			return status;

#	if B_HAIKU_PHYSICAL_BITS > 32
		if (wakeVector.address >= 0x100000000LL) {
			ERROR("prepare_sleep_state(): ACPI 2.0c says use 32 bit "
				"vector, but we have a physical address >= 4 GB\n");
		}
#	endif
		acpiStatus = AcpiSetFirmwareWakingVector(wakeVector.address,
			wakeVector.address);
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

	cpu_status cpu = disable_interrupts();
	status = AcpiEnterSleepState(state);
	restore_interrupts(cpu);
	panic("AcpiEnterSleepState should not return.");
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


status_t
read_bit_register(uint32 regid, uint32 *val)
{
	return AcpiReadBitRegister(regid, (UINT32 *)val);
}


status_t
write_bit_register(uint32 regid, uint32 val)
{
	return AcpiWriteBitRegister(regid, val);
}


struct acpi_module_info gACPIModule = {
	{
		B_ACPI_MODULE_NAME,
		B_KEEP_LOADED,
		acpi_std_ops
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
	walk_namespace,
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
	get_table,
	read_bit_register,
	write_bit_register
};
