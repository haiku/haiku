/*
 *  Printers Preference Application.
 *  Copyright (C) 2001, 2002 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ADDPRINTERDIALOG_H
#define ADDPRINTERDIALOG_H

class AddPrinterDialog;

#include <Button.h>
#include <String.h>
#include <TextControl.h>
#include <PopUpMenu.h>
#include <Window.h>

class AddPrinterDialog : public BWindow
{
	typedef BWindow Inherited;
public:
	static status_t Start();

private:
				AddPrinterDialog();
	void        MessageReceived(BMessage* msg);

	void		BuildGUI(int stage);
	void        FillMenu(BMenu* menu, const char* path, uint32 what);
	void        AddPortSubMenu(BMenu* menu, const char* transport, const char* port);
	void        Update();

	BTextControl* fName;
	BPopUpMenu*   fPrinter;
	BPopUpMenu*   fTransport;
	BButton*      fOk;
	
	BString       fNameText;
	BString       fPrinterText;
	BString       fTransportText;
	BString       fTransportPathText;
};

#endif
