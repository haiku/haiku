/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_CONFIG_HANDLER_ROSTER_H
#define CONNECTION_CONFIG_HANDLER_ROSTER_H

#include <Locker.h>
#include <ObjectList.h>
#include <String.h>

#include "ConnectionConfigView.h"


class ConnectionConfigHandler;
class TargetHostInterfaceInfo;


class ConnectionConfigHandlerRoster {
public:
								ConnectionConfigHandlerRoster();
	virtual						~ConnectionConfigHandlerRoster();

	static	ConnectionConfigHandlerRoster* Default();
	static	status_t			CreateDefault();
	static	void				DeleteDefault();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			status_t			Init();

			bool				HasHandlerFor(TargetHostInterfaceInfo* info)
									const;

			status_t			CreateConfigView(TargetHostInterfaceInfo* info,
									ConnectionConfigView::Listener* listener,
									ConnectionConfigView*& _view) const;

private:
			typedef BObjectList<ConnectionConfigHandler> HandlerList;

private:
			bool				_GetHandler(const BString& name,
									ConnectionConfigHandler*& _handler) const;
			status_t			_RegisterHandlers();

private:
			BLocker				fLock;
	static	ConnectionConfigHandlerRoster* sDefaultInstance;

			HandlerList			fConfigHandlers;
};

#endif	// CONNECTION_CONFIG_HANDLER_ROSTER_H
