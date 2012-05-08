/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _SYSTEM_BOOT_PLATFORM_PI_GPIO_H
#define _SYSTEM_BOOT_PLATFORM_PI_GPIO_H


#include <SupportDefs.h>

#include <arch/arm/bcm2708.h>


#define GPIO_IN		0
#define GPIO_OUT	1
#define GPIO_ALT0	4
#define GPIO_ALT1	5
#define GPIO_ALT2	6
#define GPIO_ALT3	7
#define GPIO_ALT4	3
#define GPIO_ALT5	2



#ifdef __cplusplus
extern "C" {
#endif


void gpio_write(addr_t base, int pin, bool value);
void gpio_mode(addr_t base, int pin, int mode);

void gpio_init();


#ifdef __cplusplus
}
#endif


#endif /* _SYSTEM_BOOT_PLATFORM_PI_GPIO_H */
