/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef __DEV_UART_PL011_H
#define __DEV_UART_PL011_H


#include <sys/types.h>
#include <SupportDefs.h>


#define PL01x_DR	0x00 // Data read or written
#define PL01x_RSR	0x04 // Receive status, read
#define PL01x_ECR	0x04 // Error clear, write
#define PL010_LCRH	0x08 // Line control, high
#define PL010_LCRM	0x0C // Line control, middle
#define PL010_LCRL	0x10 // Line control, low
#define PL010_CR	0x14 // Control
#define PL01x_FR	0x18 // Flag (r/o)
#define PL010_IIR	0x1C // Interrupt ID (r)
#define PL010_ICR	0x1C // Interrupt clear (w)
#define PL01x_ILPR	0x20 // IrDA low power
#define PL011_IBRD	0x24 // Interrupt baud rate divisor
#define PL011_FBRD	0x28 // Fractional baud rate divisor
#define PL011_LCRH	0x2C // Line control
#define PL011_CR	0x30 // Control
#define PL011_IFLS	0x34 // Interrupt fifo level
#define PL011_IMSC	0x38 // Interrupt mask
#define PL011_RIS	0x3C // Raw interrupt
#define PL011_MIS	0x40 // Masked interrupt
#define PL011_ICR	0x44 // Interrupt clear
#define PL011_DMACR	0x48 // DMA control register

#define PL011_DR_OE		(1 << 11)
#define PL011_DR_BE		(1 << 10)
#define PL011_DR_PE		(1 << 9)
#define PL011_DR_FE		(1 << 8)

#define PL01x_RSR_OE	0x08
#define PL01x_RSR_BE	0x04
#define PL01x_RSR_PE	0x02
#define PL01x_RSR_FE	0x01

#define PL011_FR_RI		0x100
#define PL011_FR_TXFE	0x080
#define PL011_FR_RXFF	0x040
#define PL01x_FR_TXFF	0x020
#define PL01x_FR_RXFE	0x010
#define PL01x_FR_BUSY	0x008
#define PL01x_FR_DCD	0x004
#define PL01x_FR_DSR	0x002
#define PL01x_FR_CTS	0x001
#define PL01x_FR_TMSK	(PL01x_FR_TXFF | PL01x_FR_BUSY)

#define PL011_CR_CTSEN	0x8000 // CTS flow control
#define PL011_CR_RTSEN	0x4000 // RTS flow control
#define PL011_CR_OUT2	0x2000 // OUT2
#define PL011_CR_OUT1	0x1000 // OUT1
#define PL011_CR_RTS	0x0800 // RTS
#define PL011_CR_DTR	0x0400 // DTR
#define PL011_CR_RXE	0x0200 // Receive enable
#define PL011_CR_TXE	0x0100 // Transmit enable
#define PL011_CR_LBR	0x0080 // Loopback enable
#define PL011_CR_RTIE	0x0040
#define PL011_CR_TIE	0x0020
#define PL011_CR_RIE	0x0010
#define PL011_CR_MSIE	0x0008
#define PL011_CR_IIRLP	0x0004 // SIR low power mode
#define PL011_CR_SIREN	0x0002 // SIR enable
#define PL011_CR_UARTEN 0x0001 // UART enable

#define PL011_LCRH_SPS		0x80
#define PL01x_LCRH_WLEN_8	0x60
#define PL01x_LCRH_WLEN_7	0x40
#define PL01x_LCRH_WLEN_6	0x20
#define PL01x_LCRH_WLEN_5	0x00
#define PL01x_LCRH_FEN		0x10
#define PL01x_LCRH_STP2		0x08
#define PL01x_LCRH_EPS		0x04
#define PL01x_LCRH_PEN		0x02
#define PL01x_LCRH_BRK		0x01

// TODO: Other PL01x registers + values?


class UartPL011 {
public:
							UartPL011(addr_t base);
							~UartPL011();

	void					InitEarly();
	void					Init();
	void					InitPort(uint32 baud);

	int						PutChar(char c);
	int						GetChar(bool wait);

	void					FlushTx();
	void					FlushRx();

private:
	void					WriteUart(uint32 reg, unsigned char data);
	unsigned char			ReadUart(uint32 reg);

	addr_t					fUARTBase;
};


#endif
