/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_CONFIG_HANDLER_H
#define CONNECTION_CONFIG_HANDLER_H

#include <String.h>
#include <SupportDefs.h>

#include "ConnectionConfigView.h"


class TargetHostInterfaceInfo;


class ConnectionConfigHandler {
public:
								ConnectionConfigHandler(const char* name);
	virtual						~ConnectionConfigHandler();

			const BString&		Name() const { return fName; }

	virtual	status_t			CreateView(TargetHostInterfaceInfo* info,
									ConnectionConfigView::Listener* listener,
									ConnectionConfigView*& _view) = 0;
private:
			BString				fName;
};


#endif	// CONNECTION_CONFIG_HANDLER_H
