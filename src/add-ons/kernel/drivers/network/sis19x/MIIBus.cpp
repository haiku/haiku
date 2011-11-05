/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "MIIBus.h"

#include <net/if_media.h>

#include "Driver.h"
#include "Settings.h"
#include "Device.h"
#include "Registers.h"


#define MII_OUI(id)		((id >> 10)	& 0xffff)
#define MII_MODEL(id)	((id >> 4)	& 0x003f)
#define MII_REV(id)		((id)		& 0x000f)

#define ISVALID(__address) ((__address) < 32)

// the marker for not initialized or currently selected PHY address
const uint8 NotInitPHY	= 0xff;

// composite ids of PHYs suported by this driver
const uint32 BroadcomBCM5461	= CARDID(0x0020, 0x60c0);
const uint32 BroadcomAC131		= CARDID(0x0143, 0xbc70);
const uint32 AgereET1101B		= CARDID(0x0282, 0xf010);
const uint32 Atheros			= CARDID(0x004d, 0xd010);
const uint32 AtherosAR8012		= CARDID(0x004d, 0xd020);
const uint32 RealtekRTL8201		= CARDID(0x0000, 0x8200);
const uint32 Marvell88E1111		= CARDID(0x0141, 0x0cc0);
const uint32 UnknownPHY			= CARDID(0x0000, 0x0000);

MIIBus::ChipInfo miiChipTable[] = {
	{ BroadcomBCM5461,	MIIBus::PHYLAN, "Broadcom BCM5461" },
	{ BroadcomAC131,	MIIBus::PHYLAN, "Broadcom AC131" },
	{ AgereET1101B,		MIIBus::PHYLAN, "Agere ET1101B" },
	{ Atheros,			MIIBus::PHYLAN, "Atheros" },
	{ AtherosAR8012,	MIIBus::PHYLAN, "Atheros AR8012" },
	{ RealtekRTL8201,	MIIBus::PHYLAN, "Realtek RTL8201" },
	{ Marvell88E1111,	MIIBus::PHYLAN, "Marvell 88E1111" },
	// unknown one must be the terminating entry!
	{ UnknownPHY,		MIIBus::PHYUnknown, "Unknown PHY" }
};


MIIBus::MIIBus(Device* device)
		:
		fDevice(device),
		fSelectedPHY(NotInitPHY),
		fGigagbitCapable(false),
		fHasRGMII(false)
{
	memset(&fLinkState, 0, sizeof(fLinkState));
}


status_t
MIIBus::Init()
{
	// reset to default state
	fPHYs.MakeEmpty();

	// iterate through all possible MII addresses
	for (uint8 addr = 0; ISVALID(addr); addr++) {
		uint16 miiStatus = _Read(MII_BMSR, addr);

		if (miiStatus == 0xffff || miiStatus == 0)
			continue;

		uint32 Id = CARDID(_Read(MII_PHYID0, addr), _Read(MII_PHYID1, addr));

		TRACE("MII Info(addr:%d,id:%#010x): OUI:%04x; Model:%04x; rev:%02x.\n",
				addr, Id, MII_OUI(Id), MII_MODEL(Id), MII_REV(Id));

		for (size_t i = 0; i < _countof(miiChipTable); i++){
			ChipInfo& info = miiChipTable[i];

			if (info.fId != UnknownPHY && info.fId != (Id & 0xfffffff0))
				continue;

			fPHYs.Put(addr, info);

			break;
		}
	}
	
	if (fPHYs.IsEmpty()) {
		TRACE_ALWAYS("No PHYs found.\n");
		return B_ENTRY_NOT_FOUND;
	}

	// select appropriate PHY
	Select();

	// Marvell 88E1111 requires extra initialization
	if (fPHYs.Get(fSelectedPHY).fId == Marvell88E1111) {
		_Write(0x1b, (fHasRGMII ? 0x808b : 0x808f), fSelectedPHY);
		spin(200);
		_Write(0x14, (fHasRGMII ? 0x0ce1 : 0x0c60), fSelectedPHY);
		spin(200);
	}

	// some chips require reset
	Reset();

	return B_OK;
}


