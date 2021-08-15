/*
 * Copyright 2012-2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _KERNEL_ARCH_DEBUG_UART_8250_OMAP_H
#define _KERNEL_ARCH_DEBUG_UART_8250_OMAP_H


#include <sys/types.h>

#include <SupportDefs.h>
#include <arch/generic/debug_uart_8250.h>

#include "debug_uart.h"


class ArchUART8250Omap : public DebugUART8250 {
public:
							ArchUART8250Omap(addr_t base, int64 clock);
							~ArchUART8250Omap();
	void					InitEarly();

	// ARM MMIO: on ARM the UART regs are aligned on 32bit
	virtual void			Out8(int reg, uint8 value);
	virtual uint8			In8(int reg);
};


DebugUART8250* arch_get_uart_8250_omap(addr_t base, int64 clock);


#endif /* _KERNEL_ARCH_DEBUG_UART_8250_OMAP_H */
