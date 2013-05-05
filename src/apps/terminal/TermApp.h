/*
 * Copyright 2001-2013, Haiku.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERM_APP_H
#define TERM_APP_H


#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <String.h>

#include "Colors.h"

class Arguments;
class BRect;
class BWindow;


class TermApp : public BApplication {
public:
								TermApp();
	virtual						~TermApp();

	static const rgb_color*		DefaultPalette()
									{ return fDefaultPalette; }
protected:
	virtual	void				ReadyToRun();
	virtual bool				QuitRequested();
	virtual	void				Quit();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ArgvReceived(int32 argc, char** argv);

private:
			status_t			_MakeTermWindow();

			void				_HandleChildCleanup();
	static	void				_SigChildHandler(int signal, void* data);
	static	status_t			_ChildCleanupThread(void* data);

			void				_Usage(char *name);
			void				_InitDefaultPalette();
private:
			bool				fStartFullscreen;
			BString				fWindowTitle;

			BWindow*			fTermWindow;
			Arguments*			fArgs;
	static	rgb_color			fDefaultPalette[kTermColorCount];
};


#endif	// TERM_APP_H
