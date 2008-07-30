/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		julun <host.haiku@gmx.de>
 */

#ifndef JOBSETUPWINDOW_H
#define JOBSETUPWINDOW_H


#include "BlockingWindow.h"
#include "PrintUtils.h"


#include <String.h>


class BMessage;
class BRadioButton;
class BTextControl;


class JobSetupWindow : public BlockingWindow
{
public:
					JobSetupWindow(BMessage *msg, const char *printer_name = NULL);
	virtual void 	MessageReceived(BMessage *msg);

	enum			{
						ALL_PAGES_MGS = 'all_',
						RANGE_SELECTION_MSG	= 'rnge',
						OK_MSG = 'ok__',
						CANCEL_MSG = 'cncl',
					};

private:
	void			UpdateJobMessage();

private:
	BString			fPrinterName;
	BMessage		*fSetupMsg;
	BRadioButton	*fAll;
	BRadioButton	*fRange;
	BTextControl	*fFrom;
	BTextControl	*fTo;
};

#endif
