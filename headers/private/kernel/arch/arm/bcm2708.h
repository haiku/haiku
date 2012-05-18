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


/*
 * Found in:
 * Broadcom BCM2835 ARM Peripherals
 *  - BCM2835-ARM-Peripherals.pdf
 */

// 1.2.2 notes that peripherals are remapped from 0x7e to 0x20
#define BCM2708_SDRAM_BASE		0x00000000
#define BCM2708_DEVICEHW_BASE	0x7E000000 // Real Hardware Base
#define BCM2708_DEVICEFW_BASE	0x20000000 // Firmware Mapped Base

#define ST_BASE			0x3000
	// System Timer, sec 12.0, page 172
#define DMA_BASE		0x7000
	// DMA Controller, sec 4.2, page 39
#define ARM_BASE		0xB000
	// BCM2708 ARM Control Block, sec 7.5, page 112
#define PM_BASE			0x100000
	// Power Management, Reset controller and Watchdog registers
#define GPIO_BASE		0x200000
	// GPIO, sec 6.1, page 90
#define UART0_BASE		0x201000
	// UART 0, sec 13.4, page 177
#define MMCI0_BASE		0x202000
	// MMC
#define UART1_BASE		0x215000
	// UART 1, sec 2.1, page 65
#define EMMC_BASE		0x300000
	// eMMC interface, sec 5, page 66
#define SMI_BASE		0x600000
	// SMI Base
#define USB_BASE		0x980000
	// USB Controller, 15.2, page 202
#define FB_BASE			0x000000
	// Fake frame buffer
#define FB_SIZE			SIZE_4K


// 7.5, page 112
#define ARM_CTRL_BASE			(ARM_BASE + 0x000)
#define ARM_CTRL_IC_BASa		(ARM_BASE + 0x200)
	// Interrupt controller
#define ARM_CTRL_TIMER0_1_BASE	(ARM_BASE + 0x400)
	// Timer 0 and 1
#define ARM_CTRL_0_SBM_BASE		(ARM_BASE + 0x800)
	// ARM Semaphores, Doorbells, and Mailboxes

#define VECT_BASE 0xffff0000
#define VECT_SIZE SIZE_4K

#define DEVICE_BASE		BCM2708_DEVICEHW_BASE
#define DEVICE_SIZE		0x1000000

#define SDRAM_BASE		BCM2708_SDRAM_BASE
#define SDRAM_SIZE		0x4000000
	// 64Mb


/* UART */
// TODO: Check these UART defines!
#define UART_RHR    0
#define UART_THR    0
#define UART_DLL    0
#define UART_IER    1
#define UART_DLH    1
#define UART_IIR    2
#define UART_FCR    2
#define UART_EFR    2
#define UART_LCR    3
#define UART_MCR    4
#define UART_LSR    5
#define UART_MSR    6
#define UART_TCR    6
#define UART_SPR    7
#define UART_TLR    7
#define UART_MDR1   8
#define UART_MDR2   9
#define UART_SFLSR  10
#define UART_RESUME 11
#define UART_TXFLL  10
#define UART_TXFLH  11
#define UART_SFREGL 12
#define UART_SFREGH 13
#define UART_RXFLL  12
#define UART_RXFLH  13
#define UART_BLR    14
#define UART_UASR   14
#define UART_ACREG  15
#define UART_SCR    16
#define UART_SSR    17
#define UART_EBLR   18
#define UART_MVR    19
#define UART_SYSC   20


#endif /* __PLATFORM_BCM2708_H */
