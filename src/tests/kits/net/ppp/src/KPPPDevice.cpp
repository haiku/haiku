//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPDevice.h>

#include <net/if.h>
#include <mbuf.h>

#include <PPPControl.h>


PPPDevice::PPPDevice(const char *name, PPPInterface& interface,
		driver_parameter *settings)
	: fInterface(interface), fSettings(settings), fMTU(1500)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "???");
	
	interface.SetDevice(this);
}


PPPDevice::~PPPDevice()
{
	Interface().SetDevice(NULL);
}


status_t
PPPDevice::InitCheck() const
{
	if(!Settings())
		return B_ERROR;
	
	return B_OK;
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
			strcpy(info->name, Name());
			info->settings = Settings();
			info->MTU = MTU();
			info->preferredMTU = PreferredMTU();
			info->inputTransferRate = InputTransferRate();
			info->outputTransferRate = OutputTransferRate();
			info->outputBytesCount = CountOutputBytes();
		} break;
		
		case PPPC_SET_MTU:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			return SetMTU(*((uint32*)data)) ? B_OK : B_ERROR;
		break;
		
		default:
			return PPP_UNHANDLED;
	}
	
	return B_OK;
}


status_t
PPPDevice::PassToInterface(struct mbuf *packet)
{
	if(!Interface().InQueue())
		return B_ERROR;
	
	IFQ_ENQUEUE(Interface().InQueue(), packet);
	
	return B_OK;
}


void
PPPDevice::Pulse()
{
	// do nothing by default
}


bool
PPPDevice::UpStarted() const
{
	return Interface().StateMachine().TLSNotify();
}


bool
PPPDevice::DownStarted() const
{
	return Interface().StateMachine().TLFNotify();
}


void
PPPDevice::UpFailedEvent()
{
	fIsUp = false;
	
	Interface().StateMachine().UpFailedEvent();
}


void
PPPDevice::UpEvent()
{
	fIsUp = true;
	
	Interface().StateMachine().UpEvent();
}


void
PPPDevice::DownEvent()
{
	fIsUp = false;
	
	Interface().StateMachine().DownEvent();
}
