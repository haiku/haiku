//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include "PPPInterfaceListener.h"

#include <Handler.h>
#include <LockerHelper.h>


PPPInterfaceListener::PPPInterfaceListener(BHandler *target)
	: fTarget(target)
{
	Construct();
}


PPPInterfaceListener::PPPInterfaceListener(const PPPInterfaceListener& copy)
	: fTarget(copy.Target())
{
	Construct();
}


PPPInterfaceListener::~PPPInterfaceListener()
{
	// TODO: send exit code to thread
//	int32 tmp;
//	wait_for_thread(fReportThread, &tmp);
}


status_t
PPPInterfaceListener::InitCheck() const
{
	if(fReportThread < 0)
		return B_ERROR;
	
	return Manager()->InitCheck();
}


void
PPPInterfaceListener::SetTarget(BHandler *target)
{
	LockerHelper locker(fLock);
	
	target = fTarget;
}


void
PPPInterfaceListener::Construct()
{
	// TODO:
	// create report thread and register it as a receiver using PPPManager
}
