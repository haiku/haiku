/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_CONSOLE_H
#define _KERNEL_CONSOLE_H

#include <stdio.h>

struct kernel_args;

int con_init(struct kernel_args *ka);
void kprintf(const char *fmt, ...) __PRINTFLIKE(1,2);
void kprintf_xy(int x, int y, const char *fmt, ...) __PRINTFLIKE(3,4);

#endif	/* _KERNEL_CONSOLE_H */
