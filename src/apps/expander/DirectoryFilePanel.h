/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// DirectoryFilePanel.h
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

#ifndef _DirectoryFilePanel_h
#define _DirectoryFilePanel_h

#include <FilePanel.h>
#include <Button.h>

const uint32 MSG_DIRECTORY = 'mDIR';


class DirectoryRefFilter : public BRefFilter {
	public:
		DirectoryRefFilter();
		bool Filter(const entry_ref *ref, BNode* node, struct stat *st, 
			const char *filetype);
};


class DirectoryFilePanel : public BFilePanel {
	public:
		DirectoryFilePanel(file_panel_mode mode = B_OPEN_PANEL,
			BMessenger *target = 0, const entry_ref *start_directory = 0,
			uint32 node_flavors = 0, bool allow_multiple_selection = true,
			BMessage *message = 0, BRefFilter * = 0,
			bool modal = false, bool hide_when_done = true);
		virtual void SelectionChanged(void);
		virtual void Show();
	
	protected:
		BButton *fCurrentButton;
};

#endif