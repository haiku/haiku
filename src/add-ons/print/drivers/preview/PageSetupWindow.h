/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 *		julun <host.haiku@gmx.de>
 */

#ifndef PAGESETUPWINDOW_H
#define PAGESETUPWINDOW_H


#include "BlockingWindow.h"
#include "PrintUtils.h"


#include <String.h>


class BMessage;
class BMenuField;
class BTextControl;
class MarginView;


class PageSetupWindow : public BlockingWindow
{
public:
					PageSetupWindow(BMessage *msg, const char *printerName = NULL);
	virtual void 	MessageReceived(BMessage *msg);

	enum			{
						OK_MSG = 'ok__',
						CANCEL_MSG = 'cncl',
						PAGE_SIZE_CHANGED = 'pgsz',
						ORIENTATION_CHANGED = 'ornt'
					};
private:
	void			UpdateSetupMessage();

private:
	BMessage *		fSetupMsg;
	BMenuField *	fPageSizeMenu;
	BMenuField *	fOrientationMenu;
	BTextControl *	fScaleControl;
	MarginView *	fMarginView;
	BString			fPrinterDirName;
	int32			fCurrentOrientation;
};

#endif
