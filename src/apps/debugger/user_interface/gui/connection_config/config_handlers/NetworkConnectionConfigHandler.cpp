/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "NetworkConnectionConfigHandler.h"

#include <AutoDeleter.h>

#include "NetworkConnectionConfigView.h"
#include "TargetHostInterfaceInfo.h"



NetworkConnectionConfigHandler::NetworkConnectionConfigHandler()
	:
	ConnectionConfigHandler("Network")
{
}


NetworkConnectionConfigHandler::~NetworkConnectionConfigHandler()
{
}


status_t
NetworkConnectionConfigHandler::CreateView(TargetHostInterfaceInfo* info,
	ConnectionConfigView::Listener* listener, ConnectionConfigView*& _view)
{
	NetworkConnectionConfigView* view = NULL;
	try {
		view = new NetworkConnectionConfigView;
		ObjectDeleter<BView> viewDeleter(view);
		status_t error = view->Init(info, listener);
		if (error != B_OK)
			return error;
		viewDeleter.Detach();
	} catch (...) {
		return B_NO_MEMORY;
	}

	_view = view;
	return B_OK;
}
