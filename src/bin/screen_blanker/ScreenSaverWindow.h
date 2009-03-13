/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SCREEN_SAVER_WINDOW_H
#define SCREEN_SAVER_WINDOW_H


#include "ScreenSaver.h"

#include <DirectWindow.h>
#include <MessageFilter.h>


const static uint32 kMsgEnableFilter = 'eflt';


class ScreenSaverFilter : public BMessageFilter {
	public:
		ScreenSaverFilter()
			: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
			fEnabled(true) {}

		virtual filter_result Filter(BMessage* message, BHandler** target);

		void SetEnabled(bool enabled);
	
	private:
		bool fEnabled;
};


class ScreenSaverWindow : public BDirectWindow {
	public:
		ScreenSaverWindow(BRect frame);
		~ScreenSaverWindow();

		void SetSaver(BScreenSaver *saver);

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
		virtual void DirectConnected(direct_buffer_info *info);

	private:
		BView *fTopView;
		BScreenSaver *fSaver;
		ScreenSaverFilter *fFilter;
};

#endif	// SCREEN_SAVER_WINDOW_H
