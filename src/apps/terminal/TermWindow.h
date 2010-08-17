/*
 * Copyright 2001-2007, Haiku.
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
#ifndef __TERMWINDOW_H
#define __TERMWINDOW_H


#include <String.h>
#include <Window.h>

class Arguments;
class BFont;
class BMenu;
class BMenuBar;
class FindWindow;
class PrefWindow;
class SmartTabView;
class TermView;
class TermViewContainerView;


class TermWindow : public BWindow {
public:
	TermWindow(BRect frame, const char* title, uint32 workspaces,
		Arguments *args);
	virtual ~TermWindow();

			void	SetSessionWindowTitle(TermView* termView,
						const char* title);
			void	SessionChanged();

protected:
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage *message);
	virtual void	WindowActivated(bool);
	virtual void	MenusBeginning();
	virtual	void	Zoom(BPoint leftTop, float width, float height);
	virtual void	FrameResized(float newWidth, float newHeight);

private:
	struct Session;
	class TabView;
	friend class TabView;

	void			_SetTermColors(TermViewContainerView *termView);
	void			_InitWindow();
	void			_SetupMenu();
	static BMenu*	_MakeEncodingMenu();
	static BMenu*	_MakeWindowSizeMenu();
	
	void			_GetPreferredFont(BFont &font);
	status_t		_DoPageSetup();
	void			_DoPrint();
	void			_AddTab(Arguments *args);
	void			_RemoveTab(int32 index);
	TermViewContainerView* _ActiveTermViewContainerView() const;
	TermViewContainerView* _TermViewContainerViewAt(int32 index) const;
	TermView*		_ActiveTermView() const;
	TermView*		_TermViewAt(int32 index) const;
	int32			_IndexOfTermView(TermView* termView) const;
	void			_CheckChildren();
	void			_ResizeView(TermView *view);
	
	int32			_NewSessionID();

	BString			fInitialTitle;
	BList			fSessions;

	TabView			*fTabView;
	TermView		*fTermView;

	BMenuBar		*fMenubar;
	BMenu			*fFilemenu;
	BMenu			*fEditmenu;
	BMenu			*fEncodingmenu;
	BMenu			*fHelpmenu;
	BMenu			*fWindowSizeMenu;
	BMenu			*fSizeMenu;

	BMessage		*fPrintSettings;
	PrefWindow		*fPrefWindow;
	FindWindow		*fFindPanel;
	BRect			fSavedFrame;
	window_look		fSavedLook;

	// Saved search parameters
	BString			fFindString;
	BMenuItem		*fFindNextMenuItem;
	BMenuItem 		*fFindPreviousMenuItem;
	BMenuItem		*fIncreaseFontSizeMenuItem;
	BMenuItem		*fDecreaseFontSizeMenuItem;

	bool			fFindSelection;
	bool			fForwardSearch;
	bool			fMatchCase;
	bool			fMatchWord;

	bool			fFullScreen;
};

#endif // __TERMWINDOW_H
