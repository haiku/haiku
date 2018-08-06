/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>
#include <Drivers.h>
#include <Errors.h>
#include <KernelExport.h>

#include <arch_system_info.h>
#include <cpu.h>
#include <cpuidle.h>
#include <smp.h>

#include "x86_cpuidle.h"


#define ACPI_PDC_REVID		0x1
#define ACPI_OSC_QUERY		(1 << 0)

#define ACPI_PDC_P_FFH		(1 << 0)
#define ACPI_PDC_C_C1_HALT	(1 << 1)
#define ACPI_PDC_T_FFH		(1 << 2)
#define ACPI_PDC_SMP_C1PT	(1 << 3)
#define ACPI_PDC_SMP_C2C3	(1 << 4)
#define ACPI_PDC_SMP_P_SW	(1 << 5)
#define ACPI_PDC_SMP_C_SW	(1 << 6)
#define ACPI_PDC_SMP_T_SW	(1 << 7)
#define ACPI_PDC_C_C1_FFH	(1 << 8)
#define ACPI_PDC_C_C2C3_FFH	(1 << 9)
#define ACPI_PDC_P_HWCOORD	(1 << 11)

// Bus Master check required
#define ACPI_PDC_GAS_BM		(1 << 1)

#define ACPI_CSTATE_HALT	0x1
#define ACPI_CSTATE_SYSIO	0x2
#define ACPI_CSTATE_FFH		0x3

// Bus Master Check
#define ACPI_FLAG_C_BM		(1 << 0)
// Bus master arbitration
#define ACPI_FLAG_C_ARB		(1 << 1)

// Copied from acpica's actypes.h, where's the best place to put?
#define ACPI_BITREG_BUS_MASTER_STATUS           0x01
#define ACPI_BITREG_BUS_MASTER_RLD              0x0F
#define ACPI_BITREG_ARB_DISABLE                 0x13

#define ACPI_STATE_C0                   (uint8) 0
#define ACPI_STATE_C1                   (uint8) 1
#define ACPI_STATE_C2                   (uint8) 2
#define ACPI_STATE_C3                   (uint8) 3
#define ACPI_C_STATES_MAX               ACPI_STATE_C3
#define ACPI_C_STATE_COUNT              4


#define ACPI_CPUIDLE_MODULE_NAME "drivers/power/x86_cpuidle/acpi/driver_v1"


struct acpicpu_reg {
	uint8	reg_desc;
	uint16	reg_reslen;
	uint8	reg_spaceid;
	uint8	reg_bitwidth;
	uint8	reg_bitoffset;
	uint8	reg_accesssize;
	uint64	reg_addr;
} __attribute__((packed));

struct acpi_cpuidle_driver_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
	uint32 flags;
	int32 cpuIndex;
};

struct acpi_cstate_info {
	uint32 address;
	uint8 skip_bm_sts;
	uint8 method;
	uint8 type;
};


static acpi_cpuidle_driver_info *sAcpiProcessor[SMP_MAX_CPUS];
static CpuidleDevice sAcpiDevice;
static device_manager_info *sDeviceManager;
static acpi_module_info *sAcpi;

CpuidleModuleInfo *gIdle;


static status_t
acpi_eval_pdc(acpi_cpuidle_driver_info *device)
{
	acpi_objects arg;
	acpi_object_type obj;
	uint32 cap[3];

	arg.count = 1;
	arg.pointer = &obj;
	cap[0] = 1;
	cap[1] = 1;
	cap[2] = ACPI_PDC_C_C1_HALT | ACPI_PDC_SMP_C1PT | ACPI_PDC_SMP_C2C3;
	cap[2] |= ACPI_PDC_SMP_P_SW | ACPI_PDC_SMP_C_SW | ACPI_PDC_SMP_T_SW;
	cap[2] |= ACPI_PDC_C_C1_FFH | ACPI_PDC_C_C2C3_FFH;
	cap[2] |= ACPI_PDC_SMP_T_SW | ACPI_PDC_P_FFH | ACPI_PDC_P_HWCOORD
		| ACPI_PDC_T_FFH;
	obj.object_type = ACPI_TYPE_BUFFER;
	obj.data.buffer.length = sizeof(cap);
	obj.data.buffer.buffer = cap;
	status_t status = device->acpi->evaluate_method(device->acpi_cookie, "_PDC",
		&arg, NULL);
	return status;
}


