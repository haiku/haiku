/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2013-2014 Haiku, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef BROWSER_WINDOW_H
#define BROWSER_WINDOW_H


#include "WebWindow.h"
#include <Messenger.h>
#include <String.h>

class BButton;
class BCheckBox;
class BDirectory;
class BFile;
class BFilePanel;
class BLayoutItem;
class BMenu;
class BMenuItem;
class BMessageRunner;
class BPath;
class BStatusBar;
class BStringView;
class BTextControl;
class BUrlContext;
class BWebView;

class BookmarkBar;
class SettingsMessage;
class TabManager;
class URLInputGroup;

namespace BPrivate {
	class BIconButton;
}

using BPrivate::BIconButton;

enum {
	INTERFACE_ELEMENT_MENU			= 1 << 0,
	INTERFACE_ELEMENT_TABS			= 1 << 1,
	INTERFACE_ELEMENT_NAVIGATION	= 1 << 2,
	INTERFACE_ELEMENT_STATUS		= 1 << 3,

	INTERFACE_ELEMENT_ALL			= 0xffff
};

enum NewPagePolicy {
	OpenBlankPage					= 0,
	OpenStartPage					= 1,
	OpenSearchPage					= 2,
	CloneCurrentPage				= 3
};

enum StartUpPolicy {
	ResumePriorSession				= 0,
	StartNewSession					= 1
};

enum {
	NEW_WINDOW						= 'nwnd',
	NEW_TAB							= 'ntab',
	WINDOW_OPENED					= 'wndo',
	WINDOW_CLOSED					= 'wndc',
	SHOW_DOWNLOAD_WINDOW			= 'sdwd',
	SHOW_SETTINGS_WINDOW			= 'sswd',
	SHOW_CONSOLE_WINDOW				= 'scwd',
	SHOW_COOKIE_WINDOW				= 'skwd'
};

#define INTEGRATE_MENU_INTO_TAB_BAR 0


class BrowserWindow : public BWebWindow {
public:
								BrowserWindow(BRect frame,
									SettingsMessage* appSettings,
									const BString& url,
									BUrlContext* context,
									uint32 interfaceElements
										= INTERFACE_ELEMENT_ALL,
									BWebView* webView = NULL);
	virtual						~BrowserWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual	void				MessageReceived(BMessage* message);
	virtual	status_t			Archive(BMessage* archive, bool deep =true) const;
	virtual	bool				QuitRequested();
	virtual	void				MenusBeginning();
	virtual	void				MenusEnded();

	virtual void				ScreenChanged(BRect screenSize,
									color_space format);
	virtual void				WorkspacesChanged(uint32 oldWorkspaces,
									uint32 newWorkspaces);

	virtual	void				SetCurrentWebView(BWebView* view);

			bool				IsBlankTab() const;
			void				CreateNewTab(const BString& url, bool select,
									BWebView* webView = 0);

			BRect				WindowFrame() const;

			void				ToggleFullscreen();

private:
	// WebPage notification API implementations
	virtual	void				NavigationRequested(const BString& url,
									BWebView* view);
	virtual	void				NewWindowRequested(const BString& url,
									bool primaryAction);
	virtual	void				CloseWindowRequested(BWebView* view);
	virtual	void				NewPageCreated(BWebView* view,
									BRect windowFrame, bool modalDialog,
									bool resizable, bool activate);
	virtual	void				LoadNegotiating(const BString& url,
									BWebView* view);
	virtual	void				LoadCommitted(const BString& url,
									BWebView* view);
	virtual	void				LoadProgress(float progress, BWebView* view);
	virtual	void				LoadFailed(const BString& url, BWebView* view);
	virtual	void				LoadFinished(const BString& url,
									BWebView* view);
	virtual	void				MainDocumentError(const BString& failingURL,
									const BString& localizedDescription,
									BWebView* view);
	virtual	void				TitleChanged(const BString& title,
									BWebView* view);
	virtual	void				IconReceived(const BBitmap* icon,
									BWebView* view);
	virtual	void				ResizeRequested(float width, float height,
									BWebView* view);
	virtual	void				SetToolBarsVisible(bool flag, BWebView* view);
	virtual	void				SetStatusBarVisible(bool flag, BWebView* view);
	virtual	void				SetMenuBarVisible(bool flag, BWebView* view);
	virtual	void				SetResizable(bool flag, BWebView* view);
	virtual	void				StatusChanged(const BString& status,
									BWebView* view);
	virtual	void				NavigationCapabilitiesChanged(
									bool canGoBackward, bool canGoForward,
									bool canStop, BWebView* view);
	virtual	void				UpdateGlobalHistory(const BString& url);
	virtual	bool				AuthenticationChallenge(BString message,
									BString& inOutUser, BString& inOutPassword,
									bool& inOutRememberCredentials,
									uint32 failureCount, BWebView* view);

private:
			void				_UpdateTitle(const BString &title);
			void				_UpdateTabGroupVisibility();
			bool				_TabGroupShouldBeVisible() const;
			void				_ShutdownTab(int32 index);
			void				_TabChanged(int32 index);

