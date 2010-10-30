#ifndef SELECT_PRINTER_DIALOG_H
#define SELECT_PRINTER_DIALOG_H


#include <storage/FindDirectory.h>

#include "DialogWindow.h"
#include "GPData.h"


class BListView;
class BButton;
class PSData;


class SelectPrinterDialog : public DialogWindow {
public:
				SelectPrinterDialog(GPData* data);

	void		MessageReceived(BMessage* msg);
private:
	void		PopulateManufacturers();
	void		PopulatePrinters();
	BString		GetSelectedItemValue(BListView* listView);
	void		PrinterSelected();
	void		Save();

	BListView*	fManufacturersListView;
	BListView*	fPrintersListView;
	BButton*	fOKButton;

	GPData*		fData;
};

#endif // SELECTPPDDLG_H
