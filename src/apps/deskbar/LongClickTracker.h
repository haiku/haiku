/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner <alex@zappotek.com>
 */

/*
 * LongClickTracker:
 * This class provides a way to track long clicks asynchronously.
 * Set the view to track and the code of the message you want to receive.
 * When a long click occurs in the view, a message will be sent to the view.
 * It contains the coordinates of the click (last postion) in a BPoint:"where"
 * 
 * Could be extended to be more parametrable and/or support other kind of
 * complex mouse tracking. This could also be done on the input server side 
 * eventually, and use this class for R5 backward compatibility.
 * 
 * Call Start() after the view has been attached to a window.
 */

#ifndef _LONG_CLICK_TRACKER_H_
#define _LONG_CLICK_TRACKER_H_

#include <Messenger.h>
#include <OS.h>

class BView;
class BMessenger;

class LongClickTracker
{
	public:
							LongClickTracker(BView *view, uint32 messageWhat);
							~LongClickTracker();

			status_t		Start();

	protected:
			static int32	_ThreadEntry(void *that);
			void			_Track();

			BView*			fView;
			BMessenger* 	fMessenger;
			int32			fMessageWhat;
			thread_id		fThread;
			bool			fQuit;
			bigtime_t		fLongClickThreshold;
};

#endif // _LONG_CLICK_TRACKER_H_
