/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#include "PPPDeskbarReplicant.h"
#include "ConnectionWindow.h"


PPPDeskbarReplicant::PPPDeskbarReplicant()
	: BView(BRect(0, 0, 15, 15), "PPPDeskbarReplicant", B_FOLLOW_NONE, 0)
{
	BRect rect(0, 0, 300, 250);
	fWindow = new ConnectionWindow(rect, id, sender);
	fWindow->MoveTo(center_on_screen(rect, fWindow));
	fWindow->Show();
}
