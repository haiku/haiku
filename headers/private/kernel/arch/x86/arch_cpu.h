/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_CPU_H
#define _KERNEL_ARCH_x86_CPU_H

#include <ktypes.h>
#include <arch/x86/descriptors.h>

#define PAGE_SIZE 4096

struct tss {
	uint16 prev_task;
	uint16 unused0;
	uint32 sp0;
	uint32 ss0;
	uint32 sp1;
	uint32 ss1;
	uint32 sp2;
	uint32 ss2;
	uint32 sp3;
	uint32 ss3;
	uint32 cr3;
	uint32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32 es, cs, ss, ds, fs, gs;
	uint32 ldt_seg_selector;
	uint16 unused1;
	uint16 io_map_base;
};

/**************************************************************************/

typedef struct ptentry {		// page table entry
	unsigned int present:1;
	unsigned int rw:1;
	unsigned int user:1;
	unsigned int write_through:1;
	unsigned int cache_disabled:1;
	unsigned int accessed:1;
	unsigned int dirty:1;
	unsigned int reserved:1;
	unsigned int global:1;
	unsigned int avail:3;
	unsigned int addr:20;
} ptentry;

typedef struct pdentry {		// page directory entry
	unsigned int present:1;
	unsigned int rw:1;
	unsigned int user:1;
	unsigned int write_through:1;
	unsigned int cache_disabled:1;
	unsigned int accessed:1;
	unsigned int reserved:1;
	unsigned int page_size:1;
	unsigned int global:1;
	unsigned int avail:3;
	unsigned int addr:20;
} pdentry;

struct iframe {
	unsigned int gs;
	unsigned int fs;
	unsigned int es;
	unsigned int ds;
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int esp;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned int orig_eax;
	unsigned int orig_edx;
	unsigned int vector;
	unsigned int error_code;
	unsigned int eip;
	unsigned int cs;
	unsigned int flags;
	unsigned int user_esp;
	unsigned int user_ss;
};

#define nop() __asm__ ("nop"::)

struct arch_thread;

void setup_system_time(unsigned int cv_factor);
void i386_context_switch(struct arch_thread *old_state, struct arch_thread *new_state, addr new_pgdir);
void i386_enter_uspace(addr entry, void *args1, void *args2, addr ustack_top);
void i386_set_tss_and_kstack(addr kstack);
void i386_switch_stack_and_call(addr stack, void (*func)(void *), void *arg);
void i386_swap_pgdir(addr new_pgdir);
void i386_fsave(void *fpu_state);
void i386_fxsave(void *fpu_state);
void i386_frstor(void *fpu_state);
void i386_fxrstor(void *fpu_state);
void i386_fsave_swap(void *old_fpu_state, void *new_fpu_state);
void i386_fxsave_swap(void *old_fpu_state, void *new_fpu_state);

#define read_ebp(value) \
	__asm__("movl	%%ebp,%0" : "=r" (value))

#define read_dr3(value) \
	__asm__("movl	%%dr3,%0" : "=r" (value))

#define write_dr3(value) \
	__asm__("movl	%0,%%dr3" : : "r" (value))

#define invalidate_TLB(va) \
	__asm__("invlpg (%0)" : : "r" (va))

#define out8(value,port) \
__asm__ ("outb %%al,%%dx" : : "a" (value), "d" (port))

#define out16(value,port) \
__asm__ ("outw %%ax,%%dx" : : "a" (value), "d" (port))

#define out32(value,port) \
__asm__ ("outl %%eax,%%dx" : : "a" (value), "d" (port))

#define in8(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al" : "=a" (_v) : "d" (port)); \
_v; \
})

#define in16(port) ({ \
unsigned short _v; \
__asm__ volatile ("inw %%dx,%%ax":"=a" (_v) : "d" (port)); \
_v; \
})

#define in32(port) ({ \
unsigned int _v; \
__asm__ volatile ("inl %%dx,%%eax":"=a" (_v) : "d" (port)); \
_v; \
})

#define out8_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:" : : "a" (value), "d" (port))

#define in8_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:" : "=a" (_v) : "d" (port)); \
_v; \
})

extern segment_descriptor *gGDT;

#endif	/* _KERNEL_ARCH_x86_CPU_H */
