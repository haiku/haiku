//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "KPPPOptionHandler.h"


PPPOptionHandler::PPPOptionHandler(const char *name, PPPInterface *interface,
		driver_parameter *settings)
	: fInterface(interface), fSettings(settings)
{
	fName = name ? strdup(name) : NULL;
	
	if(interface)
		interface->LCP().AddOptionHandler(this);
}


PPPOptionHandler::~PPPOptionHandler()
{
	free(fName);
	
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
