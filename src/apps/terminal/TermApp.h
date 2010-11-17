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
#ifndef TERM_APP_H
#define TERM_APP_H


#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <String.h>


class Arguments;
class BRect;
class BWindow;


class TermApp : public BApplication {
public:
								TermApp();
	virtual						~TermApp();

protected:
	virtual	void				ReadyToRun();
	virtual bool				QuitRequested();
	virtual	void				Quit();
	virtual	void				AboutRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ArgvReceived(int32 argc, char** argv);

private:
			status_t			_MakeTermWindow(BRect& frame,
									uint32 workspaces);
			status_t			_GetWindowPositionFile(BFile* file,
									uint32 openMode);
			status_t			_LoadWindowPosition(BRect* frame,
									uint32* workspaces);
			status_t			_SaveWindowPosition(BMessage* message);
			void				_SwitchTerm();
			void				_ActivateTermWindow(team_id id);
			bool				_IsSwitchTarget(team_id id);
			void				_SanitizeIDs(BMessage* data, uint8* windows,
									ssize_t length);
			bool				_UpdateIDs(bool set, uint8* windows,
									ssize_t maxLength, ssize_t* _length);
			void				_UpdateRegistration(bool set);
			void				_UnregisterTerminal();
			void				_RegisterTerminal();

			void				_HandleChildCleanup();
	static	void				_SigChildHandler(int signal, void* data);
	static	status_t			_ChildCleanupThread(void* data);

			void				_Usage(char *name);

private:
			bool				fStartFullscreen;
			BString				fWindowTitle;
			bool				fWindowTitleUserDefined;
			int32				fWindowNumber;

			BWindow*			fTermWindow;
			BRect				fTermFrame;
			uint32				fTermWorkspaces;
			Arguments*			fArgs;
};


#endif	// TERM_APP_H
