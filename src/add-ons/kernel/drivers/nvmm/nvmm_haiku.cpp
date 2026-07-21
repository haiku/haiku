/*
 * Copyright 2024 Daniel Martin, dalmemail@gmail.com
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Drivers.h>
#include <arch/x86/arch_cpu.h>

extern "C" {
#include "nvmm.h"
#include "nvmm_internal.h"
#include "nvmm_os.h"
#include "x86/nvmm_x86.h"
}

#include <drivers/KernelExport.h>
#include <OS.h>
#include <StackOrHeapArray.h>

#include <arch/x86/arch_system_info.h>
#include <kernel/heap.h>
#include <kernel/smp.h>
#include <kernel/thread.h>

#include <sys/ioccom.h>

#include <vm/VMCache.h>
#include <vm/VMAddressSpace.h>
#include <vm/vm_page.h>
#include "vm/VMAnonymousCache.h"
#include "VMVirtualAddressSpace.h"

#include <paging/nested/X86VMTranslationMapEPT.h>
#include <paging/nested/X86VMTranslationMapRVI.h>


extern "C" void
x86_get_cpuid2(uint32_t eax, uint32_t ecx, cpuid_desc_t *descriptors)
{
	cpuid_info info;
	if (get_current_cpuid(&info, eax, ecx) != B_OK) {
		*descriptors = {};
		return;
	}

	descriptors->eax = info.regs.eax;
	descriptors->ebx = info.regs.ebx;
	descriptors->ecx = info.regs.ecx;
	descriptors->edx = info.regs.edx;
}


extern "C" void
x86_get_cpuid(uint32_t eax, cpuid_desc_t *descriptors)
{
	x86_get_cpuid2(eax, 0, descriptors);
}


extern "C" int
haiku_get_xsave_mask()
{
	if (x86_check_feature(IA32_FEATURE_EXT_XSAVE, FEATURE_EXT))
		return IA32_XCR0_X87 | IA32_XCR0_SSE;

	return 0;
}


extern "C" void
haiku_curthread_save_fpu()
{
#if KDEBUG
	uint32 sseControl;
	asm volatile("stmxcsr %0" : "=m" (sseControl));
	if ((sseControl & ~0x3F) != 0x1F80) {
		cpu_status status = disable_interrupts();
		dprintf("nvmm: curthread_save_fpu: non-default MXCSR\n");
		restore_interrupts(status);
	}
#endif
}


extern "C" void
haiku_curthread_restore_fpu()
{
	// Haiku allows FPU usage in the kernel, so saving/restoring the overall
	// FPU state isn't needed. However, on x86, MXCSR is callee-saved, so we
	// have to reset it when "restoring" state.
	uint32 sseControl = 0x1F80;
	asm volatile("ldmxcsr %0" : : "m" (sseControl));
}


extern "C" void
haiku_save_fpu(void* area, uint64_t xsave_features)
{
	switch (xsave_features) {
		case IA32_XCR0_X87:
			asm volatile("fnsave %0" : "=m" (*(char*)area));
			break;

		case IA32_XCR0_X87 | IA32_XCR0_SSE:
			asm volatile("fxsaveq %0" : "=m" (*(char*)area));
			break;

		default:
			panic("nvmm save_fpu: unimplemented xsave_features state");
	}
}


extern "C" void
haiku_restore_fpu(const void* area, uint64_t xsave_features)
{
	switch (xsave_features) {
		case IA32_XCR0_X87:
			asm volatile("frstor %0" :: "m" (*(const char*)area));
			break;

		case IA32_XCR0_X87 | IA32_XCR0_SSE:
			asm volatile("fxrstorq %0" :: "m" (*(const char*)area));
			break;

		default:
			panic("nvmm restore_fpu: unimplemented xsave_features state");
	}
}


extern "C" int32
haiku_smp_get_current_cpu()
{
	return smp_get_current_cpu();
}


extern "C" int32
haiku_smp_get_num_cpus()
{
	return smp_get_num_cpus();
}


extern "C" os_cpu_t*
haiku_get_cpu_struct(uint32 cpu_number)
{
	return &gCPU[cpu_number];
}


extern "C" int
os_cpu_number(os_cpu_t *cpu)
{
	return cpu->cpu_num;
}


extern "C" thread_id
haiku_get_current_thread_id()
{
	return thread_get_current_thread_id();
}


extern "C" void
os_ipi_unicast(os_cpu_t *cpu, void (*func)(void *, int), void *arg)
{
	OS_ASSERT(os_cpu_number(cpu) >= 0 && os_cpu_number(cpu) < haiku_smp_get_num_cpus());
	call_single_cpu_sync((uint32)os_cpu_number(cpu), func, arg);
}


extern "C" int
haiku_thread_pin()
{
	thread_pin_to_current_cpu(thread_get_current_thread());
	return 0;
}


extern "C" void
haiku_thread_unpin()
{
	thread_unpin_from_current_cpu(thread_get_current_thread());
}


extern "C" void
os_preempt_disable()
{
	thread_get_current_thread()->cpu->reschedule_disabled = true;
}


extern "C" void
os_preempt_enable()
{
	thread_get_current_thread()->cpu->reschedule_disabled = false;
	if (os_return_needed())
		scheduler_reschedule_if_necessary();
}


extern "C" bool
os_preempt_disabled()
{
	return thread_get_current_thread()->cpu->reschedule_disabled;
}


extern "C" bool
os_return_needed()
{
	return thread_get_current_thread()->cpu->invoke_scheduler;
}


extern "C" status_t
os_mtx_lock(os_mtx_t *lock)
{
	if (os_preempt_disabled()) {
		// This happens under the VMX backend during ASID allocation, at least.
		int32 spins = 0;
		while (mutex_trylock(lock) != B_OK) {
			cpu_pause();
			spins++;
			if (spins > 1000000)
				panic("os_mtx_lock: failed to acquire mutex for a long time");
		}
		return B_OK;
	}

	return mutex_lock(lock);
}


extern "C" void
x86_curthread_restore_dbregs(uint64_t *drs)
{
	// TODO: If necessary, restore kernel debug registers from arch_team_debug_info
}


// this should be called with preemption disabled
// otherwise we might return a GDT not of the
// current CPU..
extern "C" void*
os_curcpu_gdt()
{
	struct gdtr {
		uint16 limit;
		uint64 base;
	} _PACKED;
	struct gdtr gdtr;
	__asm __volatile("sgdt %0" : "=m" (gdtr));

	return (void*)gdtr.base;
}


extern "C" uint64
os_curcpu_idt()
{
	struct idtr {
		uint16 limit;
		uint64 base;
	} _PACKED;
	struct idtr idtr;
	__asm __volatile("sidt %0" : "=m" (idtr));

	return idtr.base;
}


extern "C" void*
os_curcpu_tss()
{
	return &gCPU[os_curcpu_number()].arch.tss;
}


extern "C" uint16
os_curcpu_tss_sel()
{
	uint16 selector;
	__asm __volatile("str %0" : "=m" (selector));

	return selector;
}


// #pragma mark - os_* APIs


// aka os_vmobj_t
struct haiku_vmobj {
	VMCache *cache;
	int32	ref_count;
};


// aka os_vmspace_t
struct haiku_vmspace {
	VMAddressSpace *address_space;
	struct pmap pmap;
};


// aka os_cpuset_t
struct haiku_cpuset {
	CPUSet set;
};


static os_vmmap_t sDummyKernelMap;
static os_vmmap_t sDummyCurrentProcessMap;
os_vmmap_t *os_kernel_map = &sDummyKernelMap;
os_vmmap_t *os_curproc_map = &sDummyCurrentProcessMap;


extern "C" void *
os_pagemem_zalloc(size_t size)
{
	void *ptr;
	size_t alloc_size = roundup(size, PAGE_SIZE);
	area_id area = create_area("os_pagemem_zalloc_area", &ptr, B_ANY_KERNEL_ADDRESS,
		alloc_size, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	if (area < 0)
		return NULL;

	memset(ptr, 0, alloc_size);

	return ptr;
}


extern "C" void
os_pagemem_free(void *ptr, size_t size __unused)
{
	delete_area(area_for(ptr));
}


extern "C" int
os_contigpa_zalloc(paddr_t *pa, vaddr_t *va, size_t npages)
{
	area_id area = create_area("os_contigpa_zalloc_area", (void **)va, B_ANY_KERNEL_ADDRESS,
		npages * PAGE_SIZE, B_CONTIGUOUS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	if (area < 0)
		return area;

	memset((void *)*va, 0, npages * PAGE_SIZE);

	physical_entry entry;
	status_t status = get_memory_map((void *)*va, 1, &entry, 1);
	if (status < B_OK) {
		delete_area(area);
		return status;
	}
	*pa = entry.address;

	return 0;
}


extern "C" void
os_contigpa_free(paddr_t pa __unused, vaddr_t va, size_t npages __unused)
{
	delete_area(area_for((void *)va));
}


extern "C" os_vmspace_t *
os_vmspace_create(vaddr_t vmin, vaddr_t vmax)
{
	if (vmax < vmin)
		return NULL;

	os_vmspace_t *ret = (os_vmspace_t *)os_mem_zalloc(sizeof(os_vmspace_t));
	if (ret == NULL)
		return NULL;

	X86VMTranslationMap *translationMap = NULL;
	status_t status = B_ERROR;
	typedef void (**callback_type)(void*);
	if (nvmm_impl == &nvmm_x86_vmx) {
		X86VMTranslationMapEPT *map = new(std::nothrow) X86VMTranslationMapEPT();
		map->SetFlushCallback((callback_type)&ret->pmap.pm_tlb_flush, &ret->pmap);
		status = map->Init();
		translationMap = map;
	} else if (nvmm_impl == &nvmm_x86_svm) {
		X86VMTranslationMapRVI *map = new(std::nothrow) X86VMTranslationMapRVI();
		map->SetFlushCallback((callback_type)&ret->pmap.pm_tlb_flush, &ret->pmap);
		status = map->Init();
		translationMap = map;
	}

	if (status != B_OK) {
		delete translationMap;
		os_mem_free(ret, sizeof(os_vmspace_t));
		return NULL;
	}

	size_t size = vmax - vmin + 1;
	ret->address_space = new(std::nothrow) VMVirtualAddressSpace(translationMap, vmin, size);
	if (ret->address_space == NULL) {
		delete translationMap;
		os_mem_free(ret, sizeof(os_vmspace_t));
		return NULL;
	}

	return ret;
}


extern "C" void
os_vmspace_destroy(os_vmspace_t *vm)
{
	if (nvmm_impl == &nvmm_x86_vmx) {
		X86VMTranslationMapEPT *map = (X86VMTranslationMapEPT *)vm->address_space->TranslationMap();
		map->Lock();
		map->SetFlushCallback(NULL, NULL);
		map->Unlock();
	} else if (nvmm_impl == &nvmm_x86_svm) {
		X86VMTranslationMapRVI *map = (X86VMTranslationMapRVI *)vm->address_space->TranslationMap();
		map->Lock();
		map->SetFlushCallback(NULL, NULL);
		map->Unlock();
	}

	vm->address_space->Put();
	os_mem_free(vm, sizeof(os_vmspace_t));
}


extern "C" int
os_vmspace_fault(os_vmspace_t *vm, vaddr_t va, vm_prot_t prot)
{
	// if va isn't present on any area vm_soft_fault() will return
	// error and print to syslog, which can heavily affect performance
	vm->address_space->ReadLock();
	if (!vm->address_space->LookupArea(va)) {
		vm->address_space->ReadUnlock();
		return 1;
	}
	vm->address_space->ReadUnlock();

	status_t status = vm_soft_fault(vm->address_space, va,
		(prot & PROT_WRITE) != 0, (prot & PROT_EXEC) != 0, true, NULL);

	return status != B_OK;
}


extern "C" struct pmap*
os_vmspace_pmap(os_vmspace_t *vm)
{
	return &vm->pmap;
}


extern "C" paddr_t
os_vmspace_pdirpa(os_vmspace_t *vm)
{
	X86VMTranslationMap *map
		= (X86VMTranslationMap*)vm->address_space->TranslationMap();

	return map->PagingStructures()->pgdir_phys;
}


extern "C" os_vmmap_t *
os_vmspace_get_vmmap(os_vmspace_t *vm)
{
	return vm;
}


extern "C" os_vmobj_t *
os_vmobj_create(voff_t size)
{
	os_vmobj_t *ret = (os_vmobj_t *)os_mem_alloc(sizeof(os_vmobj_t));
	if (ret == NULL)
		return NULL;

	int32 numPages = size / PAGE_SIZE;
	if (size % PAGE_SIZE != 0)
		numPages++;

	// TODO: We can't enable swapping unless we know A/D bits are supported.
	status_t status = VMCacheFactory::CreateAnonymousCache(ret->cache, false,
		numPages, 0, false, 0);
	if (status != B_OK) {
		os_mem_free(ret, sizeof(os_vmobj_t));
		return NULL;
	}
	ret->ref_count = 0;
	ret->cache->temporary = 1;
	ret->cache->virtual_base = 0;
	ret->cache->virtual_end = size;

	return ret;
}


extern "C" void
os_vmobj_ref(os_vmobj_t *vmobj)
{
	atomic_add(&vmobj->ref_count, 1);
}


extern "C" void
os_vmobj_rel(os_vmobj_t *vmobj)
{
	int32 previous = atomic_add(&vmobj->ref_count, -1);
	if (previous != 0)
		return;

	while (true) {
		vmobj->cache->Lock();
		VMArea *area = vmobj->cache->areas.First();
		if (area == NULL) {
			vmobj->cache->Unlock();
			break;
		}
		vaddr_t start = area->Base();
		VMAddressSpace *address_space = area->address_space;
		address_space->Get();
		vmobj->cache->Unlock();
		
		address_space->WriteLock();
		if (address_space->LookupArea(start) != area
				|| area->cache != vmobj->cache) {
			// Something changed; restart.
			address_space->WriteUnlock();
			address_space->Put();
			continue;
		}

		vm_unmap_address_range(address_space, start, area->Size(), true);

		address_space->WriteUnlock();
		address_space->Put();
	}

	vmobj->cache->ReleaseRef();
	os_mem_free(vmobj, sizeof(os_vmobj_t));
}


static VMAddressSpace*
address_space_for(os_vmmap_t *map)
{
	if (map == &sDummyCurrentProcessMap)
		return thread_get_current_thread()->team->address_space;
	else if (map == &sDummyKernelMap)
		return VMAddressSpace::GetKernel();

	ASSERT(map->address_space != NULL);
	return map->address_space;
}


//! shared indicates whether the mapping is inherit on fork calls or not
extern "C" int
os_vmobj_map(os_vmmap_t *map, vaddr_t *addr, vsize_t size, os_vmobj_t *vmobj,
	voff_t offset, bool wired, bool fixed, bool shared __unused, int prot, int maxprot)
{
	if (!vmobj->cache->Lock())
		return B_ERROR;

	VMAddressSpace* addressSpace = address_space_for(map);
	status_t status = addressSpace->WriteLock();
	if (status != B_OK)
		return status;

	uint32 wiring = (wired || dynamic_cast<VMAnonymousCache*>(vmobj->cache) == NULL) ?
		B_FULL_LOCK : B_NO_LOCK;
	uint32 flags = fixed ? CREATE_AREA_UNMAP_ADDRESS_RANGE : 0;
	bool kernel = false;
	if (addressSpace == VMAddressSpace::Kernel())
		kernel = true;

	virtual_address_restrictions addressRestrictions = {
		.address = (void *)*addr,
		.address_specification = fixed ? B_EXACT_ADDRESS : B_ANY_ADDRESS,
		.alignment = B_PAGE_SIZE,
	};
	VMArea *area;
	status = vm_map_cache(addressSpace, vmobj->cache,
		vmobj->cache->virtual_base + offset, "nvmm_vmobj_area",
		size, wiring, prot, maxprot, REGION_NO_PRIVATE_MAP, flags,
		&addressRestrictions, kernel, &area, (void **)addr);

	// RefCount() must always be number of areas + 1
	if (status == B_OK)
		vmobj->cache->AcquireRefLocked();

	addressSpace->WriteUnlock();
	vmobj->cache->Unlock();

	return status;
}


//! the range [start, end-1] will be unmapped
extern "C" void
os_vmobj_unmap(os_vmmap_t *map, vaddr_t start, vaddr_t end,
	bool wired __unused)
{
	VMAddressSpace* addressSpace = address_space_for(map);
	addressSpace->WriteLock();
	vm_unmap_address_range(addressSpace, start, end - start - 1, true);
	addressSpace->WriteUnlock();
}


extern "C" status_t
os_cpuset_init(os_cpuset_t **cpuset)
{
	*cpuset = new os_cpuset_t;
	if (*cpuset == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


extern "C" void
os_cpuset_destroy(os_cpuset_t *cpuset)
{
	free(cpuset);
}


extern "C" bool
os_cpuset_isset(os_cpuset_t *cpuset, int32 cpu)
{
	return cpuset->set.GetBitAtomic(cpu);
}


extern "C" void
os_cpuset_clear(os_cpuset_t *cpuset, int32 cpu)
{
	cpuset->set.ClearBitAtomic(cpu);
}


extern "C" void
os_cpuset_setrunning(os_cpuset_t *cpuset)
{
	cpuset->set.SetAll();
}


// #pragma mark - driver API


int32 api_version = B_CUR_DRIVER_API_VERSION;
static const char *sDevices[] = { "misc/nvmm", NULL };


static status_t
nvmm_open_hook(const char *name, uint32 flags, void **cookie)
{
	if (!(flags & O_CLOEXEC))
		return B_BAD_VALUE;

	struct nvmm_owner *owner;
	if ((flags & O_ACCMODE) == O_WRONLY)
		owner = &nvmm_root_owner;
	else {
		owner = (struct nvmm_owner *)os_mem_alloc(sizeof(*owner));
		if (owner == NULL)
			return B_NO_MEMORY;

		owner->pid = getpid();
	}

	*cookie = owner;
	return B_OK;
}


static status_t
nvmm_close_hook(void *cookie)
{
	if (cookie == NULL)
		return B_NO_INIT;

	struct nvmm_owner *owner = (struct nvmm_owner *)cookie;
	nvmm_kill_machines(owner);
	return B_OK;
}


static status_t
nvmm_free_hook(void* cookie)
{
	if (cookie == NULL)
		return B_NO_INIT;

	if (cookie != &nvmm_root_owner)
		os_mem_free(cookie, sizeof(struct nvmm_owner));

	return B_OK;
}


static status_t
nvmm_control_hook(void *cookie, uint32 op, void *data, size_t len)
{
	len = IOCPARM_LEN(op);
	BStackOrHeapArray<char, 128> kernel_data(len);

	status_t status = user_memcpy(kernel_data, data, len);
	if (status < 0)
		return status;

	status_t ioctl_status = nvmm_ioctl((struct nvmm_owner *)cookie, op, kernel_data);

	status = user_memcpy(data, kernel_data, len);
	if (status < 0)
		return status;

	return ioctl_status;
}


static device_hooks sHooks = {
	.open = nvmm_open_hook,
	.close = nvmm_close_hook,
	.free = nvmm_free_hook,
	.control = nvmm_control_hook,
};


status_t
init_hardware(void)
{
	if (nvmm_ident() == NULL) {
		TRACE_ALWAYS("nvmm: CPU not supported\n");
		return B_ERROR;
	}

	return B_OK;
}


const char**
publish_devices(void)
{
	return sDevices;
}


device_hooks*
find_device(const char* name)
{
	return &sHooks;
}


status_t
init_driver(void)
{
	if (nvmm_init())
		return B_ERROR;

	TRACE_ALWAYS("nvmm: init_driver OK\n");
	return B_OK;
}


void
uninit_driver(void)
{
	TRACE_ALWAYS("nvmm: uninit_driver\n");
	nvmm_fini();
}
