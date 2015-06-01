/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include <Application.h>
#include <MessageRunner.h>

#ifndef GREP_APP_H
#define GREP_APP_H

class GrepApp : public BApplication {
public:
								GrepApp();
	virtual						~GrepApp();

	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();

private:
			void				_TryQuit();
			void				_NewUnfocusedGrepWindow();

			bool				fGotArgvOnStartup;
			bool				fGotRefsOnStartup;

			BMessageRunner*		fQuitter;
};

#endif	// GREP_APP_H
