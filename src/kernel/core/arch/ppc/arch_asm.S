/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define	FUNC(name) .align 4 ; .globl ##name ; ##name:

.text
.globl reboot
reboot:

.globl arch_int_restore_interrupts
arch_int_restore_interrupts:

.globl arch_int_enable_interrupts
arch_int_enable_interrupts:

.globl arch_int_disable_interrupts
arch_int_disable_interrupts:

.global dbg_save_registers
dbg_save_registers:

