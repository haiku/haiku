/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ConnectionConfigHandlerRoster.h"

#include <AutoDeleter.h>

#include "NetworkConnectionConfigHandler.h"
#include "TargetHostInterfaceInfo.h"


/*static*/ ConnectionConfigHandlerRoster*
	ConnectionConfigHandlerRoster::sDefaultInstance = NULL;


ConnectionConfigHandlerRoster::ConnectionConfigHandlerRoster()
	:
	fLock("config handler roster lock"),
	fConfigHandlers(10, true)
{
}


ConnectionConfigHandlerRoster::~ConnectionConfigHandlerRoster()
{
}


/*static*/ ConnectionConfigHandlerRoster*
ConnectionConfigHandlerRoster::Default()
{
	return sDefaultInstance;
}


/*static*/ status_t
ConnectionConfigHandlerRoster::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	ConnectionConfigHandlerRoster* roster
		= new(std::nothrow) ConnectionConfigHandlerRoster;
	if (roster == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<ConnectionConfigHandlerRoster> rosterDeleter(roster);

	status_t error = roster->Init();
	if (error != B_OK)
		return error;

	sDefaultInstance = roster;
	rosterDeleter.Detach();
	return B_OK;
}


/*static*/ void
ConnectionConfigHandlerRoster::DeleteDefault()
{
	ConnectionConfigHandlerRoster* roster = sDefaultInstance;
	sDefaultInstance = NULL;
	delete roster;
}


status_t
ConnectionConfigHandlerRoster::Init()
{
	return _RegisterHandlers();
}


bool
ConnectionConfigHandlerRoster::HasHandlerFor(TargetHostInterfaceInfo* info)
	const
{
	ConnectionConfigHandler* handler = NULL;
	return _GetHandler(info->Name(), handler);
}


status_t
ConnectionConfigHandlerRoster::CreateConfigView(TargetHostInterfaceInfo* info,
	ConnectionConfigView::Listener* listener,
	ConnectionConfigView*& _view) const
{
	ConnectionConfigHandler* handler = NULL;

	if (!_GetHandler(info->Name(), handler))
		return B_NOT_SUPPORTED;

	return handler->CreateView(info, listener, _view);
}


bool
ConnectionConfigHandlerRoster::_GetHandler(const BString& name,
	ConnectionConfigHandler*& _handler) const
{
	ConnectionConfigHandler* handler = NULL;
	for (int32 i = 0; i < fConfigHandlers.CountItems(); i++) {
		handler = fConfigHandlers.ItemAt(i);
		if (handler->Name() == name) {
			_handler = handler;
			return true;
		}
 	}

 	return false;
}


status_t
ConnectionConfigHandlerRoster::_RegisterHandlers()
{
	ConnectionConfigHandler* handler = NULL;
	ObjectDeleter<ConnectionConfigHandler> handlerDeleter;

	#undef REGISTER_HANDLER_INFO
	#define REGISTER_HANDLER_INFO(type) \
		handler = new(std::nothrow) type##ConnectionConfigHandler; \
		if (handler == NULL) \
			return B_NO_MEMORY; \
		handlerDeleter.SetTo(handler); \
		if (!fConfigHandlers.AddItem(handler)) \
			return B_NO_MEMORY; \
		handlerDeleter.Detach(); \

	REGISTER_HANDLER_INFO(Network)

	return B_OK;
}
