/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */
#ifndef _UART_H_
#define _UART_H_

// References:
// http://www.beyondlogic.org/serial/serial.htm


// 8250 UART registers
#define THB		0
#define RBR		0
#define DLLB	0
#define IER		1
#define DLHB	1
#define IIR		2
#define FCR		2
#define LCR		3
#define MCR		4
#define LSR		5
#define MSR		6
#define SR		7

// bits
#define IER_RDA			(1 << 0)
#define IER_THRE		(1 << 1)
#define IER_RLS			(1 << 2)
#define IER_MS			(1 << 3)
#define IER_SM			(1 << 4)	// 16750
#define IER_LPM			(1 << 5)	// 16750

#define IIR_PENDING		(1 << 0)
#define IIR_IMASK		(0x03 << 1)
#define IIR_MS			(0 << 1)
#define IIR_THRE		(1 << 1)
#define IIR_RDA			(2 << 1)
#define IIR_RLS			(3 << 1)
#define IIR_TO			(1 << 3)	// 16550
#define IIR_F64EN		(1 << 5)	// 16750
#define IIR_FMASK		(3 << 6)

#define FCR_ENABLE		(1 << 0)
#define FCR_RX_RST		(1 << 1)
#define FCR_TX_RST		(1 << 2)
#define FCR_DMA_EN		(1 << 3)
#define FCR_F64EN		(1 << 5)
#define FCR_FMASK		(0x03 << 6)
#define FCR_F_1			(0 << 6)
#define FCR_F_4			(1 << 6)
#define FCR_F_8			(2 << 6)
#define FCR_F_14		(3 << 6)

#define LCR_5BIT		0
#define LCR_6BIT		1
#define LCR_7BIT		2
#define LCR_8BIT		3
#define LCR_2STOP		(1 << 2)
#define LCR_P_EN		(1 << 3)
#define LCR_P_EVEN		(1 << 4)
#define LCR_P_MARK		(1 << 5)
#define LCR_BREAK		(1 << 6)
#define LCR_DLAB		(1 << 7)

#define MCR_DTR			(1 << 0)
#define MCR_RTS			(1 << 1)
#define MCR_AUX1		(1 << 2)
#define MCR_AUX2		(1 << 3)
#define MCR_IRQ_EN		(1 << 3)	// ?
#define MCR_LOOP		(1 << 4)
#define MCR_AUTOFLOW	(1 << 5)	// 16750

#define LSR_DR			(1 << 0)
#define LSR_OVERRUN		(1 << 1)
#define LSR_PARITY		(1 << 2)
#define LSR_FRAMING		(1 << 3)
#define LSR_BREAK		(1 << 4)
#define LSR_THRE		(1 << 5)
#define LSR_TSRE		(1 << 6)
#define LSR_FIFO		(1 << 7)

#define MSR_DCTS		(1 << 0)
#define MSR_DDSR		(1 << 1)
#define MSR_TERI		(1 << 2)
#define MSR_DDCD		(1 << 3)
#define MSR_CTS			(1 << 4)
#define MSR_DSR			(1 << 5)
#define MSR_RI			(1 << 6)
#define MSR_DCD			(1 << 7)

// speeds
//#define BPS_50		


#endif	// _UART_H_
