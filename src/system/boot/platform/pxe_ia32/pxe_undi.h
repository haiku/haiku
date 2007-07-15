/*
 * Copyright 2006, Marcus Overhagen <marcus@overhagen.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PXE_UNDI_H
#define _PXE_UNDI_H

extern "C" uint16 call_pxe_bios(void *pxe, uint16 opcode, void *param);

#define UNDI_OPEN				0x0006
#define UNDI_TRANSMIT			0x0008
#define UNDI_GET_INFORMATION	0x000C
#define UNDI_ISR				0x0014
#define UNDI_GET_STATE			0x0015

#define	TFTP_OPEN				0x0020
#define	TFTP_CLOSE				0x0021
#define	TFTP_READ				0x0022
#define	TFTP_GET_FILE_SIZE		0x0025

#define GET_CACHED_INFO			0x0071

#define SEG(ptr)	((uint16)(((uint32)(ptr)) >> 4))
#define OFS(ptr)	((uint16)(((uint32)(ptr)) & 15))

#define MAC_ADDR_LEN 16
typedef uint8 MAC_ADDR[MAC_ADDR_LEN];

struct SEGSEL
{
	uint16 a;
};

struct SEGOFF16
{
	uint16 ofs;
	uint16 seg;
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

struct PXENV_GET_CACHED_INFO
{
	uint16	Status;
	uint16	PacketType;
	#define PXENV_PACKET_TYPE_DHCP_DISCOVER 1
	#define PXENV_PACKET_TYPE_DHCP_ACK      2
	#define PXENV_PACKET_TYPE_CACHED_REPLY  3
	uint16	BufferSize;
	SEGOFF16 Buffer;
	uint16	BufferLimit;
};

struct PXENV_UNDI_MCAST_ADDRESS
{
	#define MAXNUM_MCADDR       8
	uint16	MCastAddrCount;
	MAC_ADDR McastAddr[MAXNUM_MCADDR];
};

struct PXENV_UNDI_OPEN
{
	uint16	Status;
	uint16	OpenFlag;
	uint16	PktFilter;
	#define FLTR_DIRECTED  0x0001
	#define FLTR_BRDCST    0x0002
	#define FLTR_PRMSCS    0x0004
	#define FLTR_SRC_RTG   0x0008
	PXENV_UNDI_MCAST_ADDRESS R_Mcast_Buf;
};

struct PXENV_UNDI_GET_STATE
{
	#define PXE_UNDI_GET_STATE_STARTED     1
	#define PXE_UNDI_GET_STATE_INITIALIZED 2
	#define PXE_UNDI_GET_STATE_OPENED      3
	uint16	Status;
	uint8	UNDIstate;
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
	MAC_ADDR CurrentNodeAddress;
	MAC_ADDR PermNodeAddress;
	SEGSEL	ROMAddress;
	uint16	RxBufCt;
	uint16	TxBufCt;
};

struct PXENV_UNDI_TRANSMIT
{
	uint16	Status;
	uint8	Protocol;
	#define P_UNKNOWN	0
	#define P_IP		1
	#define P_ARP		2
	#define P_RARP		3
	uint8	XmitFlag;
	#define XMT_DESTADDR   0x0000
	#define XMT_BROADCAST  0x0001
	SEGOFF16 DestAddr;
	SEGOFF16 TBD;
	uint32 Reserved[2];
};

#define MAX_DATA_BLKS   8
struct PXENV_UNDI_TBD
{
	uint16	ImmedLength;
	SEGOFF16 Xmit;
	uint16	DataBlkCount;
	struct DataBlk
	{
		uint8	TDPtrType;
		uint8	TDRsvdByte;
		uint16	TDDataLen;
    	SEGOFF16 TDDataPtr;
	} DataBlock[MAX_DATA_BLKS];
};

struct PXENV_UNDI_ISR
{
	uint16	Status;
	uint16	FuncFlag;
	uint16	BufferLength;
	uint16	FrameLength;
	uint16	FrameHeaderLength;
	SEGOFF16 Frame;
	uint8	ProtType;
	uint8	PktType;
};

#define PXENV_UNDI_ISR_IN_START		1
#define PXENV_UNDI_ISR_IN_PROCESS	2
#define PXENV_UNDI_ISR_IN_GET_NEXT	3

/* One of these will be returned for
	PXENV_UNDI_ISR_IN_START
*/
#define PXENV_UNDI_ISR_OUT_OURS		0
#define PXENV_UNDI_USR_OUT_NOT_OURS	1

/* One of these will be returned for
	PXENV_UNDI_ISR_IN_PROCESS and
	PXENV_UNDI_ISR_IN_GET_NEXT
*/
#define PXENV_UNDI_ISR_OUT_DONE      0
#define PXENV_UNDI_ISR_OUT_TRANSMIT  2
#define PXENV_UNDI_ISR_OUT_RECEIVE   3
#define PXENV_UNDI_ISR_OUT_BUSY      4


typedef union {
	uint32	num;
	uint8	array[4];
} pxenv_ip4;

struct pxenv_tftp_open {
	uint16		status;
	pxenv_ip4	server_ip;
	pxenv_ip4	gateway_ip;
	char		file_name[128];
	uint16		port;
	uint16		packet_size;
} __attribute__((packed));

struct pxenv_tftp_close {
	uint16		status;
} __attribute__((packed));

struct pxenv_tftp_read {
	uint16		status;
	uint16		packet_number;
	uint16		buffer_size;
	SEGOFF16	buffer;
} __attribute__((packed));

struct pxenv_tftp_get_fsize {
	uint16		status;
	pxenv_ip4	server_ip;
	pxenv_ip4	gateway_ip;
	char		file_name[128];
	uint32		file_size;
} __attribute__((packed));


PXE_STRUCT * pxe_undi_find_data();

#endif
