/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 */
#ifndef _KERNEL_ARCH_DEBUG_UART_8250_H
#define _KERNEL_ARCH_DEBUG_UART_8250_H


#include <SupportDefs.h>
#include <sys/types.h>

#include "debug_uart.h"


class DebugUART8250 : public DebugUART {
public:
							DebugUART8250(addr_t base, int64 clock);
							~DebugUART8250();

	void					InitEarly();
	void					Init();
	void					InitPort(uint32 baud);

	int						PutChar(char c);
	int						GetChar(bool wait);

	void					FlushTx();
	void					FlushRx();
};


extern DebugUART8250 *arch_get_uart_8250(addr_t base, int64 clock);

#endif /* _KERNEL_ARCH_DEBUG_UART_8250_H */
