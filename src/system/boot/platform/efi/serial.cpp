/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2013-2014, Fredrik Holmqvist, fredrik.holmqvist@gmail.com.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "efi_platform.h"
#include <efi/protocol/serial-io.h>
#include "serial.h"

#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <string.h>


static efi_guid sSerialIOProtocolGUID = EFI_SERIAL_IO_PROTOCOL_GUID;
static const uint32 kSerialBaudRate = 115200;

static efi_serial_io_protocol *sSerial = NULL;
static bool sSerialEnabled = false;
static bool sSerialUsesEFI = true;


enum serial_register_offsets {
	SERIAL_TRANSMIT_BUFFER		= 0,
	SERIAL_RECEIVE_BUFFER		= 0,
	SERIAL_DIVISOR_LATCH_LOW	= 0,
	SERIAL_DIVISOR_LATCH_HIGH	= 1,
	SERIAL_FIFO_CONTROL			= 2,
	SERIAL_LINE_CONTROL			= 3,
	SERIAL_MODEM_CONTROL		= 4,
	SERIAL_LINE_STATUS			= 5,
	SERIAL_MODEM_STATUS			= 6,
};

static uint16 sSerialBasePort = 0x3f8;


static void
serial_putc(char ch)
{
	if (!sSerialEnabled)
		return;

	if (sSerialUsesEFI) {
		size_t bufSize = 1;
		sSerial->Write(sSerial, &bufSize, &ch);
		return;
	}

	#if defined(__x86__) || defined(__x86_64__)
	while ((in8(sSerialBasePort + SERIAL_LINE_STATUS) & 0x20) == 0)
		asm volatile ("pause;");

	out8(ch, sSerialBasePort + SERIAL_TRANSMIT_BUFFER);
	#endif
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (!sSerialEnabled || (sSerial == NULL && sSerialUsesEFI))
		return;

	while (size-- != 0) {
		char ch = string[0];

		if (ch == '\n') {
			serial_putc('\r');
			serial_putc('\n');
		} else if (ch != '\r')
			serial_putc(ch);

		string++;
	}
}


extern "C" void
serial_disable(void)
{
	sSerialEnabled = false;
}


extern "C" void
serial_enable(void)
{
	sSerialEnabled = true;
}


extern "C" void
serial_init(void)
{
	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&sSerialIOProtocolGUID, NULL, (void**)&sSerial);

	if (status != EFI_SUCCESS || sSerial == NULL) {
		sSerial = NULL;
		return;
	}

	// Setup serial, 0, 0 = Default Receive FIFO queue and default timeout
	status = sSerial->SetAttributes(sSerial, kSerialBaudRate, 0, 0, NoParity, 8,
		OneStopBit);

	if (status != EFI_SUCCESS) {
		sSerial = NULL;
		return;
	}
}


#if defined(__x86__) || defined(__x86_64__)
extern "C" void
serial_switch_to_legacy(void)
{
	sSerial = NULL;
	sSerialUsesEFI = false;

	memset(gKernelArgs.platform_args.serial_base_ports, 0,
		sizeof(uint16) * MAX_SERIAL_PORTS);

	gKernelArgs.platform_args.serial_base_ports[0] = sSerialBasePort;

	uint16 divisor = uint16(115200 / kSerialBaudRate);

	out8(0x80, sSerialBasePort + SERIAL_LINE_CONTROL);
		// set divisor latch access bit
	out8(divisor & 0xf, sSerialBasePort + SERIAL_DIVISOR_LATCH_LOW);
	out8(divisor >> 8, sSerialBasePort + SERIAL_DIVISOR_LATCH_HIGH);
	out8(3, sSerialBasePort + SERIAL_LINE_CONTROL);
		// 8N1
}
#endif
