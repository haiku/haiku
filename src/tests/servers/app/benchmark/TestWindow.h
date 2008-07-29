/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include <Messenger.h>
#include <View.h>
#include <Window.h>


enum {
	MSG_TEST_FINISHED	= 'tstf',
	MSG_TEST_CANCELED	= 'tstc'
};


class Test;

// TestView
class TestView : public BView {
public:
								TestView(BRect frame, Test* test,
									drawing_mode mode,
									const BMessenger& target);

	virtual	void				Draw(BRect updateRect);

private:
			Test*				fTest;
			BMessenger			fTarget;
};

// TestWindow
class TestWindow : public BWindow {
public:
								TestWindow(BRect fame, Test* test,
									drawing_mode mode,
									const BMessenger& target);

	virtual	bool				QuitRequested();

			void				SetAllowedToQuit(bool allowed);
private:
			BMessenger			fTarget;
			bool				fAllowedToQuit;
};

#endif // TEST_WINDOW_H
