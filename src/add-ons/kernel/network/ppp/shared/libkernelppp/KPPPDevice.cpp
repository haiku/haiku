//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPDevice.h>

#include <net/if.h>
#include <core_funcs.h>

#include <PPPControl.h>


PPPDevice::PPPDevice(const char *name, PPPInterface& interface,
		driver_parameter *settings)
	: PPPLayer(name, PPP_DEVICE_LEVEL),
	fMTU(1500),
	fInterface(interface),
	fSettings(settings),
	fConnectionPhase(PPP_DOWN_PHASE)
{
}


PPPDevice::~PPPDevice()
{
	if(Interface().Device() == this)
		Interface().SetDevice(NULL);
}


status_t
PPPDevice::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_DEVICE_INFO: {
			if(length < sizeof(ppp_device_info_t) || !data)
				return B_NO_MEMORY;
			
			ppp_device_info *info = (ppp_device_info*) data;
			memset(info, 0, sizeof(ppp_device_info_t));
			strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->settings = Settings();
			info->MTU = MTU();
			info->inputTransferRate = InputTransferRate();
			info->outputTransferRate = OutputTransferRate();
			info->outputBytesCount = CountOutputBytes();
			info->isUp = IsUp();
		} break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


bool
PPPDevice::IsAllowedToSend() const
{
	return true;
		// our connection status will be reported in Send()
}


status_t
PPPDevice::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	// let the interface handle the packet
	if(protocolNumber == 0)
		return Interface().ReceiveFromDevice(packet);
	else
		return Interface().Receive(packet, protocolNumber);
}


void
PPPDevice::Pulse()
{
	// do nothing by default
}


bool
PPPDevice::UpStarted()
{
	fConnectionPhase = PPP_ESTABLISHMENT_PHASE;
	
	return Interface().StateMachine().TLSNotify();
}


bool
PPPDevice::DownStarted()
{
	fConnectionPhase = PPP_TERMINATION_PHASE;
	
	return Interface().StateMachine().TLFNotify();
}


void
PPPDevice::UpFailedEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().UpFailedEvent();
}


void
PPPDevice::UpEvent()
{
	fConnectionPhase = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent();
}


void
PPPDevice::DownEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent();
}
