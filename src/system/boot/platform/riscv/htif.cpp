/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Htif.h>


// This address is used by TinyEMU and it is not present in FDT.
HtifRegs* volatile gHtifRegs = (HtifRegs* volatile)0x40008000;


uint64_t
HtifCmd(uint32_t device, uint8_t cmd, uint32_t arg)
{
	uint64_t htifTohost = ((uint64_t)device << 56)
		+ ((uint64_t)cmd << 48) + arg;
	gHtifRegs->toHostLo = htifTohost % ((uint64_t)1 << 32);
	gHtifRegs->toHostHi = htifTohost / ((uint64_t)1 << 32);
	return (uint64_t)gHtifRegs->fromHostLo
		+ ((uint64_t)gHtifRegs->fromHostHi << 32);
}


void
HtifShutdown()
{
	HtifCmd(0, 0, 1);
}


void
HtifOutChar(char ch)
{
	HtifCmd(1, 1, ch);
}


void
HtifOutString(const char* str)
{
	for (; *str != '\0'; str++) HtifOutChar(*str);
}


void
HtifOutString(const char* str, size_t len)
{
	for (; len > 0; str++, len--) HtifOutChar(*str);
}
