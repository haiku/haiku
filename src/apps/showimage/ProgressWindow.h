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
		ProgressWindow(BWindow* referenceWindow);
		virtual ~ProgressWindow();

		virtual void MessageReceived(BMessage *message);

		void Start();
		void Stop();

	private:
		BStatusBar*		fStatusBar;
		BMessageRunner*	fRunner;
		bool			fRetrievedUpdate;
		bool			fRetrievedShow;
};

#endif	// PROGRESS_WINDOW_H
