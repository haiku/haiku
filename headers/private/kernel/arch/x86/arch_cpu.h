/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_CPU_H
#define _KERNEL_ARCH_x86_CPU_H

/* ??? why include this as we're normally included from that file! */
#include <arch/cpu.h>

#define KERNEL_CODE_SEG 0x8
#define KERNEL_DATA_SEG 0x10
#define USER_CODE_SEG 0x1b
#define USER_DATA_SEG 0x23
#define TSS 0x28
#define PAGE_SIZE 4096

#define _BIG_ENDIAN 0
#define _LITTLE_ENDIAN 1

typedef struct desc_struct {
	unsigned int a,b;
} desc_table;

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

struct tss_descriptor {
	uint16 limit_00_15;
	uint16 base_00_15;
	uint32 base_23_16 : 8;
	uint32 type : 4;
	uint32 zero : 1;
	uint32 dpl : 2;
	uint32 present : 1;
	uint32 limit_19_16 : 4;
	uint32 avail : 1;
	uint32 zero1 : 1;
	uint32 zero2 : 1;
	uint32 granularity : 1;
	uint32 base_31_24 : 8;
};

typedef struct ptentry {
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

typedef struct pdentry {
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

#define nop() __asm__ ("nop"::)

void setup_system_time(unsigned int cv_factor);
void i386_context_switch(unsigned int **old_esp, unsigned int *new_esp, addr new_pgdir);
void i386_enter_uspace(addr entry, void *args, addr ustack_top);
void i386_set_kstack(addr kstack);
void i386_switch_stack_and_call(addr stack, void (*func)(void *), void *arg);
void i386_swap_pgdir(addr new_pgdir);
void i386_fsave_swap(void *old_fpu_state, void *new_fpu_state);
void i386_fxsave_swap(void *old_fpu_state, void *new_fpu_state);

#define read_dr3(value) \
	__asm__("movl	%%dr3,%0" : "=r" (value))

#define write_dr3(value) \
	__asm__("movl	%0,%%dr3" :: "r" (value))

#define invalidate_TLB(va) \
	__asm__("invlpg (%0)" : : "r" (va))

#define out8(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))

#define out16(value,port) \
__asm__ ("outw %%ax,%%dx"::"a" (value),"d" (port))

#define out32(value,port) \
__asm__ ("outl %%eax,%%dx"::"a" (value),"d" (port))

#define in8(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \
_v; \
})

#define in16(port) ({ \
unsigned short _v; \
__asm__ volatile ("inw %%dx,%%ax":"=a" (_v):"d" (port)); \
_v; \
})

#define in32(port) ({ \
unsigned int _v; \
__asm__ volatile ("inl %%dx,%%eax":"=a" (_v):"d" (port)); \
_v; \
})

#define out8_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

#define in8_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})

#endif

