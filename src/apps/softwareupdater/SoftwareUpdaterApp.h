/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@tycho.email>
 */
#ifndef _SOFTWARE_UPDATER_APP_H
#define _SOFTWARE_UPDATER_APP_H


#include <Application.h>

#include "WorkingLooper.h"


class SoftwareUpdaterApp : public BApplication {
public:
								SoftwareUpdaterApp();
								~SoftwareUpdaterApp();
			virtual bool		QuitRequested();
			virtual void		ReadyToRun();
			virtual void		ArgvReceived(int32 argc, char **argv);
			void				MessageReceived(BMessage* message);

private:
			WorkingLooper*		fWorker;
			BMessenger			fWindowMessenger;
			bool				fFinalQuitFlag;
			update_type			fActionRequested;
			bool				fVerbose;
			bool				fArgvsAccepted;
};


#endif
