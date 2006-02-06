/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_WINDOW_H
#define SHOW_IMAGE_WINDOW_H


#include "PrintOptionsWindow.h"

#include <FilePanel.h>
#include <Menu.h>
#include <String.h>
#include <TranslationDefs.h>
#include <Window.h>

class ShowImageView;
class ShowImageStatusView;

// BMessage field names used in Save messages
#define TRANSLATOR_FLD "be:translator"
#define TYPE_FLD "be:type"

class RecentDocumentsMenu : public BMenu {
	public:
		RecentDocumentsMenu(const char *title, menu_layout layout = B_ITEMS_IN_COLUMN);	
		bool AddDynamicItem(add_state s);

	private:
		void UpdateRecentDocumentsMenu();
};

class ShowImageWindow : public BWindow {
	public:
		ShowImageWindow(const entry_ref *pref, const BMessenger& trackerMessenger);
		virtual ~ShowImageWindow();

		virtual void FrameResized(float width, float height);
		virtual void MessageReceived(BMessage *pmsg);
		virtual bool QuitRequested();
		virtual	void Zoom(BPoint origin, float width, float height);

		status_t InitCheck();
		ShowImageView *GetShowImageView() const { return fImageView; }

		void UpdateTitle();
		void BuildViewMenu(BMenu *menu);
		void AddMenus(BMenuBar *pbar);
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

#endif	// SHOW_IMAGE_WINDOW_H