static status_t
acpi_eval_osc(acpi_cpuidle_driver_info *device)
{
	// guid for intel platform
	dprintf("%s@%p\n", __func__, device->acpi_cookie);
	static uint8 uuid[] = {
		0x16, 0xA6, 0x77, 0x40, 0x0C, 0x29, 0xBE, 0x47,
		0x9E, 0xBD, 0xD8, 0x70, 0x58, 0x71, 0x39, 0x53
	};
	uint32 cap[2];
	cap[0] = 0;
	cap[1] = ACPI_PDC_C_C1_HALT | ACPI_PDC_SMP_C1PT | ACPI_PDC_SMP_C2C3;
	cap[1] |= ACPI_PDC_SMP_P_SW | ACPI_PDC_SMP_C_SW | ACPI_PDC_SMP_T_SW;
	cap[1] |= ACPI_PDC_C_C1_FFH | ACPI_PDC_C_C2C3_FFH;
	cap[1] |= ACPI_PDC_SMP_T_SW | ACPI_PDC_P_FFH | ACPI_PDC_P_HWCOORD
		| ACPI_PDC_T_FFH;

	acpi_objects arg;
	acpi_object_type obj[4];

	arg.count = 4;
	arg.pointer = obj;

	obj[0].object_type = ACPI_TYPE_BUFFER;
	obj[0].data.buffer.length = sizeof(uuid);
	obj[0].data.buffer.buffer = uuid;
	obj[1].object_type = ACPI_TYPE_INTEGER;
	obj[1].data.integer = ACPI_PDC_REVID;
	obj[2].object_type = ACPI_TYPE_INTEGER;
	obj[2].data.integer = sizeof(cap)/sizeof(cap[0]);
	obj[3].object_type = ACPI_TYPE_BUFFER;
	obj[3].data.buffer.length = sizeof(cap);
	obj[3].data.buffer.buffer = (void *)cap;

	acpi_data buf;
	buf.pointer = NULL;
	buf.length = ACPI_ALLOCATE_LOCAL_BUFFER;
	status_t status = device->acpi->evaluate_method(device->acpi_cookie, "_OSC",
		&arg, &buf);
	if (status != B_OK)
		return status;
	acpi_object_type *osc = (acpi_object_type *)buf.pointer;
	if (osc->object_type != ACPI_TYPE_BUFFER)
		return B_BAD_TYPE;
	if (osc->data.buffer.length != sizeof(cap))
		return B_BUFFER_OVERFLOW;
	return status;
}


static inline bool
acpi_cstate_bm_check(void)
{
	uint32 val;
	sAcpi->read_bit_register(ACPI_BITREG_BUS_MASTER_STATUS, &val);
	if (!val)
		return false;
	sAcpi->write_bit_register(ACPI_BITREG_BUS_MASTER_STATUS, 1);

	return true;
}


static inline void
acpi_cstate_ffh_enter(CpuidleCstate *cState)
{
	cpu_ent *cpu = get_cpu_struct();
	if (cpu->invoke_scheduler)
		return;

	x86_monitor((void *)&cpu->invoke_scheduler, 0, 0);
	if (!cpu->invoke_scheduler)
		x86_mwait((unsigned long)cState->pData, 1);
}


static inline void
acpi_cstate_halt(void)
{
	cpu_ent *cpu = get_cpu_struct();
	if (cpu->invoke_scheduler)
		return;
	asm("hlt");
}


