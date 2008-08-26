/*
 * Copyright 2003-2008, Haiku, Inc. All Rights Reserved.
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


#include <Window.h>


class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuItem;
class ShowImageView;
class ShowImageStatusView;


class ShowImageWindow : public BWindow {
	public:
		ShowImageWindow(const entry_ref *ref, const BMessenger& trackerMessenger);
		virtual ~ShowImageWindow();

		virtual void FrameResized(float width, float height);
		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
		virtual void ScreenChanged(BRect frame, color_space mode);
		// virtual	void Zoom(BPoint origin, float width, float height);

		status_t InitCheck();
		ShowImageView *GetShowImageView() const { return fImageView; }

		void UpdateTitle();
		void AddMenus(BMenuBar *bar);
		void BuildContextMenu(BMenu *menu);
		void WindowRedimension(BBitmap *bitmap);

	private:
		class RecentDocumentsMenu;

		void BuildViewMenu(BMenu *menu, bool popupMenu);
		BMenuItem *AddItemMenu(BMenu *menu, const char *label,
			uint32 what, const char shortcut, uint32 modifier,
			const BHandler *target, bool enabled = true);
		BMenuItem* AddDelayItem(BMenu *menu, const char *label, float value);

		bool ToggleMenuItem(uint32 what);
		void EnableMenuItem(BMenu *menu, uint32 what, bool enable);
		void MarkMenuItem(BMenu *menu, uint32 what, bool marked);
		void MarkSlideShowDelay(float value);
		void ResizeToWindow(bool shrink, uint32 what);

		void SaveAs(BMessage *message);
			// Handle Save As submenu choice
		void SaveToFile(BMessage *message);
			// Handle save file panel message
		bool ClosePrompt();
		void ToggleFullScreen();
		void LoadSettings();
		void SavePrintOptions();
		bool PageSetup();
		void PrepareForPrint();
		void Print(BMessage *msg);

		void OpenResizerWindow(int32 width, int32 height);
		void UpdateResizerWindow(int32 width, int32 height);
		void CloseResizerWindow();

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
		BMessenger *fResizerWindowMessenger;
		BMenuItem *fResizeItem;
		int32 fHeight, fWidth;
};

#endif	// SHOW_IMAGE_WINDOW_H
