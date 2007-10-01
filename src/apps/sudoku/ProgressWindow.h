/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H


#include <Window.h>

class BMessageRunner;
class BStatusBar;


class ProgressWindow : public BWindow {
	public:
		ProgressWindow(BWindow* referenceWindow, BMessage* abortMessage = NULL);
		virtual ~ProgressWindow();

		virtual void MessageReceived(BMessage *message);

		void Start(BWindow* referenceWindow);
		void Stop();

	private:
		void _Center(BWindow* referenceWindow);

		BStatusBar*		fStatusBar;
		BMessageRunner*	fRunner;
		bool			fRetrievedUpdate;
		bool			fRetrievedShow;
};

static const uint32 kMsgProgressStatusUpdate = 'SIup';

#endif	// PROGRESS_WINDOW_H
