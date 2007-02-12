/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Michael Pfeiffer
 */

#ifndef ADD_PRINTER_DIALOG_H
#define ADD_PRINTER_DIALOG_H

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
		enum MessageKind {
			kPrinterSelectedMsg = 'adlg',
			kTransportSelectedMsg,
			kNameChangedMsg,
		};
	
		AddPrinterDialog();
		void MessageReceived(BMessage *msg);
	
		void AddPrinter(BMessage *msg);
		void StorePrinter(BMessage *msg);
		void HandleChangedTransport(BMessage *msg);
			
		void BuildGUI(int stage);
		void FillMenu(BMenu *menu, const char *path, uint32 what);
		void AddPortSubMenu(BMenu *menu, const char *transport, const char *port);
		void Update();
		
		BTextControl *fName;
		BPopUpMenu *fPrinter;
		BPopUpMenu *fTransport;
		BButton *fOk;
		
		BString fNameText;
		BString fPrinterText;
		BString fTransportText;
		BString fTransportPathText;
};

#endif
