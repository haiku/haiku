/*****************************************************************************/
// ImageWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// ImageWindow.cpp
//
// BWindow class for displaying an image.  Uses ImageView class for its
// view. 
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


#include "ImageWindow.h"
#include "Constants.h"
#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Menu.h>
#include <ScrollView.h>
#include <Alert.h>

ImageWindow::ImageWindow(BRect rect, const char *name)
	: BWindow(rect, name, B_DOCUMENT_WINDOW, 0)
{
	// Setup menu bar
	BRect rctbar(0, 0, 100, 10);
	BMenuBar *pbar = new BMenuBar(rctbar, "MenuBar");
	BMenu *pmnufile = new BMenu("File");
	
	BMenuItem *pitmopen = new BMenuItem("Open...", new BMessage(M_OPEN_IMAGE), 'O', 0);
	BMenuItem *pitmsave = new BMenuItem("Save...", new BMessage(M_SAVE_IMAGE), 'S', 0);
	BMenuItem *pitmquit = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q', 0);
	
	pmnufile->AddItem(pitmopen);
	pmnufile->AddItem(pitmsave);
	pmnufile->AddSeparatorItem();
	pmnufile->AddItem(pitmquit);
	pbar->AddItem(pmnufile);
	AddChild(pbar);
	
	// Setup image view
	BRect rctview = Bounds();
	rctview.top = pbar->Frame().bottom + 1;
	rctview.right -= B_V_SCROLL_BAR_WIDTH;
	rctview.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	fpimageView = new ImageView(rctview, "ImageView");
	AddChild(new BScrollView("ImageScroll", fpimageView, B_FOLLOW_ALL_SIDES, 0, true, true));
	
	// Setup file open panel
	fpopenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, 0, false,
		new BMessage(M_OPEN_FILE_PANEL), NULL, false, true);
		
	SetSizeLimits(200, 10000, 150, 10000);
}

ImageWindow::~ImageWindow()
{
	delete fpopenPanel;
	fpopenPanel = NULL;
}

void
ImageWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case M_OPEN_IMAGE:
			fpopenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			fpopenPanel->Show();
			break;
			
		case M_SAVE_IMAGE:
			if (fpimageView->HasImage()) {
				BAlert *palert = new BAlert(NULL, "Save feature not implemented yet.", "Bummer");
				palert->Go();
			} else {
				BAlert *palert = new BAlert(NULL, "No image available to save.", "OK");
				palert->Go();
			}
			break;
		
		case M_OPEN_FILE_PANEL:
		case B_SIMPLE_DATA:
			fpimageView->SetImage(pmsg);
			break;
			
		case B_CANCEL:
			break;
			
		default:
			BWindow::MessageReceived(pmsg);
			break;
	}
}

bool
ImageWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

