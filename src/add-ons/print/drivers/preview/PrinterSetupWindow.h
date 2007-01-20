/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 */

#ifndef PRINTERSETUPWINDOW_H
#define PRINTERSETUPWINDOW_H

#include <InterfaceKit.h>
#include "InterfaceUtils.h"

class PrinterSetupWindow : public BlockingWindow 
{
public:
	// Constructors, destructors, operators...

							PrinterSetupWindow(char *printerName);

	typedef BlockingWindow 		inherited;

	// public constantes
	enum	{
		OK_MSG				= 'ok__',
		CANCEL_MSG			= 'cncl',
		MODEL_MSG			= 'modl'
	};
			
	// Virtual function overrides
public:	
	virtual void 			MessageReceived(BMessage *msg);

	// From here, it's none of your business! ;-)
private:
	BButton					*fOkButton;
	BListView				*fModelList;
	char					*fPrinterName;
};

#endif

