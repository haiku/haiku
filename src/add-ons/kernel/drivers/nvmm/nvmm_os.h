/*
 * Copyright (c) 2021 Maxime Villard, m00nbsd.net
 * Copyright (c) 2021 The DragonFly Project.
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Aaron LI <aly@aaronly.me>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NVMM_OS_H_
#define _NVMM_OS_H_

#ifndef _KERNEL
#error "This file should not be included by userland programs."
#endif

#if defined(__NetBSD__)
#include <sys/cpu.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>
#elif defined(__DragonFly__)
#include <sys/lock.h>
#include <sys/malloc.h> /* contigmalloc, contigfree */
#include <sys/proc.h> /* LWP_MP_URETMASK */
#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pager.h>
#include <vm/vm_param.h> /* KERN_SUCCESS, etc. */
#include <vm/pmap.h> /* pmap_ept_transform, pmap_npt_transform */
#include <machine/cpu.h> /* hvm_break_wanted */
#include <machine/cpufunc.h> /* ffsl, ffs, etc. */
#elif defined(__HAIKU__)
#include <sys/cdefs.h>
#include <Drivers.h>
#include <interrupts.h>
#include <machine/specialreg.h>
#include <kernel/lock.h>
#include <SupportDefs.h>
#include <stdlib.h>
#include <string.h>
#endif

/* Types. */
#if defined(__NetBSD__)
typedef struct vm_map		os_vmmap_t;
typedef struct vmspace		os_vmspace_t;
typedef struct uvm_object	os_vmobj_t;
typedef krwlock_t		os_rwl_t;
typedef kmutex_t		os_mtx_t;
#elif defined(__DragonFly__)
typedef struct vm_map		os_vmmap_t;
typedef struct vmspace		os_vmspace_t;
typedef struct vm_object	os_vmobj_t;
typedef struct lock		os_rwl_t;
typedef struct lock		os_mtx_t;
/* A few standard types. */
typedef vm_offset_t		vaddr_t;
typedef vm_offset_t		voff_t;
typedef vm_size_t		vsize_t;
typedef vm_paddr_t		paddr_t;
#elif defined(__HAIKU__)
typedef struct haiku_vmspace os_vmspace_t;
typedef struct haiku_vmspace os_vmmap_t;
typedef struct haiku_vmobj	os_vmobj_t;
typedef phys_addr_t		paddr_t;
typedef addr_t			vaddr_t;
typedef off_t			voff_t;
typedef size_t			vsize_t;
typedef uint32			vm_prot_t;
typedef rw_lock			os_rwl_t;
typedef mutex			os_mtx_t;
#endif

/* Attributes. */
#if defined(__DragonFly__)
#define __cacheline_aligned	__cachealign
#define __diagused		__debugvar
#elif defined(__HAIKU__)
#define __cacheline_aligned
#define __read_mostly
#endif

/* Macros. */
#if defined(__DragonFly__)
#define __arraycount(__x)	(sizeof(__x) / sizeof(__x[0]))
#define __insn_barrier()	__asm __volatile("":::"memory")
#elif defined (__HAIKU__)
#define __arraycount		B_COUNT_OF
// roundup() taken from headers/private/firewire/fwglue.h
#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))  /* to any y */
#define __diagused
#define __insn_barrier()	__asm __volatile("":::"memory")
#define PAGE_SIZE		B_PAGE_SIZE
#endif

/* Bitops. */
#if defined(__NetBSD__)
#include <sys/bitops.h>
#elif defined(__DragonFly__)
#include <sys/bitops.h>
#ifdef __x86_64__
#undef  __BIT
#define __BIT(__n)		__BIT64(__n)
#undef  __BITS
#define __BITS(__m, __n)	__BITS64(__m, __n)
#endif /* __x86_64__ */
#elif defined(__HAIKU__)
#include "nvmm_bitops.h"
#endif

/* Maps. */
#if defined(__NetBSD__) || defined(__DragonFly__)
#define os_kernel_map		kernel_map
#define os_curproc_map		&curproc->p_vmspace->vm_map
#elif defined(__HAIKU__)
extern os_vmmap_t *os_kernel_map;
os_vmmap_t* os_get_curproc_map();
void os_free_curproc_map(os_vmmap_t *map);
#endif

