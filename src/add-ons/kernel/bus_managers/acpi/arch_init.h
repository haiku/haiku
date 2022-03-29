/*
 * Copyright 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ARCH_INIT_H
#define ARCH_INIT_H


/** Find the ACPI root pointer using arch or platform-specific code.
 *
 * For example on x86, search for it in the BIOS.
 */
ACPI_PHYSICAL_ADDRESS arch_init_find_root_pointer();


/** Initialize platform specific interrupt controller configuration.
 *
 * For example on x86, enable PIC or APIC depending on boot options.
 */
void arch_init_interrupt_controller();


#endif /* !ARCH_INIT_H */
