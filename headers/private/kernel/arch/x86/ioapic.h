/*
 * Copyright 2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_IOAPIC_H
#define _KERNEL_ARCH_x86_IOAPIC_H

struct kernel_args;

void ioapic_map(kernel_args* args);
void ioapic_init(kernel_args* args);

#endif // _KERNEL_ARCH_x86_IOAPIC_H
