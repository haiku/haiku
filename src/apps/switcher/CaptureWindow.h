/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CAPTURE_WINDOW_H
#define CAPTURE_WINDOW_H


#include <Window.h>


class CaptureView;


class CaptureWindow : public BWindow {
public:
								CaptureWindow();
	virtual						~CaptureWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			CaptureView*		fCaptureView;
};


#endif	// CAPTURE_WINDOW_H
