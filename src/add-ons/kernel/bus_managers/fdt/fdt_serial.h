/*
 * Copyright 2012-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef __FDT_SERIAL_H
#define __FDT_SERIAL_H


#include <KernelExport.h>
#include <arch/generic/debug_uart.h>


DebugUART * debug_uart_from_fdt(const void *fdt);


#endif /*__FDT_SERIAL_H*/
