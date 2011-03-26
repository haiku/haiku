/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 */
#ifndef SHOW_IMAGE_APP_H
#define SHOW_IMAGE_APP_H


#include "ShowImageSettings.h"

#include <Application.h>
#include <FilePanel.h>


enum {
	MSG_FILE_OPEN				= 'mFOP',
};


class ShowImageApp : public BApplication {
public:
								ShowImageApp();
	virtual						~ShowImageApp();

	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				ReadyToRun();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Pulse();
	virtual	void				RefsReceived(BMessage* message);
	virtual	bool				QuitRequested();

			ShowImageSettings* 	Settings() { return &fSettings; }

private:
			void				_StartPulse();
			void				_Open(const entry_ref& ref,
									const BMessenger& trackerMessenger);
			void				_BroadcastToWindows(BMessage* message);
			void				_CheckClipboard();
			void				_UpdateLastWindowFrame();

private:
			BFilePanel*			fOpenPanel;
			bool				fPulseStarted;
			ShowImageSettings	fSettings;
			BRect				fLastWindowFrame;
};


extern const char* kApplicationSignature;

#define my_app dynamic_cast<ShowImageApp*>(be_app)


#endif	// SHOW_IMAGE_APP_H
