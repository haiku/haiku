/*
 * Copyright (c) 2012 Haiku Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef __PLATFORM_BCM2708_H
#define __PLATFORM_BCM2708_H


#define SIZE_4K 0x00001000

#define BCM2708_SDRAM_BASE	0x00000000
#define BCM2708_PERI_BASE	0x20000000


#define SDRAM_BASE		BCM2708_SDRAM_BASE
#define ST_BASE			(BCM2708_PERI_BASE + 0x3000)
	// System Timer
#define DMA_BASE		(BCM2708_PERI_BASE + 0x7000)
	// DMA Controller
#define ARM_BASE		(BCM2708_PERI_BASE + 0xB000)
	// BCM2708 ARM Control Block
#define PM_BASE			(BCM2708_PERI_BASE + 0x100000)
	// Power Management, Reset controller and Watchdog registers
#define GPIO_BASE		(BCM2708_PERI_BASE + 0x200000)
	// GPIO
#define UART0_BASE		(BCM2708_PERI_BASE + 0x201000)
	// UART 0
#define MMCI0_BASE		(BCM2708_PERI_BASE + 0x202000)
	// MMC
#define UART1_BASE		(BCM2708_PERI_BASE + 0x215000)
	// UART 1
#define EMMC_BASE		(BCM2708_PERI_BASE + 0x300000)
	// eMMC interface
#define SMI_BASE		(BCM2708_PERI_BASE + 0x600000)
	// SMI Base
#define USB_BASE		(BCM2708_PERI_BASE + 0x980000)
	// USB Controller
#define FB_BASE			(BCM2708_PERI_BASE + 0x0000)
	// Fake frame buffer
#define FB_SIZE			SIZE_4K


#define ARM_CTRL_BASE			(ARM_BASE + 0x000)
#define ARM_CTRL_IC_BASa		(ARM_BASE + 0x200)
	// Interrupt controller
#define ARM_CTRL_TIMER0_1_BASE	(ARM_BASE + 0x400)
	// Timer 0 and 1
#define ARM_CTRL_0_SBM_BASE		(ARM_BASE + 0x800)
	// ARM Semaphores, Doorbells, and Mailboxes

#define VECT_BASE 0xffff0000
	// TODO: Double check this
#define VECT_SIZE SIZE_4K
	// TODO: Guessed Vector size of 4k

#define DEVICE_BASE		ARM_CTRL_BASE
#define DEVICE_SIZE		SIZE_4K


#endif /* __PLATFORM_BCM2708_H */
