/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "gpio.h"


void
gpio_init()
{
	// Set up pointer to Raspberry Pi GPIO base
	gGPIOBase = (volatile unsigned *)GPIO_BASE;

	// Take control of ok led and general use pins
	int pin = 0;
	for (pin = 16; pin <= 25; pin++) {
		GPIO_IN(pin);
		GPIO_OUT(pin);
	}
}
