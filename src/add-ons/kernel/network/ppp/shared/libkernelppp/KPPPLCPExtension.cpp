//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include <KPPPLCPExtension.h>

#include <PPPControl.h>


KPPPLCPExtension::KPPPLCPExtension(const char *name, uint8 code, KPPPInterface& interface,
		driver_parameter *settings)
	: fInterface(interface),
	fSettings(settings),
	fCode(code),
	fEnabled(true)
{
	if(name)
		fName = strdup(name);
	else
		fName = NULL;
}


KPPPLCPExtension::~KPPPLCPExtension()
{
	free(fName);
	
	Interface().LCP().RemoveLCPExtension(this);
}


status_t
KPPPLCPExtension::InitCheck() const
{
	return fInitStatus;
}


status_t
KPPPLCPExtension::Control(uint32 op, void *data, size_t length)
{
	switch(op) {
		case PPPC_GET_SIMPLE_HANDLER_INFO: {
			if(length < sizeof(ppp_simple_handler_info_t) || !data)
				return B_ERROR;
			
			ppp_simple_handler_info *info = (ppp_simple_handler_info*) data;
			memset(info, 0, sizeof(ppp_simple_handler_info_t));
			if(Name())
				strncpy(info->name, Name(), PPP_HANDLER_NAME_LENGTH_LIMIT);
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
KPPPLCPExtension::StackControl(uint32 op, void *data)
{
	switch(op) {
		default:
			return B_BAD_VALUE;
	}
	
	return B_OK;
}


void
KPPPLCPExtension::ProfileChanged()
{
	// do nothing by default
}


void
KPPPLCPExtension::Reset()
{
	// do nothing by default
}


void
KPPPLCPExtension::Pulse()
{
	// do nothing by default
}
