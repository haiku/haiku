/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _ARCH_UART_SIFIVE_H_
#define _ARCH_UART_SIFIVE_H_


#include <arch/generic/debug_uart.h>


// UARTSifiveRegs.ie, ip
enum {
	kUartSifiveTxwm = 1 << 0,
	kUartSifiveRxwm = 1 << 1,
};


struct UARTSifiveRegs {
	union Txdata {
		struct {
			uint32 data:      8;
			uint32 reserved: 23;
			uint32 isFull:    1;
		};
		uint32 val;
	} txdata;

	union Rxdata {
		struct {
			uint32 data:      8;
			uint32 reserved: 23;
			uint32 isEmpty:   1;
		};
		uint32 val;
	} rxdata;

	union Txctrl {
		struct {
			uint32 enable:     1;
			uint32 nstop:      1;
			uint32 reserved1: 14;
			uint32 cnt:        3;
			uint32 reserved2: 13;
		};
		uint32 val;
	} txctrl;

	union Rxctrl {
		struct {
			uint32 enable:     1;
			uint32 reserved1: 15;
			uint32 cnt:        3;
			uint32 reserved2: 13;
		};
		uint32 val;
	} rxctrl;

	uint32 ie; // interrupt enable
	uint32 ip; // interrupt pending
	uint32 div;
	uint32 unused;
};


class ArchUARTSifive : public DebugUART {
public:
							ArchUARTSifive(addr_t base, int64 clock);
							~ArchUARTSifive();

	virtual	void			InitEarly();
	virtual	void			Init();
	virtual	void			InitPort(uint32 baud);

	virtual	void			Enable();
	virtual	void			Disable();

	virtual	int				PutChar(char ch);
	virtual	int				GetChar(bool wait);

	virtual	void			FlushTx();
	virtual	void			FlushRx();

protected:
	virtual	void			Barrier();

	inline volatile UARTSifiveRegs*
							Regs();
};


volatile UARTSifiveRegs*
ArchUARTSifive::Regs()
{
	return (volatile UARTSifiveRegs*)Base();
}


#endif	// _ARCH_UART_SIFIVE_H_
