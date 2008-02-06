/*
 * Copyright 2006, Bryan Varner. All rights reserved.
 * Copyright 2005, Nathan Whitehorn. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <ACPI.h>
#include <dpc.h>
#include <PCI.h>
#include <KernelExport.h>

#include <malloc.h>
#include <safemode.h>
#include <stdio.h>
#include <string.h>

#include "acpi.h"
#include "acpixf.h"
#include "acpi_priv.h"

status_t acpi_std_ops(int32 op,...);
status_t acpi_rescan_stub(void);

#define TRACE(x...) dprintf("acpi: " x)
#define ERROR(x...) dprintf("acpi: " x)

extern dpc_module_info *gDPC;
void *gDPChandle = NULL;

extern pci_module_info *gPCIManager;

struct acpi_module_info acpi_module = {
	{
		{
			B_ACPI_MODULE_NAME,
			B_KEEP_LOADED,
			acpi_std_ops
		},
		
		acpi_rescan_stub
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
	enter_sleep_state
};


status_t 
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
					true, true);
				unload_driver_settings(settings);
			}

			if (!acpiDisabled) {
				// check if safemode settings disable DMA
				settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
				if (settings != NULL) {
					acpiDisabled = get_driver_boolean_parameter(settings,
						B_SAFEMODE_DISABLE_ACPI, false, false);
					unload_driver_settings(settings);
				}
			}

			if (acpiDisabled) {
				ERROR("ACPI disabled");
				return ENOSYS;
			}

#ifndef __HAIKU__
			{
				// Once upon a time, there was no module(s) dependency(ies) automatic loading feature.
				// Let's do it the old way
				status_t status;

				status = get_module(B_DPC_MODULE_NAME, (module_info **)&gDPC);
				if (status != B_OK)
					return status;

				status = get_module(B_PCI_MODULE_NAME,
					(module_info **)&gPCIManager);
				if (status != B_OK) {
					put_module(B_DPC_MODULE_NAME);
					return status;
				}
			}
#endif

			if (gDPC->new_dpc_queue(&gDPChandle, "acpi_task", B_NORMAL_PRIORITY) != B_OK) {
				ERROR("AcpiInitializeSubsystem failed (new_dpc_queue() failed!)\n");
				goto err;
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
#ifndef __HAIKU__
			put_module(B_DPC_MODULE_NAME);
			put_module(B_PCI_MODULE_NAME);
#endif
			return B_ERROR;
		}

		case B_MODULE_UNINIT:
		{
			if (AcpiTerminate() != AE_OK)
				ERROR("Could not bring system out of ACPI mode. Oh well.\n");

				/* This isn't so terrible. We'll just fail silently */
			if (gDPChandle != NULL) {
				gDPC->delete_dpc_queue(gDPChandle);
				gDPChandle = NULL;
			}

#ifndef __HAIKU__
			// Once upon a time, there was no module(s) dependency(ies) automatic UNloading feature.
			// Let's do it the old way
			put_module(B_DPC_MODULE_NAME);
			put_module(B_PCI_MODULE_NAME);
#endif
			break;
		}

		default:
			return B_ERROR;
	}
	return B_OK;
}


status_t 
acpi_rescan_stub(void)
{
	return B_OK;
}


void 
enable_fixed_event(uint32 event)
{
	AcpiEnableEvent(event,0);
}


void 
disable_fixed_event(uint32 event)
{
	AcpiDisableEvent(event,0);
}


uint32 
fixed_event_status(uint32 event)
{
	ACPI_EVENT_STATUS status = 0;
	AcpiGetEventStatus(event,&status);
	return (status/* & ACPI_EVENT_FLAG_SET*/);
}


void 
reset_fixed_event(uint32 event)
{
	AcpiClearEvent(event);
}


status_t 
install_fixed_event_handler (uint32 event, interrupt_handler *handler, void *data)
{
	return ((AcpiInstallFixedEventHandler(event,handler,data) == AE_OK) ? B_OK : B_ERROR);
}


