/*
 * Copyright 2004-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Andre Alves Garzia, andre@andregarzia.com
 */


#include "NetworkWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <Locale.h>

#include "EthernetSettingsView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkWindow"


NetworkWindow::NetworkWindow()
	:
	BWindow(BRect(50, 50, 269, 302), B_TRANSLATE_SYSTEM_NAME("Network"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
		| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	GetLayout()->AddView(new EthernetSettingsView());
}
