/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_CPU_H
#define _KERNEL_ARCH_x86_CPU_H


#include "../common_x86/cpu.h"

#ifndef _ASSEMBLER
# ifndef _BOOT_MODE
#  include <arch/x86/descriptors.h>
# endif
#endif


// iframe types
#define IFRAME_TYPE_SYSCALL				0x1
#define IFRAME_TYPE_OTHER				0x2
#define IFRAME_TYPE_MASK				0xf


#ifndef _ASSEMBLER

struct X86PagingStructures;

struct tss {
	uint16 prev_task;
	uint16 unused0;
	uint32 sp0;
	uint32 ss0;
	uint32 sp1;
	uint32 ss1;
	uint32 sp2;
	uint32 ss2;
	uint32 cr3;
	uint32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32 es, cs, ss, ds, fs, gs;
	uint32 ldt_seg_selector;
	uint16 unused1;
	uint16 io_map_base;
};

struct iframe {
	uint32 type;	// iframe type
	uint32 gs;
	uint32 fs;
	uint32 es;
	uint32 ds;
	uint32 edi;
	uint32 esi;
	uint32 ebp;
	uint32 esp;
	uint32 ebx;
	uint32 edx;
	uint32 ecx;
	uint32 eax;
	uint32 orig_eax;
	uint32 orig_edx;
	uint32 vector;
	uint32 error_code;
	uint32 eip;
	uint32 cs;
	uint32 flags;

	// user_esp and user_ss are only present when the iframe is a userland
	// iframe (IFRAME_IS_USER()). A kernel iframe is shorter.
	uint32 user_esp;
	uint32 user_ss;
};

struct vm86_iframe {
	uint32 type;	// iframe type
	uint32 __null_gs;
	uint32 __null_fs;
	uint32 __null_es;
	uint32 __null_ds;
	uint32 edi;
	uint32 esi;
	uint32 ebp;
	uint32 __kern_esp;
	uint32 ebx;
	uint32 edx;
	uint32 ecx;
	uint32 eax;
	uint32 orig_eax;
	uint32 orig_edx;
	uint32 vector;
	uint32 error_code;
	uint32 eip;
	uint16 cs, __csh;
	uint32 flags;
	uint32 esp;
	uint16 ss, __ssh;

	/* vm86 mode specific part */
	uint16 es, __esh;
	uint16 ds, __dsh;
	uint16 fs, __fsh;
	uint16 gs, __gsh;
};

#define IFRAME_IS_USER(f) ((f)->cs == USER_CODE_SEG \
							|| ((f)->flags & 0x20000) != 0)
#define IFRAME_IS_VM86(f) (((f)->flags & 0x20000) != 0)

typedef struct arch_cpu_info {
	// saved cpu info
	enum x86_vendors	vendor;
	uint32				feature[FEATURE_NUM];
	char				model_name[49];
	const char*			vendor_name;
	int					type;
	int					family;
	int					extended_family;
	int					stepping;
	int					model;
	int					extended_model;

	struct X86PagingStructures* active_paging_structures;

	uint32				dr6;	// temporary storage for debug registers (cf.
	uint32				dr7;	// x86_exit_user_debug_at_kernel_entry())

	// local TSS for this cpu
	struct tss			tss;
	struct tss			double_fault_tss;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif


struct arch_thread;

void __x86_setup_system_time(uint32 conversionFactor,
	uint32 conversionFactorNsecs, bool conversionFactorNsecsShift);
void x86_context_switch(struct arch_thread* oldState,
	struct arch_thread* newState);
void x86_userspace_thread_exit(void);
void x86_end_userspace_thread_exit(void);
void x86_swap_pgdir(uint32 newPageDir);
void i386_set_tss_and_kstack(addr_t kstack);
void i386_fnsave(void* fpuState);
void i386_fxsave(void* fpuState);
void i386_frstor(const void* fpuState);
void i386_fxrstor(const void* fpuState);
void i386_noop_swap(void* oldFpuState, const void* newFpuState);
void i386_fnsave_swap(void* oldFpuState, const void* newFpuState);
void i386_fxsave_swap(void* oldFpuState, const void* newFpuState);
uint32 x86_read_ebp();
uint32 x86_read_cr0();
void x86_write_cr0(uint32 value);
uint32 x86_read_cr4();
void x86_write_cr4(uint32 value);
uint64 x86_read_msr(uint32 registerNumber);
void x86_write_msr(uint32 registerNumber, uint64 value);
void x86_set_task_gate(int32 cpu, int32 n, int32 segment);
void* x86_get_idt(int32 cpu);
uint32 x86_count_mtrrs(void);
void x86_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type);
status_t x86_get_mtrr(uint32 index, uint64* _base, uint64* _length,
	uint8* _type);
void x86_set_mtrrs(uint8 defaultType, const x86_mtrr_info* infos,
	uint32 count);
void x86_init_fpu();
bool x86_check_feature(uint32 feature, enum x86_feature_type type);
void* x86_get_double_fault_stack(int32 cpu, size_t* _size);
int32 x86_double_fault_get_cpu(void);
void x86_double_fault_exception(struct iframe* frame);
void x86_page_fault_exception_double_fault(struct iframe* frame);


#ifndef _BOOT_MODE
extern segment_descriptor* gGDT;
#endif


#ifdef __cplusplus
}	// extern "C" {
#endif

#endif	// !_ASSEMBLER

#endif	/* _KERNEL_ARCH_x86_CPU_H */
