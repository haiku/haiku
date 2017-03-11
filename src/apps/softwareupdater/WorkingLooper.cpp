/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */


#include "WorkingLooper.h"


WorkingLooper::WorkingLooper()
	:
	BLooper("WorkingLooper")
{
	Run();
	PostMessage(kMsgStart);
}


void
WorkingLooper::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgStart:
		{
			fAction.Perform();
			break;
		}
		
		default:
			BLooper::MessageReceived(message);
	}
}