/* R/W locks. */
#if defined(__NetBSD__)
#define os_rwl_init(lock)	rw_init(lock)
#define os_rwl_destroy(lock)	rw_destroy(lock)
#define os_rwl_rlock(lock)	rw_enter(lock, RW_READER);
#define os_rwl_wlock(lock)	rw_enter(lock, RW_WRITER);
#define os_rwl_unlock(lock)	rw_exit(lock)
#define os_rwl_wheld(lock)	rw_write_held(lock)
#elif defined(__DragonFly__)
#define os_rwl_init(lock)	lockinit(lock, "nvmmrw", 0, 0)
#define os_rwl_destroy(lock)	lockuninit(lock)
#define os_rwl_rlock(lock)	lockmgr(lock, LK_SHARED);
#define os_rwl_wlock(lock)	lockmgr(lock, LK_EXCLUSIVE);
#define os_rwl_unlock(lock)	lockmgr(lock, LK_RELEASE)
#define os_rwl_wheld(lock)	(lockstatus(lock, curthread) == LK_EXCLUSIVE)
#elif defined(__HAIKU__)
#define os_rwl_init(lock)	rw_lock_init(lock, NULL)
#define os_rwl_destroy(lock)	rw_lock_destroy(lock)
#define os_rwl_rlock(lock)	rw_lock_read_lock(lock)
#define os_rwl_wlock(lock)	rw_lock_write_lock(lock)
#define os_rwl_wheld(lock)	((lock)->holder == haiku_get_current_thread_id())
#define os_rwl_unlock(lock)     (os_rwl_wheld(lock) \
				? rw_lock_write_unlock(lock) : rw_lock_read_unlock(lock))
#endif

/* Mutexes. */
#if defined(__NetBSD__)
#define os_mtx_init(lock)	mutex_init(lock, MUTEX_DEFAULT, IPL_NONE)
#define os_mtx_destroy(lock)	mutex_destroy(lock)
#define os_mtx_lock(lock)	mutex_enter(lock)
#define os_mtx_unlock(lock)	mutex_exit(lock)
#define os_mtx_owned(lock)	mutex_owned(lock)
#elif defined(__DragonFly__)
#define os_mtx_init(lock)	lockinit(lock, "nvmmmtx", 0, 0)
#define os_mtx_destroy(lock)	lockuninit(lock)
#define os_mtx_lock(lock)	lockmgr(lock, LK_EXCLUSIVE)
#define os_mtx_unlock(lock)	lockmgr(lock, LK_RELEASE)
#define os_mtx_owned(lock)	(lockstatus(lock, curthread) == LK_EXCLUSIVE)
#elif defined(__HAIKU__)
#define os_mtx_init(lock)	mutex_init(lock, NULL)
#define os_mtx_destroy(lock)	mutex_destroy(lock)
status_t os_mtx_lock(os_mtx_t *lock);
#define os_mtx_unlock(lock)	mutex_unlock(lock)
#endif

/* Malloc. */
#if defined(__NetBSD__)
#include <sys/kmem.h>
#define os_mem_alloc(size)	kmem_alloc(size, KM_SLEEP)
#define os_mem_zalloc(size)	kmem_zalloc(size, KM_SLEEP)
#define os_mem_free(ptr, size)	kmem_free(ptr, size)
#elif defined(__DragonFly__)
#include <sys/malloc.h>
MALLOC_DECLARE(M_NVMM);
#define os_mem_alloc(size)	kmalloc(size, M_NVMM, M_WAITOK)
#define os_mem_zalloc(size)	kmalloc(size, M_NVMM, M_WAITOK | M_ZERO)
#define os_mem_free(ptr, size)	kfree(ptr, M_NVMM)
#elif defined(__HAIKU__)
#define os_mem_alloc(size)	malloc(size)
#define os_mem_zalloc(size)	calloc(1, size)
#define os_mem_free(ptr, size)  free(ptr)
#endif

/* Printf. */
#if defined(__NetBSD__)
#define os_printf		printf
#elif defined(__DragonFly__)
#define os_printf		kprintf
#elif defined(__HAIKU__)
#define TRACE_ALWAYS(a...)	dprintf(a)
#define os_printf		TRACE_ALWAYS
#endif

