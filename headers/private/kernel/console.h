/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_CONSOLE_H
#define _KERNEL_CONSOLE_H

#include <stage2.h>
#include <cdefs.h>

int con_init(kernel_args *ka);
void kprintf(const char *fmt, ...) __PRINTFLIKE(1,2);
void kprintf_xy(int x, int y, const char *fmt, ...) __PRINTFLIKE(3,4);

#endif
