/*
 * Copyright 2006-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <apm.h>
#include <descriptors.h>
#include <generic_syscall.h>
#include <safemode.h>
#include <boot/kernel_args.h>


#define TRACE_APM
#ifdef TRACE_APM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define APM_ERROR_DISABLED				0x01
#define APM_ERROR_DISCONNECTED			0x03
#define APM_ERROR_UNKNOWN_DEVICE		0x09
#define APM_ERROR_OUT_OF_RANGE			0x0a
#define APM_ERROR_DISENGAGED			0x0b
#define APM_ERROR_NOT_SUPPORTED			0x0c
#define APM_ERROR_RESUME_TIMER_DISABLED	0x0d
#define APM_ERROR_UNABLE_TO_ENTER_STATE	0x60
#define APM_ERROR_NO_EVENTS_PENDING		0x80
#define APM_ERROR_APM_NOT_PRESENT		0x86

#define CARRY_FLAG	0x01

extern segment_descriptor *gGDT;
extern void *gDmaAddress;
extern addr_t gBiosBase;

static bool sAPMEnabled = false;
static struct {
	uint32	offset;
	uint16	segment;
} sAPMBiosEntry;


struct bios_regs {
	bios_regs() : eax(0), ebx(0), ecx(0), edx(0), esi(0), flags(0) {}
	uint32	eax;
	uint32	ebx;
	uint32	ecx;
	uint32	edx;
	uint32	esi;
	uint32	flags;
};


#ifdef TRACE_APM
static const char *
apm_error(uint32 error)
{
	switch (error >> 8) {
		case APM_ERROR_DISABLED:
			return "Power Management disabled";
		case APM_ERROR_DISCONNECTED:
			return "Interface disconnected";
		case APM_ERROR_UNKNOWN_DEVICE:
			return "Unrecognized device ID";
		case APM_ERROR_OUT_OF_RANGE:
			return "Parameter value out of range";
		case APM_ERROR_DISENGAGED:
			return "Interface not engaged";
		case APM_ERROR_NOT_SUPPORTED:
			return "Function not supported";
		case APM_ERROR_RESUME_TIMER_DISABLED:
			return "Resume timer disabled";
		case APM_ERROR_UNABLE_TO_ENTER_STATE:
			return "Unable to enter requested state";
		case APM_ERROR_NO_EVENTS_PENDING:
			return "No power management events pending";
		case APM_ERROR_APM_NOT_PRESENT:
			return "APM not present";

		default:
			return "Unknown error";
	}
}
#endif	// TRACE_APM


static status_t
call_apm_bios(bios_regs *regs)
{
#if __GNUC__ < 4
	// TODO: Fix this for GCC 4.3! The direct reference to sAPMBiosEntry
	// in the asm below causes undefined references.
	asm volatile(
		"pushfl; "
		"pushl %%ebp; "
		"lcall *%%cs:sAPMBiosEntry; "
		"popl %%ebp; "
		"pushfl; "
		"popl %%edi; "
		"movl %%edi, %5; "
		"popfl; "
		: "=a" (regs->eax), "=b" (regs->ebx), "=c" (regs->ecx), "=d" (regs->edx),
		  "=S" (regs->esi), "=m" (regs->flags)
		: "a" (regs->eax), "b" (regs->ebx), "c" (regs->ecx)
		: "memory", "edi", "cc");

	if (regs->flags & CARRY_FLAG)
		return B_ERROR;

	return B_OK;
#else
	return B_ERROR;
#endif
}


static status_t
apm_get_event(uint16 &event, uint16 &info)
{
	bios_regs regs;
	regs.eax = BIOS_APM_GET_EVENT;

	if (call_apm_bios(&regs) != B_OK)
		return B_ERROR;

	event = regs.ebx & 0xffff;
	info = regs.ecx & 0xffff;
	return B_OK;
}


static status_t
apm_set_state(uint16 device, uint16 state)
{
	bios_regs regs;
	regs.eax = BIOS_APM_SET_STATE;
	regs.ebx = device;
	regs.ecx = state;

	status_t status = call_apm_bios(&regs);
	if (status == B_OK)
		return B_OK;

	TRACE(("apm_set_state() error: %s\n", apm_error(regs.eax)));
	return status;
}


static status_t
apm_enable_power_management(uint16 device, bool enable)
{
	bios_regs regs;
	regs.eax = BIOS_APM_ENABLE;
	regs.ebx = device;
	regs.ecx = enable ? 0x01 : 0x00;

	return call_apm_bios(&regs);
}


static status_t
apm_engage_power_management(uint16 device, bool engage)
{
	bios_regs regs;
	regs.eax = BIOS_APM_ENGAGE;
	regs.ebx = device;
	regs.ecx = engage ? 0x01 : 0x00;

	return call_apm_bios(&regs);
}


status_t
apm_driver_version(uint16 version)
{
	dprintf("version: %x\n", version);
	bios_regs regs;
	regs.eax = BIOS_APM_VERSION;
	regs.ecx = version;

	if (call_apm_bios(&regs) != B_OK)
		return B_ERROR;

	dprintf("eax: %lx, flags: %lx\n", regs.eax, regs.flags);

	return B_OK;
}


static void
apm_daemon(void *arg, int iteration)
{
	uint16 event;
	uint16 info;
	if (apm_get_event(event, info) != B_OK)
		return;

	dprintf("APM event: %x, info: %x\n", event, info);
}


static status_t
get_apm_battery_info(apm_battery_info *info)
{
	bios_regs regs;
	regs.eax = BIOS_APM_GET_POWER_STATUS;
	regs.ebx = APM_ALL_DEVICES;
	regs.ecx = 0;

	status_t status = call_apm_bios(&regs);
	if (status != B_OK)
		return status;

	uint16 lineStatus = (regs.ebx >> 8) & 0xff;
	if (lineStatus == 0xff)
		return B_NOT_SUPPORTED;

	info->online = lineStatus != 0 && lineStatus != 2;
	info->percent = regs.ecx & 0xff;
	if (info->percent > 100 || info->percent < 0)
		info->percent = -1;

	info->time_left = info->percent >= 0 ? (int32)(regs.edx & 0xffff) : -1;
	if (info->time_left & 0x8000)
		info->time_left = (info->time_left & 0x7fff) * 60;

	return B_OK;
}


static status_t
apm_control(const char *subsystem, uint32 function,
	void *buffer, size_t bufferSize)
{
	struct apm_battery_info info;
	if (bufferSize != sizeof(struct apm_battery_info))
		return B_BAD_VALUE;

	switch (function) {
		case APM_GET_BATTERY_INFO:
			status_t status = get_apm_battery_info(&info);
			if (status < B_OK)
				return status;

			return user_memcpy(buffer, &info, sizeof(struct apm_battery_info));
	}

	return B_BAD_VALUE;
}


//	#pragma mark -


status_t
apm_shutdown(void)
{
	if (!sAPMEnabled)
		return B_NOT_SUPPORTED;

	cpu_status state = disable_interrupts();

	status_t status = apm_set_state(APM_ALL_DEVICES, APM_POWER_STATE_OFF);

	restore_interrupts(state);
	return status;
}


status_t
apm_init(kernel_args *args)
{
	apm_info &info = args->platform_args.apm;

	TRACE(("apm_init()\n"));

	if ((info.version & 0xf) < 2) {
		// no APM or connect failed
		return B_ERROR;
	}

	TRACE(("  code32: 0x%x, 0x%lx, length 0x%x\n",
		info.code32_segment_base, info.code32_segment_offset, info.code32_segment_length));
	TRACE(("  code16: 0x%x, length 0x%x\n",
		info.code16_segment_base, info.code16_segment_length));
	TRACE(("  data: 0x%x, length 0x%x\n",
		info.data_segment_base, info.data_segment_length));

	// get APM setting - safemode settings override kernel settings

	bool apm = false;

	void *handle = load_driver_settings("kernel");
	if (handle != NULL) {
		apm = get_driver_boolean_parameter(handle, "apm", false, false);
		unload_driver_settings(handle);
	}

	handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (handle != NULL) {
		apm = !get_driver_boolean_parameter(handle, B_SAFEMODE_DISABLE_APM, !apm, !apm);
		unload_driver_settings(handle);
	}

	if (!apm)
		return B_OK;

	// Apparently, some broken BIOS try to access segment 0x40 for the BIOS
	// data section - we make sure it can by setting up the GDT accordingly
	// (the first 640kB are mapped as DMA area in arch_vm_init()).
	addr_t biosData = (addr_t)gDmaAddress + 0x400;

	set_segment_descriptor(&gGDT[BIOS_DATA_SEGMENT >> 3],
		biosData, B_PAGE_SIZE - biosData,
		DT_DATA_WRITEABLE, DPL_KERNEL);

	// TODO: test if APM segments really are in the BIOS ROM area (especially the
	//	data segment)

	// Setup APM GDTs

	// We ignore their length, and just set their segments to 64 kB which
	// shouldn't cause any headaches

	set_segment_descriptor(&gGDT[APM_CODE32_SEGMENT >> 3],
		gBiosBase + (info.code32_segment_base << 4) - 0xe0000, 0xffff,
		DT_CODE_READABLE, DPL_KERNEL);
	set_segment_descriptor(&gGDT[APM_CODE16_SEGMENT >> 3],
		gBiosBase + (info.code16_segment_base << 4) - 0xe0000, 0xffff,
		DT_CODE_READABLE, DPL_KERNEL);
	gGDT[APM_CODE16_SEGMENT >> 3].d_b = 0;
		// 16-bit segment

	if ((info.data_segment_base << 4) < 0xe0000) {
		// use the BIOS data segment as data segment for APM

		if (info.data_segment_length == 0)
			info.data_segment_length = B_PAGE_SIZE - info.data_segment_base;

		set_segment_descriptor(&gGDT[APM_DATA_SEGMENT >> 3],
			(addr_t)gDmaAddress + (info.data_segment_base << 4), info.data_segment_length,
			DT_DATA_WRITEABLE, DPL_KERNEL);
	} else {
		// use the BIOS area as data segment
		set_segment_descriptor(&gGDT[APM_DATA_SEGMENT >> 3],
			gBiosBase + (info.data_segment_base << 4) - 0xe0000, 0xffff,
			DT_DATA_WRITEABLE, DPL_KERNEL);
	}

	// setup APM entry point

	sAPMBiosEntry.segment = APM_CODE32_SEGMENT;
	sAPMBiosEntry.offset = info.code32_segment_offset;

	apm_driver_version(info.version);

	if (apm_enable_power_management(APM_ALL_DEVICES, true) != B_OK)
		dprintf("APM: cannot enable power management.\n");
	if (apm_engage_power_management(APM_ALL_DEVICES, true) != B_OK)
		dprintf("APM: cannot engage.\n");

	register_kernel_daemon(apm_daemon, NULL, 10);
		// run the daemon once every second

	register_generic_syscall(APM_SYSCALLS, apm_control, 1, 0);
	sAPMEnabled = true;
	return B_OK;
}

