#ifndef SELECTPPDDLG_H
#define SELECTPPDDLG_H

#include "DialogWindow.h"
#include "PSData.h"

#include <storage/FindDirectory.h>

class BListView;
class BButton;
class PSData;

class SelectPPDDlg : public DialogWindow {
public:
	SelectPPDDlg(PSData *data);
	void MessageReceived(BMessage *msg);
private:
	void PopulateManufacturers(directory_which data_dir);
	void PopulatePrinters(directory_which data_dir);
	void PrinterSelected();
	void Save();

	BListView *fManufacturersListView;
	BListView *fPrintersListView;
	BButton *fOKButton;

	PSData *fPSData;
};

#endif /* SELECTPPDDLG_H */