status_t
MIIBus::Select(uint16* currentStatus /*= NULL*/)
{
	if (fPHYs.IsEmpty()) {
		TRACE_ALWAYS("Error: No PHYs found or available.\n");
		return B_ENTRY_NOT_FOUND;
	}

	uint8 lanPHY	= NotInitPHY;
	uint8 homePHY	= NotInitPHY;
	fSelectedPHY	= NotInitPHY;

	for (ChipInfoMap::Iterator i = fPHYs.Begin(); i != fPHYs.End(); i++) {
		uint8 address	= i->Key();
		ChipInfo& info	= i->Value();

		uint16 status = _Status(address);

		if ((status & BMSR_Link) && !ISVALID(fSelectedPHY)
								&& (info.fType != PHYUnknown))
		{
			fSelectedPHY = address;
		} else {
			uint16 control = _Read(MII_BMCR, address);
			control |= BMCR_Isolate | BMCR_ANegEnabled;
			_Write(MII_BMCR, control, address);

			if (info.fType == PHYLAN)
				lanPHY = address;
			if (info.fType == PHYHome)
				homePHY = address;
		}
	}

	if (!ISVALID(fSelectedPHY)) {
		if (ISVALID(homePHY))
			fSelectedPHY = homePHY;
		else if (ISVALID(lanPHY))
			fSelectedPHY = lanPHY;
		else
			fSelectedPHY = fPHYs.Begin()->Key();
	}

	uint16 control = _Read(MII_BMCR, fSelectedPHY);
	control &= ~BMCR_Isolate;
	_Write(MII_BMCR, control, fSelectedPHY);

//	TRACE("Selected PHY:%s\n", fPHYs.Get(fSelectedPHY).fName);

	if (currentStatus != NULL) {
		*currentStatus = _Status(fSelectedPHY);
	}

	return B_OK;
}


status_t
MIIBus::Reset(uint16* currentStatus /*=NULL*/)
{
	if (fPHYs.IsEmpty()) {
		TRACE_ALWAYS("Error: No PHYs found or available.\n");
		return B_ENTRY_NOT_FOUND;
	}

	uint16 status = _Status(fSelectedPHY);
	_Write(MII_BMCR, BMCR_Reset | BMCR_ANegEnabled | BMCR_ANegRestart, fSelectedPHY);

	if (currentStatus != NULL)
		*currentStatus = status;

	return B_OK;
}


void
MIIBus::_ControlSMInterface(uint32 control)
{
	fDevice->WritePCI32(SMInterface, control);
	spin(10);

	for (size_t i = 0; i < 1000; i++) {
		if ((fDevice->ReadPCI32(SMInterface) & SMIReq) == 0) {
			return;
		}
		spin(10);
	}

	TRACE_ALWAYS("Timeout writing SMI control.\n");
}


uint16
MIIBus::_Read(uint16 miiRegister, uint32 phyAddress)
{
	uint32 control = SMIOpRead | SMIReq;
	control |= phyAddress << SMIPHYShift;
	control |= miiRegister << SMIRegShift;

	_ControlSMInterface(control);

	return (fDevice->ReadPCI32(SMInterface) & SMIData) >> SMIDataShift;
}


status_t
MIIBus::Read(uint16 miiRegister, uint16 *value)
{
	if (fSelectedPHY >= 32) {
		TRACE_ALWAYS("Error: MII is not ready\n");
		return B_ENTRY_NOT_FOUND;
	}

	*value = _Read(miiRegister, fSelectedPHY);

	return B_OK;
}


void
MIIBus::_Write(uint16 miiRegister, uint16 value, uint32 phyAddress)
{
	uint32 control = SMIOpWrite | SMIReq;
	control |= phyAddress << SMIPHYShift;
	control |= miiRegister << SMIRegShift;
	control |= value << SMIDataShift;

	_ControlSMInterface(control);
}


status_t
MIIBus::Write(uint16 miiRegister, uint16 value)
{
	if (fSelectedPHY >= 32) {
		TRACE_ALWAYS("Error: MII is not ready\n");
		return B_ENTRY_NOT_FOUND;
	}
	
	_Write(miiRegister, value, fSelectedPHY);

	return B_OK;
}


status_t
MIIBus::Status(uint16 *status)
{
	return Read(MII_BMSR, status);
}


uint16
MIIBus::_Status(uint8 phyAddress)
{
			_Read(MII_BMSR, phyAddress);
	return	_Read(MII_BMSR, phyAddress);
}


bool
MIIBus::IsLinkUp()
{
	return (_Status(fSelectedPHY) & BMSR_Link) != 0;
}


