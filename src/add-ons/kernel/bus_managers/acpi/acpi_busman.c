/*
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
#include "acpixf.h"
#include "acpi_priv.h"

//#define TRACE_ACPI_BUS
#ifdef TRACE_ACPI_BUS
#define TRACE(x...) dprintf("acpi: " x)
#else
#define TRACE(x...)
#endif

#define ERROR(x...) dprintf("acpi: " x)


extern dpc_module_info* gDPC;
void* gDPCHandle = NULL;

extern pci_module_info* gPCIManager;


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
			bool acpiDisabled = false;
			void *settings;

			settings = load_driver_settings("kernel");
			if (settings != NULL) {
				acpiDisabled = !get_driver_boolean_parameter(settings, "acpi",
					false, false);
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

#ifdef ACPI_DEBUG_OUTPUT
			AcpiDbgLevel = ACPI_DEBUG_ALL | ACPI_LV_VERBOSE;
			AcpiDbgLayer = ACPI_ALL_COMPONENTS;
#endif

			status = AcpiInitializeSubsystem();
			if (status != AE_OK) {
				ERROR("AcpiInitializeSubsystem failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiInitializeTables(NULL, 0, TRUE);
			if (status != AE_OK) {
				ERROR("AcpiInitializeTables failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiLoadTables();
			if (status != AE_OK) {
				ERROR("AcpiLoadTables failed (%s)\n",
					AcpiFormatException(status));
				goto err;
			}

			status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
			if (status != AE_OK) {
				ERROR("AcpiEnableSubsystem failed (%s)\n",
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

				/* This isn't so terrible. We'll just fail silently */
			if (gDPC != NULL) {
				if (gDPCHandle != NULL) {
					gDPC->delete_dpc_queue(gDPCHandle);
					gDPCHandle = NULL;
				}

				put_module(B_DPC_MODULE_NAME);
			}

			break;
		}

		default:
			return B_ERROR;
	}
	return B_OK;
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
get_device_hid(const char *path, char *hid)
{
	ACPI_HANDLE handle;
	ACPI_OBJECT info;
	ACPI_BUFFER infoBuffer;

	TRACE("get_device_hid: path %s, hid %s\n", path, hid);
	if (AcpiGetHandle(NULL, (ACPI_STRING)path, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

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
evaluate_object(const char* object, acpi_object_type* returnValue,
	size_t bufferLength)
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	buffer.Pointer = returnValue;
	buffer.Length = bufferLength;

	status = AcpiEvaluateObject(NULL, (ACPI_STRING)object, NULL,
		returnValue != NULL ? &buffer : NULL);
	return status == AE_OK ? B_OK : B_ERROR;
}


status_t
evaluate_method(const char* object, const char* method,
	acpi_object_type *returnValue, size_t bufferLength, acpi_object_type *args,
	int numArgs)
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	ACPI_OBJECT_LIST acpiArgs;
	ACPI_HANDLE handle;

	if (AcpiGetHandle(NULL, (ACPI_STRING)object, &handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;

	buffer.Pointer = returnValue;
	buffer.Length = bufferLength;

	acpiArgs.Count = numArgs;
	acpiArgs.Pointer = (ACPI_OBJECT *)args;

	status = AcpiEvaluateObject(handle, (ACPI_STRING)method,
		args != NULL ? &acpiArgs : NULL, returnValue != NULL ? &buffer : NULL);
	return status == AE_OK ? B_OK : B_ERROR;
}


static status_t
prepare_sleep_state(uint8 state, void (*wakeFunc)(void), size_t size)
{
	ACPI_STATUS status;

	TRACE("prepare_sleep_state %d, %p, %ld\n", state, wakeFunc, size);

	if (state != ACPI_POWER_STATE_OFF) {
		physical_entry wakeVector;

		status = lock_memory(&wakeFunc, size, 0);
		if (status != B_OK)
			return status;

		status = get_memory_map(&wakeFunc, size, &wakeVector, 1);
		if (status != B_OK)
			return status;

		status = AcpiSetFirmwareWakingVector((addr_t)wakeVector.address);
		if (status != AE_OK)
			return B_ERROR;
	}

	status = AcpiEnterSleepStatePrep(state);
	if (status != AE_OK)
		return B_ERROR;

	return B_OK;
}


static status_t
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


struct acpi_module_info gACPIModule = {
	{
		B_ACPI_MODULE_NAME,
		B_KEEP_LOADED,
		acpi_std_ops
	},

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
	evaluate_object,
	evaluate_method,
	prepare_sleep_state,
	enter_sleep_state
};
