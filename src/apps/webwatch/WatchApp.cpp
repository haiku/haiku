/*
 * Copyright (c) 1999-2003 Matthijs Hollemans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <Roster.h>
#include <Deskbar.h>

#include "WatchApp.h"
#include "WatchView.h"

///////////////////////////////////////////////////////////////////////////////

BView* instantiate_deskbar_item()
{
	return new WatchView;
}

///////////////////////////////////////////////////////////////////////////////

WatchApp::WatchApp() : BApplication(APP_SIGNATURE)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("WebWatch");

	// Here we tell the Deskbar that we want to add a new replicant, and
	// where it can find this replicant (in our app). Because we only run
	// less than a second, there is no need for our title to appear inside
	// the Deskbar. Therefore, the application flags inside our resource
	// file should be set to B_BACKGROUND_APP.

	BDeskbar deskbar;
	if (!deskbar.HasItem(DESKBAR_ITEM_NAME))
	{
		entry_ref ref;
		be_roster->FindApp(APP_SIGNATURE, &ref);
		deskbar.AddItem(&ref);
	}

	PostMessage(B_QUIT_REQUESTED);
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
	WatchApp watchApp;
	watchApp.Run();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
