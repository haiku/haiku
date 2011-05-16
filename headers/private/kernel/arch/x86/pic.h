/*
 * Copyright 2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_PIC_H
#define _KERNEL_ARCH_x86_PIC_H

#include <SupportDefs.h>

void pic_init();
void pic_disable(uint16& enabledInterrupts);

#endif // _KERNEL_ARCH_x86_PIC_H
