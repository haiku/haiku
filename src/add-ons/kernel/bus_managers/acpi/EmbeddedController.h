/*-
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
#ifndef ACPI_EMBEDDED_CONTROLLER_H
#define ACPI_EMBEDDED_CONTROLLER_H


#include <ctype.h>

#include <ACPI.h>
#include <condition_variable.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <lock.h>

extern "C" {
#	include "acpi.h"
#	include "accommon.h"
#	include "acnamesp.h"
#	include "actypes.h"
#	include "ACPIPrivate.h"
}


// #define TRACE_EMBEDDED_CONTROLLER
#ifdef TRACE_EMBEDDED_CONTROLLER
#	define TRACE(x...) dprintf("EC: " x)
#else
#	define TRACE(x...)
#endif

#define ERROR(x...) dprintf("EC: " x)


typedef uint8							EC_COMMAND;

#define EC_COMMAND_UNKNOWN				((EC_COMMAND) 0x00)
#define EC_COMMAND_READ					((EC_COMMAND) 0x80)
#define EC_COMMAND_WRITE				((EC_COMMAND) 0x81)
#define EC_COMMAND_BURST_ENABLE			((EC_COMMAND) 0x82)
#define EC_COMMAND_BURST_DISABLE		((EC_COMMAND) 0x83)
#define EC_COMMAND_QUERY				((EC_COMMAND) 0x84)

/*
 * EC_STATUS:
 * ----------
 * The encoding of the EC status register is illustrated below.
 * Note that a set bit (1) indicates the property is TRUE
 * (e.g. if bit 0 is set then the output buffer is full).
 * +-+-+-+-+-+-+-+-+
 * |7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +- Output Buffer Full?
 *  | | | | | | +--- Input Buffer Full?
 *  | | | | | +----- <reserved>
 *  | | | | +------- Data Register is Command Byte?
 *  | | | +--------- Burst Mode Enabled?
 *  | | +----------- SCI Event?
 *  | +------------- SMI Event?
 *  +--------------- <reserved>
 *
 */

typedef uint8							EC_STATUS;

#define EC_FLAG_OUTPUT_BUFFER			((EC_STATUS) 0x01)
#define EC_FLAG_INPUT_BUFFER			((EC_STATUS) 0x02)
#define EC_FLAG_DATA_IS_CMD				((EC_STATUS) 0x08)
#define EC_FLAG_BURST_MODE				((EC_STATUS) 0x10)

/*
 * EC_EVENT:
 * ---------
 */
typedef uint8							EC_EVENT;

#define EC_EVENT_UNKNOWN				((EC_EVENT) 0x00)
#define EC_EVENT_OUTPUT_BUFFER_FULL		((EC_EVENT) 0x01)
#define EC_EVENT_INPUT_BUFFER_EMPTY		((EC_EVENT) 0x02)
#define EC_EVENT_SCI					((EC_EVENT) 0x20)
#define EC_EVENT_SMI					((EC_EVENT) 0x40)

/* Data byte returned after burst enable indicating it was successful. */
#define EC_BURST_ACK					0x90


/*
 * Register access primitives
 */
#define EC_GET_DATA(sc) \
		bus_space_read_1((sc)->ec_data_pci_address)

#define EC_SET_DATA(sc, v) \
		bus_space_write_1((sc)->ec_data_pci_address, (v))

#define EC_GET_CSR(sc) \
		bus_space_read_1((sc)->ec_csr_pci_address)

#define EC_SET_CSR(sc, v) \
		bus_space_write_1((sc)->ec_csr_pci_address, (v))

#define ACPI_PKG_VALID(pkg, size) \
		((pkg) != NULL && (pkg)->object_type == ACPI_TYPE_PACKAGE && \
		(pkg)->data.package.count >= (size))


/*
 * Driver cookie.
 */
struct acpi_ec_cookie {
	device_node*				ec_dev;
	acpi_module_info*			ec_acpi_module;
	acpi_device_module_info* 	ec_acpi;
	acpi_device					ec_handle;
	int							ec_uid;
	acpi_handle					ec_gpehandle;
	uint8						ec_gpebit;

	int							ec_data_pci_address;
	int							ec_csr_pci_address;

	int							ec_glk;
	uint32						ec_glkhandle;
	mutex						ec_lock;
	int							ec_burstactive;
	int							ec_sci_pending;
	vint32						ec_gencount;
	ConditionVariable			ec_condition_var;
	int							ec_suspending;
};


/*
 * XXX njl
 * I couldn't find it in the spec but other implementations also use a
 * value of 1 ms for the time to acquire global lock.
 */
#define EC_LOCK_TIMEOUT 1000

/* Default delay in microseconds between each run of the status polling loop. */
#define EC_POLL_DELAY	5

/* Total time in ms spent waiting for a response from EC. */
#define EC_TIMEOUT		750

#define EVENT_READY(event, status) \
		(((event) == EC_EVENT_OUTPUT_BUFFER_FULL && \
		((status) & EC_FLAG_OUTPUT_BUFFER) != 0) || \
		((event) == EC_EVENT_INPUT_BUFFER_EMPTY && \
		((status) & EC_FLAG_INPUT_BUFFER) == 0))



static int		ec_burst_mode = 1;
static int		ec_polled_mode = 0;

static int		ec_timeout = EC_TIMEOUT;


static status_t
EcLock(struct acpi_ec_cookie *sc)
{
	/* If _GLK is non-zero, acquire the global lock. */
	status_t status = B_OK;
	if (sc->ec_glk) {
		status = sc->ec_acpi_module->acquire_global_lock(EC_LOCK_TIMEOUT,
			&sc->ec_glkhandle);
		if (status != B_OK)
			return status;
	}
	mutex_lock(&sc->ec_lock);
	return status;
}


static void
EcUnlock(struct acpi_ec_cookie *sc)
{
	mutex_unlock(&sc->ec_lock);
	if (sc->ec_glk)
		sc->ec_acpi_module->release_global_lock(sc->ec_glkhandle);
}


static uint32			EcGpeHandler(acpi_handle gpeDevice, uint32 gpeNumber,
							void *context);

static acpi_status		EcSpaceSetup(acpi_handle region, uint32 function,
							void *context, void **return_Context);
static acpi_status		EcSpaceHandler(uint32 function,
							acpi_physical_address address,
							uint32 width, int *value,
							void *context, void *regionContext);
static acpi_status		EcWaitEvent(struct acpi_ec_cookie *sc, EC_EVENT event,
							int32 gen_count);
static acpi_status		EcCommand(struct acpi_ec_cookie *sc, EC_COMMAND cmd);
static acpi_status		EcRead(struct acpi_ec_cookie *sc, uint8 address,
							uint8 *readData);
static acpi_status		EcWrite(struct acpi_ec_cookie *sc, uint8 address,
							uint8 writeData);


#endif	// ACPI_EMBEDDED_CONTROLLER_H
