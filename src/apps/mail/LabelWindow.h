/*
 * Copyright 2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LABELWINDOW_H
#define _LABELWINDOW_H


#include <Messenger.h>
#include <Window.h>

class BTextControl;


class TLabelWindow : public BWindow {
	public:
		TLabelWindow(BRect frame, BMessenger target);
		virtual ~TLabelWindow();

	protected:
		virtual void MessageReceived(BMessage* message);

	private:
		BButton*		fOK;
		BMessenger		fTarget;
		BTextControl*	fLabel;
};

#endif // #ifndef _LABELWINDOW_H

