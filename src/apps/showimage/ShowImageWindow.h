/*****************************************************************************/
// ShowImageWindow
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageWindow.h
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

#ifndef _ShowImageWindow_h
#define _ShowImageWindow_h

#include <Menu.h>
#include <Window.h>
#include <FilePanel.h>
#include <TranslationDefs.h>
#include <String.h>

#include "PrintOptionsWindow.h"

class ShowImageView;
class ShowImageStatusView;

// BMessage field names used in Save messages
#define TRANSLATOR_FLD "be:translator"
#define TYPE_FLD "be:type"

class RecentDocumentsMenu : public BMenu
{
public:
	RecentDocumentsMenu(const char *title, menu_layout layout = B_ITEMS_IN_COLUMN);	
	bool AddDynamicItem(add_state s);

private:
	void UpdateRecentDocumentsMenu();
};

class ShowImageWindow : public BWindow {
public:
	ShowImageWindow(const entry_ref *pref);
	virtual ~ShowImageWindow();

	virtual void FrameResized(float width, float height);
	virtual void MessageReceived(BMessage *pmsg);
	virtual bool QuitRequested();
	virtual void Quit();
	
	status_t InitCheck();
	ShowImageView *GetShowImageView() const { return fImageView; }
	
	void UpdateTitle();
	void BuildViewMenu(BMenu *menu);
	void LoadMenus(BMenuBar *pbar);
	void WindowRedimension(BBitmap *pbitmap);

private:
	BMenuItem *AddItemMenu(BMenu *pmenu, char *caption,
		long unsigned int msg, char shortcut, uint32 modifier,
		char target, bool enabled);

	BMenuItem* AddDelayItem(BMenu *pmenu, char *caption, float value);
	
	bool ToggleMenuItem(uint32 what);
	void EnableMenuItem(BMenu *menu, uint32 what, bool enable);
	void MarkMenuItem(BMenu *menu, uint32 what, bool marked);
	void MarkSlideShowDelay(float value);
	void ResizeToWindow(bool shrink, uint32 what);
			
	void SaveAs(BMessage *pmsg);
		// Handle Save As submenu choice
	void SaveToFile(BMessage *pmsg);
		// Handle save file panel message
	bool ClosePrompt();
	bool CanQuit();
		// returns true if the window can be closed safely, false if not
	void ToggleFullScreen();
	void LoadSettings();
	void SavePrintOptions();
	bool PageSetup();
	void PrepareForPrint();
	void Print(BMessage *msg);

	BFilePanel *fSavePanel;
	BMenuBar *fBar;
	BMenu *fOpenMenu;
	BMenu *fBrowseMenu;
	BMenu *fGoToPageMenu;
	BMenu *fSlideShowDelay;
	ShowImageView *fImageView;
	ShowImageStatusView *fStatusView;
	bool fModified;
	bool fFullScreen;
	BRect fWindowFrame;
	bool fShowCaption;
	BMessage *fPrintSettings;
	PrintOptions fPrintOptions;
};

#endif /* _ShowImageWindow_h */
