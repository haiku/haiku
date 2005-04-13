/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <boot/stage2.h>

#include <kernel/arch/dbg_console.h>

int arch_dbg_con_init(kernel_args *ka)
{
	volatile uint16 *scif16 = (uint16*)0xffe80000;
	volatile uint8 *scif8 = (uint8*)0xffe80000;
	int x;

	/* Disable interrupts, transmit/receive, and use internal clock */
	scif16[8/2] = 0;

	/* 8N1, use P0 clock */
	scif16[0] = 0;

        /* Set baudrate, N = P0/(32*B)-1 */
//	scif8[4] = (50000000 / (32 * baud_rate)) - 1;

//	scif8[4] = 80; // 19200
//	scif8[4] = 40; // 38400
//	scif8[4] = 26; // 57600
	scif8[4] = 13; // 115200

	/* Reset FIFOs, enable hardware flow control */
	scif16[24/2] = 4; //12;

	for(x = 0; x < 100000; x++);
	scif16[24/2] = 0; //8;

	/* Disable manual pin control */
	scif16[32/2] = 0;

	/* Clear status */
	scif16[16/2] = 0x60;
	scif16[36/2] = 0;

	/* Enable transmit/receive */
	scif16[8/2] = 0x30;

	for(x = 0; x < 100000; x++);

	arch_dbg_con_puts("serial initted\n");

	return 0;
}

char arch_dbg_con_read()
{
	volatile uint16 *status = (uint16*)0xffe8001c;
	volatile uint16 *ack = (uint16*)0xffe80010;
	volatile uint8 *fifo = (uint8*)0xffe80014;
	char c;

	/* Check input FIFO */
	while ((*status & 0x1f) == 0);

	/* Get the input char */
	c = *fifo;

	/* Ack */
	*ack &= 0x6d;

	return c;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
static void arch_dbg_con_flush() {
	volatile uint16 *ack = (uint16*)0xffe80010;

	*ack &= 0xbf;
	while (!(*ack & 0x40))
                ;
	*ack &= 0xbf;
}

static void _arch_dbg_con_putch(const char c)
{
	volatile uint16 *ack = (uint16*)0xffe80010;
	volatile uint8 *fifo = (uint8*)0xffe8000c;

	/* Wait until the transmit buffer has space */
	while (!(*ack & 0x20))
                ;
	/* Send the char */
	*fifo = c;

	/* Clear status */
	*ack &= 0x9f;
}

char arch_dbg_con_putch(const char c)
{
	if (c == '\n') {
		_arch_dbg_con_putch('\r');
		_arch_dbg_con_putch('\n');
	} else if (c != '\r')
		_arch_dbg_con_putch(c);

	return c;
}

void arch_dbg_con_puts(const char *s)
{
	while(*s != '\0') {
		arch_dbg_con_putch(*s);
		s++;
	}
}

