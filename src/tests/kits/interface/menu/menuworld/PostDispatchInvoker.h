//--------------------------------------------------------------------
//	
//	PostDispatchInvoker.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _PostDispatchInvoker_h
#define _PostDispatchInvoker_h

#include <MessageFilter.h>
#include <Invoker.h>

//====================================================================
//	CLASS: PostDispatchInvoker
// ---------------------------
// A simple kind of message filter which, when it receives a
// message of the target type, dispatches it directly to the
// looper, then posts a message to a specified handler when
// the dispatch is completed. Two fields are appended to the
// message before it is delivered:
// Dispatched Message	B_MESSAGE_TYPE
// Dispatch Target		B_POINTER_TYPE (BHandler*)
// 
// Most message filters are designed to intercept messages
// before they're delievered; this one allows you to do some
// handling after they've been delivered.

class PostDispatchInvoker : public BMessageFilter, public BInvoker
{
	//----------------------------------------------------------------
	//	Constructors, destructors, operators

public:
					PostDispatchInvoker(uint32 cmdFilter,
						BMessage* invokeMsg, BHandler* invokeHandler,
						BLooper* invokeLooper = NULL);

	
	//----------------------------------------------------------------
	//	Virtual member function overrides

public:	
	filter_result	Filter(BMessage* message, BHandler** target);	
};

#endif /* _PostDispatchInvoker_h */