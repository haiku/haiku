/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// DirectoryFilePanel.cpp
//
// Copyright (c) 2004 OpenBeOS Project
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

#include "DirectoryFilePanel.h"
#include <Window.h>
#include <stdio.h>

DirectoryRefFilter::DirectoryRefFilter()
	: BRefFilter()
{
}


bool
DirectoryRefFilter::Filter(const entry_ref *ref, BNode* node, struct stat *st, 
	const char *filetype)
{
	return (node->IsDirectory());
}


DirectoryFilePanel::DirectoryFilePanel(file_panel_mode mode, BMessenger *target, 
	const entry_ref *start_directory, uint32 node_flavors, 
	bool allow_multiple_selection, BMessage *message, BRefFilter *filter,	
	bool modal,	bool hide_when_done)
	:BFilePanel(mode,target,start_directory, node_flavors, 
		allow_multiple_selection,message,filter,modal,hide_when_done),
		fCurrentButton(NULL)
{
}


void
DirectoryFilePanel::Show()
{
	if (!fCurrentButton) {
		entry_ref ref;
		char label[50];
		GetPanelDirectory(&ref);
		sprintf(label, "Select '%s'", ref.name);
		Window()->Lock();
		BView *background = Window()->ChildAt(0); 
		fCurrentButton = new BButton(
			BRect(113, background->Bounds().bottom-35, 269, background->Bounds().bottom-10),
			"directoryButton", label, new BMessage(MSG_DIRECTORY), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		background->AddChild(fCurrentButton);
		SetButtonLabel(B_DEFAULT_BUTTON, "Select");
		fCurrentButton->SetTarget(Messenger());
		Window()->Unlock();
	}
	BFilePanel::Show();
}

void
DirectoryFilePanel::SelectionChanged(void)
{
	entry_ref ref;
	char label[50];
	GetPanelDirectory(&ref);
	Window()->Lock();
	sprintf(label, "Select '%s'", ref.name);
	fCurrentButton->SetLabel(label);
	Window()->Unlock();
	BFilePanel::SelectionChanged();
}