uint32
MIIBus::TimerHandler(bool* linkChanged)
{
	// XXX ?
	/*if (!fNegotiationComplete) {
		_UpdateLinkState();
		if ((fLinkState.media & IFM_ACTIVE) != 0) {
			_SetMedia();
		}
		return 0;
	}*/

	if ((fLinkState.media & IFM_ACTIVE) == 0) {
		Select();
		if ((_Status(fSelectedPHY) & BMSR_Link) != 0) {
			UpdateLinkState();
			SetMedia();
			if (fHasRGMII) {
				if (fPHYs.Get(fSelectedPHY).fId == BroadcomBCM5461) {
					_Write(0x18, 0xf1c7, fSelectedPHY);
					spin(200);
					_Write(0x1c, 0x8c00, fSelectedPHY);
				}

				fDevice->WritePCI32(RGDelay, 0x0441);
				fDevice->WritePCI32(RGDelay, 0x0440);
			}
			// start Rx
			uint32 control = fDevice->ReadPCI32(RxControl);
			control |= 0x00000010;
			fDevice->WritePCI32(RxControl, control);

			*linkChanged = true;
		}
	} else {
		if ((_Status(fSelectedPHY) & BMSR_Link) == 0) {
			// stop Rx
			uint32 control = fDevice->ReadPCI32(RxControl);
			control &= ~(0x00000010);
			fDevice->WritePCI32(RxControl, control);
			UpdateLinkState();
			
			*linkChanged = true;
		}
	}

	//if (*linkChanged) {
	//	TRACE_FLOW("Medium state: %s, %lld MBit/s, %s duplex.\n",
	//			(fLinkState.media & IFM_ACTIVE) ? "active" : "inactive",
	//			fLinkState.speed / 1000,
	//			(fLinkState.media & IFM_FULL_DUPLEX) ? "full" : "half");
	//}

	return 0;
}


status_t
MIIBus::UpdateLinkState(ether_link_state* state /*=NULL*/)
{
	if (state == NULL) {
		state = &fLinkState;
	}

	state->quality	= 1000;
	state->speed	= 0;
	state->media	= IFM_ETHER;

	uint16 status = _Status(fSelectedPHY);

	if ((status & BMSR_Link) == 0) {
		return B_OK;
	}

	state->speed	 = 10000;
	state->media	|= IFM_ACTIVE;

	uint16 regAnar	 = _Read(MII_ANAR,	 fSelectedPHY);
	uint16 regAnlpar = _Read(MII_ANLPAR, fSelectedPHY);
	uint16 regAner	 = _Read(MII_ANER,	 fSelectedPHY);

	if (fGigagbitCapable && (regAnlpar & ANAR_NP) && (regAner & 0x0001)) {
		uint16 regGAnar		= _Read(MII_GANAR, fSelectedPHY);
		uint16 regGAnlpar	= _Read(MII_GANLPAR, fSelectedPHY);
		
		status = regGAnar & (regGAnlpar >> 2);
		if (status & 0x0200) {
			state->speed	 = 1000000;
			state->media	|= IFM_FULL_DUPLEX;

		} else if (status & 0x0100){
			state->speed	 = 1000000;
			state->media	|= IFM_HALF_DUPLEX;

		} else {
			state->media	|= IFM_HALF_DUPLEX;
		}

	} else {
		status = regAnar & regAnlpar;
		
		if (status & (ANAR_TX_HD | ANAR_TX_FD))
			state->speed = 100000;
		if (status & (ANAR_TX_FD | ANAR_10_FD))
			state->media |= IFM_FULL_DUPLEX;
		else
			state->media |= IFM_HALF_DUPLEX;
	}

	switch(state->speed) {
		case 10000:		state->media |= IFM_10_T;	break;
		case 100000:	state->media |= IFM_100_TX;	break;
		case 1000000:	state->media |= IFM_1000_T;	break;
	}

//	fNegotiationComplete = true;
	return B_OK;
}


status_t
MIIBus::SetMedia(ether_link_state* state /*=NULL*/)
{
	if (state == NULL) {
		state = &fLinkState;
	}

	uint32 control = fDevice->ReadPCI32(StationControl);

	control &= ~(0x0f000000 | SC_FullDuplex | SC_Speed);

	switch(state->speed) {
		case 1000000:
			control |= (SC_Speed1000 | (0x3 << 24) | (0x1 << 26));
			break;
		case 100000:
			control |= (SC_Speed100 | (0x1 << 26));
			break;
		case 10000:
			control |= (SC_Speed10 | (0x1 << 26));
			break;
		default:
			TRACE_ALWAYS("Unsupported linkspeed:%d\n", state->speed);
			break;
	}

	if ((state->media & IFM_FULL_DUPLEX) != 0) {
		control |= SC_FullDuplex;
	}

	if (fHasRGMII) {
		if (fPHYs.Get(fSelectedPHY).fId == BroadcomBCM5461) {
			_Write(0x18, 0xf1c7, fSelectedPHY);
			spin(200);
			_Write(0x1c, 0x8c00, fSelectedPHY);
		}

		control |= (0x3 << 24);
	}

	fDevice->WritePCI32(StationControl, control);

	return B_OK;
}