static void
acpi_cstate_enter(CpuidleCstate *cState)
{
	acpi_cstate_info *ci = (acpi_cstate_info *)cState->pData;
	if (ci->method == ACPI_CSTATE_FFH)
		acpi_cstate_ffh_enter(cState);
	else if (ci->method == ACPI_CSTATE_SYSIO)
		in8(ci->address);
	else
		acpi_cstate_halt();
}


static int32
acpi_cstate_idle(int32 state, CpuidleDevice *device)
{
	CpuidleCstate *cState = &device->cStates[state];
	acpi_cstate_info *ci = (acpi_cstate_info *)cState->pData;
	if (!ci->skip_bm_sts) {
		// we fall back to C1 if there's bus master activity
		if (acpi_cstate_bm_check())
			state = 1;
	}
	if (ci->type != ACPI_STATE_C3)
		acpi_cstate_enter(cState);

	// set BM_RLD for Bus Master to activity to wake the system from C3
	// With Newer chipsets BM_RLD is a NOP Since DMA is automatically handled
	// during C3 State
	acpi_cpuidle_driver_info *pi = sAcpiProcessor[smp_get_current_cpu()];
	if (pi->flags & ACPI_FLAG_C_BM)
		sAcpi->write_bit_register(ACPI_BITREG_BUS_MASTER_RLD, 1);

	// disable bus master arbitration during C3
	if (pi->flags & ACPI_FLAG_C_ARB)
		sAcpi->write_bit_register(ACPI_BITREG_ARB_DISABLE, 1);

	acpi_cstate_enter(cState);

	// clear BM_RLD and re-enable the arbiter
	if (pi->flags & ACPI_FLAG_C_BM)
		sAcpi->write_bit_register(ACPI_BITREG_BUS_MASTER_RLD, 0);

	if (pi->flags & ACPI_FLAG_C_ARB)
		sAcpi->write_bit_register(ACPI_BITREG_ARB_DISABLE, 0);

	return state;
}


static status_t
acpi_cstate_add(acpi_object_type *object, CpuidleCstate *cState)
{
	acpi_cstate_info *ci = (acpi_cstate_info *)malloc(sizeof(acpi_cstate_info));
	if (!ci)
		return B_NO_MEMORY;

	if (object->object_type != ACPI_TYPE_PACKAGE) {
		dprintf("invalid _CST object\n");
		goto error;
	}

	if (object->data.package.count != 4) {
		dprintf("invalid _CST number\n");
		goto error;
	}

	// type
	acpi_object_type * pointer = &object->data.package.objects[1];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		goto error;
	}
	uint32 n = pointer->data.integer;
	if (n < 1 || n > 3) {
		dprintf("invalid _CST elem value\n");
		goto error;
	}
	ci->type = n;
	dprintf("C%" B_PRId32 "\n", n);
	snprintf(cState->name, sizeof(cState->name), "C%" B_PRId32, n);

	// Latency
	pointer = &object->data.package.objects[2];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		goto error;
	}
	n = pointer->data.integer;
	cState->latency = n;
	dprintf("Latency: %" B_PRId32 "\n", n);

	// power
	pointer = &object->data.package.objects[3];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		goto error;
	}
	n = pointer->data.integer;
	dprintf("power: %" B_PRId32 "\n", n);

	// register
	pointer = &object->data.package.objects[0];
	if (pointer->object_type != ACPI_TYPE_BUFFER) {
		dprintf("invalid _CST elem type\n");
		goto error;
	}
	if (pointer->data.buffer.length < 15) {
		dprintf("invalid _CST elem length\n");
		goto error;
	}

	struct acpicpu_reg *reg = (struct acpicpu_reg *)pointer->data.buffer.buffer;
	switch (reg->reg_spaceid) {
		case ACPI_ADR_SPACE_SYSTEM_IO:
			dprintf("IO method\n");
			if (reg->reg_addr == 0) {
				dprintf("illegal address\n");
				goto error;
			}
			if (reg->reg_bitwidth != 8) {
				dprintf("invalid source length\n");
				goto error;
			}
			ci->address = reg->reg_addr;
			ci->method = ACPI_CSTATE_SYSIO;
			break;
		case ACPI_ADR_SPACE_FIXED_HARDWARE:
		{
			dprintf("FFH method\n");
			ci->method = ACPI_CSTATE_FFH;
			ci->address = reg->reg_addr;

			// skip checking BM_STS if ACPI_PDC_GAS_BM is cleared
			cpu_ent *cpu = get_cpu_struct();
			if ((cpu->arch.vendor == VENDOR_INTEL) &&
				!(reg->reg_accesssize & ACPI_PDC_GAS_BM))
				ci->skip_bm_sts = 1;
			break;
		}
		default:
			dprintf("invalid spaceid %" B_PRId8 "\n", reg->reg_spaceid);
			break;
	}
	cState->pData = ci;
	cState->EnterIdle = acpi_cstate_idle;

	return B_OK;