status_t 
remove_fixed_event_handler	(uint32 event, interrupt_handler *handler) 
{
	return ((AcpiRemoveFixedEventHandler(event,handler) == AE_OK) ? B_OK : B_ERROR);
}


status_t 
get_next_entry (uint32 object_type, const char *base, char *result, size_t len, void **counter)
{
	ACPI_STATUS result_status;
	ACPI_HANDLE parent,child,new_child;
	ACPI_BUFFER buffer;
	
	if ((base == NULL) || (strcmp(base,"\\") == 0)) {
		parent = ACPI_ROOT_OBJECT;
	} else {
		result_status = AcpiGetHandle(NULL,base,&parent);
		if (result_status != AE_OK)
			return B_ENTRY_NOT_FOUND;
	}
	
	child = *counter;
	
	result_status = AcpiGetNextObject(object_type,parent,child,&new_child);
	if (result_status != AE_OK)
		return B_ENTRY_NOT_FOUND;
		
	*counter = new_child;
	buffer.Length = len;
	buffer.Pointer = result;
	
	result_status = AcpiGetName(new_child,ACPI_FULL_PATHNAME,&buffer);
	if (result_status != AE_OK)
		return B_NO_MEMORY; /* Corresponds to AE_BUFFER_OVERFLOW */
	
	return B_OK;
}


static ACPI_STATUS 
get_device_by_hid_callback(ACPI_HANDLE object, UINT32 depth, void *context, void **return_val) 
{
	ACPI_STATUS result;
	ACPI_BUFFER buffer;
	uint32 *counter = (uint32 *)(context);
	
	*return_val = NULL;
	
	if (counter[0] == counter[1]) {
		buffer.Length = 254;
		buffer.Pointer = malloc(255);
		
		result = AcpiGetName(object,ACPI_FULL_PATHNAME,&buffer);
		if (result != AE_OK) {
			free(buffer.Pointer);
			return AE_CTRL_TERMINATE;
		}
		
		((char*)(buffer.Pointer))[buffer.Length] = '\0';
		*return_val = buffer.Pointer;
		return AE_CTRL_TERMINATE;
	}
	
	counter[1]++;
	return AE_OK;
}


status_t 
get_device (const char *hid, uint32 index, char *result) 
{
	ACPI_STATUS status;
	uint32 counter[2] = {index,0};
	char *result2 = NULL;
	
	status = AcpiGetDevices((char *)hid,&get_device_by_hid_callback,counter,&result2);
	if ((status != AE_OK) || (result2 == NULL))
		return B_ENTRY_NOT_FOUND;
		
	strcpy(result,result2);
	free(result2);
	return B_OK;
}