/* Atomics. */
#if defined(__NetBSD__)
#include <sys/atomic.h>
#define os_atomic_inc_uint(x)	atomic_inc_uint(x)
#define os_atomic_dec_uint(x)	atomic_dec_uint(x)
#define os_atomic_load_uint(x)	atomic_load_relaxed(x)
#define os_atomic_inc_64(x)	atomic_inc_64(x)
#elif defined(__DragonFly__)
#include <machine/atomic.h>
#define os_atomic_inc_uint(x)	atomic_add_int(x, 1)
#define os_atomic_dec_uint(x)	atomic_subtract_int(x, 1)
#define os_atomic_load_uint(x)	atomic_load_acq_int(x)
#define os_atomic_inc_64(x)	atomic_add_64(x, 1)
#elif defined(__HAIKU__)
#define os_atomic_inc_uint(x)	atomic_add((int32*)x, 1)
#define os_atomic_dec_uint(x)	atomic_add((int32*)x, -1)
#define os_atomic_load_uint(x)	atomic_get((int32*)x)
#define os_atomic_inc_64(x)		atomic_add64((int64*)x, 1)
#endif

/* Pmap. */
#if defined(__NetBSD__)
extern bool pmap_ept_has_ad;
#define os_vmspace_pmap(vm)	((vm)->vm_map.pmap)
#define os_vmspace_pdirpa(vm)	((vm)->vm_map.pmap->pm_pdirpa[0])
#define os_pmap_mach(pm)	((pm)->pm_data)
#elif defined(__DragonFly__)
#define os_vmspace_pmap(vm)	vmspace_pmap(vm)
#define os_vmspace_pdirpa(vm)	(vtophys(vmspace_pmap(vm)->pm_pml4))
#elif defined(__HAIKU__)
struct pmap {
	void (*pm_tlb_flush)(struct pmap *);
	void *pm_data;
};
#define os_pmap_mach(pm)	((pm)->pm_data)
struct pmap *os_vmspace_pmap(os_vmspace_t *vm);
paddr_t os_vmspace_pdirpa(os_vmspace_t *vm);
os_vmmap_t *os_vmspace_get_vmmap(os_vmspace_t *vm);
#endif

/* CPU. */
#if defined(__NetBSD__)
#include <sys/cpu.h>
typedef struct cpu_info		os_cpu_t;
#define OS_MAXCPUS		MAXCPUS
#define OS_CPU_FOREACH(cpu)	for (CPU_INFO_FOREACH(, cpu))
#define os_cpu_number(cpu)	cpu_index(cpu)
#define os_curcpu()		curcpu()
#define os_curcpu_number()	cpu_number()
#define os_curcpu_tss_sel()	curcpu()->ci_tss_sel
#define os_curcpu_tss()		curcpu()->ci_tss
#define os_curcpu_gdt()		curcpu()->ci_gdt
#define os_curcpu_idt()		curcpu()->ci_idtvec.iv_idt
#elif defined(__DragonFly__)
#include <sys/globaldata.h>
#include <machine/segments.h>
typedef struct globaldata	os_cpu_t;
#define OS_MAXCPUS		SMP_MAXCPU
#define OS_CPU_FOREACH(cpu)	\
	for (int idx = 0; idx < ncpus && (cpu = globaldata_find(idx)); idx++)
#define os_cpu_number(cpu)	(cpu)->gd_cpuid
#define os_curcpu()		mycpu
#define os_curcpu_number()	mycpuid
#define os_curcpu_tss_sel()	GSEL(GPROC0_SEL, SEL_KPL)
#define os_curcpu_tss()		&mycpu->gd_prvspace->common_tss
#define os_curcpu_gdt()		mdcpu->gd_gdt
#define os_curcpu_idt()		r_idt_arr[mycpuid].rd_base
#elif defined(__HAIKU__)
/*
 * On NetBSD/DragonFlyBSD each CPU has a unique structure. NVMM
 * uses the unique pointers to those structures as a way to check
 * whether two CPUs are the same or not. In our case we'll use
 * the cpu_ent structure that is declared on cpu.h which can't be
 * included from C. We declare it here as an incomplete type.
 */
struct cpu_ent;

typedef struct cpu_ent			os_cpu_t;
os_cpu_t* haiku_get_cpu_struct(uint32 cpu_number);
#define OS_CPU_FOREACH(cpu)	\
	uint32 curcpu = 0; \
	uint32 _ncpus = haiku_smp_get_num_cpus(); \
	for (cpu = haiku_get_cpu_struct(0); curcpu < _ncpus; \
		cpu = haiku_get_cpu_struct(++curcpu))
int os_cpu_number(os_cpu_t *cpu);
#define os_curcpu()		haiku_get_cpu_struct(haiku_smp_get_current_cpu())
#define os_curcpu_number()	haiku_smp_get_current_cpu()
uint16 os_curcpu_tss_sel();
void *os_curcpu_tss();
void *os_curcpu_gdt();
uint64 os_curcpu_idt();
#endif

