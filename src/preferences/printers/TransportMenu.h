/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#ifndef _TRANSPORT_MENU_H
#define _TRANSPORT_MENU_H


#include <Menu.h>
#include <Messenger.h>
#include <String.h>


class TransportMenu : public BMenu
{
public:
			TransportMenu(const char* title, uint32 what,
				const BMessenger& messenger, const BString& transportName);

	bool	AddDynamicItem(add_state s);

private:
	uint32		fWhat;
	BMessenger	fMessenger;
	BString		fTransportName;
};

#endif
