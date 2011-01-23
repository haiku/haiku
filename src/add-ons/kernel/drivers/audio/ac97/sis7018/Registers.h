/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Copyright for ali5451 support:
 *		(c) 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 */
#ifndef _SiS7018_REGISTERS_H_
#define _SiS7018_REGISTERS_H_


enum Registers {
	// shared registers
	RegCodecWrite	= 0x40,
	RegCodecRead	= 0x44,
	RegCodecStatus	= 0x48,
		CodecStatusSBCtrl	= 0x40,
		CodecStatusActive	= 0x20,
		CodecStatusReady	= 0x10,
		CodecStatusADCON	= 0x08,
		CodecStatusDACON	= 0x02,
		CodecStatusReset	= 0x01,
		CodecTimeout		= 0xffff,

	RegRecChannel	= 0x70,
	RegStartA		= 0x80,
	RegStopA		= 0x84,
	RegCurSPFA		= 0x90,
	RegAddrINTA		= 0x98,
	RegChIndex		= 0xa0,
		ChIndexMask		= 0x0000003f,
		ChIndexEndEna	= 0x00001000,
		ChIndexMidEna	= 0x00002000,
	RegEnaINTA		= 0xa4,
	RegMiscINT		= 0xb0,
	RegStartB		= 0xb4,
	RegStopB		= 0xb8,
	RegCurSPFB		= 0xbc,
	RegAddrINTB		= 0xd8,
	RegEnaINTB		= 0xdc,
	RegCSOAlphaFMS	= 0xe0,
	RegLBA_PPTR		= 0xe4,
	RegDeltaESO		= 0xe8,
	RegRVolCVolFMC	= 0xec,
	RegGVSelVolCtrl	= 0xf0,
	RegEBuf1		= 0xf4,
	RegEBuf2		= 0xf8,


	// SiS7018 specific registers
	RegSiSCodecWrite	= RegCodecWrite,
	RegSiSCodecRead		= RegCodecRead,
	RegSiSCodecStatus	= RegCodecStatus,
		SiSCodecResetOff	= 0x000f0000,
	RegSiSCodecGPIO		= 0x4c,
		ChIndexSiSEnaB	= 0x00010000,

	// ALi5451 registers
	RegALiVolumeA		= 0xa8,
	RegALiMPUR2			= 0x22,
	RegALiSTimer		= 0xc8,
	RegALiDigiMixer		= 0xd4,
		ALiDigiMixerPCMIn	= 0x80000000,

	// Trident NX specific registers
	RegNXCodecStatus	= 0x40,
		NXCodecStatusReady2	= 0x40,
		NXCodecStatusADC2ON	= 0x20,
		NXCodecStatusDAC2ON	= 0x10,
		NXCodecStatusReady1	= 0x08,
		NXCodecStatusADC1ON	= 0x04,
		NXCodecStatusDAC1ON	= 0x02,
		NXCodecStatusReset	= 0x01,
	RegNXCodecWrite	= 0x44,
	RegNXCodecRead	= 0x48,
	RegNXCodecRead2	= 0x4c
};


#endif // _SIS7018_REGISTERS_H_

