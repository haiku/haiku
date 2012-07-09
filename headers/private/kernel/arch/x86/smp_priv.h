/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_SMP_PRIV_H
#define _KERNEL_ARCH_x86_SMP_PRIV_H

#include <SupportDefs.h>

void arch_smp_ack_interrupt(void);
status_t arch_smp_set_apic_timer(bigtime_t relativeTimeout);
status_t arch_smp_clear_apic_timer(void);

#endif	/* _KERNEL_ARCH_x86_SMP_PRIV_H */
