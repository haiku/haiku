/*
 * Copyright 2003-2007, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class KPPPDevice
	\brief Represents a device at the lowest level of the communcation stack.
	
	A device may be, for example: Modem, PPPoE, PPTP. \n
	It encapsulates the packet and sends it over a line to the other end.
	The device is the first layer that receives a packet.
*/

#include <KPPPDevice.h>

#include <net/if.h>
#include <core_funcs.h>

#include <PPPControl.h>


/*!	\brief Initializes the device.
	
	\param name The device's type name (e.g.: PPPoE).
	\param overhead Length of the header that is prepended to each packet.
	\param interface Owning interface.
	\param settings Device's settings.
*/
KPPPDevice::KPPPDevice(const char *name, uint32 overhead, KPPPInterface& interface,
		driver_parameter *settings)
	: KPPPLayer(name, PPP_DEVICE_LEVEL, overhead),
	fMTU(1500),
	fInterface(interface),
	fSettings(settings),
	fConnectionPhase(PPP_DOWN_PHASE)
{
}


//!	Destructor. Removes device from interface.
KPPPDevice::~KPPPDevice()
{
	if (Interface().Device() == this)
		Interface().SetDevice(NULL);
}


/*!	\brief Allows private extensions.
	
	If you override this method you must call the parent's method for unknown ops.
*/
status_t
KPPPDevice::Control(uint32 op, void *data, size_t length)
{
	switch (op) {
		case PPPC_GET_DEVICE_INFO:
		{
			if (length < sizeof(ppp_device_info_t) || !data)
				return B_NO_MEMORY;

			ppp_device_info *info = (ppp_device_info*) data;
			memset(info, 0, sizeof(ppp_device_info_t));
			if (Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->MTU = MTU();
			info->inputTransferRate = InputTransferRate();
			info->outputTransferRate = OutputTransferRate();
			info->outputBytesCount = CountOutputBytes();
			info->isUp = IsUp();
			break;
		}

		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


//!	Returns \c true (indicates to KPPPInterface we are always allowed to send).
bool
KPPPDevice::IsAllowedToSend() const
{
	return true;
		// our connection status will be reported in Send()
}


//!	This method is never used.
status_t
KPPPDevice::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	// let the interface handle the packet
	if (protocolNumber == 0)
		return Interface().ReceiveFromDevice(packet);
	else
		return Interface().Receive(packet, protocolNumber);
}


/*!	\brief Report that device is going up.
	
	Called by Up(). \n
	From now on, the connection attempt can may be aborted by calling Down().
	
	\return
		- \c true: You are allowed to connect.
		- \c false: You should abort immediately. Down() will \e not be called!
*/
bool
KPPPDevice::UpStarted()
{
	fConnectionPhase = PPP_ESTABLISHMENT_PHASE;
	
	return Interface().StateMachine().TLSNotify();
}


/*!	\brief Report that device is going down.
	
	Called by Down().
	
	\return
		- \c true: You are allowed to disconnect.
		- \c false: You must not disconnect!
*/
bool
KPPPDevice::DownStarted()
{
	fConnectionPhase = PPP_TERMINATION_PHASE;
	
	return Interface().StateMachine().TLFNotify();
}


//!	Reports that device failed going up. May only be called after Up() was called.
void
KPPPDevice::UpFailedEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().UpFailedEvent();
}


//!	Reports that device went up. May only be called after Up() was called.
void
KPPPDevice::UpEvent()
{
	fConnectionPhase = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent();
}


//!	Reports that device went down. This may be called to indicate connection loss.
void
KPPPDevice::DownEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent();
}
