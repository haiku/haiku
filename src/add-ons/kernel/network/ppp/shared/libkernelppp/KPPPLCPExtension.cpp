//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPLCPExtension.h>

#include <PPPControl.h>


PPPLCPExtension::PPPLCPExtension(const char *name, uint8 code, PPPInterface& interface,
		driver_parameter *settings)
	: fInterface(interface),
	fSettings(settings),
	fCode(code),
	fEnabled(true)
{
	if(name)
		fName = strdup(name);
	else
		fName = strdup("Unknown");
}


PPPLCPExtension::~PPPLCPExtension()
{
	free(fName);
	
	Interface().LCP().RemoveLCPExtension(this);
}


status_t
PPPLCPExtension::InitCheck() const
{
	return fInitStatus;
}


status_t
PPPLCPExtension::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_SIMPLE_HANDLER_INFO: {
			if(length < sizeof(ppp_simple_handler_info_t) || !data)
				return B_ERROR;
			
			ppp_simple_handler_info *info = (ppp_simple_handler_info*) data;
			memset(info, 0, sizeof(ppp_simple_handler_info_t));
			strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
			info->settings = Settings();
			info->isEnabled = IsEnabled();
		} break;
		
		case PPPC_ENABLE:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetEnabled(*((uint32*)data));
		break;
		
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


status_t
PPPLCPExtension::StackControl(uint32 op, void *data)
{
	switch(op) {
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


void
PPPLCPExtension::Reset()
{
	// do nothing by default
}


void
PPPLCPExtension::Pulse()
{
	// do nothing by default
}
