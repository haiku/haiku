//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <KPPPOptionHandler.h>


PPPOptionHandler::PPPOptionHandler(const char *name, PPPInterface *interface,
		driver_parameter *settings)
	: fInterface(interface), fSettings(settings)
{
	if(name) {
		strncpy(fName, name, PPP_HANDLER_NAME_LENGTH_LIMIT);
		fName[PPP_HANDLER_NAME_LENGTH_LIMIT] = 0;
	} else
		strcpy(fName, "");
	
	if(interface)
		interface->LCP().AddOptionHandler(this);
}


PPPOptionHandler::~PPPOptionHandler()
{
	if(Interface())
		Interface()->LCP().RemoveOptionHandler(this);
}


status_t
PPPOptionHandler::InitCheck() const
{
	if(!Interface() || !Settings())
		return B_ERROR;
	
	return B_OK;
}
