/*
 * Copyright 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ARCH_INIT_H
#define ARCH_INIT_H


/** Initialize platform specific interrupt controller configuration.
 *
 * For example on x86, enable PIC or APIC depending on boot options.
 */
void arch_init_interrupt_controller();


#endif /* !ARCH_INIT_H */