			status_t			_BookmarkPath(BPath& path) const;
			void				_CreateBookmark();
			void				_ShowBookmarks();
			bool				_CheckBookmarkExists(BDirectory& directory,
									const BString& fileName,
									const BString& url) const;
			bool				_ReadURLAttr(BFile& bookmarkFile,
									BString& url) const;
			void				_AddBookmarkURLsRecursively(
									BDirectory& directory,
									BMessage* message,
									uint32& addedCount) const;

			void				_SetPageIcon(BWebView* view,
									const BBitmap* icon);

			void				_InitSearchEngines();

			void				_UpdateHistoryMenu();
			void				_UpdateClipboardItems();

			bool				_ShowPage(BWebView* view);

			void				_ToggleFullscreen();
			void				_ResizeToScreen();
			void				_SetAutoHideInterfaceInFullscreen(bool doIt);
			void				_CheckAutoHideInterface();
			void				_ShowInterface(bool show);
			void				_ShowProgressBar(bool);
			void				_InvokeButtonVisibly(BButton* button);

			BString				_NewTabURL(bool isNewWindow) const;

			BString				_EncodeURIComponent(const BString& search);
			void				_VisitURL(const BString& url);
			void				_VisitSearchEngine(const BString& search);
	inline 	bool				_IsValidDomainChar(char ch);
			void 				_SmartURLHandler(const BString& url);

			void				_HandlePageSourceResult(
									const BMessage* message);

			void				_ShowBookmarkBar(bool show);

private:
			BMenu*				fHistoryMenu;
			int32				fHistoryMenuFixedItemCount;

			BMenuItem*			fCutMenuItem;
			BMenuItem*			fCopyMenuItem;
			BMenuItem*			fPasteMenuItem;
			BMenuItem*			fFindPreviousMenuItem;
			BMenuItem*			fFindNextMenuItem;
			BMenuItem*			fZoomTextOnlyMenuItem;
			BMenuItem*			fFullscreenItem;
			BMenuItem*			fBackMenuItem;
			BMenuItem*			fForwardMenuItem;

			BIconButton*		fBackButton;
			BIconButton*		fForwardButton;
			BIconButton*		fStopButton;
			BIconButton*		fHomeButton;
			URLInputGroup*		fURLInputGroup;
			BStringView*		fStatusText;
			BStatusBar*			fLoadingProgressBar;

			BLayoutItem*		fMenuGroup;
			BLayoutItem*		fTabGroup;
			BLayoutItem*		fNavigationGroup;
			BLayoutItem*		fFindGroup;
			BLayoutItem*		fStatusGroup;
			BLayoutItem*		fToggleFullscreenButton;

			BTextControl*		fFindTextControl;
			BButton*			fFindPreviousButton;
			BButton*			fFindNextButton;
			BButton*			fFindCloseButton;
			BCheckBox*			fFindCaseSensitiveCheckBox;
			TabManager*			fTabManager;

			bool				fIsFullscreen;
			bool				fInterfaceVisible;
			bool				fMenusRunning;
			BRect				fNonFullscreenWindowFrame;
			BMessageRunner*		fPulseRunner;
			uint32				fVisibleInterfaceElements;
			bigtime_t			fLastMouseMovedTime;
			BPoint				fLastMousePos;

			BUrlContext*		fContext;

			// cached settings
			SettingsMessage*	fAppSettings;
			bool				fZoomTextOnly;
			bool				fShowTabsIfSinglePageOpen;
			bool				fAutoHideInterfaceInFullscreenMode;
			bool				fAutoHidePointer;
			uint32				fNewWindowPolicy;
			uint32				fNewTabPolicy;
			BString				fStartPageURL;
			BString				fSearchPageURL;

			BMenuItem*			fBookmarkBarMenuItem;
			BookmarkBar*		fBookmarkBar;
			BFilePanel*			fSavePanel;
			int					kSearchEngineCount = 8;
			struct SearchEngine {
				const char* shortcut;
				const char* url;
			};

			// FIXME use a BObjectList
			SearchEngine*		fSearchEngines;
};


#endif // BROWSER_WINDOW_H
