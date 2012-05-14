/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Gordon Henderson, gordon@drogon.net
 */


#include "gpio.h"


//  Define the shift up for the 3 bits per pin in each GPFSEL port
static uint8_t
kGPIOToShift[] =
{
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
	0, 3, 6, 9, 12, 15, 18, 21, 24, 27,
};


//  Map a BCM_GPIO pin to it's control port. (GPFSEL 0-5)
static uint8_t
kGPIOToGPFSEL[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};


// (Word) offset to the GPIO Set registers for each GPIO pin
static uint8_t
kGPIOToGPSET[] =
{
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

// (Word) offset to the GPIO Clear registers for each GPIO pin
static uint8_t
kGPIOToGPCLR[] =
{
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
};


/*!
 * At GPIO (base) (pin) state set (value)
 */
void
gpio_write(addr_t base, int pin, bool value)
{
	volatile addr_t *gpio = (addr_t*)base;

	if (value == 1)
		*(gpio + kGPIOToGPSET[pin]) = 1 << pin;
	else
		*(gpio + kGPIOToGPCLR[pin]) = 1 << pin;
}


/*!
 * At GPIO (base) (pin) set mode (mode)
 */
void
gpio_mode(addr_t base, int pin, int mode)
{
	int sel = kGPIOToGPFSEL[pin];
	int shift = kGPIOToShift[pin];

	volatile addr_t *gpio = (addr_t*)base + sel;

	if (mode == GPIO_IN)
		*gpio = (*gpio & ~(7 << shift));
	else if (mode == GPIO_OUT)
		*gpio = (*gpio & ~(7 << shift)) | (1 << shift);
	else
		*gpio = (*gpio & ~(7 << shift)) | (mode << shift);
}


void
gpio_init()
{
	// ** Take control of ok led, and general use pins
	int pin = 0;
	for (pin = 16; pin <= 25; pin++)
		gpio_mode(GPIO_BASE, pin, GPIO_OUT);
}
