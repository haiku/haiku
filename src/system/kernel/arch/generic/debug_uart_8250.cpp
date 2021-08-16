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


#include <arch/generic/debug_uart_8250.h>
#include <debug.h>
#include <new>


DebugUART8250::DebugUART8250(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
}


DebugUART8250::~DebugUART8250()
{
}


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

#define LCR_8N1		0x03

#define FCR_FIFO_EN	0x01	/* Fifo enable */
#define FCR_RXSR	0x02	/* Receiver soft reset */
#define FCR_TXSR	0x04	/* Transmitter soft reset */

#define MCR_DTR		0x01
#define MCR_RTS		0x02
#define MCR_DMA_EN	0x04
#define MCR_TX_DFR	0x08

#define LCR_WLS_MSK	0x03	/* character length select mask */
#define LCR_WLS_5	0x00	/* 5 bit character length */
#define LCR_WLS_6	0x01	/* 6 bit character length */
#define LCR_WLS_7	0x02	/* 7 bit character length */
#define LCR_WLS_8	0x03	/* 8 bit character length */
#define LCR_STB		0x04	/* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define LCR_PEN		0x08	/* Parity eneble */
#define LCR_EPS		0x10	/* Even Parity Select */
#define LCR_STKP	0x20	/* Stick Parity */
#define LCR_SBRK	0x40	/* Set Break */
#define LCR_BKSE	0x80	/* Bank select enable */

#define LSR_DR		0x01	/* Data ready */
#define LSR_OE		0x02	/* Overrun */
#define LSR_PE		0x04	/* Parity error */
#define LSR_FE		0x08	/* Framing error */
#define LSR_BI		0x10	/* Break */
#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LSR_TEMT	0x40	/* Xmitter empty */
#define LSR_ERR		0x80	/* Error */


void
DebugUART8250::InitPort(uint32 baud)
{
	Disable();

	uint16 baudDivisor = Clock() / (16 * baud);

	// Write standard uart settings
	Out8(UART_LCR, LCR_8N1);
	// 8N1
	Out8(UART_IER, 0);
	// Disable interrupt
	Out8(UART_FCR, 0);
	// Disable FIFO
	Out8(UART_MCR, MCR_DTR | MCR_RTS);
	// DTR / RTS

	// Gain access to, and program baud divisor
	unsigned char buffer = In8(UART_LCR);
	Out8(UART_LCR, buffer | LCR_BKSE);
	Out8(UART_DLL, baudDivisor & 0xff);
	Out8(UART_DLH, (baudDivisor >> 8) & 0xff);
	Out8(UART_LCR, buffer & ~LCR_BKSE);

//	Out8(UART_MDR1, 0); // UART 16x mode
//	Out8(UART_LCR, 0xBF); // config mode B
//	Out8(UART_EFR, (1<<7)|(1<<6)); // hw flow control
//	Out8(UART_LCR, LCR_8N1); // operational mode

	Enable();
}


void
DebugUART8250::InitEarly()
{
}


void
DebugUART8250::Init()
{
}


int
DebugUART8250::PutChar(char c)
{
	while (!(In8(UART_LSR) & (1<<6)));
		// wait for the last char to get out
	Out8(UART_THR, c);
	return 0;
}


/* returns -1 if no data available */
int
DebugUART8250::GetChar(bool wait)
{
	if (wait) {
		while (!(In8(UART_LSR) & (1<<0)));
			// wait for data to show up in the rx fifo
	} else {
		if (!(In8(UART_LSR) & (1<<0)))
			return -1;
	}
	return In8(UART_RHR);
}


void
DebugUART8250::FlushTx()
{
	while (!(In8(UART_LSR) & (1<<6)));
		// wait for the last char to get out
}


void
DebugUART8250::FlushRx()
{
	// empty the rx fifo
	while (In8(UART_LSR) & (1<<0)) {
		volatile char c = In8(UART_RHR);
		(void)c;
	}
}


void
DebugUART8250::Barrier()
{
	// Simple memory barriers
#if defined(__POWERPC__)
	asm volatile("eieio; sync");
#elif defined(__ARM__)
	asm volatile ("" : : : "memory");
#endif
}


DebugUART8250*
arch_get_uart_8250(addr_t base, int64 clock)
{
	static char buffer[sizeof(DebugUART8250)];
	DebugUART8250* uart = new(buffer) DebugUART8250(base, clock);
	return uart;
}

