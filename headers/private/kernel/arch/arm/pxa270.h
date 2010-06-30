/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
 */
#ifndef __PLATFORM_PXA270_H
#define __PLATFORM_PXA270_H

#define SDRAM_BASE 0xa2000000

#define VECT_BASE 0x00000000
#define VECT_SIZE 0x1000

#define DEVICE_BASE 0x40000000
#define DEVICE_SIZE 0x5000000

/* UART */
#define FFUART_BASE    0x40100000
#define BTUART_BASE    0x40200000
#define STUART_BASE    0x40700000
#define CKEN           0x41300004
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

/* DMA controller */

typedef struct pxa27x_dma_descriptor {
	uint32 ddadr;
	uint32 dsadr;
	uint32 dtadr;
	uint32 dcmd;
} pxa27x_dma_descriptor __attribute__ ((aligned(16)));

/* LCD controller */

#define LCC_BASE	0x44000000

#define LCCR0		(LCC_BASE+0x00)
#define LCCR1		(LCC_BASE+0x04)
#define LCCR2		(LCC_BASE+0x08)
#define LCCR3		(LCC_BASE+0x0C)
#define LCCR4		(LCC_BASE+0x10)
#define LCCR5		(LCC_BASE+0x14)

#define LCSR1		(LCC_BASE+0x34)
#define LCSR0		(LCC_BASE+0x38)
#define LIIDR		(LCC_BASE+0x3C)

#define OVL1C1		(LCC_BASE+0x50)
#define OVL1C2		(LCC_BASE+0x60)
#define OVL2C1		(LCC_BASE+0x70)
#define OVL2C2		(LCC_BASE+0x80)

#define LCC_CCR		(LCC_BASE+0x90)
#define LCC_CMDCR	(LCC_BASE+0x100)

#define FDADR0		(LCC_BASE+0x200)
#define FBR0		(LCC_BASE+0x020)
#define FSADR0		(LCC_BASE+0x204)

typedef struct pxa27x_lcd_dma_descriptor {
	uint32 fdadr;
	uint32 fsadr;
	uint32 fidr;
	uint32 ldcmd;
} pxa27x_lcd_dma_descriptor __attribute__ ((aligned(16)));

#endif /* __PLATFORM_PXA270_H */
