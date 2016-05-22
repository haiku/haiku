/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ValueHandlerRoster.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AddressValueHandler.h"
#include "BoolValueHandler.h"
#include "EnumerationValueHandler.h"
#include "FloatValueHandler.h"
#include "StringValueHandler.h"
#include "Value.h"


/*static*/ ValueHandlerRoster* ValueHandlerRoster::sDefaultInstance = NULL;


ValueHandlerRoster::ValueHandlerRoster()
	:
	fLock("value handler roster")
{
}


ValueHandlerRoster::~ValueHandlerRoster()
{
}

/*static*/ ValueHandlerRoster*
ValueHandlerRoster::Default()
{
	return sDefaultInstance;
}


/*static*/ status_t
ValueHandlerRoster::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	ValueHandlerRoster* roster = new(std::nothrow) ValueHandlerRoster;
	if (roster == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<ValueHandlerRoster> rosterDeleter(roster);

	status_t error = roster->Init();
	if (error != B_OK)
		return error;

	error = roster->RegisterDefaultHandlers();
	if (error != B_OK)
		return error;

	sDefaultInstance = rosterDeleter.Detach();
	return B_OK;
}


/*static*/ void
ValueHandlerRoster::DeleteDefault()
{
	ValueHandlerRoster* roster = sDefaultInstance;
	sDefaultInstance = NULL;
	delete roster;
}


status_t
ValueHandlerRoster::Init()
{
	return fLock.InitCheck();
}


status_t
ValueHandlerRoster::RegisterDefaultHandlers()
{
	status_t error;

	#undef REGISTER_HANDLER
	#define REGISTER_HANDLER(name)											\
		{																	\
			name##ValueHandler* handler										\
				= new(std::nothrow) name##ValueHandler;						\
			if (handler == NULL)											\
				return B_NO_MEMORY;											\
			BReference<name##ValueHandler> handlerReference(handler, true);	\
																			\
			error = handler->Init();										\
			if (error != B_OK)												\
				return error;												\
																			\
			if (!RegisterHandler(handler))									\
				return B_NO_MEMORY;											\
		}

	REGISTER_HANDLER(Address)
	REGISTER_HANDLER(Bool)
	REGISTER_HANDLER(Enumeration)
	REGISTER_HANDLER(Float)
	REGISTER_HANDLER(Integer)
	REGISTER_HANDLER(String)

	return B_OK;
}


status_t
ValueHandlerRoster::FindValueHandler(Value* value, ValueHandler*& _handler)
{
	// find the best-supporting handler
	AutoLocker<BLocker> locker(fLock);

	ValueHandler* bestHandler = NULL;
	float bestSupport = 0;

	for (int32 i = 0; ValueHandler* handler = fValueHandlers.ItemAt(i); i++) {
		float support = handler->SupportsValue(value);
		if (support > 0 && support > bestSupport) {
			bestHandler = handler;
			bestSupport = support;
		}
	}

	if (bestHandler == NULL)
		return B_ENTRY_NOT_FOUND;

	bestHandler->AcquireReference();
	_handler = bestHandler;
	return B_OK;
}


status_t
ValueHandlerRoster::GetValueFormatter(Value* value,
	ValueFormatter*& _formatter)
{
	// get the best supporting value handler
	ValueHandler* handler;
	status_t error = FindValueHandler(value, handler);
	if (error != B_OK)
		return error;
	BReference<ValueHandler> handlerReference(handler, true);

	// create the formatter
	return handler->GetValueFormatter(value, _formatter);
}


status_t
ValueHandlerRoster::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	// get the best supporting value handler
	ValueHandler* handler;
	status_t error = FindValueHandler(value, handler);
	if (error != B_OK)
		return error;
	BReference<ValueHandler> handlerReference(handler, true);

	// create the renderer
	return handler->GetTableCellValueRenderer(value, _renderer);
}


bool
ValueHandlerRoster::RegisterHandler(ValueHandler* handler)
{
	if (!fValueHandlers.AddItem(handler))
		return false;

	handler->AcquireReference();
	return true;
}


void
ValueHandlerRoster::UnregisterHandler(ValueHandler* handler)
{
	if (fValueHandlers.RemoveItem(handler))
		handler->ReleaseReference();
}
