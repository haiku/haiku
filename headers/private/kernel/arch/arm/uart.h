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


addr_t uart_base_port(int port);
addr_t uart_base_debug();


#endif
