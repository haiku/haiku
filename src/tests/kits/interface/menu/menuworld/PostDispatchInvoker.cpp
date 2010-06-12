//--------------------------------------------------------------------
//	
//	PostDispatchInvoker.cpp
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PostDispatchInvoker.h"
#include <Looper.h>

//====================================================================
//	PostDispatchInvoker Implementation


//--------------------------------------------------------------------
//	PostDispatchInvoker constructors, destructors, operators

PostDispatchInvoker::PostDispatchInvoker(uint32 cmdFilter,
	BMessage* invokeMsg, BHandler* invokeHandler,
	BLooper* invokeLooper)
	: BMessageFilter(cmdFilter, NULL),
	BInvoker(invokeMsg, invokeHandler, invokeLooper)
{ }



//--------------------------------------------------------------------
//	PostDispatchInvoker virtual function overrides

filter_result PostDispatchInvoker::Filter(BMessage* message,
	BHandler** target)
{
	Looper()->DispatchMessage(message, *target);
	BMessage* pInvMsg = Message();
	pInvMsg->AddMessage("Dispatched Message", message);
	pInvMsg->AddPointer("Dispatch Target", *target);	
	Invoke();
	return B_SKIP_MESSAGE;
}
