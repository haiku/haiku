/*
 * AddPrinterDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include "DialogWindow.h"

#include <ListItem.h>
#include <View.h>

class BListView;
class BTextView;

class PrinterData;
class PrinterCap;
class ProtocolClassCap;

class ProtocolClassItem : public BStringItem {
public:
	ProtocolClassItem(const ProtocolClassCap* cap);

	int getProtocolClass();
	const char *getDescription();

private:
	const ProtocolClassCap *fProtocolClassCap;	
};

class AddPrinterView : public BView {
public:
	AddPrinterView(BRect frame, PrinterData *printer_data, const PrinterCap *printer_cap);
	~AddPrinterView();
	virtual void AttachedToWindow();
	void MessageReceived(BMessage *msg);

	void Save();
private:
	ProtocolClassItem *CurrentSelection();

	PrinterData       *fPrinterData;
	const PrinterCap  *fPrinterCap;
	
	BListView         *fProtocolClassList;
	BTextView         *fDescription;
};

 
class AddPrinterDlg : public DialogWindow {
public:
	AddPrinterDlg(PrinterData *printerData, const PrinterCap *printerCap);
	void MessageReceived(BMessage *msg);

private:
	AddPrinterView *fAddPrinterView;
};

