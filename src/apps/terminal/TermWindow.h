/*
 * Copyright 2001-2010, Haiku.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#ifndef TERM_WINDOW_H
#define TERM_WINDOW_H


#include <MessageRunner.h>
#include <String.h>
#include <Window.h>

#include "SmartTabView.h"
#include "TermView.h"


class Arguments;
class BFont;
class BMenu;
class BMenuBar;
class FindWindow;
class PrefWindow;
class TermViewContainerView;


class TermWindow : public BWindow, private SmartTabView::Listener,
	private TermView::Listener {
public:
								TermWindow(BRect frame, const BString& title,
									bool isUserDefinedTitle, int32 windowIndex,
									uint32 workspaces, Arguments* args);
	virtual						~TermWindow();

			void				SessionChanged();

protected:
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);
	virtual void				WindowActivated(bool);
	virtual void				MenusBeginning();
	virtual	void				Zoom(BPoint leftTop, float width, float height);
	virtual void				FrameResized(float newWidth, float newHeight);

private:
	// SmartTabView::Listener
	virtual	void				TabSelected(SmartTabView* tabView, int32 index);
	virtual	void				TabDoubleClicked(SmartTabView* tabView,
									BPoint point, int32 index);
	virtual	void				TabMiddleClicked(SmartTabView* tabView,
									BPoint point, int32 index);
	virtual	void				TabRightClicked(SmartTabView* tabView,
									BPoint point, int32 index);

	// TermView::Listener
	virtual	void				NotifyTermViewQuit(TermView* view,
									int32 reason);
	virtual	void				SetTermViewTitle(TermView* view,
									const char* title);
	virtual	void				PreviousTermView(TermView* view, bool move);
	virtual	void				NextTermView(TermView* view, bool move);

private:
			struct Title {
				BString			title;
				BString			pattern;
				bool			patternUserDefined;
			};

			struct SessionID {
								SessionID(int32 id);
								SessionID(const BMessage& message,
									const char* field);

				status_t		AddToMessage(BMessage& message,
									const char* field) const;

			private:
				int32			fID;
			};

			struct Session;

			void				_SetTermColors(TermViewContainerView* termView);
			void				_InitWindow();
			void				_SetupMenu();
			static BMenu*		_MakeEncodingMenu();
			static BMenu*		_MakeWindowSizeMenu();

			void				_GetPreferredFont(BFont &font);
			status_t			_DoPageSetup();
			void				_DoPrint();

			void				_NewTab();
			void				_AddTab(Arguments* args,
									const BString& currentDirectory
										= BString());
			void				_RemoveTab(int32 index);
			void				_NavigateTab(int32 index, int32 direction,
									bool move);

			bool				_CanClose(int32 index);
			TermViewContainerView* _ActiveTermViewContainerView() const;
			TermViewContainerView* _TermViewContainerViewAt(int32 index) const;
			TermView*			_ActiveTermView() const;
			TermView*			_TermViewAt(int32 index) const;
			int32				_IndexOfTermView(TermView* termView) const;
			void				_CheckChildren();
			void				_ResizeView(TermView* view);

			void				_TitleSettingsChanged();
			void				_UpdateTitles();
			void				_UpdateSessionTitle(int32 index);

			SessionID			_NewSessionID();
			int32				_NewSessionIndex();

private:
			Title				fTitle;
			BString				fSessionTitlePattern;
			int32				fWindowIndex;
			BMessageRunner		fTitleUpdateRunner;

			BList				fSessions;
			int32				fNextSessionID;

			SmartTabView*		fTabView;
			TermView*			fTermView;

			BMenuBar*			fMenubar;
			BMenu*				fFilemenu;
			BMenu*				fEditmenu;
			BMenu*				fEncodingmenu;
			BMenu*				fHelpmenu;
			BMenu*				fWindowSizeMenu;
			BMenu*				fSizeMenu;

			BMessage*			fPrintSettings;
			PrefWindow*			fPrefWindow;
			FindWindow*			fFindPanel;
			BRect				fSavedFrame;
			window_look			fSavedLook;

			// Saved search parameters
			BString				fFindString;
			BMenuItem*			fFindNextMenuItem;
			BMenuItem *			fFindPreviousMenuItem;
			BMenuItem*			fIncreaseFontSizeMenuItem;
			BMenuItem*			fDecreaseFontSizeMenuItem;

			bool				fFindSelection;
			bool				fForwardSearch;
			bool				fMatchCase;
			bool				fMatchWord;

			bool				fFullScreen;
};


#endif // TERM_WINDOW_H
