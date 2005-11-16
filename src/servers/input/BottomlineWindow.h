/*
 * Copyright 2004-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef BOTTOMLINE_WINDOW_H
#define BOTTOMLINE_WINDOW_H


#include "InputServer.h"

#include <Message.h>
#include <Window.h>

class BTextView;


class BottomlineWindow : public BWindow {
	public:
		BottomlineWindow();
		virtual ~BottomlineWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

		void HandleInputMethodEvent(BMessage* event, EventList& newEvents);

	private:
		BTextView *fTextView;
};

#endif	// BOTTOMLINE_WINDOW_H
