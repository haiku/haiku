/******************************************************************************
/
/	File:			VIP.cpp
/
/	Description:	ATI Radeon Video Input Port (VIP) interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "VIPPort.h"

CVIPPort::CVIPPort(CRadeon & radeon)
	:	fRadeon(radeon)
{
	PRINT(("CVIPPort::CVIPPort()\n"));	
}

CVIPPort::~CVIPPort()
{
	PRINT(("CVIPPort::~CVIPPort()\n"));
}

status_t CVIPPort::InitCheck() const
{
	return fRadeon.InitCheck();
}

CRadeon & CVIPPort::Radeon()
{
	return fRadeon;
}
