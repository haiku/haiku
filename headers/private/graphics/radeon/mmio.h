/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver
		
	Macros for easier memory mapped I/O access
*/

#ifndef _MMIO_H
#define _MMIO_H

// read 8-bit register
#define INREG8( regs, addr ) (*(regs + (addr)))
// write 8-bit register
#define OUTREG8( regs, addr, val ) do { *(regs + (addr)) = (val); } while( 0 )
// read 32-bit register
#define INREG( regs, addr ) (*((vuint32 *)(regs + (addr))))
// write 32-bit register
#define OUTREG( regs, addr, val ) do { *(vuint32 *)(regs + (addr)) = (val); } while( 0 )
// write partial 32-bit register, keeping bits "mask"
#define OUTREGP( regs, addr, val, mask )                                  \
	do {                                                                  \
		uint32 tmp = INREG( (regs), (addr) );                             \
		tmp &= (mask);                                                    \
		tmp |= (val) & ~(mask);                                           \
		OUTREG( (regs), (addr), tmp );                                    \
	} while (0)

#endif
