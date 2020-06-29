/*
 * Copyright 2003-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_WINDOW_H
#define SHOW_IMAGE_WINDOW_H


#include <ToolBar.h>
#include <Window.h>

#include "ImageFileNavigator.h"
#include "PrintOptionsWindow.h"


class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuItem;
class BMessageRunner;
class BScrollBar;
class BScrollView;
class ProgressWindow;
class ShowImageView;
class ShowImageStatusView;


// public message constants
enum {
	MSG_MODIFIED				= 'mMOD',
	MSG_UPDATE_STATUS			= 'mUPS',
	MSG_UPDATE_STATUS_TEXT		= 'mUPT',
	MSG_UPDATE_STATUS_ZOOM		= 'mUPZ',
	MSG_SELECTION				= 'mSEL',
	MSG_FILE_NEXT				= 'mFLN',
	MSG_FILE_PREV				= 'mFLP',
	kMsgDeleteCurrentFile		= 'mDcF',
	MSG_SLIDE_SHOW				= 'mSSW',
	kMsgStopSlideShow			= 'msss',
	MSG_FULL_SCREEN				= 'mFSC',
	kShowToolBarIfEnabled		= 'mSTE',
	MSG_EXIT_FULL_SCREEN		= 'mEFS',
	MSG_WINDOW_HAS_QUIT			= 'wndq'
};


class ShowImageWindow : public BWindow {
public:
								ShowImageWindow(BRect frame,
									const entry_ref& ref,
									const BMessenger& trackerMessenger);
	virtual						~ShowImageWindow();

			void				BuildContextMenu(BMenu* menu);

protected:
	virtual void				Zoom(BPoint origin, float width, float height);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_AddMenus(BMenuBar* bar);
			void				_ResizeWindowToImage();
			void				_BuildViewMenu(BMenu* menu, bool popupMenu);
			BMenu*				_BuildRatingMenu();
			BMenuItem*			_AddItemMenu(BMenu* menu, const char* label,
									uint32 what, char shortcut, uint32 modifier,
									const BHandler* target,
									bool enabled = true);
			BMenuItem*			_AddDelayItem(BMenu* menu, const char* label,
									bigtime_t delay);

			bool				_ToggleMenuItem(uint32 what);
			void				_EnableMenuItem(BMenu* menu, uint32 what,
									bool enable);
			void				_MarkMenuItem(BMenu* menu, uint32 what,
									bool marked);
			void				_MarkSlideShowDelay(bigtime_t delay);

			void				_UpdateStatusText(const BMessage* message);
			void				_LoadError(const entry_ref& ref);
			void				_SaveAs(BMessage* message);
									// Handle Save As submenu choice
			void				_SaveToFile(BMessage* message);
									// Handle save file panel message
			bool				_ClosePrompt();
			status_t			_LoadImage(bool forward = true);
			bool				_PreloadImage(bool forward, entry_ref& ref);
			void				_ToggleFullScreen();
			void				_ApplySettings();
			void				_SavePrintOptions();
			bool				_PageSetup();
			void				_PrepareForPrint();
			void				_Print(BMessage* msg);

			void				_SetSlideShowDelay(bigtime_t delay);
			void				_StartSlideShow();
			void				_StopSlideShow();
			void				_ResetSlideShowDelay();

			void				_UpdateRatingMenu();

			void				_GetFileInfo(const entry_ref& ref);

			void				_SetToolBarVisible(bool visible,
									bool animate = false);
			void				_SetToolBarBorder(bool visible);

			void				_SaveWidthAndHeight();

private:
			ImageFileNavigator	fNavigator;
			BFilePanel*			fSavePanel;
			BMenuBar*			fBar;
			BMenu*				fBrowseMenu;
			BMenu*				fGoToPageMenu;
			BMenu*				fSlideShowDelayMenu;
			BMenu*				fRatingMenu;
			BToolBar*			fToolBar;
			bool				fToolBarVisible;
			BScrollView*		fScrollView;
			ShowImageView*		fImageView;
			ShowImageStatusView* fStatusView;
			ProgressWindow*		fProgressWindow;
			bool				fModified;
			bool				fFullScreen;
			bool				fShowCaption;
			bool				fShowToolBar;
			BRect				fWindowFrame;
			BMessage*			fPrintSettings;
			PrintOptions		fPrintOptions;

			BString				fImageType;

			BMessageRunner*		fSlideShowRunner;
			bigtime_t			fSlideShowDelay;
};


#endif	// SHOW_IMAGE_WINDOW_H
