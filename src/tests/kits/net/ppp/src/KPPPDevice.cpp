//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPDevice.h>

#include <net/if.h>
#include <mbuf.h>


PPPDevice::PPPDevice(const char *name, uint32 overhead, PPPInterface *interface,
		driver_parameter *settings)
	: fOverhead(overhead), fInterface(interface),
	fSettings(settings)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "");
	
	SetMTU(1500);
	
	if(interface)
		interface->SetDevice(this);
}


PPPDevice::~PPPDevice()
{
	if(Interface())
		Interface()->SetDevice(NULL);
}


status_t
PPPDevice::InitCheck() const
{
	if(!Interface() || !Settings())
		return B_ERROR;
	
	return B_OK;
}


status_t
PPPDevice::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		// TODO:
		// get:
		// - name
		// - mtu (+ preferred)
		// - status
		// - transfer rates
		
		default:
			return B_ERROR;
	}
	
	return B_OK;
}


bool
PPPDevice::SetMTU(uint32 MTU)
{
	fMTU = MTU;
	
	return true;
}


status_t
PPPDevice::PassToInterface(struct mbuf *packet)
{
	if(!Interface() || !Interface()->InQueue())
		return B_ERROR;
	
	IFQ_ENQUEUE(Interface()->InQueue(), packet);
	
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
	if(!Interface())
		return false;
	
	return Interface()->StateMachine().TLSNotify();
}


bool
PPPDevice::DownStarted() const
{
	if(!Interface())
		return false;
	
	return Interface()->StateMachine().TLFNotify();
}


void
PPPDevice::UpFailedEvent()
{
	fIsUp = false;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpFailedEvent();
}


void
PPPDevice::UpEvent()
{
	fIsUp = true;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().UpEvent();
}


void
PPPDevice::DownEvent()
{
	fIsUp = false;
	
	if(!Interface())
		return;
	
	Interface()->StateMachine().DownEvent();
}
