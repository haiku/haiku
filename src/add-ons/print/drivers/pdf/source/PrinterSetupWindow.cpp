/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <InterfaceKit.h>
#include <StorageKit.h>
#include <SupportKit.h>

#include "PrinterSetupWindow.h"

// --------------------------------------------------
PrinterSetupWindow::PrinterSetupWindow(char *printerName)
	:	HWindow(BRect(0,0,300,300), printerName, B_TITLED_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE)
{
	fExitSem 		= create_sem(0, "PrinterSetup");
	fResult 		= B_ERROR;
	fPrinterName 	= printerName;

	if (printerName) {
		BString	title;
		title << printerName << " Printer Setup";
		SetTitle(title.String());
	} else
		SetTitle("Printer Setup");
	
	// ---- Ok, build a default job setup user interface
	BRect			r;
	BButton 		*button;
	float			x, y, w, h;
	font_height		fh;
	
	r = Bounds();

	// add a *dialog* background
	BBox *panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		B_PLAIN_BORDER);


	const int kInterSpace = 8;
	const int kHorzMargin = 10;
	const int kVertMargin = 10;

	x = kHorzMargin;
	y = kVertMargin;

	// add a label before the list
	const char *kModelLabel = "Printer model";

	be_plain_font->GetHeight(&fh);

	w = Bounds().Width();
	w -= 2 * kHorzMargin;
	h = 150;
	
	BBox * model_group = new BBox(BRect(x, y, x+w, y+h), "model_group", B_FOLLOW_ALL_SIDES);
	model_group->SetLabel(kModelLabel);
	
	BRect rlv = model_group->Bounds();
	
	rlv.InsetBy(kHorzMargin, kVertMargin);
	rlv.top 	+= fh.ascent + fh.descent + fh.leading;
	rlv.right	-= B_V_SCROLL_BAR_WIDTH;
	fModelList		= new BListView(rlv, "model_list",
											B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES );
										
	BScrollView * sv	= new BScrollView( "model_list_scrollview", fModelList,
											B_FOLLOW_ALL_SIDES,	B_WILL_DRAW | B_FRAME_EVENTS, false, true );
	model_group->AddChild(sv);
	
	panel->AddChild(model_group);

	y += (h + kInterSpace);

	x = r.right - kHorzMargin;

	// add a "OK" button, and make it default
	fOkButton 	= new BButton(BRect(x, y, x + 400, y), NULL, "OK", new BMessage(OK_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOkButton->ResizeToPreferred();
	fOkButton->GetPreferredSize(&w, &h);
	x -= w;
	fOkButton->MoveTo(x, y);
	fOkButton->MakeDefault(true);
	fOkButton->SetEnabled(false);

	panel->AddChild(fOkButton);

	x -= kInterSpace;

	// add a "Cancel" button	
	button 	= new BButton(BRect(x, y, x + 400, y), NULL, "Cancel", new BMessage(CANCEL_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->GetPreferredSize(&w, &h);
	x -= w;
	button->MoveTo(x, y);
	panel->AddChild(button);

	y += (h + kInterSpace);

	panel->ResizeTo(Bounds().Width(), y);
	ResizeTo(Bounds().Width(), y);
	
	float minWidth, maxWidth, minHeight, maxHeight;

	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(panel->Frame().Width(), panel->Frame().Width(),
		panel->Frame().Height(), maxHeight);

	// Finally, add our panel to window
	AddChild(panel);

	BDirectory	Folder;
	BEntry		entry;
	
	Folder.SetTo ("/boot/beos/etc/bubblejet");
	if (Folder.InitCheck() != B_OK)
		return;
		
	while (Folder.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_NO_ERROR)
			fModelList->AddItem (new BStringItem(name));
	}

	fModelList->SetSelectionMessage(new BMessage(MODEL_MSG));
	fModelList->SetInvocationMessage(new BMessage(OK_MSG));
}


// --------------------------------------------------
PrinterSetupWindow::~PrinterSetupWindow()
{
	delete_sem(fExitSem);
}


// --------------------------------------------------
bool 
PrinterSetupWindow::QuitRequested()
{
	release_sem(fExitSem);
	return true;
}


// --------------------------------------------------
void 
PrinterSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG:
			{
				// Test model selection (if any), save it in printerName node and return
				BNode  	spoolDir;
				BPath *	path;
	
				if (fModelList->CurrentSelection() < 0)
					break;
	
				BStringItem * item = dynamic_cast<BStringItem*>
					(fModelList->ItemAt(fModelList->CurrentSelection()));

				if (!item)
					break;
	
				path = new BPath();
	
				find_directory(B_USER_SETTINGS_DIRECTORY, path);
				path->Append("printers");
				path->Append(fPrinterName);
	
				spoolDir.SetTo(path->Path());
				delete path;
	
				if (spoolDir.InitCheck() != B_OK) {
					BAlert * alert = new BAlert("Uh oh!", 
						"Couldn't find printer spool directory.", "OK");
					alert->Go();
				} else {			
					spoolDir.WriteAttr("printer_model", B_STRING_TYPE, 0, item->Text(), 
						strlen(item->Text()));
					fResult = B_OK;
				}
				
				release_sem(fExitSem);
				break;
			}
		
		case CANCEL_MSG:
			fResult = B_ERROR;
			release_sem(fExitSem);
			break;
		
		case MODEL_MSG:
			fOkButton->SetEnabled((fModelList->CurrentSelection() >= 0));
			break;
		
		default:
			inherited::MessageReceived(msg);
			break;
		};
}


// --------------------------------------------------
status_t 
PrinterSetupWindow::Go()
{
	MoveTo(300, 300);
	Show();
	acquire_sem(fExitSem);
	Lock();
	Quit();

	return fResult;
}
