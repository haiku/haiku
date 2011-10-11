/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *	
 */
#ifndef _SiS19X_MII_BUS_H_
#define _SiS19X_MII_BUS_H_


#include <ether_driver.h>
#include <util/VectorMap.h>

#include "Driver.h"


enum MII_Register {
	MII_BMCR	= 0x00,
	MII_BMSR	= 0x01,
	MII_PHYID0	= 0x02,
	MII_PHYID1	= 0x03,
	MII_ANAR	= 0x04,
	MII_ANLPAR	= 0x05,
	MII_ANER	= 0x06,
	MII_GANAR	= 0x09,
	MII_GANLPAR	= 0x0a
};


enum MII_BMCR {
	BMCR_FullDuplex		= 0x0100,
	BMCR_ANegRestart	= 0x0200,
	BMCR_Isolate		= 0x0400,
	BMCR_PowerDown		= 0x0800,
	BMCR_ANegEnabled	= 0x1000,
	BMCR_SpeedSelection	= 0x2000,
	BMCR_Loopback		= 0x4000,
	BMCR_Reset			= 0x8000
};


enum MII_BMSR {
	BMSR_CAP_100BASE_T4		= 0x8000, // PHY is able to perform 100base-T4
	BMSR_CAP_100BASE_TXFD	= 0x4000, // PHY is able to perform 100base-TX full duplex 
	BMSR_CAP_100BASE_TXHD	= 0x2000, // PHY is able to perform 100base-TX half duplex 
	BMSR_CAP_10BASE_TXFD	= 0x1000, // PHY is able to perform 10base-TX full duplex  
	BMSR_CAP_10BASE_TXHD	= 0x0800, // PHY is able to perform 10base-TX half duplex  
	BMSR_MFPS				= 0x0040, // Management frame preamble supression
	BMSR_ANC				= 0x0020, // Auto-negotiation complete
	BMSR_RF					= 0x0010, // Remote fault
	BMSR_CAP_AN				= 0x0008, // PHY is able to perform auto-negotiation 
	BMSR_Link				= 0x0004, // link state
	BMSR_Jabber				= 0x0002, // Jabber condition detected
	BMSR_CAP_Ext			= 0x0001  // Extended register capable
};


enum MII_ANAR {
	ANAR_NP			= 0x8000, // Next page available
	ANAR_ACK		= 0x4000, // Link partner data reception ability acknowledged
	ANAR_RF			= 0x2000, // Fault condition detected and advertised
	ANAR_PAUSE		= 0x0400, // Pause operation enabled for full-duplex links
	ANAR_T4			= 0x0200, // 100BASE-T4 supported
	ANAR_TX_FD		= 0x0100, // 100BASE-TX full duplex supported
	ANAR_TX_HD		= 0x0080, // 100BASE-TX half duplex supported
	ANAR_10_FD		= 0x0040, // 10BASE-TX full duplex supported
	ANAR_10_HD		= 0x0020, // 10BASE-TX half duplex supported
	ANAR_SELECTOR	= 0x0001  // Protocol selection bits (hardcoded to ethernet)
};


enum MII_ANLPAR {
	ANLPAR_NP		= 0x8000, // Link partner next page enabled
	ANLPAR_ACK		= 0x4000, // Link partner data reception ability acknowledged
	ANLPAR_RF		= 0x2000, // Remote fault indicated by link partner
	ANLPAR_PAUSE	= 0x0400, // Pause operation supported by link partner
	ANLPAR_T4		= 0x0200, // 100BASE-T4 supported by link partner
	ANLPAR_TX_FD	= 0x0100, // 100BASE-TX full duplex supported by link partner
	ANLPAR_TX_HD	= 0x0080, // 100BASE-TX half duplex supported by link partner
	ANLPAR_10_FD	= 0x0040, // 10BASE-TX full duplex supported by link partner
	ANLPAR_10_HD	= 0x0020, // 10BASE-TX half duplex supported by link partner
	ANLPAR_SELECTOR	= 0x0001  // Link partner's binary encoded protocol selector
};


class Device;

class MIIBus {
public:
		enum Type {	
			PHYUnknown	= 0,
			PHYHome		= 1,
			PHYLAN		= 2,
			PHYMix		= 3
		};

		struct ChipInfo {
			uint32		fId;
			Type		fType;
			const char*	fName;
		};

		typedef VectorMap<uint8, ChipInfo> ChipInfoMap;
		
						MIIBus(Device* device);

		status_t		Init();
		status_t		InitCheck();

		status_t		Read(uint16 miiRegister, uint16 *value);
		status_t		Write(uint16 miiRegister, uint16 value);

		status_t		Status(uint16 *status);
		status_t		Select(uint16* status = NULL);
		status_t		Reset(uint16* status = NULL);
		uint32			TimerHandler(bool* linkChanged);

		bool			IsLinkUp();
		status_t		UpdateLinkState(ether_link_state* state = NULL);
		status_t		SetMedia(ether_link_state* state = NULL);

		bool			IsGigagbitCapable() { return fGigagbitCapable; }
		void			SetGigagbitCapable(bool on) { fGigagbitCapable = on; }

		bool			HasRGMII() { return fHasRGMII; }
		void			SetRGMII(bool on) { fHasRGMII = on; }

	const	ether_link_state&	LinkState() const { return fLinkState; }

private:
		uint16			_Status(uint8 phyAddress);
		void			_ControlSMInterface(uint32 control);
		uint16			_Read(uint16 miiRegister, uint32 phyAddress);
		void			_Write(uint16 miiRegister, uint16 value, uint32 phyAddress);

		Device*			fDevice;
		uint8			fSelectedPHY;
		ChipInfoMap		fPHYs;

		bool			fGigagbitCapable;
		bool			fHasRGMII;

		ether_link_state	fLinkState;
};

#endif //_SiS19X_MII_BUS_H_

