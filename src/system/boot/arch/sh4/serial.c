/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include <stdio.h>
#include "serial.h"

int dprintf(const char *fmt, ...)
{
	int ret = 0;
	va_list args;
	char temp[128];

	va_start(args, fmt);
	ret = vsprintf(temp, fmt, args);
	va_end(args);

	serial_puts(temp);
	return ret;
}

int serial_init()
{
	volatile unsigned short *scif16 = (unsigned short*)0xffe80000;
	volatile unsigned char *scif8 = (unsigned char*)0xffe80000;
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

	serial_puts("serial initted\n");

	return 0;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
static void serial_flush() {
	volatile unsigned short *ack = (unsigned short*)0xffe80010;

	*ack &= 0xbf;
	while (!(*ack & 0x40))
                ;
	*ack &= 0xbf;
}

static void _serial_putch(const char c)
{
	volatile unsigned short *ack = (unsigned short*)0xffe80010;
	volatile unsigned char *fifo = (unsigned char*)0xffe8000c;

	/* Wait until the transmit buffer has space */
	while (!(*ack & 0x20))
                ;
	/* Send the char */
	*fifo = c;

	/* Clear status */
	*ack &= 0x9f;
}

char serial_putch(const char c)
{
	if (c == '\n') {
		_serial_putch('\r');
		_serial_putch('\n');
	} else if (c != '\r')
		_serial_putch(c);

	return c;
}

void serial_puts(const char *s)
{
	while(*s != '\0') {
		serial_putch(*s);
		s++;
	}
}

