/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _STAGE2_PRIV_H
#define _STAGE2_PRIV_H

#include <boot/stage2.h>
#include <ktypes.h>

extern void clearscreen(void);
extern void kputs(const char *str);
extern int dprintf(const char *fmt, ...);
extern void sleep(uint64 time);
extern void execute_n_instructions(int count);
void system_time_setup(long a);
uint64 rdtsc();
unsigned int get_eflags(void);
void set_eflags(uint32 val);
void cpuid(uint32 selector, uint32 *data);

//void put_uint_dec(unsigned int a);
//void put_uint_hex(unsigned int a);
//void nmessage(const char *str1, unsigned int a, const char *str2);
//void nmessage2(const char *str1, unsigned int a, const char *str2, unsigned int b, const char *str3);

#define ROUNDUP(a, b) (((a) + ((b) - 1)) & (~((b) - 1)))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))

#define PAGE_SIZE 0x1000
#define KERNEL_BASE 0x80000000
#define KERNEL_ENTRY 0x80000080
#define STACK_SIZE 2
#define DEFAULT_PAGE_FLAGS (1 | 2) // present/rw
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 24
#define ADDR_MASK 0xfffff000

struct gdt_idt_descr {
	uint16 a;
	uint32 *b;
} _PACKED;

// SMP stuff
extern int smp_boot(kernel_args *ka, uint32 kernel_entry);
extern void smp_trampoline(void);
extern void smp_trampoline_end(void);

#endif	/* _STAGE2_PRIV_H */
