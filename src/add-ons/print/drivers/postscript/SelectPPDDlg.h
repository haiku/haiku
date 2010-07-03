#ifndef SELECTPPDDLG_H
#define SELECTPPDDLG_H


#include <storage/FindDirectory.h>

#include "DialogWindow.h"
#include "PSData.h"


class BListView;
class BButton;
class PSData;


class SelectPPDDlg : public DialogWindow {
public:
				SelectPPDDlg(PSData* data);
	void		MessageReceived(BMessage* msg);
private:
	void		PopulateManufacturers(directory_which data_dir);
	void		PopulatePrinters(directory_which data_dir);
	void		PrinterSelected();
	void		Save();

	BListView*	fManufacturersListView;
	BListView*	fPrintersListView;
	BButton*	fOKButton;

	PSData*		fPSData;
};

#endif // SELECTPPDDLG_H
