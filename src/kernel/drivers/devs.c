/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <devs.h>

#ifdef ARCH_x86
#include <arch/x86/console_dev.h>
#include <arch/x86/keyboard.h>
#include <arch/x86/ps2mouse.h>
#endif
#ifdef ARCH_sh4
#include <kernel/dev/arch/sh4/maple/maple_bus.h>
#include <kernel/dev/arch/i386/keyboard/keyboard.h>
#include <kernel/dev/arch/sh4/console/console_dev.h>
#include <kernel/dev/arch/sh4/rtl8139/rtl8139_dev.h>
#endif
#include <fb_console.h>

int devs_init(kernel_args *ka)
{

#ifdef ARCH_x86
	keyboard_dev_init(ka);
	console_dev_init(ka);
#endif

#ifdef ARCH_sh4
	maple_bus_init(ka);
	keyboard_dev_init(ka);
	rtl8139_dev_init(ka);
#endif
	fb_console_dev_init(ka);

	return 0;
}
