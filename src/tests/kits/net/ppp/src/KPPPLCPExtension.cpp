//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPLCPExtension.h>

#include <PPPControl.h>


PPPLCPExtension::PPPLCPExtension(const char *name, uint8 code, PPPInterface& interface,
		driver_parameter *settings)
	: fInterface(interface),
	fSettings(settings),
	fCode(code),
	fEnabled(true)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "???");
	
	fInitStatus = interface.LCP().AddLCPExtension(this) ? B_OK : B_ERROR;
}


PPPLCPExtension::~PPPLCPExtension()
{
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
			strcpy(info->name, Name());
			info->settings = Settings();
			info->isEnabled = IsEnabled();
		} break;
		
		case PPPC_SET_ENABLED:
			if(length < sizeof(uint32) || !data)
				return B_ERROR;
			
			SetEnabled(*((uint32*)data));
		break;
		
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
