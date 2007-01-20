/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 */

#ifndef JOBSETUPWINDOW_H
#define JOBSETUPWINDOW_H

#include <InterfaceKit.h>
#include "InterfaceUtils.h"
#include "Utils.h"

class JobSetupWindow : public BlockingWindow 
{
public:
	// Constructors, destructors, operators...

							JobSetupWindow(BMessage *msg, const char *printer_name = NULL);

	typedef BlockingWindow 		inherited;

	// public constantes
	enum	{
		NB_COPIES_MSG		= 'copy',
		ALL_PAGES_MGS		= 'all_',
		RANGE_SELECTION_MSG	= 'rnge',
		RANGE_FROM_MSG		= 'from',
		RANGE_TO_MSG		= 'to__',
		OK_MSG				= 'ok__',
		CANCEL_MSG			= 'cncl',
	};
			
	// Virtual function overrides
public:	
	virtual void 			MessageReceived(BMessage *msg);
	
	// From here, it's none of your business! ;-)
private:
	BString                 fPrinterName;
	BMessage				*fSetupMsg;
	BRadioButton			*fAll;
	BRadioButton			*fRange;
	BTextControl			*fFrom;
	BTextControl			*fTo;
	
	void UpdateJobMessage();
};

#endif

