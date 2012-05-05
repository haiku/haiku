/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _SYSTEM_BOOT_PLATFORM_PI_GPIO_H
#define _SYSTEM_BOOT_PLATFORM_PI_GPIO_H


#include <arch/arm/bcm2708.h>


// Macros for easy GPIO pin access in loader
#define GPIO_IN(g) *(gGPIOBase + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define GPIO_OUT(g) *(gGPIOBase + ((g)/10)) |=  (1<<(((g)%10)*3))
#define GPIO_SET(g) *(gGPIOBase + 7) = (1<<g)
#define GPIO_CLR(g) *(gGPIOBase + 10) = (1<<g)


volatile unsigned *gGPIOBase;


#ifdef __cplusplus
extern "C" {
#endif


void gpio_init();


#ifdef __cplusplus
}
#endif


#endif /* _SYSTEM_BOOT_PLATFORM_PI_GPIO_H */
