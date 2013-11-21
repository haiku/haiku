/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <FilePanel.h>
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
		BMenu* fDatabitsMenu;
		BMenu* fStopbitsMenu;
		BMenu* fParityMenu;
		BMenu* fFlowcontrolMenu;
		BMenu* fBaudrateMenu;
		BFilePanel* fLogFilePanel;

		static const int kBaudrates[];
		static const char* kWindowTitle;
};
