/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _SERIAL_H
#define _SERIAL_H

int dprintf(const char *fmt, ...);
int serial_init();
char serial_putch(const char c);
void serial_puts(const char *str);

#endif

