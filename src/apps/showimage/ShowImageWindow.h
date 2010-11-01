/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_WINDOW_H
#define SHOW_IMAGE_WINDOW_H


#include <Window.h>

#include "ImageFileNavigator.h"
#include "PrintOptionsWindow.h"


class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuItem;
class ShowImageView;
class ShowImageStatusView;


class ShowImageWindow : public BWindow {
public:
								ShowImageWindow(const entry_ref* ref,
									const BMessenger& trackerMessenger);
	virtual						~ShowImageWindow();

	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			ShowImageView*		GetShowImageView() const { return fImageView; }

			void				UpdateTitle();
			void				AddMenus(BMenuBar* bar);
			void				BuildContextMenu(BMenu* menu);
			void				WindowRedimension(BBitmap* bitmap);

private:
	class RecentDocumentsMenu;

			void				_BuildViewMenu(BMenu* menu, bool popupMenu);
			BMenuItem*			_AddItemMenu(BMenu* menu, const char* label,
									uint32 what, char shortcut, uint32 modifier,
									const BHandler* target,
									bool enabled = true);
			BMenuItem*			_AddDelayItem(BMenu* menu, const char* label,
									float value);

			bool				_ToggleMenuItem(uint32 what);
			void				_EnableMenuItem(BMenu* menu, uint32 what,
									bool enable);
			void				_MarkMenuItem(BMenu* menu, uint32 what,
									bool marked);
			void				_MarkSlideShowDelay(float value);
			void				_ResizeToWindow(bool shrink, uint32 what);

			void				_UpdateStatusText(const BMessage* message);
			void				_LoadError(const entry_ref& ref);
			void				_SaveAs(BMessage* message);
									// Handle Save As submenu choice
			void				_SaveToFile(BMessage* message);
									// Handle save file panel message
			bool				_ClosePrompt();
			void				_ToggleFullScreen();
			void				_LoadSettings();
			void				_SavePrintOptions();
			bool				_PageSetup();
			void				_PrepareForPrint();
			void				_Print(BMessage* msg);

			void				_OpenResizerWindow(int32 width, int32 height);
			void				_UpdateResizerWindow(int32 width, int32 height);
			void				_CloseResizerWindow();

private:
			ImageFileNavigator	fNavigator;
			BFilePanel*			fSavePanel;
			BMenuBar*			fBar;
			BMenu*				fOpenMenu;
			BMenu*				fBrowseMenu;
			BMenu*				fGoToPageMenu;
			BMenu*				fSlideShowDelay;
			ShowImageView*		fImageView;
			ShowImageStatusView* fStatusView;
			bool				fModified;
			bool				fFullScreen;
			bool				fShowCaption;
			BRect				fWindowFrame;
			BMessage*			fPrintSettings;
			PrintOptions		fPrintOptions;
			BMessenger*			fResizerWindowMessenger;
			BMenuItem*			fResizeItem;
			BString				fImageType;
};


#endif	// SHOW_IMAGE_WINDOW_H
