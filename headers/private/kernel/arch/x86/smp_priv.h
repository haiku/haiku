/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_SMP_PRIV_H
#define _KERNEL_ARCH_x86_SMP_PRIV_H

int i386_smp_interrupt(int vector);
void arch_smp_ack_interrupt(void);
int arch_smp_set_apic_timer(bigtime_t relative_timeout);
int arch_smp_clear_apic_timer(void);
int smp_intercpu_int_handler(void);

#endif	/* _KERNEL_ARCH_x86_SMP_PRIV_H */