/* Cpusets. */
#if defined(__NetBSD__)
#include <sys/kcpuset.h>
typedef kcpuset_t		os_cpuset_t;
#define os_cpuset_init(s)	kcpuset_create(s, true)
#define os_cpuset_destroy(s)	kcpuset_destroy(s)
#define os_cpuset_isset(s, c)	kcpuset_isset(s, c)
#define os_cpuset_clear(s, c)	kcpuset_clear(s, c)
#define os_cpuset_setrunning(s)	kcpuset_copy(s, kcpuset_running)
#elif defined(__DragonFly__)
#include <sys/cpumask.h>
#include <machine/smp.h> /* smp_active_mask */
typedef cpumask_t		os_cpuset_t;
#define os_cpuset_init(s)	\
	({ *(s) = kmalloc(sizeof(cpumask_t), M_NVMM, M_WAITOK | M_ZERO); })
#define os_cpuset_destroy(s)	kfree((s), M_NVMM)
#define os_cpuset_isset(s, c)	CPUMASK_TESTBIT(*(s), c)
#define os_cpuset_clear(s, c)	ATOMIC_CPUMASK_NANDBIT(*(s), c)
#define os_cpuset_setrunning(s)	ATOMIC_CPUMASK_ORMASK(*(s), smp_active_mask)
#elif defined(__HAIKU__)
typedef struct haiku_cpuset	os_cpuset_t;
status_t os_cpuset_init(os_cpuset_t **cpuset);
void os_cpuset_destroy(os_cpuset_t *cpuset);
bool os_cpuset_isset(os_cpuset_t *cpuset, int32 cpu);
void os_cpuset_clear(os_cpuset_t *cpuset, int32 cpu);
void os_cpuset_setrunning(os_cpuset_t *cpuset);
#endif

/* Preemption. */
#if defined(__NetBSD__)
#define os_preempt_disable()	kpreempt_disable()
#define os_preempt_enable()	kpreempt_enable()
#define os_preempt_disabled()	kpreempt_disabled()
#elif defined(__DragonFly__)
/*
 * In DragonFly, a thread in kernel mode will not be preemptively migrated
 * to another CPU or preemptively switched to another normal kernel thread,
 * but can be preemptively switched to an interrupt thread (which switches
 * back to the kernel thread it preempted the instant it is done or blocks).
 *
 * However, we still need to use a critical section to prevent this nominal
 * interrupt thread preemption to avoid exposing interrupt threads to
 * guest DB and FP register state.  We operate under the assumption that
 * the hard interrupt code won't mess with this state.
 */
#define os_preempt_disable()	crit_enter()
#define os_preempt_enable()	crit_exit()
#define os_preempt_disabled()	(curthread->td_critcount != 0)
#elif defined(__HAIKU__)
void os_preempt_disable();
void os_preempt_enable();
bool os_preempt_disabled();
#endif

/* Asserts. */
#if defined(__NetBSD__)
#define OS_ASSERT		KASSERT
#elif defined(__DragonFly__)
#define OS_ASSERT		KKASSERT
#elif defined(__HAIKU__)
#define OS_ASSERT		ASSERT
#endif

/* Misc. */
#if defined(__DragonFly__) || defined(__HAIKU__)
#define uimin(a, b)		((u_int)a < (u_int)b ? (u_int)a : (u_int)b)
#endif

/* Memory protection */
#if defined(__HAIKU__)
#define PROT_READ	B_READ_AREA
#define PROT_WRITE	B_WRITE_AREA
#define PROT_EXEC	B_EXECUTE_AREA
#define copyin(from, to, size)	user_memcpy(to, from, size)
#define copyout(from, to, size)	user_memcpy(to, from, size)
#endif

/* -------------------------------------------------------------------------- */

os_vmspace_t *	os_vmspace_create(vaddr_t, vaddr_t);
void		os_vmspace_destroy(os_vmspace_t *);
int		os_vmspace_fault(os_vmspace_t *, vaddr_t, vm_prot_t);

os_vmobj_t *	os_vmobj_create(voff_t);
void		os_vmobj_ref(os_vmobj_t *);
void		os_vmobj_rel(os_vmobj_t *);

int		os_vmobj_map(os_vmmap_t *, vaddr_t *, vsize_t, os_vmobj_t *,
		    voff_t, bool, bool, bool, int, int);
