/*
 * Copyright 2012-2025, Haiku, Inc. All rights reserved.
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
#include <thread.h>

#include "x86_mwait.h"


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


#define ACPI_CPUIDLE_MODULE_NAME CPUIDLE_MODULES_PREFIX "/x86_acpi_cstates/v1"


struct acpicpu_reg {
	uint8	reg_desc;
	uint16	reg_reslen;
	uint8	reg_spaceid;
	uint8	reg_bitwidth;
	uint8	reg_bitoffset;
	uint8	reg_accesssize;
	uint64	reg_addr;
} __attribute__((packed));

struct acpi_cstate_info {
	char name[B_OS_NAME_LENGTH];
	uint32 latency;

	uint32 address;
	uint8 skip_bm_sts;
	uint8 method;
	uint8 type;
};

struct acpi_cpuidle_driver_info {
	device_node *processor;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
	uint32 flags;

#define MAX_CSTATES 8
	int32 state_count;
	acpi_cstate_info states[MAX_CSTATES];
};


static acpi_cpuidle_driver_info *sAcpiProcessor[SMP_MAX_CPUS];
static device_manager_info *sDeviceManager;
static acpi_module_info *sAcpi;

static int32 sStateIndex = -1;


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
	obj.buffer.length = sizeof(cap);
	obj.buffer.buffer = cap;
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
	obj[0].buffer.length = sizeof(uuid);
	obj[0].buffer.buffer = uuid;
	obj[1].object_type = ACPI_TYPE_INTEGER;
	obj[1].integer.integer = ACPI_PDC_REVID;
	obj[2].object_type = ACPI_TYPE_INTEGER;
	obj[2].integer.integer = sizeof(cap)/sizeof(cap[0]);
	obj[3].object_type = ACPI_TYPE_BUFFER;
	obj[3].buffer.length = sizeof(cap);
	obj[3].buffer.buffer = (void *)cap;

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
	if (osc->buffer.length != sizeof(cap))
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
acpi_cstate_ffh_enter(acpi_cstate_info *ci)
{
	int dummy;
	x86_monitor(&dummy, 0, 0);
	x86_mwait((ci->type << 4), MWAIT_INTERRUPTS_BREAK);
}


static inline void
acpi_cstate_halt(void)
{
	// The idle routine may perform extra steps before HLT depending on CPU.
	gCpuIdleFunc();
}


static void
acpi_cstate_enter(acpi_cstate_info *ci)
{
	if (ci->method == ACPI_CSTATE_FFH)
		acpi_cstate_ffh_enter(ci);
	else if (ci->method == ACPI_CSTATE_SYSIO)
		in8(ci->address);
	else
		acpi_cstate_halt();
}


static void
acpi_cstate_set_scheduler_mode(scheduler_mode mode)
{
	int maxState;
	if (mode == SCHEDULER_MODE_POWER_SAVING)
		maxState = ACPI_STATE_C3;
	else
		maxState = ACPI_STATE_C1;

	acpi_cpuidle_driver_info *pi = sAcpiProcessor[0];
	int32 index = -1;
	for (int i = 0; i < pi->state_count; i++) {
		if (pi->states[i].type > maxState)
			break;

		index = i;
	}

	sStateIndex = index;
}


static void
acpi_cstate_idle()
{
	Thread* thread = thread_get_current_thread();
	if (thread->pinned_to_cpu <= 0 || thread->post_interrupt_callback != NULL)
		panic("invalid thread state");

	acpi_cpuidle_driver_info *pi = sAcpiProcessor[smp_get_current_cpu()];
	acpi_cstate_info *ci = NULL;
	int32 stateIndex = -1;

	// If any interrupts occur, we have to back out and start over.
	struct IdlingState {
		jmp_buf reset_jump;
		int32 preparing;
	} idlingState;

	idlingState.preparing = 1;
	int result = setjmp(idlingState.reset_jump);
	if (result != 0) {
		enable_interrupts();
		if (ci->type == ACPI_STATE_C3)
			goto C3_out;

		thread->post_interrupt_callback = NULL;
		return;
	}

	thread->post_interrupt_data = &idlingState;
	thread->post_interrupt_callback = [](void* state) {
		thread_get_current_thread()->post_interrupt_callback = NULL;

		IdlingState* idlingState = (IdlingState*)state;
		if (idlingState->preparing == 1) {
			// No need to longjmp, just unset this value.
			// (We may be in the middle of calling ACPI or other routines,
			// so using longjmp may not be safe here anyway.)
			idlingState->preparing = 0;
			return;
		}

		longjmp(idlingState->reset_jump, EINTR);
	};

	if ((stateIndex = sStateIndex) < 0) {
		// No C-state currently set.
		thread->post_interrupt_callback = NULL;
		acpi_cstate_halt();
		return;
	}

	ci = &pi->states[stateIndex];
	if (!ci->skip_bm_sts) {
		// we fall back to C1 if there's bus master activity
		if (acpi_cstate_bm_check())
			ci = &pi->states[0];
	}
	if (ci->type != ACPI_STATE_C3) {
		if (atomic_test_and_set(&idlingState.preparing, 0, 1) == 1)
			acpi_cstate_enter(ci);

		thread->post_interrupt_callback = NULL;
		return;
	}

	// set BM_RLD for Bus Master to activity to wake the system from C3
	// With Newer chipsets BM_RLD is a NOP Since DMA is automatically handled
	// during C3 State
	if (pi->flags & ACPI_FLAG_C_BM)
		sAcpi->write_bit_register(ACPI_BITREG_BUS_MASTER_RLD, 1);

	// disable bus master arbitration during C3
	if (pi->flags & ACPI_FLAG_C_ARB)
		sAcpi->write_bit_register(ACPI_BITREG_ARB_DISABLE, 1);

	if (atomic_test_and_set(&idlingState.preparing, 0, 1) == 1)
		acpi_cstate_enter(ci);

C3_out:
	thread->post_interrupt_callback = NULL;

	// clear BM_RLD and re-enable the arbiter
	if (pi->flags & ACPI_FLAG_C_BM)
		sAcpi->write_bit_register(ACPI_BITREG_BUS_MASTER_RLD, 0);

	if (pi->flags & ACPI_FLAG_C_ARB)
		sAcpi->write_bit_register(ACPI_BITREG_ARB_DISABLE, 0);
}


static void
acpi_cstate_wait(int32* variable, int32 test)
{
	arch_cpu_pause();
}


static status_t
acpi_cstate_add(acpi_object_type *object, acpi_cstate_info *ci)
{
	if (object->object_type != ACPI_TYPE_PACKAGE) {
		dprintf("invalid _CST object\n");
		return B_ERROR;
	}

	if (object->package.count != 4) {
		dprintf("invalid _CST number\n");
		return B_ERROR;
	}

	// type
	acpi_object_type * pointer = &object->package.objects[1];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		return B_ERROR;
	}
	uint32 n = pointer->integer.integer;
	if (n < 1 || n > 3) {
		dprintf("invalid _CST elem value\n");
		return B_ERROR;
	}
	ci->type = n;
	dprintf("C%" B_PRId32 " ", n);
	snprintf(ci->name, sizeof(ci->name), "C%" B_PRId32, n);

	// Latency
	pointer = &object->package.objects[2];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		return B_ERROR;
	}
	n = pointer->integer.integer;
	ci->latency = n;
	dprintf("latency: %" B_PRId32 ", ", n);

	// power
	pointer = &object->package.objects[3];
	if (pointer->object_type != ACPI_TYPE_INTEGER) {
		dprintf("invalid _CST elem type\n");
		return B_ERROR;
	}
	n = pointer->integer.integer;
	dprintf("power: %" B_PRId32 ", ", n);

	// register
	pointer = &object->package.objects[0];
	if (pointer->object_type != ACPI_TYPE_BUFFER) {
		dprintf("invalid _CST elem type\n");
		return B_ERROR;
	}
	if (pointer->buffer.length < 15) {
		dprintf("invalid _CST elem length\n");
		return B_ERROR;
	}

	struct acpicpu_reg *reg = (struct acpicpu_reg *)pointer->buffer.buffer;
	switch (reg->reg_spaceid) {
		case ACPI_ADR_SPACE_SYSTEM_IO:
			dprintf("IO method\n");
			if (reg->reg_addr == 0) {
				dprintf("illegal address\n");
				return B_ERROR;
			}
			if (reg->reg_bitwidth != 8) {
				dprintf("invalid source length\n");
				return B_ERROR;
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
			if (cpu->arch.vendor == VENDOR_INTEL &&
					(reg->reg_accesssize & ACPI_PDC_GAS_BM) == 0)
				ci->skip_bm_sts = 1;
			break;
		}
		default:
			dprintf("invalid spaceid %" B_PRId8 "\n", reg->reg_spaceid);
			break;
	}

	return B_OK;
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
	if (object->package.count < 2)
		dprintf("invalid _CST count\n");

	acpi_object_type *pointer = object->package.objects;
	if (pointer[0].object_type != ACPI_TYPE_INTEGER)
		dprintf("invalid _CST type 2\n");
	uint32 n = pointer[0].integer.integer;
	if (n != object->package.count - 1)
		dprintf("invalid _CST count 2\n");
	if (n > MAX_CSTATES) {
		dprintf("_CST has too many states\n");
		n = MAX_CSTATES;
	}
	dprintf("cpuidle found %" B_PRId32 " cstates\n", n);
	uint32 count = 0;
	for (uint32 i = 1; i <= n; i++) {
		pointer = &object->package.objects[i];
		if (acpi_cstate_add(pointer, &device->states[count]) == B_OK)
			++count;
	}
	device->state_count = count;
	free(buffer.pointer);

	// TODO we assume BM is a must and ARB_DIS is always available
	device->flags |= ACPI_FLAG_C_ARB | ACPI_FLAG_C_BM;

	acpi_cstate_quirks(device);

	return B_OK;
}


static status_t
acpi_processor_init(acpi_cpuidle_driver_info *device)
{
	// get the CPU index
	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status_t status = device->acpi->evaluate_method(device->acpi_cookie, NULL,
		NULL, &buffer);
	if (status != B_OK) {
		dprintf("failed to get processor obj\n");
		return status;
	}

	acpi_object_type *tmpObject = (acpi_object_type *)buffer.pointer;
	const uint32 processor_cpu_id = tmpObject->processor.cpu_id;
	free(buffer.pointer);

	// Find this processor's CPU index.
	int32 cpuIndex = -1;
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		cpu_ent* cpu = &gCPU[i];
		if ((uint32)cpu->arch.acpi_processor_id == processor_cpu_id) {
			cpuIndex = i;
			break;
		}
	}

	if (cpuIndex < 0) {
		dprintf("can't find matching cpu_ent for acpi cpu %" B_PRId32 "\n",
			processor_cpu_id);
		return B_ERROR;
	}

	dprintf("acpi cpu %" B_PRId32 " maps to cpu %" B_PRId32 "\n",
		processor_cpu_id, cpuIndex);

	sAcpiProcessor[cpuIndex] = device;
	return status;
}


static void
acpi_cpuidle_uninit()
{
	for (size_t i = 0; i < B_COUNT_OF(sAcpiProcessor); i++) {
		if (sAcpiProcessor[i] == NULL)
			continue;

		sDeviceManager->put_node(sAcpiProcessor[i]->processor);
		free(sAcpiProcessor[i]);
		sAcpiProcessor[i] = NULL;
	}
}


static status_t
acpi_cpuidle_init()
{
	if (x86_check_feature(IA32_FEATURE_EXT_HYPERVISOR, FEATURE_EXT))
		return B_ERROR;

	device_node* root = sDeviceManager->get_root_node();

	status_t status = B_OK;
	int32 processors = 0;
	device_node* processor = NULL;
	while (true) {
		device_attr acpiAttrs[] = {
			{ B_DEVICE_BUS, B_STRING_TYPE, { .string = "acpi" }},
			{ ACPI_DEVICE_TYPE_ITEM, B_UINT32_TYPE, { .ui32 = ACPI_TYPE_PROCESSOR }},
			{ NULL }
		};

		if (sDeviceManager->find_child_node(root, acpiAttrs, &processor) != B_OK)
			break;

		acpi_cpuidle_driver_info *device;
		device = (acpi_cpuidle_driver_info *)calloc(1, sizeof(*device));
		if (device == NULL) {
			sDeviceManager->put_node(processor);
			status = B_NO_MEMORY;
			break;
		}

		device->processor = processor;
		sDeviceManager->get_driver(processor, (driver_module_info **)&device->acpi,
			(void **)&device->acpi_cookie);

		status = acpi_processor_init(device);
		if (status != B_OK) {
			sDeviceManager->put_node(processor);
			free(device);

			// Ignore the error and continue: there are sometimes processor
			// objects that don't map to cpu_ents, apparently.
			status = B_OK;
			continue;
		}

		processors++;
	}

	sDeviceManager->put_node(root);
	if (status == B_OK && processors != smp_get_num_cpus()) {
		dprintf("can't use x86 ACPI idle: missing %" B_PRId32 " processor objects\n",
			smp_get_num_cpus() - processors);
		status = B_NOT_SUPPORTED;
	}

	for (int32 i = 0; status == B_OK && i < smp_get_num_cpus(); i++)
		status = acpi_cpuidle_setup(sAcpiProcessor[i]);

	if (status == B_OK) {
		acpi_cstate_set_scheduler_mode(SCHEDULER_MODE_LOW_LATENCY);
		dprintf("using x86 ACPI idle\n");
	}

	if (status != B_OK)
		acpi_cpuidle_uninit();

	return status;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return acpi_cpuidle_init();

		case B_MODULE_UNINIT:
			acpi_cpuidle_uninit();
			return B_OK;
	}

	return B_ERROR;
}


static cpuidle_module_info sAcpiidleModule = {
	{
		ACPI_CPUIDLE_MODULE_NAME,
		0,
		std_ops,
	},

	0.2f,

	acpi_cstate_set_scheduler_mode,

	acpi_cstate_idle,
	acpi_cstate_wait
};


module_info *modules[] = {
	(module_info *)&sAcpiidleModule,
	NULL
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&sAcpi },
	{}
};
