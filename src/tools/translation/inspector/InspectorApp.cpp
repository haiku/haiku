/*****************************************************************************/
// InspectorApp
// Written by Michael Wilber, OBOS Translation Kit Team
//
// InspectorApp.cpp
//
// BApplication object for the Inspector application.  The purpose of
// Inspector is to provide the user with as much relevant information as
// possible about the currently open document.
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
#include "InspectorApp.h"
#include "ImageWindow.h"
#include "TranslatorItem.h"

#include <Directory.h>
#include <Message.h>
#include <String.h>
#include <Window.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InspectorApp"


InspectorApp::InspectorApp()
	: BApplication(APP_SIG)
{
	fpActivesWin = NULL;
	fpInfoWin = NULL;

	AddToTranslatorsList("/system/add-ons/Translators",
		SYSTEM_TRANSLATOR);
	AddToTranslatorsList(
		"/boot/home/config/add-ons/Translators",
		USER_TRANSLATOR);

	// Show application window
	BRect rect(100, 100, 500, 400);
	ImageWindow *pwin = new ImageWindow(rect, IMAGEWINDOW_TITLE);
	pwin->Show();
}

void
InspectorApp::AddToTranslatorsList(const char *folder, int32 group)
{
	BDirectory dir;
	if (dir.SetTo(folder) == B_OK) {

		BEntry ent;
		while (dir.GetNextEntry(&ent) == B_OK) {
			BPath path;
			if (ent.GetPath(&path) == B_OK)
				flstTranslators.AddItem(
					new BTranslatorItem(path.Leaf(), path.Path(), group));
		}
	}
}

void
InspectorApp::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case M_ACTIVE_TRANSLATORS_WINDOW:
			if (!fpActivesWin)
				fpActivesWin = new ActiveTranslatorsWindow(
					BRect(625, 350, 800, 600),
					B_TRANSLATE("Active Translators"),
					GetTranslatorsList());
			break;
		case M_ACTIVE_TRANSLATORS_WINDOW_QUIT:
			fpActivesWin = NULL;
			break;

		case M_INFO_WINDOW:
			if (!fpInfoWin)
				fpInfoWin = new InfoWindow(BRect(625, 50, 800, 300),
					B_TRANSLATE_COMMENT("Info Win",
					"This is a quite narrow info window and title 'Info Win' "
					"is therefore shortened."), fbstrInfo.String());
			break;
		case M_INFO_WINDOW_QUIT:
			fpInfoWin = NULL;
			break;
		case M_INFO_WINDOW_TEXT:
			// If image view is telling me to
			// update the info window...
			pmsg->FindString("text", &fbstrInfo);
			if (fpInfoWin)
				fpInfoWin->PostMessage(pmsg);
			break;

		default:
			BApplication::MessageReceived(pmsg);
			break;
	}
}

void
InspectorApp::RefsReceived(BMessage *pmsg)
{
	pmsg->what = M_OPEN_FILE_PANEL;
	BWindow *pwin = WindowAt(0);
	if (pwin)
		pwin->PostMessage(pmsg);
}

BList *
InspectorApp::GetTranslatorsList()
{
	return &flstTranslators;
}

int main(int argc, char **argv)
{
	InspectorApp *papp = new InspectorApp();

	papp->Run();

	delete papp;
	papp = NULL;

	return 0;
}

