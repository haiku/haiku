/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef __DEV_UART_H
#define __DEV_UART_H


#include <sys/types.h>

#include "uart_8250.h"
#include "uart_pl011.h"


#ifdef __cplusplus
extern "C" {
#endif


struct uart_info {
	addr_t base;

	// Pointers to functions based on port type
	void (*init)(addr_t base);
	void (*init_early)(void);
	void (*init_port)(addr_t base, uint baud);
	int (*putchar)(addr_t base, char c);
	int (*getchar)(addr_t base, bool wait);
	void (*flush_tx)(addr_t base);
	void (*flush_rx)(addr_t base);
};


uart_info* uart_create(void);
void uart_destroy();
addr_t uart_base_port(int port);
addr_t uart_debug_port(void);


#ifdef __cplusplus
}
#endif
#endif
