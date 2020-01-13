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


#include <InterfaceDefs.h>
#include <MessageRunner.h>
#include <String.h>
#include <Window.h>

#include "SmartTabView.h"
#include "SetTitleDialog.h"
#include "TerminalRoster.h"
#include "TermView.h"


class Arguments;
class BFile;
class BFont;
class BMenu;
class BMenuBar;
class FindWindow;
class PrefWindow;
class TermViewContainerView;


class TermWindow : public BWindow, private SmartTabView::Listener,
	private TermView::Listener, private SetTitleDialog::Listener,
	private TerminalRoster::Listener {
public:
								TermWindow(const BString& title,
									Arguments* args);
	virtual						~TermWindow();

			void				SessionChanged();

	static	void				MakeEncodingMenu(BMenu*);
	static	void				MakeWindowSizeMenu(BMenu*);

protected:
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);
	virtual void				WindowActivated(bool activated);
	virtual void				MenusBeginning();
	virtual	void				Zoom(BPoint leftTop, float width, float height);
	virtual void				FrameResized(float newWidth, float newHeight);
	virtual void				WorkspacesChanged(uint32 oldWorkspaces,
									uint32 newWorkspaces);
	virtual void				WorkspaceActivated(int32 workspace,
									bool state);
	virtual void				Minimize(bool minimize);

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
	virtual	void				PreviousTermView(TermView* view);
	virtual	void				NextTermView(TermView* view);

	// SetTitleDialog::Listener
	virtual	void				TitleChanged(SetTitleDialog* dialog,
									const BString& title,
									bool titleUserDefined);
	virtual	void				SetTitleDialogDone(SetTitleDialog* dialog);

	// TerminalRoster::Listener
	virtual	void				TerminalInfosUpdated(TerminalRoster* roster);

private:
			struct Title {
				BString			title;
				BString			pattern;
				bool			patternUserDefined;
			};

			struct SessionID {
								SessionID(int32 id = -1);
								SessionID(const BMessage& message,
									const char* field);

				bool			IsValid() const		{ return fID >= 0; }

				status_t		AddToMessage(BMessage& message,
									const char* field) const;

				bool			operator==(const SessionID& other) const
									{ return fID == other.fID; }
				bool			operator!=(const SessionID& other) const
									{ return !(*this == other); }

			private:
				int32			fID;
			};

			struct Session;

private:
			void				_SetTermColors(
									TermViewContainerView* containerView);
			void				_InitWindow();
			void				_SetupMenu();
	static	BMenu*				_MakeFontSizeMenu(uint32 command,
									uint8 defaultSize);
			void				_UpdateSwitchTerminalsMenuItem();

			status_t			_GetWindowPositionFile(BFile* file,
									uint32 openMode);
			status_t			_LoadWindowPosition(BRect* frame,
									uint32* workspaces);
			status_t			_SaveWindowPosition();

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
	inline	Session*			_SessionAt(int32 index) const;
			Session*			_SessionForID(const SessionID& sessionID) const;
	inline	int32				_IndexOfSession(Session* session) const;

			void				_CheckChildren();
			void				_ResizeView(TermView* view);

			void				_TitleSettingsChanged();
			void				_UpdateTitles();
			void				_UpdateSessionTitle(int32 index);
			void				_OpenSetTabTitleDialog(int32 index);
			void				_OpenSetWindowTitleDialog();
			void				_FinishTitleDialog();

			void				_SwitchTerminal();
			team_id				_FindSwitchTerminalTarget();

			SessionID			_NewSessionID();
			int32				_NewSessionIndex();

			void				_MoveWindowInScreen(BWindow* window);

			void				_UpdateKeymap();

private:
			TerminalRoster		fTerminalRoster;

			Title				fTitle;
			BString				fSessionTitlePattern;
			BMessageRunner		fTitleUpdateRunner;

			BList				fSessions;
			int32				fNextSessionID;

			SmartTabView*		fTabView;

			BMenuBar*			fMenuBar;
			BMenuItem*			fSwitchTerminalsMenuItem;
			BMenu*				fEncodingMenu;
			BMenu*				fFontSizeMenu;

			BMessage*			fPrintSettings;
			PrefWindow*			fPrefWindow;
			FindWindow*			fFindPanel;
			BRect				fSavedFrame;
			window_look			fSavedLook;

			SetTitleDialog*		fSetWindowTitleDialog;
			SetTitleDialog*		fSetTabTitleDialog;
			SessionID			fSetTabTitleSession;

			// Saved search parameters
			BString				fFindString;
			BMenuItem*			fFindNextMenuItem;
			BMenuItem*			fFindPreviousMenuItem;
			BMenuItem*			fIncreaseFontSizeMenuItem;
			BMenuItem*			fDecreaseFontSizeMenuItem;

			bool				fFindSelection;
			bool				fForwardSearch;
			bool				fMatchCase;
			bool				fMatchWord;

			bool				fFullScreen;

			key_map*			fKeymap;
			char*				fKeymapChars;
};


#endif // TERM_WINDOW_H
