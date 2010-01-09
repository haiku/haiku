/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_SH4_CPU_H
#define _NEWOS_KERNEL_ARCH_SH4_CPU_H

#include <arch/cpu.h>
#include <arch/sh4/sh4.h>

unsigned int get_fpscr();
unsigned int get_sr();
void sh4_context_switch(unsigned int **old_sp, unsigned int *new_sp);
void sh4_function_caller();
void sh4_set_kstack(addr kstack);
void sh4_enter_uspace(addr entry, void *args, addr ustack_top);
void sh4_set_user_pgdir(addr pgdir);
void sh4_invl_page(addr va);
#define PAGE_SIZE 4096

#define _BIG_ENDIAN 0
#define _LITTLE_ENDIAN 1
#endif

