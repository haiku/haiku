/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes@gmail.com
 */

#include <debug.h>
#include <arch/arm/reg.h>
#include <arch/generic/debug_uart.h>
#include <arch/arm64/arch_uart_linflex.h>
#include <new>


using namespace LINFlexRegisters;


ArchUARTlinflex::ArchUARTlinflex(addr_t base, int64 clock)
	:
	DebugUART(base, clock)
{
	Barrier();

	if (LinflexCell()->LINCR1.B.SLEEP == 0) {
		// This periperal is initialized
		if ((LinflexCell()->UARTCR.B.TXEN == 1)
			&& (LinflexCell()->UARTCR.B.RXEN == 1)
			&& (LinflexCell()->UARTCR.B.UART == 1)) {
			// LinFlex already configured as UART mode
			// TODO: good to go
		} else {

		}
	}
}


ArchUARTlinflex::~ArchUARTlinflex()
{
}


void
ArchUARTlinflex::Barrier()
{
	asm volatile ("" : : : "memory");
}


void
ArchUARTlinflex::InitPort(uint32 baud)
{
	// Calculate baud divisor
	uint32 baudDivisor = Clock() / (16 * baud);
	uint32 remainder = Clock() % (16 * baud);
	uint32 baudFractional = ((8 * remainder) / baud >> 1)
		+ ((8 * remainder) / baud & 1);

	// Disable UART
	Disable();

	// Set baud divisor

	// Set LCR 8n1, enable fifo

	// Set FIFO levels

	// Enable UART
	Enable();
}


void
ArchUARTlinflex::InitEarly()
{
	// Perform special hardware UART configuration
}


void
ArchUARTlinflex::Enable()
{

	DebugUART::Enable();
}


void
ArchUARTlinflex::Disable()
{

	DebugUART::Disable();
}


int
ArchUARTlinflex::PutChar(char c)
{
	if (Enabled() == true) {
		// Wait until there is room in fifo
		bool fifo = LinflexCell()->UARTCR.B.TFBM == 1;

		if (fifo) {
			// TFF is set by hardware in UART FIFO mode (TFBM = 1) when TX FIFO is full.
			while (LinflexCell()->UARTSR.B.DTF == 1) {
				Barrier();
			}
		}

		Out<uint8, vuint32>(&LinflexCell()->BDRL.R, c);

		if (!fifo) {
			// DTF is set by hardware in UART buffer mode (TFBM = 0) and
			// indicates that data transmission is completed. DTF should be cleared by software.
			while (LinflexCell()->UARTSR.B.DTF == 0) {
				Barrier();
			}

			auto uartsr = BitfieldRegister<UARTSR_register>();
			uartsr.B.DTF = 1;
			LinflexCell()->UARTSR.R = uartsr.R;
		}

		return 0;
	}

	return -1;
}


int
ArchUARTlinflex::GetChar(bool wait)
{
	int character;

	if (Enabled() == true) {
		bool fifo = LinflexCell()->UARTCR.B.RFBM == 1;

		if (fifo) {
			// RFE is set by hardware in UART FIFO mode (RFBM = 1) when the RX FIFO is empty.
			if (wait) {
				// Wait until a character is received
				while (LinflexCell()->UARTSR.B.DRF == 1) {
					Barrier();
				}
			} else {
				if (LinflexCell()->UARTSR.B.DRF == 1)
					return -1;
			}
		} else {
			// DRF is set by hardware in UART buffer mode (RFBM = 0) and
			// indicates that the number of bytes programmed in RDFL have been received.
			// DRF should be cleared by software.
			if (wait) {
				while (LinflexCell()->UARTSR.B.DRF == 0) {
					Barrier();
				}
			} else {
				if (LinflexCell()->UARTSR.B.DRF == 0)
					return -1;
			}
		}

		character = In<uint8, vuint32>(&LinflexCell()->BDRM.R);

		// Clear status
		auto uartsr = BitfieldRegister<UARTSR_register>();
		uartsr.B.RMB = 1;
		uartsr.B.DRF = 1;
		LinflexCell()->UARTSR.R = uartsr.R;

		return character;
	}

	return -1;
}


void
ArchUARTlinflex::FlushTx()
{
	// Wait until transmit fifo empty
}


void
ArchUARTlinflex::FlushRx()
{
	// Wait until receive fifo empty
}


ArchUARTlinflex*
arch_get_uart_linflex(addr_t base, int64 clock)
{
	static char buffer[sizeof(ArchUARTlinflex)];
	ArchUARTlinflex *uart = new(buffer) ArchUARTlinflex(base, clock);
	return uart;
}
