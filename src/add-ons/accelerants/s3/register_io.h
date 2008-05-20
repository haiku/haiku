/*
	Copyright 2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2008
*/

#ifndef __REGISTER_IO_H__
#define __REGISTER_IO_H__


// PIO address of various VGA registers.  If accessing these registers using
// MMIO, add 0x8000 to thses addresses.

#define VGA_ENABLE		0x3c3
#define MISC_OUT_R		0x3cc		// read
#define MISC_OUT_W		0x3c2		// write
#define CRTC_INDEX		0x3d4
#define CRTC_DATA		0x3d5
#define SEQ_INDEX		0x3c4
#define SEQ_DATA		0x3c5


// Prototypes of I/O functions for accessing registers using PIO.

uint32	ReadPIO(uint32 addr, uint8 numBytes);
void	WritePIO(uint32 addr, uint8 numBytes, uint32 value);

inline uint8 ReadPIO_8(uint32 addr)	{ return ReadPIO(addr, 1); }
inline void WritePIO_8(uint32 addr, uint8 value) { WritePIO(addr, 1, value); }


// Prototypes of I/O functions for accessing registers using PIO or MMIO
// depending upon the type of S3 chip.

uint8  ReadReg8(uint32 addr);
uint16 ReadReg16(uint32 addr);
uint32 ReadReg32(uint32 addr);

void   WriteReg8(uint32 addr, uint8 value);
void   WriteReg16(uint32 addr, uint16 value);
void   WriteReg32(uint32 addr, uint32 value);

uint8  ReadCrtcReg(uint8 index);
void   WriteCrtcReg(uint8 index, uint8 value);
void   WriteCrtcReg(uint8 index, uint8 value, uint8 mask);

uint8  ReadSeqReg(uint8 index);
void   WriteSeqReg(uint8 index, uint8 value);
void   WriteSeqReg(uint8 index, uint8 value, uint8 mask);

uint8  ReadMiscOutReg();
void   WriteMiscOutReg(uint8 value);

void   WriteIndexedColor(uint8 index, uint8 red, uint8 green, uint8 blue);


#endif // __REGISTER_IO_H__
