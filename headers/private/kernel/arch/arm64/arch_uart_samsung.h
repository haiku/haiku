/*
 * Copyright 2026 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sam Roberts, sam@smrobtzz.dev
 */
#ifndef __DEV_UART_SAMSUNG_H
#define __DEV_UART_SAMSUNG_H


#include <arch/generic/debug_uart.h>


#define UART_KIND_SAMSUNG "samsung"


class ArchUARTSamsung : public DebugUART {
public:
						ArchUARTSamsung(addr_t base, int64 clock);
						~ArchUARTSamsung();


			void		InitPort(uint32 baud);

			void		Enable();
			void		Disable();

			int			PutChar(char c);
			int			GetChar(bool wait);

			void		FlushTx();
			void		FlushRx();

private:
			void		Out32(int reg, uint32 data);
			uint32		In32(int reg);
};


ArchUARTSamsung *arch_get_uart_samsung(addr_t base, int64 clock);


#endif
