/*
	Copyright 2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2008
*/

#include "accel.h"

#include <unistd.h>


// Macros for memory mapped I/O.
//==============================

#define INREG8(addr)		*((vuint8*)(gInfo.regs + addr))
#define INREG16(addr)		*((vuint16*)(gInfo.regs + addr))
#define INREG32(addr)		*((vuint32*)(gInfo.regs + addr))

#define OUTREG8(addr, val)	*((vuint8*)(gInfo.regs + addr)) = val
#define OUTREG16(addr, val)	*((vuint16*)(gInfo.regs + addr)) = val
#define OUTREG32(addr, val)	*((vuint32*)(gInfo.regs + addr)) = val


// Functions for PIO.
//===================

uint32 ReadPIO(uint32 addr, uint8 numBytes)
{
	S3GetSetPIO gsp;
	gsp.magic = S3_PRIVATE_DATA_MAGIC;
	gsp.offset = addr;
	gsp.size = numBytes;
	gsp.value = 0;

	status_t result = ioctl(gInfo.deviceFileDesc, S3_GET_PIO, &gsp, sizeof(gsp));
	if (result != B_OK)
		TRACE("ReadPIO() failed, result = 0x%x\n", result);

	return gsp.value;
}


void WritePIO(uint32 addr, uint8 numBytes, uint32 value)
{
	S3GetSetPIO gsp;
	gsp.magic = S3_PRIVATE_DATA_MAGIC;
	gsp.offset = addr;
	gsp.size = numBytes;
	gsp.value = value;

	status_t result = ioctl(gInfo.deviceFileDesc, S3_SET_PIO, &gsp, sizeof(gsp));
	if (result != B_OK)
		TRACE("WritePIO() failed, result = 0x%x\n", result);
}


// Functions to read 8/16/32 bit registers.
//=========================================

uint8 ReadReg8(uint32 addr)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		return ReadPIO(addr, 1);

	return INREG8(addr);
}


uint16 ReadReg16(uint32 addr)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		return ReadPIO(addr, 2);

	return INREG16(addr);
}


uint32 ReadReg32(uint32 addr)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		return ReadPIO(addr, 4);

	return INREG32(addr);
}


// Functions to write 8/16/32 bit registers.
//=========================================

void WriteReg8(uint32 addr, uint8 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		WritePIO(addr, 1, value);
	else
		OUTREG8(addr, value);
}


void WriteReg16(uint32 addr, uint16 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		WritePIO(addr, 2, value);
	else
		OUTREG16(addr, value);
}


void WriteReg32(uint32 addr, uint32 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		WritePIO(addr, 4, value);
	else
		OUTREG32(addr, value);
}


// Functions to read/write CRTC registers.
//========================================

uint8 ReadCrtcReg(uint8 index)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3d4, index);
		return ReadPIO_8(0x3d5);
	}

	OUTREG8(0x83d4, index);
	return INREG8(0x83d5);
}


void WriteCrtcReg(uint8 index, uint8 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3d4, index);
		WritePIO_8(0x3d5, value);
	} else {
		OUTREG8(0x83d4, index);
		OUTREG8(0x83d5, value);
	}
}


void WriteCrtcReg(uint8 index, uint8 value, uint8 mask)
{
	// Write a value to a CRTC reg using a mask.  The mask selects the
	// bits to be modified.

	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3d4, index);
		WritePIO_8(0x3d5, (ReadPIO_8(0x3d5) & ~mask) | (value & mask));
	} else {
		OUTREG8(0x83d4, index);
		OUTREG8(0x83d5, (INREG8(0x83d5) & ~mask) | (value & mask));
	}
}


// Functions to read/write Sequence registers.
//============================================

uint8 ReadSeqReg(uint8 index)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3c4, index);
		return ReadPIO_8(0x3c5);
	}

	OUTREG8(0x83c4, index);
	return INREG8(0x83c5);
}


void WriteSeqReg(uint8 index, uint8 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3c4, index);
		WritePIO_8(0x3c5, value);
	} else {
		OUTREG8(0x83c4, index);
		OUTREG8(0x83c5, value);
	}
}


void WriteSeqReg(uint8 index, uint8 value, uint8 mask)
{
	// Write a value to a Sequencer reg using a mask.  The mask selects the
	// bits to be modified.

	if (gInfo.sharedInfo->chipType == S3_TRIO64) {
		WritePIO_8(0x3c4, index);
		WritePIO_8(0x3c5, (ReadPIO_8(0x3c5) & ~mask) | (value & mask));
	} else {
		OUTREG8(0x83c4, index);
		OUTREG8(0x83c5, (INREG8(0x83c5) & ~mask) | (value & mask));
	}
}


// Functions to read/write the misc output register.
//==================================================

uint8 ReadMiscOutReg()
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		return ReadPIO_8(0x3cc);

	return INREG8(0x83cc);
}


void WriteMiscOutReg(uint8 value)
{
	if (gInfo.sharedInfo->chipType == S3_TRIO64)
		WritePIO_8(0x3c2, value);
	else
		OUTREG8(0x83c2, value);
}


void WriteIndexedColor(uint8 index, uint8 red, uint8 green, uint8 blue)
{
	// Write an indexed color.  Argument index is the index (0-255) of the
	// color, and arguments red, green, & blue are the components of the color.

	// Note that although the Trio64V+ chip supports MMIO in nearly all areas,
	// it does not support MMIO for setting indexed colors;  thus, use PIO to
	// set the indexed color.

	if (gInfo.sharedInfo->chipType == S3_TRIO64
			|| gInfo.sharedInfo->chipType == S3_TRIO64_VP) {
		WritePIO_8(0x3c8, index);	// color index
		WritePIO_8(0x3c9, red);
		WritePIO_8(0x3c9, green);
		WritePIO_8(0x3c9, blue);
	} else {
		OUTREG8(0x83c8, index);		// color index
		OUTREG8(0x83c9, red);
		OUTREG8(0x83c9, green);
		OUTREG8(0x83c9, blue);
	}
}
