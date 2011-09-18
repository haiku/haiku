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


#include "AX88772Device.h"

#include <net/if_media.h>

#include "ASIXVendorRequests.h"
#include "Settings.h"


// Most of vendor requests for all supported chip types use the same
// constants (see ASIXVendorRequests.h) but the layout of request data
// may be slightly diferrent for specific chip type. Below is a quick
// reference for AX88772 vendor requests data layout.

// READ_RXTX_SRAM,		//C002_AA0B_0C00_0800 Rx/Tx SRAM Read
// WRITE_RXTX_SRAM,		//4003_AA0B_0C00_0800 Rx/Tx SRAM Write
// SW_MII_OP,			//4006_0000_0000_0000 SW Serial Management Control
// READ_MII,			//c007_aa00_cc00_0200 PHY Read
// WRITE_MII,			//4008_aa00_cc00_0200 PHY Write
// READ_MII_OP_MODE,	//c009_0000_0000_0100 Serial Management Status
// HW_MII_OP,			//400a_0000_0000_0000 HW Serial Management Control
// READ_SROM,			//C00B_AA00_0000_0200 SROM Read
// WRITE_SROM,			//400C_AA00_CCDD_0000 SROM Write
// WRITE_SROM_ENABLE,	//400D_0000_0000_0000 SROM Write Enable
// WRITE_SROM_DISABLE,	//400E_0000_0000_0000 SROM Write Disable
// READ_RX_CONTROL,		//C00F_0000_0000_0200 Read Rx Control
// WRITE_RX_CONTROL,	//4010_AABB_0000_0000 Write Rx Control
// READ_IPGS,			//C011_0000_0000_0300 Read IPG/IPG1/IPG2 Register
// WRITE_IPGS,			//4012_AABB_CC00_0000 Write IPG/IPG1/IPG2 Register
// READ_NODEID,			//C013_0000_0000_0600 Read Node ID
// WRITE_NODEID,		//4014_0000_0000_0600 Write Node ID
// READ_MF_ARRAY,		//C015_0000_0000_0800 Read Multicast Filter Array
// WRITE_MF_ARRAY,		//4016_0000_0000_0800 Write Multicast Filter Array
// READ_TEST,			//4017_AA00_0000_0000 Write Test Register
// READ_PHYID,			//C019_0000_0000_0200 Read Ethernet/HomePNA PHY Address
// READ_MEDIUM_STATUS,	//C01A_0000_0000_0200 Read Medium Status
// WRITE_MEDIUM_MODE,	//401B_AABB_0000_0000 Write Medium Mode Register
// GET_MONITOR_MODE,	//C01C_0000_0000_0100 Read Monitor Mode Status
// SET_MONITOR_MODE,	//401D_AA00_0000_0000 Write Monitor Mode Register
// READ_GPIOS,			//C01E_0000_0000_0100 Read GPIOs Status
// WRITE_GPIOS,			//401F_AA00_0000_0000 Write GPIOs
// WRITE_SOFT_RESET,	//4020_AA00_0000_0000 Write Software Reset
// READ_PHY_SEL_STATE,	//C021_AA00_0000_0100 Read Software PHY Select Status
// WRITE_PHY_SEL,		//4022_AA00_0000_0000 Write Software PHY Select

// RX Control Register bits
// RXCTL_PROMISCUOUS,	// forward all frames up to the host
// RXCTL_ALL_MULTICAT,	// forward all multicast frames up to the host
// RXCTL_SEP,			// forward frames with CRC error up to the host
// RXCTL_BROADCAST,		// forward broadcast frames up to the host
// RXCTL_MULTICAST,		// forward multicast frames that are
//							matching to multicast filter up to the host
// RXCTL_AP,			// forward unicast frames that are matching
//							to multicast filter up to the host
// RXCTL_START,			// ethernet MAC start operating
// RXCTL_USB_MFB,		// Max Frame Burst TX on USB


// PHY IDs request answer data layout
struct AX88772_PhyIDs {
	uint8 SecPhyID;
	uint8 PriPhyID2;
} _PACKED;


// Medium state bits
enum AX88772_MediumState {
	MEDIUM_STATE_FD		= 0x0002,
	MEDIUM_STATE_BIT2	= 0x0004, // must be always set
	MEDIUM_STATE_RFC	= 0x0010,
	MEDIUM_STATE_TFC	= 0x0020,
	MEDIUM_STATE_PF_ON	= 0x0040,
	MEDIUM_STATE_PF_OFF	= 0x0000,
	MEDIUM_STATE_RE	   	= 0x0100,
	MEDIUM_STATE_PS_100	= 0x0200,
	MEDIUM_STATE_PS_10 	= 0x0000,
	MEDIUM_STATE_SBP1  	= 0x0800,
	MEDIUM_STATE_SBP0  	= 0x0000,
	MEDIUM_STATE_SM_ON 	= 0x1000
};


