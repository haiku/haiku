/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill <supernova@warpmail.net>
 */
#ifndef _WORKING_LOOPER_H
#define _WORKING_LOOPER_H


#include <Looper.h>
#include <Message.h>

#include "UpdateAction.h"


const uint32 kMsgStart = 'iSTA';


class WorkingLooper : public BLooper {
public:
							WorkingLooper();
			void			MessageReceived(BMessage* message);

private:
			UpdateAction	fAction;
};


#endif // _WORKING_LOOPER_H
