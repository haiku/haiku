/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SCREEN_SAVER_WINDOW_H
#define SCREEN_SAVER_WINDOW_H


#include "ScreenSaver.h"

#include <DirectWindow.h>


class ScreenSaverWindow : public BDirectWindow {
	public:
		ScreenSaverWindow(BRect frame);
		~ScreenSaverWindow();

		void SetSaver(BScreenSaver *saver);

		virtual bool QuitRequested();
		virtual void DirectConnected(direct_buffer_info *info);

	private:
		BView *fTopView;
		BScreenSaver *fSaver;
};

#endif	// SCREEN_SAVER_WINDOW_H
