/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright (c) 2009 Clemens Zeidler
 * Copyright (c) 2003-2007 Nate Lawson
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "EmbeddedController.h"

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable.h>
#include <Errors.h>
#include <KernelExport.h>
#include <drivers/PCI.h>


#define ACPI_EC_DRIVER_NAME "drivers/power/acpi_embedded_controller/driver_v1"

#define ACPI_EC_DEVICE_NAME "drivers/power/acpi_embedded_controller/device_v1"

/* Base Namespace devices are published to */
#define ACPI_EC_BASENAME "power/embedded_controller/%d"

// name of pnp generator of path ids
#define ACPI_EC_PATHID_GENERATOR "embedded_controller/path_id"


uint8
bus_space_read_1(int address)
{
	return gPCIManager->read_io_8(address);
}


void
bus_space_write_1(int address, uint8 value)
{
	gPCIManager->write_io_8(address, value);
}


status_t
acpi_GetInteger(acpi_device_module_info* acpi, acpi_device& acpiCookie,
	const char* path, int* number)
{
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);

	// Assume that what we've been pointed at is an Integer object, or
	// a method that will return an Integer.
	status_t status = acpi->evaluate_method(acpiCookie, path, NULL, &buf);
	if (status == B_OK) {
		if (object.object_type == ACPI_TYPE_INTEGER)
			*number = object.integer.integer;
		else
			status = B_BAD_VALUE;
	}
	return status;
}


acpi_handle
acpi_GetReference(acpi_module_info* acpi, acpi_handle scope,
	acpi_object_type* obj)
{
	if (obj == NULL)
		return NULL;

	switch (obj->object_type) {
		case ACPI_TYPE_LOCAL_REFERENCE:
		case ACPI_TYPE_ANY:
			return obj->reference.handle;

		case ACPI_TYPE_STRING:
		{
			// The String object usually contains a fully-qualified path, so
			// scope can be NULL.
			// TODO: This may not always be the case.
			acpi_handle handle;
			if (acpi->get_handle(scope, obj->string.string, &handle)
					== B_OK)
				return handle;
		}
	}

	return NULL;
}


status_t
acpi_PkgInt(acpi_object_type* res, int idx, int* dst)
{
	acpi_object_type* obj = &res->package.objects[idx];
	if (obj == NULL || obj->object_type != ACPI_TYPE_INTEGER)
		return B_BAD_VALUE;
	*dst = obj->integer.integer;

	return B_OK;
}


status_t
acpi_PkgInt32(acpi_object_type* res, int idx, uint32* dst)
{
	int tmp;

	status_t status = acpi_PkgInt(res, idx, &tmp);
	if (status == B_OK)
		*dst = (uint32) tmp;

	return status;
}


acpi_status
embedded_controller_io_ports_parse_callback(ACPI_RESOURCE* resource,
	void* _context)
{
	acpi_ec_cookie* sc = (acpi_ec_cookie*)_context;
	if (resource->Type != ACPI_RESOURCE_TYPE_IO)
		return AE_OK;
	if (sc->ec_data_pci_address == 0) {
		sc->ec_data_pci_address = resource->Data.Io.Minimum;
	} else if (sc->ec_csr_pci_address == 0) {
		sc->ec_csr_pci_address = resource->Data.Io.Minimum;
	} else {
		return AE_CTRL_TERMINATE;
	}

	return AE_OK;
}


// #pragma mark -


static status_t
embedded_controller_open(void* initCookie, const char* path, int flags,
	void** cookie)
{
	acpi_ec_cookie* device = (acpi_ec_cookie*) initCookie;
	*cookie = device;

	return B_OK;
}


static status_t
embedded_controller_close(void* cookie)
{
	return B_OK;
}


static status_t
embedded_controller_read(void* _cookie, off_t position, void* buffer,
	size_t* numBytes)
{
	return B_IO_ERROR;
}


static status_t
embedded_controller_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	return B_IO_ERROR;
}


status_t
embedded_controller_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
embedded_controller_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


