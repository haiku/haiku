/*

PictureTestWindow

Copyright (c) 2002 OpenBeOS. 

Author: 
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

#include "PictureTestWindow.hpp"

#include "PictureTestView.hpp"
#include "TestView.hpp"
#include "PicturePrinter.h"

#include <stdio.h>

#include <Application.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <View.h>

PictureTestWindow::PictureTestWindow()
	: Inherited(BRect(100,100,500,300), "OpenBeOS Picture Test App", B_DOCUMENT_WINDOW, 0)
{
	BuildGUI();
}

bool PictureTestWindow::QuitRequested()
{
	bool isOk = Inherited::QuitRequested();
	if (isOk) {
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	return isOk;
}


void PictureTestWindow::BuildGUI()
{
	BView* backdrop = new BView(Bounds(), "backdrop", B_FOLLOW_ALL, B_WILL_DRAW);
	backdrop->SetViewColor(::ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(backdrop);
	
	BMenuBar* mb = new BMenuBar(Bounds(), "menubar");
	BMenu* m = new BMenu("File");
		m->AddItem(new BMenuItem("Picture"B_UTF8_ELLIPSIS, new BMessage('Pict'), 'P'));
		m->AddSeparatorItem();
		m->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	mb->AddItem(m);

	backdrop->AddChild(mb);

	BRect b = Bounds();
	b.top = mb->Bounds().bottom +1;
	backdrop->AddChild(new PictureTestView(b));
}

void PictureTestWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case 'Pict':
			DoPictureTest();
			break;
	}
}

void PictureTestWindow::DoPictureTest() {
	fView = new TestView(BRect(0, 0, 10000, 10000));
	AddChild(fView);
	bool continue_test = true;
	for (int i = 0; continue_test; i++) {
		Begin();
		continue_test = fView->Test(i, fName);
		End(continue_test);
	}
	RemoveChild(fView);
	delete fView;
}

void PictureTestWindow::Begin() {
	fView->BeginPicture(new BPicture());
}

void PictureTestWindow::End(bool write) {
	fView->Sync(); 
	if (!write) return;
	BPicture* picture = fView->EndPicture();
	printf("\n%s\n", fName.String());
	if (picture) {
		bool saved = false;
		{
			BFile file(fName.String(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
			if (file.InitCheck() == B_OK) {
				if (picture->Flatten(&file) != B_OK) 
					fprintf(stderr, "Error %s could not flatten BPicture\n", fName.String());
				else saved = true;
			} else fprintf(stderr, "Error %s could not create file\n", fName.String());
			delete picture;
		}
		if (saved) {
			BFile file(fName.String(), B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				BPicture p;
				if (p.Unflatten(&file) == B_OK) {
					PicturePrinter printer;
					printer.Iterate(&p);
				} else {
					fprintf(stderr, "Error %s could not unflatten BPicture\n", fName.String());
				}
			} else {
				fprintf(stderr, "Error %s could not open file\n", fName.String());
			}
		}
	}
}
