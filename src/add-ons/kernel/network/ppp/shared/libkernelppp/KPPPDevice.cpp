//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPDevice.h>

#include <net/if.h>
#include <core_funcs.h>

#include <PPPControl.h>


KPPPDevice::KPPPDevice(const char *name, uint32 overhead, KPPPInterface& interface,
		driver_parameter *settings)
	: KPPPLayer(name, PPP_DEVICE_LEVEL, overhead),
	fMTU(1500),
	fInterface(interface),
	fSettings(settings),
	fConnectionPhase(PPP_DOWN_PHASE)
{
}


KPPPDevice::~KPPPDevice()
{
	if(Interface().Device() == this)
		Interface().SetDevice(NULL);
}


status_t
KPPPDevice::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_DEVICE_INFO: {
			if(length < sizeof(ppp_device_info_t) || !data)
				return B_NO_MEMORY;
			
			ppp_device_info *info = (ppp_device_info*) data;
			memset(info, 0, sizeof(ppp_device_info_t));
			strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
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
KPPPDevice::IsAllowedToSend() const
{
	return true;
		// our connection status will be reported in Send()
}


status_t
KPPPDevice::Receive(struct mbuf *packet, uint16 protocolNumber)
{
	// let the interface handle the packet
	if(protocolNumber == 0)
		return Interface().ReceiveFromDevice(packet);
	else
		return Interface().Receive(packet, protocolNumber);
}


void
KPPPDevice::Pulse()
{
	// do nothing by default
}


bool
KPPPDevice::UpStarted()
{
	fConnectionPhase = PPP_ESTABLISHMENT_PHASE;
	
	return Interface().StateMachine().TLSNotify();
}


bool
KPPPDevice::DownStarted()
{
	fConnectionPhase = PPP_TERMINATION_PHASE;
	
	return Interface().StateMachine().TLFNotify();
}


void
KPPPDevice::UpFailedEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().UpFailedEvent();
}


void
KPPPDevice::UpEvent()
{
	fConnectionPhase = PPP_ESTABLISHED_PHASE;
	
	Interface().StateMachine().UpEvent();
}


void
KPPPDevice::DownEvent()
{
	fConnectionPhase = PPP_DOWN_PHASE;
	
	Interface().StateMachine().DownEvent();
}
