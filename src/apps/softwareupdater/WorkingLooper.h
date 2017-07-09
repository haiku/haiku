/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@tycho.email>
 */
#ifndef _WORKING_LOOPER_H
#define _WORKING_LOOPER_H


#include <Looper.h>
#include <Message.h>

#include "CheckAction.h"
#include "UpdateAction.h"


const uint32 kMsgStart = 'iSTA';


class WorkingLooper : public BLooper {
public:
							WorkingLooper(update_type action, bool verbose);
							~WorkingLooper();
			void			MessageReceived(BMessage* message);

private:
			UpdateAction*	fUpdateAction;
			CheckAction*	fCheckAction;
			update_type		fActionRequested;
			bool			fVerbose;
			
};


#endif // _WORKING_LOOPER_H