static int32
acpi_get_type(device_node* dev)
{
	const char *bus;
	if (gDeviceManager->get_attr_string(dev, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return -1;

	uint32 deviceType;
	if (gDeviceManager->get_attr_uint32(dev, ACPI_DEVICE_TYPE_ITEM,
			&deviceType, false) != B_OK)
		return -1;

	return deviceType;
}


static float
embedded_controller_support(device_node* dev)
{
	TRACE("embedded_controller_support()\n");

	// Check that this is a device
	if (acpi_get_type(dev) != ACPI_TYPE_DEVICE)
		return 0.0;

	const char* name;
	if (gDeviceManager->get_attr_string(dev, ACPI_DEVICE_HID_ITEM, &name, false)
			!= B_OK)
		return 0.0;

	// Test all known IDs

	static const char* kEmbeddedControllerIDs[] = { "PNP0C09" };

	for (size_t i = 0; i < sizeof(kEmbeddedControllerIDs)
			/ sizeof(kEmbeddedControllerIDs[0]); i++) {
		if (!strcmp(name, kEmbeddedControllerIDs[i])) {
			TRACE("supported device found %s\n", name);
			return 0.6;
		}
	}

	return 0.0;
}


static status_t
embedded_controller_register_device(device_node* node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: "ACPI embedded controller" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, ACPI_EC_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
embedded_controller_init_driver(device_node* dev, void** _driverCookie)
{
	TRACE("init driver\n");

	acpi_ec_cookie* sc;
	sc = (acpi_ec_cookie*)malloc(sizeof(acpi_ec_cookie));
	if (sc == NULL)
		return B_NO_MEMORY;

	memset(sc, 0, sizeof(acpi_ec_cookie));

	*_driverCookie = sc;
	sc->ec_dev = dev;

	sc->ec_condition_var.Init(NULL, "ec condition variable");
	mutex_init(&sc->ec_lock, "ec lock");
	device_node* parent = gDeviceManager->get_parent_node(dev);
	gDeviceManager->get_driver(parent, (driver_module_info**)&sc->ec_acpi,
		(void**)&sc->ec_handle);
	gDeviceManager->put_node(parent);

	if (get_module(B_ACPI_MODULE_NAME, (module_info**)&sc->ec_acpi_module)
			!= B_OK)
		return B_ERROR;

	acpi_data buf;
	buf.pointer = NULL;
	buf.length = ACPI_ALLOCATE_BUFFER;

	// Read the unit ID to check for duplicate attach and the
	// global lock value to see if we should acquire it when
	// accessing the EC.
	status_t status = acpi_GetInteger(sc->ec_acpi, sc->ec_handle, "_UID",
		&sc->ec_uid);
	if (status != B_OK)
		sc->ec_uid = 0;
	status = acpi_GetInteger(sc->ec_acpi, sc->ec_handle, "_GLK", &sc->ec_glk);
	if (status != B_OK)
		sc->ec_glk = 0;

	// Evaluate the _GPE method to find the GPE bit used by the EC to
	// signal status (SCI).  If it's a package, it contains a reference
	// and GPE bit, similar to _PRW.
	status = sc->ec_acpi->evaluate_method(sc->ec_handle, "_GPE", NULL, &buf);
	if (status != B_OK) {
		ERROR("can't evaluate _GPE\n");
		goto error;
	}

	acpi_object_type* obj;
	obj = (acpi_object_type*)buf.pointer;
	if (obj == NULL)
		goto error;

	switch (obj->object_type) {
		case ACPI_TYPE_INTEGER:
			sc->ec_gpehandle = NULL;
			sc->ec_gpebit = obj->integer.integer;
			break;
		case ACPI_TYPE_PACKAGE:
			if (!ACPI_PKG_VALID(obj, 2))
				goto error;
			sc->ec_gpehandle = acpi_GetReference(sc->ec_acpi_module, NULL,
				&obj->package.objects[0]);
			if (sc->ec_gpehandle == NULL
				|| acpi_PkgInt32(obj, 1, (uint32*)&sc->ec_gpebit) != B_OK)
				goto error;
			break;
		default:
			ERROR("_GPE has invalid type %i\n", int(obj->object_type));
			goto error;
	}

	sc->ec_suspending = FALSE;

	// Attach bus resources for data and command/status ports.
	status = sc->ec_acpi->walk_resources(sc->ec_handle, (ACPI_STRING)"_CRS",
		embedded_controller_io_ports_parse_callback, sc);
	if (status != B_OK) {
		ERROR("Error while getting IO ports addresses\n");
		goto error;
	}

	// Install a handler for this EC's GPE bit.  We want edge-triggered
	// behavior.
	TRACE("attaching GPE handler\n");
	status = sc->ec_acpi_module->install_gpe_handler(sc->ec_gpehandle,
		sc->ec_gpebit, ACPI_GPE_EDGE_TRIGGERED, &EcGpeHandler, sc);
	if (status != B_OK) {
		TRACE("can't install ec GPE handler\n");
		goto error;
	}

	// Install address space handler
	TRACE("attaching address space handler\n");
	status = sc->ec_acpi->install_address_space_handler(sc->ec_handle,
		ACPI_ADR_SPACE_EC, &EcSpaceHandler, &EcSpaceSetup, sc);
	if (status != B_OK) {
		ERROR("can't install address space handler\n");
		goto error;
	}

	// Enable runtime GPEs for the handler.
	status = sc->ec_acpi_module->enable_gpe(sc->ec_gpehandle, sc->ec_gpebit);
	if (status != B_OK) {
		ERROR("AcpiEnableGpe failed.\n");
		goto error;
	}

	return 0;

error:
	free(buf.pointer);

	sc->ec_acpi_module->remove_gpe_handler(sc->ec_gpehandle, sc->ec_gpebit,
		&EcGpeHandler);
	sc->ec_acpi->remove_address_space_handler(sc->ec_handle, ACPI_ADR_SPACE_EC,
		EcSpaceHandler);

	return ENXIO;
}


static void
embedded_controller_uninit_driver(void* driverCookie)
{
	acpi_ec_cookie* sc = (struct acpi_ec_cookie*)driverCookie;
	mutex_destroy(&sc->ec_lock);
	free(sc);
	put_module(B_ACPI_MODULE_NAME);
}


static status_t
embedded_controller_register_child_devices(void* _cookie)
{
	device_node* node = ((acpi_ec_cookie*)_cookie)->ec_dev;

	int pathID = gDeviceManager->create_id(ACPI_EC_PATHID_GENERATOR);
	if (pathID < 0) {
		TRACE("register_child_device couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_EC_BASENAME, pathID);

	return gDeviceManager->publish_device(node, name, ACPI_EC_DEVICE_NAME);
}


static status_t
embedded_controller_init_device(void* driverCookie, void** cookie)
{
	return B_ERROR;
}


static void
embedded_controller_uninit_device(void* _cookie)
{
	acpi_ec_cookie* device = (acpi_ec_cookie*)_cookie;
	free(device);
}


driver_module_info embedded_controller_driver_module = {
	{
		ACPI_EC_DRIVER_NAME,
		0,
		NULL
	},

	embedded_controller_support,
	embedded_controller_register_device,
	embedded_controller_init_driver,
	embedded_controller_uninit_driver,
	embedded_controller_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info embedded_controller_device_module = {
	{
		ACPI_EC_DEVICE_NAME,
		0,
		NULL
	},

	embedded_controller_init_device,
	embedded_controller_uninit_device,
	NULL,

	embedded_controller_open,
	embedded_controller_close,
	embedded_controller_free,
	embedded_controller_read,
	embedded_controller_write,
	NULL,
	embedded_controller_control,

	NULL,
	NULL
};


// #pragma mark -


static acpi_status
EcCheckStatus(struct acpi_ec_cookie* sc, const char* msg, EC_EVENT event)
{
	acpi_status status = AE_NO_HARDWARE_RESPONSE;
	EC_STATUS ec_status = EC_GET_CSR(sc);

	if (sc->ec_burstactive && !(ec_status & EC_FLAG_BURST_MODE)) {
		TRACE("burst disabled in waitevent (%s)\n", msg);
		sc->ec_burstactive = false;
	}
	if (EVENT_READY(event, ec_status)) {
		TRACE("%s wait ready, status %#x\n", msg, ec_status);
		status = AE_OK;
	}
	return status;
}


static void
EcGpeQueryHandlerSub(struct acpi_ec_cookie *sc)
{
	// Serialize user access with EcSpaceHandler().
	status_t status = EcLock(sc);
	if (status != B_OK) {
		TRACE("GpeQuery lock error.\n");
		return;
	}

	// Send a query command to the EC to find out which _Qxx call it
	// wants to make.  This command clears the SCI bit and also the
	// interrupt source since we are edge-triggered.  To prevent the GPE
	// that may arise from running the query from causing another query
	// to be queued, we clear the pending flag only after running it.
	acpi_status acpi_status = AE_ERROR;
	for (uint8 retry = 0; retry < 2; retry++) {
		acpi_status = EcCommand(sc, EC_COMMAND_QUERY);
		if (acpi_status == AE_OK)
			break;
		if (EcCheckStatus(sc, "retr_check",
			EC_EVENT_INPUT_BUFFER_EMPTY) != AE_OK)
			break;
	}

	if (acpi_status != AE_OK) {
		EcUnlock(sc);
		TRACE("GPE query failed.\n");
		return;
	}
	uint8 data = EC_GET_DATA(sc);

	// We have to unlock before running the _Qxx method below since that
	// method may attempt to read/write from EC address space, causing
	// recursive acquisition of the lock.
	EcUnlock(sc);

	// Ignore the value for "no outstanding event". (13.3.5)
	TRACE("query ok,%s running _Q%02X\n", data ? "" : " not", data);
	if (data == 0)
		return;

	// Evaluate _Qxx to respond to the controller.
	char qxx[5];
	snprintf(qxx, sizeof(qxx), "_Q%02X", data);
	AcpiUtStrupr(qxx);
	status = sc->ec_acpi->evaluate_method(sc->ec_handle, qxx, NULL, NULL);
	if (status != B_OK) {
		TRACE("evaluation of query method %s failed\n", qxx);
	}
}


static void
EcGpeQueryHandler(void* context)
{
	struct acpi_ec_cookie* sc = (struct acpi_ec_cookie*)context;
	int32 pending;

	ASSERT(context != NULL);

	do {
		// Read the current pending count
		pending = atomic_get(&sc->ec_sci_pending);

		// Call GPE handler function
		EcGpeQueryHandlerSub(sc);

		// Try to reset the pending count to zero. If this fails we
		// know another GPE event has occurred while handling the
		// current GPE event and need to loop.
	} while (atomic_test_and_set(&sc->ec_sci_pending, 0, pending));
}


/*!	The GPE handler is called when IBE/OBF or SCI events occur.  We are
	called from an unknown lock context.
*/
static uint32
EcGpeHandler(acpi_handle gpeDevice, uint32 gpeNumber, void* context)
{
	struct acpi_ec_cookie* sc = (acpi_ec_cookie*)context;

	ASSERT(context != NULL);//, ("EcGpeHandler called with NULL"));
	TRACE("gpe handler start\n");

	// Notify EcWaitEvent() that the status register is now fresh.  If we
	// didn't do this, it wouldn't be possible to distinguish an old IBE
	// from a new one, for example when doing a write transaction (writing
	// address and then data values.)
	atomic_add(&sc->ec_gencount, 1);
	sc->ec_condition_var.NotifyAll();

	// If the EC_SCI bit of the status register is set, queue a query handler.
	// It will run the query and _Qxx method later, under the lock.
	EC_STATUS ecStatus = EC_GET_CSR(sc);
	if ((ecStatus & EC_EVENT_SCI) && atomic_add(&sc->ec_sci_pending, 1) == 0) {
		TRACE("gpe queueing query handler\n");
		acpi_status status = AcpiOsExecute(OSL_GPE_HANDLER, EcGpeQueryHandler,
			context);
		if (status != AE_OK) {
			dprintf("EcGpeHandler: queuing GPE query handler failed\n");
			atomic_add(&sc->ec_sci_pending, -1);
		}
	}
	return ACPI_REENABLE_GPE;
}


static acpi_status
EcSpaceSetup(acpi_handle region, uint32 function, void* context,
	void** regionContext)
{
	// If deactivating a region, always set the output to NULL.  Otherwise,
	// just pass the context through.
	if (function == ACPI_REGION_DEACTIVATE)
		*regionContext = NULL;
	else
		*regionContext = context;

	return AE_OK;
}


static acpi_status
EcSpaceHandler(uint32 function, acpi_physical_address address, uint32 width,
	int* value, void* context, void* regionContext)
{
	TRACE("enter EcSpaceHandler\n");
	struct acpi_ec_cookie* sc = (struct acpi_ec_cookie*)context;

	if (function != ACPI_READ && function != ACPI_WRITE)
		return AE_BAD_PARAMETER;
	if (width % 8 != 0 || value == NULL || context == NULL)
		return AE_BAD_PARAMETER;
	if (address + width / 8 > 256)
		return AE_BAD_ADDRESS;

	// If booting, check if we need to run the query handler.  If so, we
	// we call it directly here as scheduling and dpc might not be up yet.
	// (Not sure if it's needed)

	if (gKernelStartup || gKernelShutdown || sc->ec_suspending) {
		if ((EC_GET_CSR(sc) & EC_EVENT_SCI) &&
			atomic_add(&sc->ec_sci_pending, 1) == 0) {
			//CTR0(KTR_ACPI, "ec running gpe handler directly");
			EcGpeQueryHandler(sc);
		}
	}

	// Serialize with EcGpeQueryHandler() at transaction granularity.
	acpi_status status = EcLock(sc);
	if (status != B_OK)
		return AE_NOT_ACQUIRED;

	// If we can't start burst mode, continue anyway.
	status = EcCommand(sc, EC_COMMAND_BURST_ENABLE);
	if (status == B_OK) {
		if (EC_GET_DATA(sc) == EC_BURST_ACK) {
			TRACE("burst enabled.\n");
			sc->ec_burstactive = TRUE;
		}
	}

	// Perform the transaction(s), based on width.
	ACPI_PHYSICAL_ADDRESS ecAddr = address;
	uint8* ecData = (uint8 *) value;
	if (function == ACPI_READ)
		*value = 0;
	do {
		switch (function) {
			case ACPI_READ:
				status = EcRead(sc, ecAddr, ecData);
				break;
			case ACPI_WRITE:
				status = EcWrite(sc, ecAddr, *ecData);
				break;
		}
		if (status != AE_OK)
			break;
		ecAddr++;
		ecData++;
	} while (ecAddr < address + width / 8);

	if (sc->ec_burstactive) {
		sc->ec_burstactive = FALSE;
		if (EcCommand(sc, EC_COMMAND_BURST_DISABLE) == AE_OK)
			TRACE("disabled burst ok.");
	}

	EcUnlock(sc);
	return status;
}


static acpi_status
EcWaitEvent(struct acpi_ec_cookie* sc, EC_EVENT event, int32 generationCount)
{
	static int32 noIntr = 0;
	acpi_status status = AE_NO_HARDWARE_RESPONSE;
	int32 count, i;

	int needPoll = ec_polled_mode || sc->ec_suspending
		|| gKernelStartup || gKernelShutdown;

	// Wait for event by polling or GPE (interrupt).
	if (needPoll) {
		count = (ec_timeout * 1000) / EC_POLL_DELAY;
		if (count == 0)
			count = 1;
		spin(10);
		for (i = 0; i < count; i++) {
			status = EcCheckStatus(sc, "poll", event);
			if (status == AE_OK)
				break;
			spin(EC_POLL_DELAY);
		}
	} else {
		// Wait for the GPE to signal the status changed, checking the
		// status register each time we get one.  It's possible to get a
		// GPE for an event we're not interested in here (i.e., SCI for
		// EC query).
		for (i = 0; i < ec_timeout; i++) {
			if (generationCount == sc->ec_gencount) {
				sc->ec_condition_var.Wait(B_RELATIVE_TIMEOUT, 1000);
			}
			/*
			 * Record new generation count.  It's possible the GPE was
			 * just to notify us that a query is needed and we need to
			 * wait for a second GPE to signal the completion of the
			 * event we are actually waiting for.
			 */
			status = EcCheckStatus(sc, "sleep", event);
			if (status == AE_OK) {
				if (generationCount == sc->ec_gencount)
					noIntr++;
				else
					noIntr = 0;
				break;
			}
			generationCount = sc->ec_gencount;
		}

		/*
		 * We finished waiting for the GPE and it never arrived.  Try to
		 * read the register once and trust whatever value we got.  This is
		 * the best we can do at this point.
		 */
		if (status != AE_OK)
			status = EcCheckStatus(sc, "sleep_end", event);
	}
	if (!needPoll && noIntr > 10) {
		TRACE("not getting interrupts, switched to polled mode\n");
		ec_polled_mode = true;
	}

	if (status != AE_OK)
		TRACE("error: ec wait timed out\n");

	return status;
}


static acpi_status
EcCommand(struct acpi_ec_cookie* sc, EC_COMMAND cmd)
{
	// Don't use burst mode if user disabled it.
	if (!ec_burst_mode && cmd == EC_COMMAND_BURST_ENABLE)
		return AE_ERROR;

	// Decide what to wait for based on command type.
	EC_EVENT event;
	switch (cmd) {
		case EC_COMMAND_READ:
		case EC_COMMAND_WRITE:
		case EC_COMMAND_BURST_DISABLE:
			event = EC_EVENT_INPUT_BUFFER_EMPTY;
			break;
		case EC_COMMAND_QUERY:
		case EC_COMMAND_BURST_ENABLE:
			event = EC_EVENT_OUTPUT_BUFFER_FULL;
			break;
		default:
			TRACE("EcCommand: invalid command %#x\n", cmd);
			return AE_BAD_PARAMETER;
	}

	// Ensure empty input buffer before issuing command.
	// Use generation count of zero to force a quick check.
	acpi_status status = EcWaitEvent(sc, EC_EVENT_INPUT_BUFFER_EMPTY, 0);
	if (status != AE_OK)
		return status;

	// Run the command and wait for the chosen event.
	TRACE("running command %#x\n", cmd);
	int32 generationCount = sc->ec_gencount;
	EC_SET_CSR(sc, cmd);
	status = EcWaitEvent(sc, event, generationCount);
	if (status == AE_OK) {
		// If we succeeded, burst flag should now be present.
		if (cmd == EC_COMMAND_BURST_ENABLE) {
			EC_STATUS ec_status = EC_GET_CSR(sc);
			if ((ec_status & EC_FLAG_BURST_MODE) == 0)
				status = AE_ERROR;
		}
	} else
		TRACE("EcCommand: no response to %#x\n", cmd);

	return status;
}


static acpi_status
EcRead(struct acpi_ec_cookie* sc, uint8 address, uint8* readData)
{
	TRACE("read from %#x\n", address);

	acpi_status status;
	for (uint8 retry = 0; retry < 2; retry++) {
		status = EcCommand(sc, EC_COMMAND_READ);
		if (status != AE_OK)
			return status;

		int32 generationCount = sc->ec_gencount;
		EC_SET_DATA(sc, address);
		status = EcWaitEvent(sc, EC_EVENT_OUTPUT_BUFFER_FULL, generationCount);
		if (status == AE_OK) {
			*readData = EC_GET_DATA(sc);
			return AE_OK;
		}
		if (EcCheckStatus(sc, "retr_check", EC_EVENT_INPUT_BUFFER_EMPTY)
				!= AE_OK) {
			break;
		}
	}

	TRACE("EcRead: failed waiting to get data\n");
	return status;
}


static acpi_status
EcWrite(struct acpi_ec_cookie* sc, uint8 address, uint8 writeData)
{
	acpi_status status = EcCommand(sc, EC_COMMAND_WRITE);
	if (status != AE_OK)
		return status;

	int32 generationCount = sc->ec_gencount;
	EC_SET_DATA(sc, address);
	status = EcWaitEvent(sc, EC_EVENT_INPUT_BUFFER_EMPTY, generationCount);
	if (status != AE_OK) {
		TRACE("EcWrite: failed waiting for sent address\n");
		return status;
	}

	generationCount = sc->ec_gencount;
	EC_SET_DATA(sc, writeData);
	status = EcWaitEvent(sc, EC_EVENT_INPUT_BUFFER_EMPTY, generationCount);
	if (status != AE_OK) {
		TRACE("EcWrite: failed waiting for sent data\n");
		return status;
	}

	return AE_OK;
}
