/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_MII_BUS_H_
#define _USB_MII_BUS_H_


#include "Driver.h"


enum MII_Register {
	MII_BMCR	= 0x00,
	MII_BMSR	= 0x01,
	MII_PHYID0	= 0x02,
	MII_PHYID1	= 0x03,
	MII_ANAR	= 0x04,
	MII_ANLPAR	= 0x05,
	MII_ANER	= 0x06
};


enum MII_BMCR {
	BMCR_Reset			= 0x8000,
	BMCR_Loopback		= 0x4000,
	BMCR_SpeedSelection	= 0x2000,
	BMCR_ANegEnabled	= 0x1000,
	BMCR_PowerDown		= 0x0800,
	BMCR_Isolate		= 0x0400,
	BMCR_ANegRestart	= 0x0200,
	BMCR_FullDuplex		= 0x0100,
	BMCR_CollTest		= 0x0080
};


enum MII_BMSR {
	BMSR_CAP_100BASE_T4		= 0x8000,	// PHY is able to perform 100base-T4 
	BMSR_CAP_100BASE_TXFD	= 0x4000,	// PHY is able to perform 100base-TX FD
	BMSR_CAP_100BASE_TXHD	= 0x2000,	// PHY is able to perform 100base-TX HD
	BMSR_CAP_10BASE_TXFD	= 0x1000,	// PHY is able to perform 10base-TX FD
	BMSR_CAP_10BASE_TXHD	= 0x0800,	// PHY is able to perform 10base-TX HD
	BMSR_MFPS				= 0x0040,	// Management frame preamble supression
	BMSR_ANC				= 0x0020,	// Auto-negotiation complete
	BMSR_RF					= 0x0010,	// Remote fault
	BMSR_CAP_AN				= 0x0008,	// PHY is able to perform a-negotiation
	BMSR_Link				= 0x0004,	// link state
	BMSR_Jabber				= 0x0002,	// Jabber condition detected
	BMSR_CAP_Ext			= 0x0001	// Extended register capable
};


enum MII_ANAR {
	ANAR_NP			= 0x8000,	// Next page available
	ANAR_ACK		= 0x4000,	// Link partner data reception ability ack-ed
	ANAR_RF			= 0x2000,	// Fault condition detected and advertised
	ANAR_PAUSE		= 0x0400,	// Pause operation enabled for full-duplex links
	ANAR_T4			= 0x0200,	// 100BASE-T4 supported
	ANAR_TX_FD		= 0x0100,	// 100BASE-TX full duplex supported
	ANAR_TX_HD		= 0x0080,	// 100BASE-TX half duplex supported
	ANAR_10_FD		= 0x0040,	// 10BASE-TX full duplex supported
	ANAR_10_HD		= 0x0020,	// 10BASE-TX half duplex supported
	ANAR_SELECTOR	= 0x0001	// Protocol sel. bits (hardcoded to ethernet)
};


enum MII_ANLPAR {
	ANLPAR_NP		= 0x8000,	// Link partner next page enabled
	ANLPAR_ACK		= 0x4000,	// Link partner data reception ability ack-ed
	ANLPAR_RF		= 0x2000,	// Remote fault indicated by link partner
	ANLPAR_PAUSE	= 0x0400,	// Pause operation supported by link partner
	ANLPAR_T4		= 0x0200,	// 100BASE-T4 supported by link partner
	ANLPAR_TX_FD	= 0x0100,	// 100BASE-TX FD supported by link partner
	ANLPAR_TX_HD	= 0x0080,	// 100BASE-TX HD supported by link partner
	ANLPAR_10_FD	= 0x0040,	// 10BASE-TX FD supported by link partner
	ANLPAR_10_HD	= 0x0020,	// 10BASE-TX HD supported by link partner
	ANLPAR_SELECTOR	= 0x0001	// Link partner's bin. encoded protocol selector
};


// index used to different PHY on MII bus
enum PHYIndex {
	CurrentPHY	 = -1,	// currently selected PHY.
						// Internally used as def. index in case no PHYs found.
	SecondaryPHY =  0,	// secondary PHY
	PrimaryPHY	 =  1,	// primary PHY
	PHYsCount	 =  2	// maximal count of PHYs on bus
};


// PHY type and id constants and masks.
enum PHYType {
	PHYTypeMask		= 0xe0,	// mask for PHY type bits
	PHYNormal		= 0x00,	// 10/100 Ethernet PHY (Link reports as normal case)
	PHYLinkAState	= 0x80,	// Special case 1 (Link reports is always active)
	PHYGigabit		= 0x20,	// Gigabit Ethernet PHY on AX88178
	PHYNotInstalled	= 0xe0,	// non-supported PHY

	PHYIDMask		= 0x1f,	// mask for PHY ID bits
	PHYIDEmbedded	= 0x10	// id for embedded PHY on AX88772
};


class MIIBus {
public:
							MIIBus();

		status_t			Init(usb_device device);
		status_t			InitCheck();

		status_t			SetupPHY();

		uint8				PHYID(PHYIndex phyIndex = CurrentPHY);
		uint8				PHYType(PHYIndex phyIndex = CurrentPHY);
		PHYIndex			ActivePHY() { return fSelectedPHY; }

		status_t			Read(uint16 miiRegister, uint16 *value,
								PHYIndex phyIndex = CurrentPHY);
		status_t			Write(uint16 miiRegister, uint16 value,
								PHYIndex phyIndex = CurrentPHY);

		status_t			Status(uint16 *status,
								PHYIndex phyIndex = CurrentPHY);
		status_t			Dump();

private:
		status_t			fStatus;
		usb_device			fDevice;
		uint8				fPHYs[PHYsCount];
		PHYIndex			fSelectedPHY;
};

#endif // _USB_MII_BUS_H_

