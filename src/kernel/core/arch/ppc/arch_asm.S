/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define	FUNC(name) .align 4 ; .globl ##name ; ##name:

.text
.globl reboot
reboot:

.globl atomic_add
atomic_add:

.globl atomic_and
atomic_and:

.globl atomic_or
atomic_or:

.globl atomic_set
atomic_set:

.globl test_and_set
test_and_set:

.globl arch_int_restore_interrupts
arch_int_restore_interrupts:

.globl arch_int_enable_interrupts
arch_int_enable_interrupts:

.globl arch_int_disable_interrupts
arch_int_disable_interrupts:

.global dbg_save_registers
dbg_save_registers:

