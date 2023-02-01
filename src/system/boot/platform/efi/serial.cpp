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
#include <arch/generic/debug_uart.h>
#include <arch/generic/debug_uart_8250.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <string.h>


static efi_guid sSerialIOProtocolGUID = EFI_SERIAL_IO_PROTOCOL_GUID;
static const uint32 kSerialBaudRate = 115200;

static efi_serial_io_protocol *sEFISerialIO = NULL;
static bool sSerialEnabled = false;
static bool sEFIAvailable = true;


DebugUART* gUART = NULL;


static void
serial_putc(char ch)
{
	if (!sSerialEnabled)
		return;

	// First we use EFI serial_io output if available
	if (sEFISerialIO != NULL) {
		size_t bufSize = 1;
		sEFISerialIO->Write(sEFISerialIO, &bufSize, &ch);
		return;
	}

#ifdef DEBUG
	// If we don't have EFI serial_io, fallback to EFI stdio
	if (sEFIAvailable) {
		char16_t ucsBuffer[2];
		ucsBuffer[0] = ch;
		ucsBuffer[1] = 0;
		kSystemTable->ConOut->OutputString(kSystemTable->ConOut, ucsBuffer);
		return;
	}
#endif

	// If EFI services are unavailable... try any UART
	// this can happen when serial_io is unavailable, or EFI
	// is exiting
	if (gUART != NULL) {
		gUART->PutChar(ch);
		return;
	}
}


extern "C" void
serial_puts(const char* string, size_t size)
{
	if (!sSerialEnabled)
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
	if (gUART != NULL)
		gUART->InitPort(kSerialBaudRate);
}


extern "C" void
serial_init(void)
{
	// Check for EFI Serial
	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&sSerialIOProtocolGUID, NULL, (void**)&sEFISerialIO);

	if (status != EFI_SUCCESS)
		sEFISerialIO = NULL;

	if (sEFISerialIO != NULL) {
		// Setup serial, 0, 0 = Default Receive FIFO queue and default timeout
		status = sEFISerialIO->SetAttributes(sEFISerialIO, kSerialBaudRate, 0, 0, NoParity, 8,
			OneStopBit);

		if (status != EFI_SUCCESS)
			sEFISerialIO = NULL;

		// serial_io was successful.
	}

#if defined(__i386__) || defined(__x86_64__)
	// On x86, we can try to setup COM1 as a gUART too
	// while this serial port may not physically exist,
	// the location is fixed on the x86 arch.
	// TODO: We could also try to pull from acpi?
	if (gUART == NULL) {
		gUART = arch_get_uart_8250(0x3f8, 1843200);
		gUART->InitEarly();
	}
#endif
}


extern "C" void
serial_kernel_handoff(void)
{
	// The console was provided by boot services, disable it ASAP
	stdout = NULL;
	stderr = NULL;

	// Disconnect from EFI serial_io services is important as we leave the bootloader
	sEFISerialIO = NULL;
	sEFIAvailable = false;

#if defined(__i386__) || defined(__x86_64__)
	// TODO: convert over to exclusively arch_args.uart?
	memset(gKernelArgs.platform_args.serial_base_ports, 0,
		sizeof(uint16) * MAX_SERIAL_PORTS);
	gKernelArgs.platform_args.serial_base_ports[0] = 0x3f8;
#endif
}
