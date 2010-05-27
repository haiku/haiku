/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>

#include "SettingsPane.h"
#include "SettingsHost.h"


SettingsPane::SettingsPane(const char* name, SettingsHost* host)
	:
	BView(name, B_WILL_DRAW),
	fHost(host)
{
}


void
SettingsPane::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kSettingChanged:
			fHost->SettingChanged();
			break;
		default:
			BView::MessageReceived(msg);
	}
}
