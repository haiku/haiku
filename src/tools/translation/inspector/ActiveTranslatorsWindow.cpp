/*****************************************************************************/
// ActiveTranslatorsWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// ActiveTranslatorsWindow.cpp
//
// BWindow class for displaying information about the currently open
// document
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "Constants.h"
#include "ActiveTranslatorsWindow.h"
#include "TranslatorItem.h"
#include <Application.h>
#include <Catalog.h>
#include <ScrollView.h>
#include <Locale.h>
#include <Message.h>
#include <String.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ActiveTranslatorsWindow"


ActiveTranslatorsWindow::ActiveTranslatorsWindow(BRect rect, const char *name,
	BList *plist)
	: BWindow(rect, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect rctframe = Bounds();
	rctframe.right -= B_V_SCROLL_BAR_WIDTH;

	fpListView = new BOutlineListView(rctframe, "translators_list",
		B_MULTIPLE_SELECTION_LIST);

	fpListView->AddItem(fpUserItem = new BStringItem(B_TRANSLATE("User Translators")));
	fpUserItem->SetEnabled(false);
	fpListView->AddItem(fpSystemItem = new BStringItem(B_TRANSLATE("System Translators")));
	fpSystemItem->SetEnabled(false);
	AddTranslatorsToList(plist, USER_TRANSLATOR, fpUserItem);
	AddTranslatorsToList(plist, SYSTEM_TRANSLATOR, fpSystemItem);

	AddChild(new BScrollView("scroll_list", fpListView, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));

	SetSizeLimits(100, 10000, 100, 10000);

	Show();
}

void
ActiveTranslatorsWindow::AddTranslatorsToList(BList *plist, int32 group,
	BStringItem *pparent)
{
	BTranslatorItem *pitem;
	for (int32 i = 0; i < plist->CountItems(); i++) {
		pitem = static_cast<BTranslatorItem *>(plist->ItemAt(i));
		if (pitem->Group() == group)
			fpListView->AddUnder(pitem, pparent);
	}
}

ActiveTranslatorsWindow::~ActiveTranslatorsWindow()
{
}

void
ActiveTranslatorsWindow::FrameResized(float width, float height)
{
}

void
ActiveTranslatorsWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		default:
			BWindow::MessageReceived(pmsg);
			break;
	}
}

void
ActiveTranslatorsWindow::Quit()
{
	// tell the app to forget about this window
	be_app->PostMessage(M_ACTIVE_TRANSLATORS_WINDOW_QUIT);
	BWindow::Quit();
}
