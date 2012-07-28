/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <Window.h>


class TermView;


class SerialWindow: public BWindow
{
	public:
		SerialWindow();

		void MessageReceived(BMessage* message);

	private:
		TermView* fTermView;

		static const char* kWindowTitle;
};
