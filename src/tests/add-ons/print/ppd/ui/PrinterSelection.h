/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PRINTER_SELECTION_H
#define _PRINTER_SELECTION_H

#include "PPD.h"

#include <Invoker.h>
#include <View.h>
#include <ListItem.h>
#include <ListView.h>

class FileItem : public BStringItem
{
private:
	BString fFile;
	
public:
	FileItem(const char* label, const char* file)
		: BStringItem(label)
		, fFile(file)
	{
	}

	const char* GetFile() { return fFile.String(); }	
};

class PrinterSelectionView : public BView, public BInvoker 
{
private:
	BListView* fVendors;
	BListView* fPrinters;
		
public:
	PrinterSelectionView(BRect rect, const char *name, uint32 resizeMask, uint32 flags);

	void AttachedToWindow();
	
	void FillVendors();
	void FillPrinters(const char* vendor);
	void MessageReceived(BMessage* msg);
};
#endif
