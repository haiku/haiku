/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <Window.h>


class BMenu;
class TermView;


class SerialWindow: public BWindow
{
	public:
		SerialWindow();
		~SerialWindow();

		void MenusBeginning();
		void MessageReceived(BMessage* message);

	private:
		TermView* fTermView;
		BMenu* fConnectionMenu;
		BFilePanel* fLogFilePanel;

		static const char* kWindowTitle;
};