error:
	free(ci);
	return B_ERROR;
}


static void
acpi_cstate_quirks(acpi_cpuidle_driver_info *device)
{
	cpu_ent *cpu = get_cpu_struct();
	// Calculated Model Value: M = (Extended Model << 4) + Model
	uint32 model = (cpu->arch.extended_model << 4) + cpu->arch.model;

	// On all recent Intel platforms, ARB_DIS is not necessary
	if (cpu->arch.vendor != VENDOR_INTEL)
		return;
	if (cpu->arch.family > 0xf || (cpu->arch.family == 6 && model >= 0xf))
		device->flags &= ~ACPI_FLAG_C_ARB;
}


static status_t
acpi_cpuidle_setup(acpi_cpuidle_driver_info *device)
{
	// _PDC is deprecated in the ACPI 3.0, we will try _OSC firstly
	// and fall back to _PDC if _OSC fail
	status_t status = acpi_eval_osc(device);
	if (status != B_OK)
		status = acpi_eval_pdc(device);
	if (status != B_OK) {
		dprintf("failed to eval _OSC and _PDC\n");
		return status;
	}

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;

	dprintf("evaluate _CST @%p\n", device->acpi_cookie);
	status = device->acpi->evaluate_method(device->acpi_cookie, "_CST", NULL,
		&buffer);
	if (status != B_OK) {
		dprintf("failed to get _CST\n");
		return B_IO_ERROR;
	}

	acpi_object_type *object = (acpi_object_type *)buffer.pointer;
	if (object->object_type != ACPI_TYPE_PACKAGE)
		dprintf("invalid _CST type\n");
	if (object->data.package.count < 2)
		dprintf("invalid _CST count\n");

	acpi_object_type *pointer = object->data.package.objects;
	if (pointer[0].object_type != ACPI_TYPE_INTEGER)
		dprintf("invalid _CST type 2\n");
	uint32 n = pointer[0].data.integer;
	if (n != object->data.package.count - 1)
		dprintf("invalid _CST count 2\n");
	if (n > 8)
		dprintf("_CST has too many states\n");
	dprintf("cpuidle found %" B_PRId32 " cstates\n", n);
	uint32 count = 1;
	for (uint32 i = 1; i <= n; i++) {
		pointer = &object->data.package.objects[i];
		if (acpi_cstate_add(pointer, &sAcpiDevice.cStates[count]) == B_OK)
			++count;
	}
	sAcpiDevice.cStateCount = count;
	free(buffer.pointer);

	// TODO we assume BM is a must and ARB_DIS is always available
	device->flags |= ACPI_FLAG_C_ARB | ACPI_FLAG_C_BM;

	acpi_cstate_quirks(device);

	return B_OK;
}