// Monitor Mode bits
enum AX88772_MonitorMode {
	MONITOR_MODE_MOM	= 0x01,
	MONITOR_MODE_RWLU	= 0x02,
	MONITOR_MODE_RWMP	= 0x04,
	MONITOR_MODE_US 	= 0x10
};


// General Purpose I/O Register
enum AX88772_GPIO {
	GPIO_OO_0EN	= 0x01,
	GPIO_IO_0	= 0x02,
	GPIO_OO_1EN	= 0x04,
	GPIO_IO_1	= 0x08,
	GPIO_OO_2EN	= 0x10,
	GPIO_IO_2	= 0x20,
	GPIO_RSE	= 0x80
};


// Software Reset Register bits
enum AX88772_SoftwareReset {
	SW_RESET_CLR	= 0x00,
	SW_RESET_RR		= 0x01,
	SW_RESET_RT		= 0x02,
	SW_RESET_PRTE	= 0x04,
	SW_RESET_PRL	= 0x08,
	SW_RESET_BZ		= 0x10,
	SW_RESET_IPRL	= 0x20,
	SW_RESET_IPPD	= 0x40
};


// Software PHY Select Status
enum AX88772_SoftwarePHYSelStatus {
	SW_PHY_SEL_STATUS_EXT	= 0x00,
	SW_PHY_SEL_STATUS_INT	= 0x01,
	SW_PHY_SEL_STATUS_ASEL	= 0x02
};


// Notification data layout
struct AX88772_Notify {
	uint8  btA1;
	uint8  bt01;
	uint8  btBB; // AX88772_BBState below
	uint8  bt03;
	uint16 regCCDD;
	uint16 regEEFF;
} _PACKED;


// Link-State bits
enum AX88772_BBState {
	LINK_STATE_PPLS		= 0x01,
	LINK_STATE_SPLS		= 0x02,
	LINK_STATE_FLE		= 0x04,
	LINK_STATE_MDINT	= 0x08
};


const uint16 maxFrameSize = 1536;


AX88772Device::AX88772Device(usb_device device, DeviceInfo& deviceInfo)
		:
		ASIXDevice(device, deviceInfo)
{
	fStatus = InitDevice();
}


status_t
AX88772Device::InitDevice()
{
	fFrameSize = maxFrameSize;
	fUseTRXHeader = true;

	fReadNodeIDRequest = READ_NODEID;

	fNotifyBufferLength = sizeof(AX88772_Notify);
	fNotifyBuffer = (uint8 *)malloc(fNotifyBufferLength);
	if (fNotifyBuffer == NULL) {
		TRACE_ALWAYS("Error of allocating memory for notify buffer.\n");
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
AX88772Device::SetupDevice(bool deviceReplugged)
{
	status_t result = ASIXDevice::SetupDevice(deviceReplugged);
	if (result != B_OK) {
		return result;
	}

	result = fMII.Init(fDevice);

	size_t actualLength = 0;
	// enable GPIO2 - magic from FreeBSD's if_axe
	uint16 GPIOs = GPIO_OO_2EN | GPIO_IO_2 | GPIO_RSE;
	result = gUSBModule->send_request(fDevice,
						USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
						WRITE_GPIOS, GPIOs, 0, 0, 0, &actualLength);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of wrinting GPIOs: %#010x\n", result);
		return result;
	}

	// select PHY
	bool useEmbeddedPHY = fMII.PHYID() == PHYIDEmbedded;
	uint16 selectPHY = useEmbeddedPHY ?
					SW_PHY_SEL_STATUS_INT : SW_PHY_SEL_STATUS_EXT;

	result = gUSBModule->send_request(fDevice,
						USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
						WRITE_PHY_SEL, selectPHY, 0, 0, 0, &actualLength);
	snooze(10000);

	TRACE("Selecting %s PHY[%#02x].\n",
						useEmbeddedPHY ? "embedded" : "external", selectPHY);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of selecting PHY:%#010x\n", result);
		return result;
	}

	struct SWReset {
		uint16 		reset;
		bigtime_t	delay;
	} resetCommands[] = {
	// EMBEDDED PHY
		// power down and reset state, pin reset state
		{ SW_RESET_CLR, 60000 },
		// power down/reset state, pin operating state
		{ SW_RESET_PRL | SW_RESET_IPPD, 150000 },
		// power up, reset
		{ SW_RESET_PRL, 0 },
		// power up, operating
		{ SW_RESET_PRL | SW_RESET_IPRL, 0 },
	// EXTERNAL PHY
		// power down/reset state, pin operating state
		{ SW_RESET_PRL | SW_RESET_IPPD, 0 }
	};

	size_t from = useEmbeddedPHY ? 0 : 4;
	size_t to   = useEmbeddedPHY ? 3 : 4;

	for (size_t i = from; i <= to; i++) {
		result = gUSBModule->send_request(fDevice,
					USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
					WRITE_SOFT_RESET, resetCommands[i].reset,
					0, 0, 0, &actualLength);

		snooze(resetCommands[i].delay);

		if (result != B_OK) {
			TRACE_ALWAYS("Error of SW reset command %d:[%#04x]: %#010x\n",
										i, resetCommands[i].reset, result);
			return result;
		}
	}

	snooze(150000);

	result = WriteRXControlRegister(0);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n", 0, result);
		return result;
	}

	result = fMII.SetupPHY();
	if (result != B_OK) {
		return result;
	}

	result = gUSBModule->send_request(fDevice,
						USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
						WRITE_MEDIUM_MODE,
						MEDIUM_STATE_FD | MEDIUM_STATE_BIT2 |
						MEDIUM_STATE_RFC| MEDIUM_STATE_TFC	|
						MEDIUM_STATE_RE | MEDIUM_STATE_PS_100,
						0, 0, 0, &actualLength);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of setting medium mode: %#010x\n", result);
	}

	TRACE_RET(result);
	return result;
}


