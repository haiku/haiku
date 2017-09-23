/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Brian Hill, supernova@tycho.email
 */

#include <Message.h>

#include "SettingsPane.h"
#include "SettingsHost.h"


SettingsPane::SettingsPane(const char* name, SettingsHost* host)
	:
	BView(name, B_WILL_DRAW),
	fHost(host)
{
}


void
SettingsPane::SettingsChanged(bool showExample)
{
	fHost->SettingChanged(showExample);
}
