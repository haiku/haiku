/*
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef LISTENER_ADAPTER_H
#define LISTENER_ADAPTER_H

#include "AbstractLOAdapter.h"
#include "Listener.h"

enum {
	MSG_OBJECT_CHANGED	= 'obch'
};

class ListenerAdapter : public Listener, public AbstractLOAdapter {
 public:
								ListenerAdapter(BHandler* handler);
	virtual						~ListenerAdapter();

	virtual	void				ObjectChanged(const Notifier* object);
};

#endif // LISTENER_ADAPTER_H
