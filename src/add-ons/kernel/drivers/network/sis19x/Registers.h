/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS19X_REGISTERS_H_
#define _SiS19X_REGISTERS_H_


// Symbolic offset to registers
enum SiS19XRegisters {
	TxControl		= 0x00,	// Tx Host Control / Status
	TxBase			= 0x04,	// Tx Home Descriptor Base
	TxReserved		= 0x08,	// Reserved
	TxStatus		= 0x0c,	// Tx Next Descriptor Control / Status
	RxControl		= 0x10,	// Rx Host Control / Status
	RxBase			= 0x14,	// Rx Home Descriptor Base
	RxReserved		= 0x18,	// Reserved
	RxStatus		= 0x1c,	// Rx Next Descriptor Control / Status
	IntSource		= 0x20,	// Interrupt Source
	IntMask			= 0x24,	// Interrupt Mask
	IntControl		= 0x28,	// Interrupt Control
	IntTimer		= 0x2c,	// Interrupt Timer
	PowControl		= 0x30,	// Power Management Control / Status
	Reserved0		= 0x34,	// Reserved
	EEPROMControl	= 0x38,	// EEPROM Control / Status
	EEPROMInterface	= 0x3c,	// EEPROM Interface
	StationControl	= 0x40,	// Station Control / Status
	SMInterface		= 0x44,	// Station Management Interface
	GIoCR			= 0x48,	// GMAC IO Compensation
	GIoControl		= 0x4c,	// GMAC IO Control
	TxMACControl	= 0x50,	// Tx MAC Control
	TxLimit			= 0x54,	// Tx MAC Timer / TryLimit
	RGDelay			= 0x58,	// RGMII Tx Internal Delay Control
	Reserved1		= 0x5c,	// Reserved
	RxMACControl	= 0x60,	// Rx MAC Control
	RxMACAddress	= 0x62,	// Rx MAC Unicast Address
	RxHashTable		= 0x68,	// Rx Multicast Hash Table
	RxWOLControl	= 0x70,	// Rx WOL Control
	RxWOLData		= 0x74,	// Rx WOL Data Access
	RxMPSControl	= 0x78,	// Rx MPS Control
	Reserved2		= 0x7c	// Reserved
};


// interrupt bits for IMR/ISR registers
enum SiS19XInterruptBits {
	INT_SOFT	= 0x40000000U,
	INT_TIMER	= 0x20000000U,
	INT_PAUSEF	= 0x00080000U,
	INT_MAGICP	= 0x00040000U,
	INT_WAKEF	= 0x00020000U,
	INT_LINK	= 0x00010000U,
	INT_RXIDLE	= 0x00000080U,
	INT_RXDONE	= 0x00000040U,
	INT_TXIDLE	= 0x00000008U,
	INT_TXDONE	= 0x00000004U,
	INT_RXHALT	= 0x00000002U,
	INT_TXHALT	= 0x00000001U
};


const uint32 knownInterruptsMask = INT_LINK
								/*| INT_RXIDLE*/ | INT_RXDONE
								/*| INT_TXIDLE*/ | INT_TXDONE
								| INT_RXHALT | INT_TXHALT;


// bits for RxControl register
enum SiS19XRxControlBits {
	RxControlPoll	= 0x00000010U,
	RxControlEnable	= 0x00000001U
};


// bits for TxControl register
enum SiS19XTxControlBits {
	TxControlPoll	= 0x00000010U,
	TxControlEnable	= 0x00000001U
};


// EEPROM Addresses
enum SiS19XEEPROMAddress {
	EEPROMSignature	= 0x00,
	EEPROMClock		= 0x01,
	EEPROMInfo		= 0x02,
	EEPROMAddress	= 0x03
};


// EEPROM Interface Register
enum SiS19XEEPROMInterface {
	EIData			= 0xffff0000,
	EIDataShift		= 16,
	EIOffset		= 0x0000fc00,
	EIOffsetShift	= 10,
	EIOp			= 0x00000300,
	EIOpShift		= 8,
	EIOpRead		= (2 << EIOpShift),
	EIOpWrite		= (1 << EIOpShift),
	EIReq			= 0x00000080,
	EI_DO			= 0x00000008,
	EI_DI			= 0x00000004,
	EIClock			= 0x00000002,
	EI_CS			= 0x00000001,

	EIInvalid		= 0xffff	// used as invalid readout from EEPROM
};


// interrupt bits for Station Control registers
enum SiS19XStationControlBits {
	SC_Loopback		= 0x80000000U,
	SC_RGMII		= 0x00008000U,
	SC_FullDuplex	= 0x00001000U,
	SC_Speed		= 0x00000c00U,
	SC_SpeedShift	= 10,
	SC_Speed1000	= (3U << SC_SpeedShift),
	SC_Speed100		= (2U << SC_SpeedShift),
	SC_Speed10		= (1U << SC_SpeedShift)
};


// Station Management Interface Register
enum SiS19XSMInterface {
	SMIData			= 0xffff0000,
	SMIDataShift	= 16,
	SMIReg			= 0x0000f800,
	SMIRegShift		= 11,
	SMIPHY			= 0x000007c0,
	SMIPHYShift		= 6,
	SMIOp			= 0x00000020,
	SMIOpShift		= 5,
	SMIOpWrite		= (1 << SMIOpShift),
	SMIOpRead		= (0 << SMIOpShift),
	SMIReq			= 0x00000010,
	SMI_MDIO		= 0x00000008,
	SMI_MDDIR		= 0x00000004,
	SMI_MDC			= 0x00000002,
	SMI_MDEN		= 0x00000001
};