void		os_vmobj_unmap(os_vmmap_t *map, vaddr_t, vaddr_t, bool);

void *		os_pagemem_zalloc(size_t);
void		os_pagemem_free(void *, size_t);

#if defined(__DragonFly__) || defined(__NetBSD__)
paddr_t		os_pa_zalloc(void);
void		os_pa_free(paddr_t);
#endif

int		os_contigpa_zalloc(paddr_t *, vaddr_t *, size_t);
void		os_contigpa_free(paddr_t, vaddr_t, size_t);

#ifndef __HAIKU__
static inline bool
os_return_needed(void)
{
#if defined(__NetBSD__)
	if (preempt_needed()) {
		return true;
	}
	if (curlwp->l_flag & LW_USERRET) {
		return true;
	}
	return false;
#elif defined(__DragonFly__)
	if (__predict_false(hvm_break_wanted())) {
		return true;
	}
	if (__predict_false(curthread->td_lwp->lwp_mpflags & LWP_MP_URETMASK)) {
		return true;
	}
	return false;
#endif
}
#endif

// Haiku auxiliary functions
#if defined(__HAIKU__)
bool os_return_needed();

int haiku_get_xsave_mask();

int32 haiku_smp_get_current_cpu();
int32 haiku_smp_get_num_cpus();

thread_id haiku_get_current_thread_id();

static __inline int
fls(int mask)
{
	return (mask == 0 ? 0 :
		8 * sizeof(mask) - __builtin_clz((u_int)mask));
}
int flsll(long long mask);
// based on ilog2() from DragonFlyBSD
// available at /sys/dev/drm/include/linux/log2.h
#define ilog2(n) (sizeof(n) <= 4) ?			\
	fls((uint32)(n)) - 1 : flsll((uint64)(n)) - 1
#endif

/* -------------------------------------------------------------------------- */

/* IPIs. */

#if defined(__NetBSD__)

#include <sys/xcall.h>
#define OS_IPI_FUNC(func)	void func(void *arg, void *unused)

static inline void
os_ipi_unicast(os_cpu_t *cpu, void (*func)(void *, void *), void *arg)
{
	xc_wait(xc_unicast(XC_HIGHPRI, func, arg, NULL, cpu));
}

static inline void
os_ipi_broadcast(void (*func)(void *, void *), void *arg)
{
	xc_wait(xc_broadcast(0, func, arg, NULL));
}

static inline void
os_ipi_kickall(void)
{
	/*
	 * XXX: this is probably too expensive. NetBSD should have a dummy
	 * interrupt handler that just IRETs without doing anything.
	 */
	pmap_tlb_shootdown(pmap_kernel(), -1, PTE_G, TLBSHOOT_NVMM);
}

#elif defined(__DragonFly__)

#include <sys/thread2.h>
#define OS_IPI_FUNC(func)	void func(void *arg)

static inline void
os_ipi_unicast(os_cpu_t *cpu, void (*func)(void *), void *arg)
{
	int seq;

	seq = lwkt_send_ipiq(cpu, func, arg);
	lwkt_wait_ipiq(cpu, seq);
}

static inline void
os_ipi_broadcast(void (*func)(void *), void *arg)
{
	cpumask_t mask;
	int i;

	for (i = 0; i < ncpus; i++) {
		CPUMASK_ASSBIT(mask, i);
		lwkt_cpusync_simple(mask, func, arg);
	}
}

/*
 * On DragonFly, no need to bind the thread, because any normal kernel
 * thread will not migrate to another CPU or be preempted (except by an
 * interrupt thread).
 */
#define curlwp_bind()		((int)0)
#define curlwp_bindx(bound)	/* nothing */

#elif defined(__HAIKU__)

#include <drivers/KernelExport.h>
#define OS_IPI_FUNC(func)	void func(void *arg, int unused)

void os_ipi_unicast(os_cpu_t *cpu, void (*func)(void *, int), void *arg);

static inline void
os_ipi_broadcast(void (*func)(void *, int), void *arg)
{
	call_all_cpus_sync(func, arg);
}

static void
haiku_dummy_ipi(void*, int)
{
}

static inline void
os_ipi_kickall(void)
{
	os_ipi_broadcast(haiku_dummy_ipi, NULL);
}

int haiku_thread_pin();
void haiku_thread_unpin();

#define curlwp_bind()		haiku_thread_pin()
#define curlwp_bindx(bound)	haiku_thread_unpin()

#endif /* __NetBSD__ */

#endif /* _NVMM_OS_H_ */
