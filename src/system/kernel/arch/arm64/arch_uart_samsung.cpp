/*
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sam Roberts, sam@smrobtzz.dev
 */

#include <arch/generic/debug_uart.h>
#include <arch/arm64/arch_uart_samsung.h>
#include <new>


#define ULCON    0x00 // Line control
#define UCON     0x04 // Control
#define UFCON    0x08 // FIFO control
#define UTRSTAT  0x10 // Transmit status
#define UFSTAT   0x18 // FIFO status
#define UTXH     0x20 // Transmit holding
#define URXH     0x24 // Receive holding
#define UBRDIV   0x28 // Baud rate divider

#define UCON_TXTHRESH_ENA (1 << 13) // Transmit threshold enable
#define UCON_RXTHRESH_ENA (1 << 12) // Receive threshold enable
#define UCON_RXTO_ENA     (1 << 9)  // Receive timeout enable
#define UCON_TXMODE       (3 << 2)  // Transmit mode
#define UCON_RXMODE       (3 << 0)  // Receive mode

#define UCON_MODE_OFF 0
#define UCON_MODE_IRQ 1

#define UTRSTAT_RXTO     (1 << 9) // Receive timeout status
#define UTRSTAT_TXTHRESH (1 << 5) // Transmit threshold status
#define UTRSTAT_RXTHRESH (1 << 4) // Receive threshold status
#define UTRSTAT_TXE      (1 << 2) // Transmit empty status
#define UTRSTAT_TXBE     (1 << 1) // Transmit buffer empty status
#define UTRSTAT_RXD      (1 << 0) // Receive data status

#define UFSTAT_TXFULL (1 << 9) // Transmit FIFO full status
#define UFSTAT_RXFULL (1 << 8) // Receive FIFO full status
#define UFSTAT_TXCNT  (15 << 4) // Transmit FIFO count
#define UFSTAT_RXCNT  (15 << 0) // Receive FIFO count


ArchUARTSamsung::ArchUARTSamsung(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
}


ArchUARTSamsung::~ArchUARTSamsung()
{
}


void
ArchUARTSamsung::Out32(int reg, uint32 data)
{
	*(volatile uint32*)(Base() + reg) = data;
}


uint32
ArchUARTSamsung::In32(int reg)
{
	return *(volatile uint32*)(Base() + reg);
}


void
ArchUARTSamsung::InitPort(uint32 baud)
{
}


void
ArchUARTSamsung::Enable()
{
	DebugUART::Enable();
}


void
ArchUARTSamsung::Disable()
{
	DebugUART::Disable();
}


int
ArchUARTSamsung::PutChar(char c)
{
	if (Enabled()) {
		while ((In32(UFSTAT) & UFSTAT_TXFULL) != 0)
			;

		Out32(UTXH, c);
		return 0;
	}

	return -1;
}


int
ArchUARTSamsung::GetChar(bool wait)
{
	if (Enabled()) {
		while (wait && ((In32(UFSTAT) & UFSTAT_RXCNT) == 0))
			;

		if ((In32(UFSTAT) & UFSTAT_RXCNT) == 0)
			return -1;

		return In32(URXH);
	}

	return -1;
}


void
ArchUARTSamsung::FlushTx()
{
}


void
ArchUARTSamsung::FlushRx()
{
}


ArchUARTSamsung*
arch_get_uart_samsung(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUARTSamsung)];
	ArchUARTSamsung* uart = new(buffer) ArchUARTSamsung(base, clock);
	return uart;
}
