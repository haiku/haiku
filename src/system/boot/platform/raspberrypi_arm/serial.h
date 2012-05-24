/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_BOOT_PLATFORM_PI_SERIAL_H
#define _SYSTEM_BOOT_PLATFORM_PI_SERIAL_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void serial_init(void);
extern void serial_cleanup(void);
extern void serial_disable(void);
extern void serial_enable(void);

extern void serial_puts(const char *string, size_t size);
extern int serial_getc(bool wait);


#ifdef __cplusplus
}
#endif


#endif /* _SYSTEM_BOOT_PLATFORM_PI_SERIAL_H */