status_t
AX88772Device::StartDevice()
{
	size_t actualLength = 0;
	status_t result = gUSBModule->send_request(fDevice,
						USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
						WRITE_IPGS, 0, 0, sizeof(fIPG), fIPG, &actualLength);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing IPGs:%#010x\n", result);
		return result;
	}

	if (actualLength != sizeof(fIPG)) {
		TRACE_ALWAYS("Mismatch of written IPGs data. "
				"%d bytes of %d written.\n", actualLength, sizeof(fIPG));
	}

	uint16 rxcontrol = RXCTL_START | RXCTL_BROADCAST;
	result = WriteRXControlRegister(rxcontrol);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing %#04x RX Control:%#010x\n",
						rxcontrol, result);
	}

	TRACE_RET(result);
	return result;
}


status_t
AX88772Device::OnNotify(uint32 actualLength)
{
	if (actualLength < sizeof(AX88772_Notify)) {
		TRACE_ALWAYS("Data underrun error. %d of %d bytes received\n",
										actualLength, sizeof(AX88772_Notify));
		return B_BAD_DATA;
	}

	AX88772_Notify *notification	= (AX88772_Notify *)fNotifyBuffer;

	if (notification->btA1 != 0xa1) {
		TRACE_ALWAYS("Notify magic byte is invalid: %#02x\n",
														notification->btA1);
	}

	uint phyIndex = 0;
	bool linkIsUp = fHasConnection;
	switch(fMII.ActivePHY()) {
		case PrimaryPHY:
			phyIndex = 1;
			linkIsUp = (notification->btBB & LINK_STATE_PPLS) == LINK_STATE_PPLS;
			break;
		case SecondaryPHY:
			phyIndex = 2;
			linkIsUp = (notification->btBB & LINK_STATE_SPLS) == LINK_STATE_SPLS;
			break;
		default:
		case CurrentPHY:
			TRACE_ALWAYS("Error: PHY is not initialized.\n");
			return B_NO_INIT;
	}

	bool linkStateChange = linkIsUp != fHasConnection;
	fHasConnection = linkIsUp;

	if (linkStateChange) {
		TRACE("Link state of PHY%d has been changed to '%s'\n",
									phyIndex, fHasConnection ? "up" : "down");
	}

	if (linkStateChange && fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


status_t
AX88772Device::GetLinkState(ether_link_state *linkState)
{
	size_t actualLength = 0;
	uint16 mediumStatus = 0;
	status_t result = gUSBModule->send_request(fDevice,
						USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
						READ_MEDIUM_STATUS, 0, 0, sizeof(mediumStatus),
						&mediumStatus, &actualLength);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading medium status:%#010x.\n", result);
		return result;
	}

	if (actualLength != sizeof(mediumStatus)) {
		TRACE_ALWAYS("Mismatch of reading medium status."
							"Read %d bytes instead of %d\n",
									actualLength, sizeof(mediumStatus));
	}

	TRACE_FLOW("Medium status is %#04x\n", mediumStatus);

	linkState->quality = 1000;

	linkState->media   = IFM_ETHER | (fHasConnection ? IFM_ACTIVE : 0);
	linkState->media  |= (mediumStatus & MEDIUM_STATE_FD) ?
							IFM_FULL_DUPLEX : IFM_HALF_DUPLEX;

	linkState->speed   = (mediumStatus & MEDIUM_STATE_PS_100)
							? 100000000 : 10000000;

	TRACE_FLOW("Medium state: %s, %lld MBit/s, %s duplex.\n",
						(linkState->media & IFM_ACTIVE) ? "active" : "inactive",
						linkState->speed / 1000000,
						(linkState->media & IFM_FULL_DUPLEX) ? "full" : "half");
	return B_OK;
}

