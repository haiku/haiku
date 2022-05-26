/*****************************************************************************/
// Print to file transport add-on.
//
// Author
//   Philippe Houdoin
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001,2002 Haiku Project
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

#include <stdio.h>

#include <InterfaceKit.h>

#include "FileSelector.h"

FileSelector::FileSelector(void)
	: BWindow(BRect(0,0,320,160), "printtofile", B_TITLED_WINDOW,
	B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE, B_CURRENT_WORKSPACE)
{
	fExitSem = create_sem(0, "FileSelector");
	fResult = B_ERROR;
	fSavePanel = NULL;
}


FileSelector::~FileSelector()
{
	delete fSavePanel;
	delete_sem(fExitSem);
}


bool 
FileSelector::QuitRequested()
{
	release_sem(fExitSem);
	return BWindow::QuitRequested();
}


void 
FileSelector::MessageReceived(BMessage * msg)
{
	switch (msg->what) {
		case START_MSG:
		{
			BMessenger messenger(this);
			fSavePanel = new BFilePanel(B_SAVE_PANEL, 
							&messenger, NULL, 0, false);

			fSavePanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			fSavePanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			entry_ref dir;
			
			if (msg->FindRef("directory", &dir) == B_OK) {
				const char* name;

				BDirectory bdir(&dir);
				if (msg->FindString("name", &name) == B_OK) {
					if (name != NULL)
						fResult = fEntry.SetTo(&bdir, name);
				};
			};

			release_sem(fExitSem);
			break;
		};
		
		case B_CANCEL:
			release_sem(fExitSem);
			break;

		default:
			inherited::MessageReceived(msg);
			break;
	};
}
			

status_t 
FileSelector::Go(entry_ref* ref)
{
	MoveTo(300,300);
	Hide();
	Show();
	PostMessage(START_MSG);
	acquire_sem(fExitSem);

	// cache result to avoid memory access of deleted window object
	// after Quit().
	status_t result = fResult;
	if (result == B_OK && ref)
		result = fEntry.GetRef(ref);

	Lock();
	Quit();

	return result;
}
			