status_t 
get_device_hid (const char *path, char *hid) 
{
	ACPI_HANDLE handle;
	ACPI_OBJECT info;
	ACPI_BUFFER info_buffer;
	
	if (AcpiGetHandle(NULL,path,&handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;
	
	info_buffer.Pointer = &info;
	info_buffer.Length = sizeof(ACPI_OBJECT);
	info.String.Pointer = hid;
	info.String.Length = 9;
	info.String.Type = ACPI_TYPE_STRING;
	
	if (AcpiEvaluateObject(handle,"_HID",NULL,&info_buffer) != AE_OK)
		return B_BAD_TYPE;
	
	if (info.Type == ACPI_TYPE_INTEGER) {
		uint32 EisaId = AcpiUtDwordByteSwap (info.Integer.Value);

	    hid[0] = (char) ('@' + (((unsigned long) EisaId >> 26) & 0x1f));
	    hid[1] = (char) ('@' + ((EisaId >> 21) & 0x1f));
	    hid[2] = (char) ('@' + ((EisaId >> 16) & 0x1f));
	    hid[3] = AcpiUtHexToAsciiChar ((ACPI_INTEGER) EisaId, 12);
	    hid[4] = AcpiUtHexToAsciiChar ((ACPI_INTEGER) EisaId, 8);
	    hid[5] = AcpiUtHexToAsciiChar ((ACPI_INTEGER) EisaId, 4);
	    hid[6] = AcpiUtHexToAsciiChar ((ACPI_INTEGER) EisaId, 0);
	    hid[7] = 0;
	}
	
	hid[ACPI_DEVICE_ID_LENGTH] = '\0';
	return B_OK;
}


uint32 
get_object_type (const char *path) 
{
	ACPI_HANDLE handle;
	ACPI_OBJECT_TYPE type;
	
	if (AcpiGetHandle(NULL,path,&handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;
		
	AcpiGetType(handle,&type);
	return type;
}


status_t 
get_object (const char *path, acpi_object_type **return_value) 
{
	ACPI_HANDLE handle;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	
	status = AcpiGetHandle(NULL, path, &handle);
	if (status != AE_OK) {
		return B_ENTRY_NOT_FOUND;
	}
	buffer.Pointer = NULL;
	buffer.Length = ACPI_ALLOCATE_BUFFER;
	
	status = AcpiEvaluateObject(handle, NULL, NULL, &buffer);
	
	*return_value = buffer.Pointer;
	return (status == AE_OK) ? B_OK : B_ERROR;
}


status_t 
get_object_typed (const char *path, acpi_object_type **return_value, uint32 object_type) 
{
	ACPI_HANDLE handle;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	
	status = AcpiGetHandle(NULL, path, &handle);
	if (status != AE_OK) {
		return B_ENTRY_NOT_FOUND;
	}
	buffer.Pointer = NULL;
	buffer.Length = ACPI_ALLOCATE_BUFFER;
	
	status = AcpiEvaluateObjectTyped(handle, NULL, NULL, &buffer, object_type);
	
	*return_value = buffer.Pointer;
	return (status == AE_OK) ? B_OK : B_ERROR;
}


status_t 
evaluate_object (const char *object, acpi_object_type *return_value, size_t buf_len) 
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	
	buffer.Pointer = return_value;
	buffer.Length = buf_len;
	
	status = AcpiEvaluateObject(NULL,object,NULL,(return_value != NULL) ? &buffer : NULL);
	return (status == AE_OK) ? B_OK : B_ERROR;
}


status_t 
evaluate_method (const char *object, const char *method, acpi_object_type *return_value, 
	size_t buf_len, acpi_object_type *args, int num_args) 
{
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	ACPI_OBJECT_LIST acpi_args;
	ACPI_HANDLE handle;
	
	if (AcpiGetHandle(NULL,object,&handle) != AE_OK)
		return B_ENTRY_NOT_FOUND;
	
	buffer.Pointer = return_value;
	buffer.Length = buf_len;
	
	acpi_args.Count = num_args;
	acpi_args.Pointer = args;
	
	status = AcpiEvaluateObject(handle,method,(args != NULL) ? &acpi_args : NULL,(return_value != NULL) ? &buffer : NULL);
	return (status == AE_OK) ? B_OK : B_ERROR;
}


void 
waking_vector(void) 
{
	//--- This should do something ---
}


status_t 
enter_sleep_state (uint8 state) 
{
	ACPI_STATUS status;
	cpu_status cpu;
	physical_entry wake_vector;
	
	lock_memory(&waking_vector,sizeof(waking_vector),0);
	get_memory_map(&waking_vector,sizeof(waking_vector),&wake_vector,1);
	
	status = AcpiSetFirmwareWakingVector(wake_vector.address);
	if (status != AE_OK)
		return B_ERROR;
	
	status = AcpiEnterSleepStatePrep(state);
	if (status != AE_OK)
		return B_ERROR;
	
	/* NOT SMP SAFE (!!!!!!!!!!!) */
	
	cpu = disable_interrupts();
	status = AcpiEnterSleepState(state);
	restore_interrupts(cpu);
	
	if (status != AE_OK)
		return B_ERROR;
	
	/*status = AcpiLeaveSleepState(state);
	if (status != AE_OK)
		return B_ERROR;*/
	
	return B_OK;
}

