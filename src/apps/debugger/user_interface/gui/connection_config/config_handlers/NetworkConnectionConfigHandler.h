/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_CONNECTION_CONFIG_HANDLER_H
#define NETWORK_CONNECTION_CONFIG_HANDLER_H

#include "ConnectionConfigHandler.h"


class NetworkConnectionConfigHandler : public ConnectionConfigHandler {
public:
								NetworkConnectionConfigHandler();
	virtual						~NetworkConnectionConfigHandler();

	virtual	status_t			CreateView(TargetHostInterfaceInfo* info,
									ConnectionConfigView::Listener* listener,
									ConnectionConfigView*& _view);
};


#endif	// NETWORK_CONNECTION_CONFIG_HANDLER_H
