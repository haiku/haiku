/*
 * Copyright 2026 John Davis
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <arch_cpu.h>

#include "hyperv_cpu.h"
#include "VMBusPrivate.h"


status_t
vmbus_detect_hyperv()
{
	CALLED();

	// Check for hypervisor.
	cpu_ent* cpu = get_cpu_struct();
	if ((cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_HYPERVISOR) == 0) {
		TRACE("No hypervisor detected\n");
		return B_ERROR;
	}

	// Check for Hyper-V CPUID leaves.
	cpuid_info cpuInfo;
	get_cpuid(&cpuInfo, IA32_CPUID_LEAF_HYPERVISOR, 0);
	if (cpuInfo.regs.eax < IA32_CPUID_LEAF_HV_IMP_LIMITS) {
		TRACE("Not running on Hyper-V\n");
		return B_ERROR;
	}

	// Check for Hyper-V signature.
	get_cpuid(&cpuInfo, IA32_CPUID_LEAF_HV_INT_ID, 0);
	if (cpuInfo.regs.eax != HV_CPUID_INTERFACE_ID) {
		TRACE("Not running on Hyper-V\n");
		return B_ERROR;
	}

	// Check for required Hyper-V features
	// Some hypervisors claim to be Hyper-V, but may not actually implement all features
	get_cpuid(&cpuInfo, IA32_CPUID_LEAF_HV_FEAT_ID, 0);
	TRACE("Hyper-V features: %08" B_PRIX32 ":%08" B_PRIX32 ":%08" B_PRIX32 ":%08" B_PRIX32 "\n",
		cpuInfo.regs.eax, cpuInfo.regs.ebx, cpuInfo.regs.ecx, cpuInfo.regs.edx);
	if ((cpuInfo.regs.eax & HV_CPUID_EAX_REQUIRED_FEATURES) == 0
			|| (cpuInfo.regs.ebx & HV_CPUID_EBX_REQUIRED_FEATURES) == 0) {
		TRACE("Missing required Hyper-V features\n");
		return B_ERROR;
	}

#ifdef TRACE_HYPERV
	get_cpuid(&cpuInfo, IA32_CPUID_LEAF_HV_SYS_ID, 0);
	TRACE("Hyper-V version: %d.%d.%d [SP%d]\n", cpuInfo.regs.ebx >> 16, cpuInfo.regs.ebx & 0xFFFF,
		cpuInfo.regs.eax, cpuInfo.regs.ecx);
#endif

	return B_OK;
}


status_t
VMBus::_EnableHypercalls()
{
	// Guest ID must be set prior to enabling hypercalls
	x86_write_msr(IA32_MSR_HV_GUEST_OS_ID, IA32_MSR_HV_GUEST_OS_ID_FREEBSD);

	uint64 msr = x86_read_msr(IA32_MSR_HV_HYPERCALL);
	msr = ((fHyperCallPhys >> HV_PAGE_SHIFT) << IA32_MSR_HV_HYPERCALL_PAGE_SHIFT)
		| (msr & IA32_MSR_HV_HYPERCALL_RSVD_MASK) | IA32_MSR_HV_HYPERCALL_ENABLE;
	x86_write_msr(IA32_MSR_HV_HYPERCALL, msr);

	// Check that hypercalls are enabled
	msr = x86_read_msr(IA32_MSR_HV_HYPERCALL);
	if ((msr & IA32_MSR_HV_HYPERCALL_ENABLE) == 0)
		return B_ERROR;

	TRACE("Hypercalls enabled at %p\n", fHypercallPage);
	return B_OK;
}


void
VMBus::_DisableHypercalls()
{
	uint64 msr = x86_read_msr(IA32_MSR_HV_HYPERCALL);
	msr &= IA32_MSR_HV_HYPERCALL_RSVD_MASK;
	x86_write_msr(IA32_MSR_HV_HYPERCALL, msr);

	TRACE("Hypercalls disabled\n");
}


uint16
VMBus::_HypercallPostMessage(phys_addr_t physAddr)
{
	uint64 status;

#if defined(__i386__)
	__asm __volatile("call *%5"
		: "=A" (status)
		: "d" (0), "a" (HYPERCALL_POST_MESSAGE), "b" (0), "c" (static_cast<uint32>(physAddr)),
		"m" (fHypercallPage));
#elif defined(__x86_64__)
	__asm __volatile("call *%3"
		: "=a" (status)
		: "c" (HYPERCALL_POST_MESSAGE), "d" (physAddr), "m" (fHypercallPage));
#endif

	return (uint16)(status & 0xFFFF);
}


uint16
VMBus::_HypercallSignalEvent(uint32 connId)
{
	uint64 status;

#if defined(__i386__)
	__asm __volatile("call *%5"
		: "=A" (status)
		: "d" (0), "a" (HYPERCALL_SIGNAL_EVENT), "b" (0), "c" (connId), "m" (fHypercallPage));
#elif defined(__x86_64__)
	__asm __volatile("call *%3"
		: "=a" (status)
		: "c" (HYPERCALL_SIGNAL_EVENT), "d" (connId), "m" (fHypercallPage));
#endif

	return (uint16)(status & 0xFFFF);
}


void
VMBus::_EnableInterruptCPU(int32 cpu)
{
	phys_addr_t messagesPhys = fCPUMessagesPhys + (sizeof(*fCPUMessages) * cpu);
	phys_addr_t eventFlagsPhys = fCPUEventFlagsPhys + (sizeof(*fCPUEventFlags) * cpu);

	// Configure SIMP and SIEFP
	uint64 msr = x86_read_msr(IA32_MSR_HV_SIMP);
	msr = ((messagesPhys >> HV_PAGE_SHIFT) << IA32_MSR_HV_SIMP_PAGE_SHIFT)
		| (msr & IA32_MSR_HV_SIMP_RSVD_MASK) | IA32_MSR_HV_SIMP_ENABLE;
	x86_write_msr(IA32_MSR_HV_SIMP, msr);
	TRACE("cpu%u: simp new msr 0x%llX\n", cpu, (unsigned long long)msr);

	msr = x86_read_msr(IA32_MSR_HV_SIEFP);
	msr = ((eventFlagsPhys >> HV_PAGE_SHIFT) << IA32_MSR_HV_SIEFP_PAGE_SHIFT)
		| (msr & IA32_MSR_HV_SIEFP_RSVD_MASK) | IA32_MSR_HV_SIEFP_ENABLE;
	x86_write_msr(IA32_MSR_HV_SIEFP, msr);
	TRACE("cpu%u: siefp new msr 0x%llX\n", cpu, (unsigned long long)msr);

	// Configure interrupt vector for incoming VMBus messages
	msr = x86_read_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_MESSAGE);
	msr = fInterruptVector | (msr & IA32_MSR_HV_SINT_RSVD_MASK);
	x86_write_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_MESSAGE, msr);
	TRACE("cpu%u: sint%u new msr 0x%llX\n", VMBUS_SINT_MESSAGE, cpu, (unsigned long long)msr);

	// Configure interrupt vector for VMBus timers
	msr = x86_read_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_TIMER);
	msr = fInterruptVector | (msr & IA32_MSR_HV_SINT_RSVD_MASK);
	x86_write_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_TIMER, msr);
	TRACE("cpu%u: sint%u new msr 0x%llX\n", VMBUS_SINT_TIMER, cpu, (unsigned long long)msr);

	// Enable interrupts
	msr = x86_read_msr(IA32_MSR_HV_SCONTROL);
	msr = (msr & IA32_MSR_HV_SCONTROL_RSVD_MASK) | IA32_MSR_HV_SCONTROL_ENABLE;
	x86_write_msr(IA32_MSR_HV_SCONTROL, msr);
	TRACE("cpu%u: scontrol new msr 0x%llX\n", cpu, (unsigned long long)msr);
}


void
VMBus::_DisableInterruptCPU(int32 cpu)
{
	// Disable interrupts
	uint64 msr = x86_read_msr(IA32_MSR_HV_SCONTROL);
	msr &= IA32_MSR_HV_SCONTROL_RSVD_MASK;
	x86_write_msr(IA32_MSR_HV_SCONTROL, msr);
	TRACE("cpu%u: scontrol new msr 0x%llX\n", cpu, (unsigned long long)msr);

	// Disable SIMP and SIEFP
	msr = x86_read_msr(IA32_MSR_HV_SIMP);
	msr &= IA32_MSR_HV_SIMP_RSVD_MASK;
	x86_write_msr(IA32_MSR_HV_SIMP, msr);
	TRACE("cpu%u: simp new msr 0x%llX\n", cpu, (unsigned long long)msr);

	msr = x86_read_msr(IA32_MSR_HV_SIEFP);
	msr &= IA32_MSR_HV_SIEFP_RSVD_MASK;
	x86_write_msr(IA32_MSR_HV_SIEFP, msr);
	TRACE("cpu%u: siefp new msr 0x%llX\n", cpu, (unsigned long long)msr);

	// Mask interrupt vector for incoming VMBus messages
	msr = x86_read_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_MESSAGE);
	msr = IA32_MSR_HV_SINT_MASKED | (msr & IA32_MSR_HV_SINT_RSVD_MASK);
	x86_write_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_MESSAGE, msr);
	TRACE("cpu%u: sint%u new msr 0x%llX\n", VMBUS_SINT_MESSAGE, cpu, (unsigned long long)msr);

	// Mask interrupt vector for VMBus timers
	msr = x86_read_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_TIMER);
	msr = IA32_MSR_HV_SINT_MASKED | (msr & IA32_MSR_HV_SINT_RSVD_MASK);
	x86_write_msr(IA32_MSR_HV_SINT0 + VMBUS_SINT_TIMER, msr);
	TRACE("cpu%u: sint%u new msr 0x%llX\n", VMBUS_SINT_TIMER, cpu, (unsigned long long)msr);
}


/*static*/ void
VMBus::_SignalEom(void*, int)
{
	x86_write_msr(IA32_MSR_HV_EOM, 0);
}
