/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Htif.h>


uint64
HtifCmd(uint32 device, uint8 cmd, uint32 arg)
{
	if (gHtifRegs == 0)
		return 0;

	uint64 htifTohost = ((uint64)device << 56)
		+ ((uint64)cmd << 48) + arg;
	gHtifRegs->toHostLo = htifTohost % ((uint64)1 << 32);
	gHtifRegs->toHostHi = htifTohost / ((uint64)1 << 32);
	return (uint64)gHtifRegs->fromHostLo
		+ ((uint64)gHtifRegs->fromHostHi << 32);
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
HtifOutString(const char *str)
{
	for (; *str != '\0'; str++)
		HtifOutChar(*str);
}


void
HtifOutString(const char *str, size_t len)
{
	for (; len > 0; str++, len--)
		HtifOutChar(*str);
}