static status_t
acpi_cpuidle_init(void)
{
	dprintf("acpi_cpuidle_init\n");

	for (int32 i = 0; i < smp_get_num_cpus(); i++)
		if (acpi_cpuidle_setup(sAcpiProcessor[i]) != B_OK)
			return B_ERROR;

	status_t status = gIdle->AddDevice(&sAcpiDevice);
	if (status == B_OK)
		dprintf("using acpi idle\n");
	return status;
}


static status_t
acpi_processor_init(acpi_cpuidle_driver_info *device)
{
	// get the CPU index
	dprintf("get acpi processor @%p\n", device->acpi_cookie);

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status_t status = device->acpi->evaluate_method(device->acpi_cookie, NULL,
		NULL, &buffer);
	if (status != B_OK) {
		dprintf("failed to get processor obj\n");
		return status;
	}

	acpi_object_type *object = (acpi_object_type *)buffer.pointer;
	dprintf("acpi cpu%" B_PRId32 ": P_BLK at %#x/%lu\n",
		object->data.processor.cpu_id,
		object->data.processor.pblk_address,
		object->data.processor.pblk_length);

	int32 cpuIndex = object->data.processor.cpu_id;
	free(buffer.pointer);

	if (cpuIndex < 0 || cpuIndex >= smp_get_num_cpus())
		return B_ERROR;

	device->cpuIndex = cpuIndex;
	sAcpiProcessor[cpuIndex] = device;

	// If nodes for all processors have been registered, init the idle callback.
	for (int32 i = smp_get_num_cpus() - 1; i >= 0; i--) {
		if (sAcpiProcessor[i] == NULL)
			return B_OK;
	}

	if (intel_cpuidle_init() == B_OK)
		return B_OK;

	status = acpi_cpuidle_init();
	if (status != B_OK)
		sAcpiProcessor[cpuIndex] = NULL;

	return status;
}


static float
acpi_cpuidle_support(device_node *parent)
{
	const char *bus;
	uint32 device_type;

	dprintf("acpi_cpuidle_support\n");
	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi") != 0)
		return 0.0;

	// check whether it's really a cpu Device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_PROCESSOR) {
		return 0.0;
	}

	return 0.6;
}


static status_t
acpi_cpuidle_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI CPU IDLE" }},
		{ NULL }
	};

	dprintf("acpi_cpuidle_register_device\n");
	return sDeviceManager->register_node(node, ACPI_CPUIDLE_MODULE_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_cpuidle_init_driver(device_node *node, void **driverCookie)
{
	dprintf("acpi_cpuidle_init_driver\n");
	acpi_cpuidle_driver_info *device;
	device = (acpi_cpuidle_driver_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	device_node *parent;
	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	status_t status = acpi_processor_init(device);
	if (status != B_OK) {
		free(device);
		return status;
	}

	*driverCookie = device;
	return B_OK;
}


static void
acpi_cpuidle_uninit_driver(void *driverCookie)
{
	dprintf("acpi_cpuidle_uninit_driver");
	acpi_cpuidle_driver_info *device = (acpi_cpuidle_driver_info *)driverCookie;
	// TODO: When the first device to be unregistered, we'd need to balance the
	// gIdle->AddDevice() call, but ATM isn't any API for that.
	sAcpiProcessor[device->cpuIndex] = NULL;
	free(device);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&sAcpi},
	{ B_CPUIDLE_MODULE_NAME, (module_info **)&gIdle },
	{}
};


static driver_module_info sAcpiidleModule = {
	{
		ACPI_CPUIDLE_MODULE_NAME,
		0,
		NULL
	},

	acpi_cpuidle_support,
	acpi_cpuidle_register_device,
	acpi_cpuidle_init_driver,
	acpi_cpuidle_uninit_driver,
	NULL,
	NULL,	// rescan
	NULL,	// removed
};


module_info *modules[] = {
	(module_info *)&sAcpiidleModule,
	NULL
};
