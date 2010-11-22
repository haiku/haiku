/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Michael Pfeiffer
 */
#ifndef _ADD_PRINTER_DIALOG_H
#define _ADD_PRINTER_DIALOG_H


#include <Button.h>
#include <String.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Window.h>


class AddPrinterDialog : public BWindow {
		typedef BWindow Inherited;
public:
								AddPrinterDialog(BWindow *parent);
		
			void				MessageReceived(BMessage *msg);
			bool				QuitRequested();
	
private:
			enum MessageKind {
				kPrinterSelectedMsg = 'adlg',
				kTransportSelectedMsg,
				kNameChangedMsg,
			};
	
	
			void				_AddPrinter(BMessage *msg);
			void				_StorePrinter(BMessage *msg);
			void				_HandleChangedTransport(BMessage *msg);

			void				_BuildGUI(int stage);
			void				_FillTransportMenu(BMenu *menu);
			void				_FillMenu(BMenu *menu, const char *path,
									uint32 what);
			void				_AddPortSubMenu(BMenu *menu,
									const char *transport, const char *port);
			void 				_Update();
		
			BMessenger			fPrintersPrefletMessenger;

			BTextControl*		fName;
			BPopUpMenu*			fPrinter;
			BPopUpMenu*			fTransport;
			BButton*			fOk;
		
			BString 			fNameText;
			BString 			fPrinterText;
			BString 			fTransportText;
			BString 			fTransportPathText;
};

#endif // _ADD_PRINTER_DIALOG_H
