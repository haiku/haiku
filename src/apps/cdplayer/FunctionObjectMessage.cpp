/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines code for sending funciton objects in messages

#ifndef _BE_H
#include <Debug.h>
#endif

#include "FunctionObjectMessage.h"


BMessage *
FunctorFactoryCommon::NewMessage(const FunctionObject *functor) 
{
	BMessage *result = new BMessage('fCmG');
	ASSERT(result);
	
	long error = result->AddData("functor", B_RAW_TYPE,
		functor, functor->Size());

	if (error != B_NO_ERROR) {
		delete result;
		result = NULL;
	}
	
	return result;
}

bool 
FunctorFactoryCommon::DispatchIfFunctionObject(BMessage *message)
{
	if (message->what != 'fCmG')
		return false;
	
	// find the functor
	long size;
	FunctionObject *functor;		
	status_t error = message->FindData("functor", B_RAW_TYPE, (const void**)&functor, &size);
	if (error != B_NO_ERROR)		
		return false;
	
	ASSERT(functor);
	// functor found, call it
	(*functor)();
	return true;
}

