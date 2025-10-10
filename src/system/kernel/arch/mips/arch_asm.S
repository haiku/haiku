/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#define	FUNC(name) .align 2 ; .globl name ; name:

.text
FUNC(reboot)

FUNC(atomic_add)

FUNC(atomic_and)

FUNC(atomic_or)

/* XXX finish */
FUNC(test_and_set)

FUNC(disable_exceptions)

FUNC(enable_exceptions)

FUNC(arch_int_restore_interrupts)
	
FUNC(arch_int_enable_interrupts)

FUNC(arch_int_disable_interrupts)