// transmit descriptor command bits
enum TxDescriptorCommandStatus {
	TDC_TXOWN	= 0x80000000U, // own bit
	TDC_TXINT	= 0x40000000U,
	TDC_THOL3	= 0x30000000U,
	TDC_THOL2	= 0x20000000U,
	TDC_THOL1	= 0x10000000U,
	TDC_THOL0	= 0x00000000U,
	TDC_LSEN	= 0x08000000U,
	TDC_IPCS	= 0x04000000U,
	TDC_TCPCS	= 0x02000000U,
	TDC_UDPCS	= 0x01000000U,
	TDC_BSTEN	= 0x00800000U,
	TDC_EXTEN	= 0x00400000U,
	TDC_DEFEN	= 0x00200000U,
	TDC_BKFEN	= 0x00100000U,
	TDC_CRSEN	= 0x00080000U,
	TDC_COLSEN	= 0x00040000U,
	TDC_CRCEN	= 0x00020000U,
	TDC_PADEN	= 0x00010000U,
	// following bits are set/filled by hardware?
	TDS_OWC		= 0x00080000U,
	TDS_ABT		= 0x00040000U,
	TDS_FIFO	= 0x00020000U,
	TDS_CRS		= 0x00010000U,
	TDS_COLLS	= 0x0000ffffU
};


const uint32 txErrorStatusBits = TDS_OWC | TDS_ABT | TDS_FIFO | TDS_CRS;
const uint32 TxDescriptorEOD = 0x80000000U;
const uint32 TxDescriptorSize = 0x0000ffffU;


struct TxDescriptor {
	uint32		fPacketSize;
	uint32		fCommandStatus;
	uint32		fBufferPointer;
	uint32		fEOD;
	
	void		Init(phys_addr_t bufferPointer, bool bEOD) volatile {
		fPacketSize = 0;
		fCommandStatus = 0;
		fBufferPointer = (uint32)bufferPointer;
		fEOD = bEOD ? TxDescriptorEOD : 0;
	}
};


// receive descriptor information bits
enum RxDescriptorInformation {
	RDI_RXOWN	= 0x80000000U,
	RDI_RXINT	= 0x40000000U,
	RDI_IPON	= 0x20000000U,
	RDI_TCPON	= 0x10000000U,
	RDI_UDPON	= 0x08000000U,
	RDI_WAKUP	= 0x00400000U,
	RDI_MAGIC	= 0x00200000U,
	RDI_PAUSE	= 0x00100000U,
	RDI_CAST	= 0x000c0000U,
		RDI_CAST_SHIFT	= 18,
	RDI_BCAST	= ( 3U << RDI_CAST_SHIFT ),
	RDI_MCAST	= ( 2U << RDI_CAST_SHIFT ),
	RDI_UCAST	= ( 1U << RDI_CAST_SHIFT ),
	RDI_CRCOFF	= 0x00020000U,
	RDI_PREADD	= 0x00010000U
};


// receive descriptor status bits
enum RxDescriptorStatus {
	RDS_TAGON	= 0x80000000U,
	RDS_DESCS	= 0x3f000000U,
		RDS_DESCS_SHIFT	= 24,
	RDS_ABORT	= 0x00800000U,
	RDS_SHORT	= 0x00400000U,
	RDS_LIMIT	= 0x00200000U,
	RDS_MIIER	= 0x00100000U,
	RDS_OVRUN	= 0x00080000U,
	RDS_NIBON	= 0x00040000U,
	RDS_COLON	= 0x00020000U,
	RDS_CRCOK	= 0x00010000U,
	RDS_SIZE	= 0x0000ffffU
};


const uint32 rxErrorStatusBits = RDS_ABORT | RDS_SHORT | RDS_LIMIT
							| RDS_MIIER | RDS_OVRUN | RDS_NIBON | RDS_COLON;
const uint32 RxDescriptorEOD = 0x80000000U;
const uint32 BufferSize = 1536;

struct RxDescriptor {
	uint32		fStatusSize;
	uint32		fPacketInfo;
	uint32		fBufferPointer;
	uint32		fEOD;

	void Init(phys_addr_t bufferPointer, bool isEOD) volatile {
		fStatusSize = 0;
		fPacketInfo = RDI_RXOWN | RDI_RXINT;
		fBufferPointer =(uint32) bufferPointer;
		fEOD = isEOD ? RxDescriptorEOD : 0;
		fEOD |= (BufferSize & 0x0000fff8);
	}
};


// RxMACControl bits
enum RxMACControlBits {
	RXM_Broadcast	= 0x0800U,
	RXM_Multicast	= 0x0400U,
	RXM_Physical	= 0x0200U,
	RXM_AllPhysical	= 0x0100U,

	RXM_Mask		= RXM_Broadcast | RXM_Multicast
						| RXM_Physical | RXM_AllPhysical
};

#endif // _SiS19X_REGISTERS_H_

