/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROGRESS_WINDOW_H
#define PROGRESS_WINDOW_H


#include <Window.h>


class BMessageRunner;
class BStatusBar;


// public message constants
enum {
	kMsgProgressUpdate = 'pwPU'
};


class ProgressWindow : public BWindow {
public:
								ProgressWindow();
	virtual						~ProgressWindow();

	virtual void 				MessageReceived(BMessage* message);

			void				Start(BWindow* referenceWindow,
									bool center = false);
			void				Stop();

private:
			BStatusBar*			fStatusBar;
			BMessageRunner*		fRunner;
			bool				fRetrievedUpdate;
			bool				fRetrievedShow;
};


#endif	// PROGRESS_WINDOW_H
