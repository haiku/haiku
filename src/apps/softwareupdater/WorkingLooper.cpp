/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 */


#include "WorkingLooper.h"


WorkingLooper::WorkingLooper(update_type action, bool verbose)
	:
	BLooper("WorkingLooper"),
	fUpdateAction(NULL),
	fCheckAction(NULL),
	fActionRequested(action),
	fVerbose(verbose)
{
	Run();
	PostMessage(kMsgStart);
}


WorkingLooper::~WorkingLooper()
{
	delete fUpdateAction;
	delete fCheckAction;
}


void
WorkingLooper::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgStart:
		{
			if (fActionRequested == UPDATE_CHECK_ONLY) {
				fCheckAction = new CheckAction(fVerbose);
				fCheckAction->Perform();
			} else {
				fUpdateAction = new UpdateAction(fVerbose);
				fUpdateAction->Perform(fActionRequested);
			}
			break;
		}
		
		default:
			BLooper::MessageReceived(message);
	}
}
