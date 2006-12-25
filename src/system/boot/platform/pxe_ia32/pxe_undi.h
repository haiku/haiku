/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PXE_UNDI_H
#define _PXE_UNDI_H

extern "C" uint16 call_pxe_bios(void *pxe, uint16 opcode, void *param);

void pxe_undi_init();



#define UNDI_GET_INFORMATION	0x000C

struct SEGSEL
{
	uint16 a;
};

struct SEGOFF16
{
	uint16 seg;
	uint16 ofs;
};

struct SEGDESC
{
	uint64 a;
};

struct PXE_STRUCT
{
	uint32	Signature;
	uint8	StructLength;
	uint8	StructCksum;
	uint8	StructRev;
	uint8	reserved_1;
  	SEGOFF16 UNDIROMID;
	SEGOFF16 BaseROMID;
	SEGOFF16 EntryPointSP;
	SEGOFF16 EntryPointESP;
	SEGOFF16 StatusCallout;
	uint8	reserved_2;
	uint8	SegDescCnt;
	SEGSEL	FirstSelector;
	SEGDESC	Stack;
	SEGDESC	UNDIData;
	SEGDESC	UNDICode;
	SEGDESC	UNDICodeWrite;
	SEGDESC	BC_Data;
	SEGDESC	BC_Code;
	SEGDESC	BC_CodeWrite;
};

struct PXENV_UNDI_GET_INFORMATION
{
	uint16	Status;
	uint16	BaseIo;
	uint16	IntNumber;
	uint16	MaxTranUnit;
	uint16	HwType;
	#define ETHER_TYPE      1
	#define EXP_ETHER_TYPE	2
	#define IEEE_TYPE       6
	#define ARCNET_TYPE     7
	uint16	HwAddrLen;
	uint8	CurrentNodeAddress[16];
	uint8	PermNodeAddress[16];
	SEGSEL	ROMAddress;
	uint16	RxBufCt;
	uint16	TxBufCt;
};


#endif
