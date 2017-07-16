/*
 * Copyright 2012-2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT licence.
 */


#include <Window.h>


class BFilePanel;
class BMenu;
class BStatusBar;
class TermView;


class SerialWindow: public BWindow
{
	public:
					SerialWindow();
					~SerialWindow();

		void		MenusBeginning();
		void		MessageReceived(BMessage* message);


	private:
						TermView*		fTermView;

						BMenu*			fConnectionMenu;
						BMenu*			fDatabitsMenu;
						BMenu*			fStopbitsMenu;
						BMenu*			fParityMenu;
						BMenu*			fFlowcontrolMenu;
						BMenu*			fBaudrateMenu;
						BMenu*			fLineTerminatorMenu;
						BMenu*			fFileMenu;
						BFilePanel*		fLogFilePanel;
						BStatusBar*		fStatusBar;

		static const	int				kBaudrates[];
		static const	int				kBaudrateConstants[];
		static const	char*			kWindowTitle;
};
